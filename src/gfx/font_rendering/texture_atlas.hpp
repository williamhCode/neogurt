#pragma once
#include "gfx/render_texture.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "utils/region.hpp"
#include "webgpu/webgpu_cpp.h"
#include <cstdint>
#include <vector>
#include <mdspan>
#include <print>

enum class GlyphFormat {
  BGRA8,
  A8,
  A32,
};

template <typename T, typename ElementType>
concept MdSpan2D =
  std::same_as<typename T::element_type, ElementType> && (T::extents_type::rank() == 2);

template <GlyphFormat Format, typename DataType>
concept ValidGlyphDataType =
    (Format == GlyphFormat::BGRA8 && MdSpan2D<DataType, uint32_t>) ||
    (Format == GlyphFormat::A8 && MdSpan2D<DataType, uint8_t>) ||
    (Format == GlyphFormat::A32 && MdSpan2D<DataType, uint32_t>);

// texture atlas for storing glyphs
// width is constant, size expands vertically
struct TextureAtlas {
  // bufferSize.x = glyphsPerRow * glyphSize
  static constexpr int glyphsPerRow = 16;

  float dpiScale;
  int trueGlyphSize; // only used for approximate scale, no precision needed
  glm::vec2 textureSize; // virtual size of texture (used in shader)
  glm::uvec2 bufferSize; // size of texture in texels

  // texture data
  struct Color {
    uint8_t r, g, b, a;
  };
  std::vector<Color> dataRaw;
  std::mdspan<Color, std::dextents<size_t, 2>> data;
  bool dirty = false;

  glm::uvec2 currentPos = {0, 0};
  // max height of glyph in current row
  // advance pos.y by this value when moving to next row
  int currMaxHeight = 0;

  wgpu::Buffer textureSizeBuffer;
  wgpu::BindGroup textureSizeBG;
  RenderTexture renderTexture;
  bool resized = false;

  TextureAtlas() = default;
  TextureAtlas(float glyphSize, float dpiScale);

  // Adds data to texture atlas, and returns the region where the data was added.
  // Region coordinates is relative to textureSize.
  template <GlyphFormat Format, typename DataType>
    requires ValidGlyphDataType<Format, DataType>
  Region AddGlyph(DataType glyphData);

  // Resize cpu side data and sizes
  void Resize();
  // Resize gpu side data and update bind group
  void Update();
};

template <GlyphFormat Format, typename DataType>
  requires ValidGlyphDataType<Format, DataType>
Region TextureAtlas::AddGlyph(DataType glyphData) {
  // check if current row is full
  // if so, move to next row
  if (currentPos.x + glyphData.extent(1) > bufferSize.x) {
    currentPos.x = 0;
    currentPos.y += currMaxHeight;
    currMaxHeight = 0;
  }
  if (currentPos.y + glyphData.extent(0) > bufferSize.y) {
    Resize();
  }

  // fill data
  if constexpr (Format == GlyphFormat::BGRA8) {
    for (size_t row = 0; row < glyphData.extent(0); row++) {
      for (size_t col = 0; col < glyphData.extent(1); col++) {
        Color& dest = data[currentPos.y + row, currentPos.x + col];
        const uint32_t pixel = glyphData[row, col];
        dest.b = pixel & 0xFF;
        dest.g = (pixel >> 8) & 0xFF;
        dest.r = (pixel >> 16) & 0xFF;
        dest.a = (pixel >> 24) & 0xFF;
      }
    }

  } else {
    for (size_t row = 0; row < glyphData.extent(0); row++) {
      for (size_t col = 0; col < glyphData.extent(1); col++) {
        Color& dest = data[currentPos.y + row, currentPos.x + col];
        dest.r = 255;
        dest.g = 255;
        dest.b = 255;
        if constexpr (Format == GlyphFormat::A8) {
          dest.a = glyphData[row, col];
        } else if constexpr (Format == GlyphFormat::A32) {
          dest.a = (glyphData[row, col] >> 24) & 0xFF;
        }
      }
    }
  }
  dirty = true;

  // calculate region
  auto regionPos = glm::vec2(currentPos) / dpiScale;
  auto regionSize = glm::vec2(glyphData.extent(1), glyphData.extent(0)) / dpiScale;

  // advance current position
  currentPos.x += glyphData.extent(1);
  currMaxHeight = std::max((size_t)currMaxHeight, glyphData.extent(0));

  return MakeRegion(regionPos, regionSize);
}
