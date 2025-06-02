#include <cpr/cpr.h>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <base64.h>

#include <chrono>
#include <ctime>

struct Tokens {
    std::string access_token;
    std::string refresh_token;
    long expires_at;
};


bool refreshAccessToken(
    Tokens& tokens,
    const std::string& clientId,
    const std::string& clientSecret
) {
    std::string auth = clientId + ":" + clientSecret;
    std::string encodedAuth = base64_encode(auth);

    auto response = cpr::Post(
        cpr::Url{"https://accounts.spotify.com/api/token"},
        cpr::Header{
            {"Authorization", "Basic " + encodedAuth},
            {"Content-Type", "application/x-www-form-urlencoded"}
        },
        cpr::Body{
            "grant_type=refresh_token&refresh_token=" + tokens.refresh_token
        }
    );

    if (response.status_code != 200) {
        std::cerr << "Failed to refresh token: " << response.status_code << " " << response.text << std::endl;
        return false;
    }

    auto json = nlohmann::json::parse(response.text);
    tokens.access_token = json["access_token"];
    // expires_in is seconds from now
    int expiresIn = json.value("expires_in", 3600);
    tokens.expires_at = std::time(nullptr) + expiresIn;

    // Optional: Spotify sometimes returns a new refresh_token; update if present
    if (json.contains("refresh_token")) {
        tokens.refresh_token = json["refresh_token"];
    }

    return true;
}


bool saveTokens(const Tokens& tokens, const std::string& filename) {
    nlohmann::json j = {
        {"access_token", tokens.access_token},
        {"refresh_token", tokens.refresh_token},
        {"expires_at", tokens.expires_at}
    };
    std::ofstream file(filename);
    if (!file) return false;
    file << j.dump(4);
    return true;
}

bool loadTokens(Tokens& tokens, const std::string& filename) {
    std::ifstream file(filename);
    if (!file) return false;
    nlohmann::json j;
    file >> j;
    tokens.access_token = j.value("access_token", "");
    tokens.refresh_token = j.value("refresh_token", "");
    tokens.expires_at = j.value("expires_at", 0);
    return true;
}


bool isTokenExpired(long expires_at) {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    return now >= expires_at;
}


bool downloadImage(const std::string& imageUrl, const std::string& filename) {
    cpr::Response r = cpr::Get(cpr::Url{imageUrl});
    // std::cout << r;
    std::cout << imageUrl;

    if (r.status_code == 200) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return false;
        }
        file.write(r.text.c_str(), r.text.size());
        file.close();
        return true;
    } else {
        std::cerr << "Failed to download image, HTTP status: " << r.status_code << std::endl;
        return false;
    }
}
