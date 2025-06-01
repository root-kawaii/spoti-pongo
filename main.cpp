#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include "spotify_api.h"
#include "login.h"
#include "token.h"

// Forward declaration
void extractUri(const nlohmann::json& result);

// Assuming SpotifyAPI and playSpotifyTrack are defined somewhere else

int main() {
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
            std::cerr << "No need to refresh." << std::endl;
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
            if (argument.empty()) {
                std::cout << "Usage: search \"Song Name\"\n";
                continue;
            }

            auto result = api.searchTrack(argument);
            if (!result) {
                std::cerr << "No result returned from search.\n";
                continue;
            }

            extractUri(*result);

        } else if (command == "play") {
            if (argument.empty()) {
                std::cout << "Usage: play TRACK_URI\n";
                continue;
            }

            std::cout << "Playing track " << argument << " (placeholder)...\n";
            api.playSpotifyTrack(argument);  // Pass argument as URI

        } else {
            std::cout << "Unknown command: " << command << "\n";
            std::cout << "Commands: search \"Song Name\", play TRACK_URI, quit\n";
        }
    }

    return 0;
}

void extractUri(const nlohmann::json& result) {
    try {
        const auto& items = result.at("tracks").at("items");

        std::ostringstream buffer;  // Buffering output in memory

        int count = std::min(15, static_cast<int>(items.size()));
        for (int i = 0; i < count; ++i) {
            const auto& track = items[i];
            std::string name = track.value("name", "Unknown");
            buffer << i + 1 << ". " << name << '\n';
        }

        std::cout << buffer.str();  // Single flush to terminal
    } catch (const std::exception& e) {
        std::cerr << "Failed to extract track names: " << e.what() << std::endl;
    }
}
