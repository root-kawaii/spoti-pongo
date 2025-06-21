#include <iostream>
#include <string>
#include "audio_cache.h"
#include <nlohmann/json.hpp>
#include "spotify_api.h"
#include "login.h"
#include "token.h"
#include "base64.h"
#include <curl/curl.h>
#include <chrono>
#include <thread>
#include <sys/ioctl.h>
#include <unistd.h>

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
void extractUri(const nlohmann::json& result, std::vector<Track>& tracks, SpotifyAPI& api);
void turnOnDevice(std::string deviceName);
void activateDevice(std::string device, std::string access );
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
std::string getSpotifyDevices(const std::string& access_token, std::string deviceName);
int getTerminalWidth();
void preCacheTrack(const std::string& uri, SpotifyAPI& api);


void enterFullscreenTerminal() {
    std::cout << "\033[?1049h\033[H\033[2J\033[?25l";  // Alternate buffer, home, clear screen, hide cursor
    std::cout.flush();
}

void exitFullscreenTerminal() {
    std::cout << "\033[?1049l\033[?25h";  // Restore buffer, show cursor
    std::cout.flush();
}



// Assuming SpotifyAPI and playSpotifyTrack are defined somewhere else

int main() {
    // enterFullscreenTerminal(); 
    std::vector<Track> tracks;
    std::string clientId = "9637f87558ee43a5a9c2557163c453a7";
    std::string clientSecret = "ea1e9c60354a44178a6677108cb25640";
    // std::string accessToken = "BQAVrhA5scIKjVCmnEfjBN6TCiEkJlYgDuSjL1mGD37f7TCi73EPM8LWzWogoor6sMPQdDrAg0xQ_ZYgWwBtIWHS7iDjEV7_0D7DCQhuKWpE7bLs48mOybGEnA_1vwrSRfn-UkSSGT9Bwg_FkMtbLSLVG5Do8M3llL9dFYCStr02eSvKAATxBcMYnkHnEy0r14Kjf5LlOtSKYOwj12DyzblXMs1KbUgBHsj5dR3o3I-f5Kk";
    Tokens tokens;

    AudioCache cache("librespot_cache.pcm");

    std::atomic<bool> stop_reader{false};

    auto start_stdin_reader = [&cache, &stop_reader]() {
        std::thread stdin_reader([&cache, &stop_reader]() {
            std::vector<uint8_t> buffer(14096);
            std::cout << "\nReader started\n";

            // Loop ends when either:
            //  1. stop_reader == true, OR
            //  2. std::cin hits EOF / error
            while (!stop_reader && std::cin.read(reinterpret_cast<char*>(buffer.data()),
                                                buffer.size())) {

                size_t n = std::cin.gcount();
                if (n == 0) continue;
                buffer.resize(n);
                cache.push_data(buffer);
            }

            std::cout << "\nReader exiting\n";
        });

        stdin_reader.detach();      // still detached, but now cooperatively stoppable
    };



    loadTokens(tokens, "tokens.json");
    turnOnDevice("Coglionazzo");
    activateDevice(getSpotifyDevices(tokens.access_token, "Coglionazzo"), tokens.access_token);

    // exchangeCodeForToken("AQCGgjt2Lv8Takz6HODddumKy2dtomP6eP6ZzZceTKHZo-uwCn6eRhDPLYzq_r5d11WHY9TtZLBJaIlGnWPS_sB1xzE2wUbVqpb_Hl0uC8F_xaP3ZmExWFDQdp1zUih69gb3-Geeah3QgDTMdbtgcrWGBiOu3WKYfkbv7NoyE3NwOH9xjsfi8GRXlHzCuaOjAqWHis5CPw5bSvOSixUjVSDM6o-BT4zaUwcr_rmw6IvwcjIBzIIkJKLBEUsIvOODoG4Neqk7y2ynbu2r_0L5xvKVxLOmFHMXFsISLQ");

    SpotifyAPI api(clientId, clientSecret, tokens.access_token, getSpotifyDevices(tokens.access_token, "Coglionazzo"));
    if (!api.authenticate()) {
        std::cerr << "Authentication failed.\n";
        exitFullscreenTerminal();
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
            extractUri(*result, tracks, api);
            start_stdin_reader();
            for (const Track& i : tracks) {
                api.playSpotifyTrack(i.song_uri);
            }
            stop_reader = true;

        } else if (command == "play") {
            if (argument.empty()) {
                std::cout << "Usage: play TRACK_URI\n";
                continue;
            }

            Track selectedTrack = tracks[std::stoi(argument)];

            std::cout << "Playing track -- " << selectedTrack.song_name << " -- " << selectedTrack.artist << "\n";
            // Later in your code, when you're ready to start reading:
            // downloadImage(selectedTrack.cover, "buffer.txt");
            // printImageGeneric("buffer.txt");
            api.playSpotifyTrack(selectedTrack.song_uri);  // Pass argument as URI
            // stop_reader = true;

        } else {
            std::cout << "Unknown command: " << command << "\n";
            std::cout << "Commands: search \"Song Name\", play TRACK_URI, quit\n";
        }
    }
    exitFullscreenTerminal();
    return 0;
}

