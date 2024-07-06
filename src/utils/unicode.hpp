#pragma once 

#include <string>

std::string UnicodeToUTF8(char32_t unicode);

char32_t UTF8ToUnicode(const std::string& utf8String);
