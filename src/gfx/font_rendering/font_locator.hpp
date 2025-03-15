#pragma once
#include "./font_descriptor.hpp"

// returns empty string if font not found
std::string GetFontPathFromName(const FontDescriptorWithName& desc);
std::string FindFallbackFontForCharacter(uint32_t unicodeChar);

// std::string GetFontPathFromFamilyAndStyle(const FontDescriptor& desc);
