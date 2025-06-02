#include <iostream>
#include <string>
#include <random>
#include <sstream>
#include <regex>
#include <cstdlib> 

#include <iomanip>
#include <sstream>

#include <cpr/cpr.h>
#include <base64.h> 


std::string urlEncode(const std::string& value) {
    std::ostringstream encoded;
    encoded << std::hex << std::uppercase;

    for (unsigned char c : value) {
        // Safe characters from RFC 3986
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        } else {
            encoded << '%' << std::setw(2) << std::setfill('0') << int(c);
        }
    }

    return encoded.str();
}


std::string generateRandomString(size_t length) {
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static std::default_random_engine rng(std::random_device{}());
    static std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i)
        result += charset[dist(rng)];
    return result;
}


std::string extractCodeFromRedirectUrl(const std::string& url) {
    std::regex codeRegex("code=([^&]+)");
    std::smatch match;
    if (std::regex_search(url, match, codeRegex)) {
        return match[1];
    }
    return "";
}

void startSpotifyLoginFlow(const std::string& clientId, const std::string& redirectUri) {
    std::string state = generateRandomString(16);
    std::string scope = "user-read-private user-read-email user-read-playback-state user-modify-playback-state";

    std::ostringstream authUrl;
    authUrl << "https://accounts.spotify.com/authorize?"
            << "response_type=code"
            << "&client_id=" << urlEncode(clientId)
            << "&scope=" << urlEncode(scope)
            << "&redirect_uri=" << urlEncode(redirectUri)
            << "&state=" << urlEncode(state);

    std::string fullUrl = authUrl.str();

    std::cout << "Please open this URL in your browser to authorize:\n" << fullUrl << "\n\n";

    // Optional: open browser automatically
#ifdef _WIN32
    system(("start " + fullUrl).c_str());
#elif __APPLE__
    system(("open " + fullUrl).c_str());
#else
    system(("xdg-open " + fullUrl).c_str());
#endif

    std::cout << "After authorization, you will be redirected to a URL.\n";
    std::cout << "Please copy and paste the full redirected URL here:\n";
    std::string redirectedUrl;
    std::getline(std::cin, redirectedUrl);

    std::string code = extractCodeFromRedirectUrl(redirectedUrl);
    if (code.empty()) {
        std::cerr << "Failed to extract authorization code.\n";
    } else {
        std::cout << "Authorization code: " << code << std::endl;
        // You can now exchange this code for tokens
        // exchangeCodeForTokens(code, clientId, clientSecret, redirectUri);
    }
}


void exchangeCodeForToken(const std::string& code) {
    std::string clientId = "9637f87558ee43a5a9c2557163c453a7";
    std::string clientSecret = "ea1e9c60354a44178a6677108cb25640";
    std::string redirectUri = "http://localhost:8888/callback";

    std::string auth = clientId + ":" + clientSecret;
    std::string encodedAuth = base64_encode(auth); // Your base64 lib

    auto response = cpr::Post(
        cpr::Url{"https://accounts.spotify.com/api/token"},
        cpr::Header{
            {"Authorization", "Basic " + encodedAuth},
            {"Content-Type", "application/x-www-form-urlencoded"}
        },
        cpr::Body{
            "grant_type=authorization_code&"
            "code=" + cpr::util::urlEncode(code) + "&"
            "redirect_uri=" + cpr::util::urlEncode(redirectUri)
        }
    );

    if (response.status_code == 200) {
        std::cout << "Access Token Response:\n" << response.text << std::endl;
    } else {
        std::cerr << "Token request failed: " << response.status_code << "\n" << response.text << std::endl;
    }
}
