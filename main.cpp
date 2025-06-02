#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include "spotify_api.h"
#include "login.h"
#include "token.h"
#include "base64.h"

struct Track {
    std::string song_name;
    std::string artist;
    std::string album; 
    std::string cover; 
    std::string song_uri; 
    long length;
    long current_time;
};

// Forward declaration
void extractUri(const nlohmann::json& result, std::vector<Track>& tracks);


// Assuming SpotifyAPI and playSpotifyTrack are defined somewhere else

int main() {
    std::vector<Track> tracks;
    std::string clientId = "9637f87558ee43a5a9c2557163c453a7";
    std::string clientSecret = "ea1e9c60354a44178a6677108cb25640";
    // std::string accessToken = "BQAVrhA5scIKjVCmnEfjBN6TCiEkJlYgDuSjL1mGD37f7TCi73EPM8LWzWogoor6sMPQdDrAg0xQ_ZYgWwBtIWHS7iDjEV7_0D7DCQhuKWpE7bLs48mOybGEnA_1vwrSRfn-UkSSGT9Bwg_FkMtbLSLVG5Do8M3llL9dFYCStr02eSvKAATxBcMYnkHnEy0r14Kjf5LlOtSKYOwj12DyzblXMs1KbUgBHsj5dR3o3I-f5Kk";
    Tokens tokens;
    loadTokens(tokens, "tokens.json");

    // exchangeCodeForToken("AQCGgjt2Lv8Takz6HODddumKy2dtomP6eP6ZzZceTKHZo-uwCn6eRhDPLYzq_r5d11WHY9TtZLBJaIlGnWPS_sB1xzE2wUbVqpb_Hl0uC8F_xaP3ZmExWFDQdp1zUih69gb3-Geeah3QgDTMdbtgcrWGBiOu3WKYfkbv7NoyE3NwOH9xjsfi8GRXlHzCuaOjAqWHis5CPw5bSvOSixUjVSDM6o-BT4zaUwcr_rmw6IvwcjIBzIIkJKLBEUsIvOODoG4Neqk7y2ynbu2r_0L5xvKVxLOmFHMXFsISLQ");

    SpotifyAPI api(clientId, clientSecret, tokens.access_token);
    if (!api.authenticate()) {
        std::cerr << "Authentication failed.\n";
        return 1;
    }

    std::cout << "Welcome to spotify_cli. Enter commands (search \"Song Name\", play TRACK_URI, or quit):\n";

    std::string line;
    while (true) {
        Tokens tokens;
        loadTokens(tokens, "tokens.json");

        if (isTokenExpired(tokens.expires_at)) {
            if (!refreshAccessToken(tokens, clientId, clientSecret)) {
                std::cerr << "Could not refresh token, please re-authenticate." << std::endl;
                // Trigger auth flow again here or exit
            } else {
                saveTokens(tokens, "tokens.json");
            }
        }
        else{
            // std::cerr << "No need to refresh." << std::endl;
        }
        std::cout << "> ";
        std::getline(std::cin, line);

        if (line.empty()) continue;

        // Simple parsing: command is the first word, rest is argument
        auto spacePos = line.find(' ');
        std::string command = (spacePos == std::string::npos) ? line : line.substr(0, spacePos);
        std::string argument = (spacePos == std::string::npos) ? "" : line.substr(spacePos + 1);

        if (command == "quit" || command == "exit") {
            std::cout << "Exiting...\n";
            break;
        } else if (command == "search") {
            tracks.clear();
            if (argument.empty()) {
                std::cout << "Usage: search \"Song Name\"\n";
                continue;
            }

            auto result = api.searchTrack(argument);
            if (!result) {
                std::cerr << "No result returned from search.\n";
                continue;
            }

            extractUri(*result, tracks);

        } else if (command == "play") {
            if (argument.empty()) {
                std::cout << "Usage: play TRACK_URI\n";
                continue;
            }

            Track selectedTrack = tracks[std::stoi(argument)];

            std::cout << "Playing track -- " << selectedTrack.song_name << " -- " << selectedTrack.artist << "\n";
            downloadImage(selectedTrack.cover, "buffer.txt");
            printImageToKittyTerminal("buffer.txt");
            api.playSpotifyTrack(selectedTrack.song_uri);  // Pass argument as URI

        } else {
            std::cout << "Unknown command: " << command << "\n";
            std::cout << "Commands: search \"Song Name\", play TRACK_URI, quit\n";
        }
    }

    return 0;
}

void extractUri(const nlohmann::json& result, std::vector<Track>& tracks) {
    try {
        const auto& items = result["tracks"]["items"];
        int limit = std::min(15, static_cast<int>(items.size()));

        for (int i = 0; i < limit; ++i) {
            Track new_track;
            const auto& track = items[i];

            std::string trackName = track.value("name", "Unknown Title");

            // Extract artists
            std::string artistNames;
            for (const auto& artist : track["artists"]) {
                if (!artistNames.empty()) artistNames += ", ";
                artistNames += artist.value("name", "Unknown Artist");
            }

            // Extract album name
            std::string albumName = track["album"].value("name", "Unknown Album");

            // Extract album cover URL (take the first image)
            std::string albumCoverUrl = "N/A";
            if (!track["album"]["images"].empty()) {
                albumCoverUrl = track["album"]["images"][0].value("url", "N/A");
            }

            // Extract duration (convert from ms to mm:ss)
            int durationMs = track.value("duration_ms", 0);
            int minutes = durationMs / 60000;
            int seconds = (durationMs % 60000) / 1000;

            // ✅ Extract URI
            std::string uri = track.value("uri", "N/A");

            new_track.album = albumName;
            new_track.song_name = trackName;
            new_track.cover = albumCoverUrl;
            new_track.length = seconds;
            new_track.artist = artistNames;
            new_track.song_uri = uri; 
            tracks.push_back(new_track);

            // Output everything
            std::cout << "Track:   " << trackName << "\n"
                      << "Artist:  " << artistNames << "\n"
                      << "Album:   " << new_track.album << "\n"
                      << "Length:  " << minutes << ":" << std::setfill('0') << std::setw(2) << seconds << "\n"
                      << "Cover:   " << albumCoverUrl << "\n"
                      << "--------------------------------------------\n";
        }


    } catch (const std::exception& e) {
        std::cerr << "Failed to extract track info: " << e.what() << std::endl;
    }
}

