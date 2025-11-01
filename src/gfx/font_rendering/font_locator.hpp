#pragma once
#include "./font_descriptor.hpp"
#include <string>

// returns empty string if font not found
std::string GetFontPathFromName(const FontDescriptorWithName& desc);
std::string FindFallbackFontForCharacter(const std::string& text);

// std::string GetFontPathFromFamilyAndStyle(const FontDescriptor& desc);
