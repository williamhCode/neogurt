#pragma once 

#include <string>

std::string Char32ToUTF8(char32_t unicode);

char32_t UTF8ToChar32(const std::string& utf8String);
