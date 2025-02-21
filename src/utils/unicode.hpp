#pragma once 

#include <string>
#include <vector>

std::string Char32ToUtf8(char32_t unicode);
char32_t Utf8ToChar32(std::string_view utf8String);
std::vector<std::string> SplitUTF8(const std::string& input);

