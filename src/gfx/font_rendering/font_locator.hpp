#pragma once
#include "./font_descriptor.hpp"
#include <string>

// returns empty string if font not found
std::string GetFontPathFromName(const FontDescriptorWithName& desc);
std::string FindFallbackFontForCharacter(const std::string& text);
// returns the CoreText family name for the font at the given path
std::string GetFontFamilyName(const std::string& path);

// std::string GetFontPathFromFamilyAndStyle(const FontDescriptor& desc);