void extractUri(const nlohmann::json& result, std::vector<Track>& tracks, SpotifyAPI& api) {
    // try {
        const auto& items = result["tracks"]["items"];
        int limit = std::min(4, static_cast<int>(items.size()));

        int termWidth = getTerminalWidth();
        int imageStartCol = 4 * termWidth / 3; // Keep some spacing

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

            std::string albumName = track["album"].value("name", "Unknown Album");

            std::string albumCoverUrl = "N/A";
            if (!track["album"]["images"].empty()) {
                albumCoverUrl = track["album"]["images"][0].value("url", "N/A");
            }

            int durationMs = track.value("duration_ms", 0);
            int minutes = durationMs / 60000;
            int seconds = (durationMs % 60000) / 1000;

            std::string uri = track.value("uri", "N/A");

            new_track.album = albumName;
            new_track.song_name = trackName;
            new_track.cover = albumCoverUrl;
            new_track.length = seconds;
            new_track.artist = artistNames;
            new_track.song_uri = uri;
            tracks.push_back(new_track);


            downloadImage(new_track.cover, "buffer.txt");



            // Save cursor position
            std::cout << "\033[s"; // Save cursor position

            // Move to the right (column 60 for example)
            std::cout << "\033[" << imageStartCol << "G" << std::flush;  // Move cursor right to column 60

            // Print the image (it will render downwards from current line)
            printImageGeneric("buffer.txt");

            // Restore cursor to left and overwrite any stray text with blank lines
            std::cout << "\033[1G"; // Restore cursor to saved left-side position

            // Print text block on the left (this overwrites any accidental titles on the right)
            std::cout << "Track:   " << trackName << "\n"
                    << "Artist:  " << artistNames << "\n"
                    << "Album:   " << new_track.album << "\n"
                    << "Length:  " << minutes << ":" << std::setfill('0') << std::setw(2) << seconds << "\n"
                    << "--------------------------------------------\n";

}

}




void turnOnDevice(std::string deviceName){
std::string command = "librespot --cache " + std::string(getenv("HOME")) + 
                      "/.cache/librespot --name " + deviceName + 
                      " --backend pipe | ffplay -f s16le -ar 88200 -nodisp - > /dev/null 2>&1 &";
system(command.c_str());
command = "librespot --cache " + std::string(getenv("HOME")) + 
                      "/.cache/librespot --name " + deviceName + "-cache" + 
                      " --backend pipe > /tmp/spotify_cache.raw &";
system(command.c_str());


}


void activateDevice(std::string device, std::string access ){
    std::this_thread::sleep_for(std::chrono::seconds(1));
    const std::string access_token = access;
    const std::string device_id = device;
    // JSON data to send
    std::string json_data = R"({"device_ids": [")" + device_id + R"("], "play": true})";

    CURL *curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize curl" << std::endl;
    }

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.spotify.com/v1/me/player");
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
    else
        std::cout << "Playback transferred successfully!" << std::endl;

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    std::this_thread::sleep_for(std::chrono::seconds(1));

}


size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::string getSpotifyDevices(const std::string& access_token, std::string deviceName) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to init curl" << std::endl;
        return "";
    }

    std::string response_str;
    struct curl_slist* headers = nullptr;
    std::string auth_header = "Authorization: Bearer " + access_token;
    headers = curl_slist_append(headers, auth_header.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.spotify.com/v1/me/player/devices");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_str);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        response_str = "";
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (response_str.empty()) {
        return "";
    }

    try {
        nlohmann::json response_json = nlohmann::json::parse(response_str);

        if (!response_json["devices"].empty()) {
            for(int j=0; j < 10000; j++){
                std::cout << response_json["devices"][j];
                if(response_json["devices"][j]["name"] == deviceName) return response_json["devices"][j]["id"].get<std::string>();
            }

            return response_json["devices"][0]["id"].get<std::string>();
        } else {
            std::cerr << "No devices found in response." << std::endl;
            return "";
        }
    } catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return "";
    }
}


int getTerminalWidth() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}


void preCacheTrack(const std::string& uri, SpotifyAPI& api) {
    std::cout << "Pre-caching track: " << uri << "\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));  // Let it buffer
    api.playTrackSilentlyForCaching(uri);
    std::this_thread::sleep_for(std::chrono::seconds(5));  // Let it buffer
    api.pausePlayback();  // You must implement this in SpotifyAPI
    std::cout << "Pre-cached successfully.\n";
}


