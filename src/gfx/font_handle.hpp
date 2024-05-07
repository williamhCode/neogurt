#pragma once
#include "font.hpp"
#include <expected>

using FontHandle = std::shared_ptr<Font>;

std::expected<FontHandle, std::string>
MakeFontHandle(const std::string& path, int size, int width, float dpiScale);

std::expected<FontHandle, std::string>
FontHandleFromName(const FontDescriptorWithName& desc, float dpiScale);


