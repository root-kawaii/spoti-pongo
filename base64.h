#pragma once
#include <string>
#include <cstdint>
#include <iostream>
#include <string>

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string base64_encode(const std::string& input);
std::string base64_encode_file(const std::string& filename);
void printImageToKittyTerminal(const std::string& filename);
void printImageGeneric(const std::string &filename);
void printImageToTerminal(const std::string &filename);

