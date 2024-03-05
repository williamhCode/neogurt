#pragma once 

#include <string>

std::string UnicodeToUTF8(uint32_t unicode);

uint32_t UTF8ToUnicode(const std::string& utf8String);
