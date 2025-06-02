#include <string>
#include <cstdint>
#include <iostream>
#include <string>
#include <fstream>
#include "base64.h"
#include <sstream>


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


std::string base64_encode_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    std::ostringstream oss;
    oss << file.rdbuf();
    std::string data = oss.str();

    static const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string ret;
    int val=0, valb=-6;
    for (uint8_t c : data) {
        val = (val<<8) + c;
        valb += 8;
        while (valb>=0) {
            ret.push_back(base64_chars[(val>>valb)&0x3F]);
            valb -= 6;
        }
    }
    if (valb>-6) ret.push_back(base64_chars[((val<<8)>>(valb+8))&0x3F]);
    while (ret.size()%4) ret.push_back('=');
    return ret;
}

void printImageToKittyTerminal(const std::string& filename) {
    std::cout << "ciao\n";
    std::string encoded = base64_encode_file(filename);
    std::cout << "\033_Gf=100,t=f,a=T;" << encoded << "\033\\";
}

