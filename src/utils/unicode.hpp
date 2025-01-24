#pragma once 

#include <string>
#include <vector>

std::string Char32ToUTF8(char32_t unicode);
char32_t UTF8ToChar32(const std::string& utf8String);
std::vector<std::string> SplitUTF8(const std::string& input);

