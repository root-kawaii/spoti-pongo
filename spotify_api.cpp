#include "spotify_api.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <base64.h>

SpotifyAPI::SpotifyAPI(const std::string& clientId, const std::string& clientSecret, const std::string& accessToken)
    : clientId(clientId), clientSecret(clientSecret), accessToken(accessToken) {}

bool SpotifyAPI::authenticate() {
    std::string auth = clientId + ":" + clientSecret;
    std::string encoded = base64_encode(auth);

    auto response = cpr::Post(cpr::Url{"https://accounts.spotify.com/api/token"},
        cpr::Header{{"Authorization", "Basic " + encoded},
                    {"Content-Type", "application/x-www-form-urlencoded"}},
        cpr::Body{"grant_type=client_credentials"});

    if (response.status_code == 200) {
        auto json = nlohmann::json::parse(response.text);
        // accessToken = json["access_token"];
        return true;
    } else {
        std::cerr << "Auth failed: " << response.status_code << "\n";
        return false;
    }
}

std::optional<nlohmann::json> SpotifyAPI::searchTrack(const std::string& query) {
    auto response = cpr::Get(cpr::Url{"https://api.spotify.com/v1/search"},
        cpr::Header{{"Authorization", "Bearer " + accessToken}},
        cpr::Parameters{{"q", query}, {"type", "track"}, {"limit", "20"}});

    if (response.status_code == 200) {
        return nlohmann::json::parse(response.text);
    } else {
        std::cerr << "Error (" << response.status_code << "): " << response.text << std::endl;
        return std::nullopt;
    }
}


void SpotifyAPI::playSpotifyTrack(const std::string& track_uri) {
    std::string url = "https://api.spotify.com/v1/me/player/play";
    nlohmann::json body = {
        {"uris", {track_uri}}  // Example: "spotify:track:4zpNfuWJA3K4d9TS4qnOIB"
    };

    cpr::Response r = cpr::Put(
        cpr::Url{url},
        cpr::Header{
            {"Authorization", "Bearer " + accessToken},
            {"Content-Type", "application/json"}
        },
        cpr::Body{body.dump()}
    );

    if (r.status_code == 204) {
        std::cout << "Track is now playing.\n";
    } else {
        std::cerr << "Error (" << r.status_code << "): " << r.text << std::endl;
    }
}
