#include "font_handle.hpp"
#include "gfx/font/locator.hpp"
#include <unordered_map>

std::unordered_map<std::string, std::weak_ptr<Font>> fontCache;

std::expected<FontHandle, std::string>
MakeFontHandle(const std::string& path, int size, int width, float dpiScale) {
  auto it = fontCache.find(path);
  if (it != fontCache.end()) {
    auto font = it->second.lock();
    if (font) {
      return font;
    }
  }

  auto font = std::make_shared<Font>(path, size, width, dpiScale);
  fontCache[path] = font;

  return font;
}

std::expected<FontHandle, std::string>
FontHandleFromName(const FontDescriptorWithName& desc, float dpiScale) {
  auto fontPath = GetFontPathFromName(desc);
  if (fontPath.empty()) {
    return std::unexpected("Failed to find font for: " + desc.name);
  }

  try {
    return MakeFontHandle(fontPath, desc.size, desc.width, dpiScale);
  } catch (const std::exception& e) {
    return std::unexpected(e.what());
  }
}
