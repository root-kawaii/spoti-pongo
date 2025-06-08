#include <string>
#include <cstdint>
#include <iostream>
#include <string>
#include <fstream>
#include "base64.h"
#include <sstream>
#include <algorithm>
#include <cstdlib>


std::string base64_encode(const std::string& input) {
    std::string ret;
    int val = 0, valb = -6;
    for (uint8_t c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            ret.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) ret.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (ret.size() % 4) ret.push_back('=');
    return ret;
}



std::string base64_encode_file(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return "";
    }
    
    std::ostringstream oss;
    oss << file.rdbuf();
    std::string data = oss.str();
    
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    int val = 0, valb = -6;
    
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(table[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        encoded.push_back(table[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    while (encoded.size() % 4) {
        encoded.push_back('=');
    }
    
    return encoded;
}

void printImageToKittyTerminal(const std::string &filename) {
    std::string base64data = base64_encode_file(filename);
    if (base64data.empty()) {
        std::cerr << "Failed to load image: " << filename << std::endl;
        return;
    }
    
    constexpr size_t chunk_size = 4096;
    size_t len = base64data.length();
    size_t offset = 0;
    bool first = true;
    
    while (offset < len) {
        size_t this_chunk_size = std::min(chunk_size, len - offset);
        std::string chunk = base64data.substr(offset, this_chunk_size);
        
        if (first) {
            // First chunk: start transmission
            std::cout << "\033_Gf=100,a=T,t=f";
            if (offset + this_chunk_size >= len) {
                // Single chunk case
                std::cout << ";";
            } else {
                // Multiple chunks
                std::cout << ",m=1;";
            }
            first = false;
        } else if (offset + this_chunk_size >= len) {
            // Final chunk
            std::cout << "\033_Gm=0;";
        } else {
            // Middle chunk
            std::cout << "\033_Gm=1;";
        }
        
        std::cout << chunk << "\033\\";
        offset += this_chunk_size;
    }
    
    std::cout << std::endl;
    std::cout.flush();
}

// Alternative function for terminals that support sixel (like xterm)
void printImageToSixelTerminal(const std::string &filename) {
    std::string command = "img2sixel \"" + filename + "\"";
    int result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "Failed to display image with img2sixel. Make sure it's installed." << std::endl;
    }
}

// Alternative function using external tools
void printImageGeneric(const std::string &filename) {
    // Try different image viewers in order of preference
    std::string viewers[] = {"kitty +kitten icat --place 8x3@100x400"}; // --scale-up --align left
    // std::string viewers[] = {"catimg -r "};

    for (const std::string &viewer : viewers) {
        std::string command = viewer + " \"" + filename + "\" 2>/dev/null";
        int result = std::system(command.c_str());
        if (result == 0) {
            return; // Success
        }
    }
    
    std::cerr << "No supported image viewer found. Please install one of:" << std::endl;
    std::cerr << "- kitty (with kitten icat)" << std::endl;
    std::cerr << "- imgcat (iTerm2)" << std::endl;
    std::cerr << "- catimg" << std::endl;
    std::cerr << "- timg" << std::endl;
    std::cerr << "- viu" << std::endl;
}

// Function to detect terminal type and use appropriate method
void printImageToTerminal(const std::string &filename) {
    const char* term = std::getenv("TERM");
    const char* term_program = std::getenv("TERM_PROGRAM");
    
    if (term_program && std::string(term_program) == "kitty") {
        std::cout << "Using Kitty terminal protocol..." << std::endl;
        printImageToKittyTerminal(filename);
    } else if (term_program && std::string(term_program) == "iTerm.app") {
        std::cout << "Using iTerm2..." << std::endl;
        printImageGeneric(filename);  // Will try imgcat first
    } else {
        std::cout << "Using generic image display..." << std::endl;
        printImageGeneric(filename);
    }
}