// login.h
#pragma once
#include <string>

std::string urlEncode(const std::string& value);
std::string generateRandomString(size_t length);
std::string extractCodeFromRedirectUrl(const std::string& url);
void startSpotifyLoginFlow(const std::string& clientId, const std::string& redirectUri);
void exchangeCodeForToken(const std::string& code);