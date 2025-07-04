#pragma once
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

class SpotifyAPI {
public:
    SpotifyAPI(const std::string& clientId, const std::string& clientSecret, const std::string& accessToken, const std::string& deviceId);
    bool authenticate();
    std::optional<nlohmann::json> searchTrack(const std::string& query);
    void playSpotifyTrack(const std::string& track_uri);
    bool pausePlayback();
    void playTrackSilentlyForCaching(const std::string& track_uri);

private:
    std::string clientId;
    std::string clientSecret;
    std::string accessToken;
    std::string deviceId;
};
