#pragma once
#include <string>


struct Tokens {
    std::string access_token;
    std::string refresh_token;
    long expires_at;
};

bool refreshAccessToken( Tokens& tokens, const std::string& clientId, const std::string& clientSecret);
bool saveTokens(const Tokens& tokens, const std::string& filename);
bool loadTokens(Tokens& tokens, const std::string& filename);
bool isTokenExpired(long expires_at);
bool downloadImage(const std::string& imageUrl, const std::string& filename);

