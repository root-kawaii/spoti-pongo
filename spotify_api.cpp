#include "spotify_api.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <base64.h>

SpotifyAPI::SpotifyAPI(const std::string& clientId, const std::string& clientSecret, const std::string& accessToken,  const std::string& deviceId)
    : clientId(clientId), clientSecret(clientSecret), accessToken(accessToken), deviceId(deviceId) {}

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

    std::this_thread::sleep_for(std::chrono::seconds(5));  // Let it buffer

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

void SpotifyAPI::playTrackSilentlyForCaching(const std::string& track_uri) {
    // Set volume to 0
    std::string volume_url = "https://api.spotify.com/v1/me/player/volume?volume_percent=0&device_id=" + deviceId;
    std::cout << volume_url;

    
    cpr::Response volume_response = cpr::Put(
        cpr::Url{volume_url},
        cpr::Header{{"Authorization", "Bearer " + accessToken}},
        cpr::Body{""}  // <-- Add this to include Content-Length: 0
    );

    if (volume_response.status_code != 204) {
        std::cerr << "Failed to set volume to 0: " << volume_response.text << std::endl;
        return;
    }

    // Play the track
    playSpotifyTrack(track_uri);

        volume_url =  "https://api.spotify.com/v1/me/player/volume?volume_percent=50&device_id=" + deviceId;
        volume_response = cpr::Put(
        cpr::Url{volume_url},
        cpr::Header{{"Authorization", "Bearer " + accessToken}},
        cpr::Body{""}  // <-- Add this to include Content-Length: 0
    );

    if (volume_response.status_code != 204) {
        std::cerr << "Failed to set volume to 25: " << volume_response.text << std::endl;
        return;
    }
}


bool SpotifyAPI::pausePlayback() {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string url = "https://api.spotify.com/v1/me/player/pause";
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return res == CURLE_OK;
}

