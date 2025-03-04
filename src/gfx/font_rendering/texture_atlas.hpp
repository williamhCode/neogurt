#pragma once
#include "gfx/render_texture.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "utils/region.hpp"
#include "webgpu/webgpu_cpp.h"
#include <cstdint>
#include <vector>
#include <mdspan>
#include <print>

template<class T, template<class...> class U>
inline constexpr bool is_instance_of_v = std::false_type{};

template<template<class...> class U, class... Vs>
inline constexpr bool is_instance_of_v<U<Vs...>,U> = std::true_type{};

template <typename T>
concept MdSpan2D = is_instance_of_v<T, std::mdspan> && T::extents_type::rank() == 2;

// texture atlas for storing glyphs
// width is constant, size expands vertically
template<bool IsColor>
struct TextureAtlas {
  static constexpr int glyphsPerRow = 32;

  float dpiScale;
  int trueHeight; // only used for approximate scale, no precision needed
  glm::vec2 textureSize; // virtual size of texture (used in shader)
  glm::uvec2 bufferSize; // size of texture in texels

  // texture data
  struct PixelRGBA {
    uint8_t r, g, b, a;
  };
  struct PixelR {
    uint8_t r;
  };
  using Pixel = std::conditional_t<IsColor, PixelRGBA, PixelR>;
  std::vector<Pixel> dataRaw;
  std::mdspan<Pixel, std::dextents<size_t, 2>> data;
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

  Region AddGlyph(MdSpan2D auto glyphData);

  // Resize cpu side data and sizes
  void Resize();
  // Resize gpu side data and update bind group
  void Update();
};


template <bool IsColor>
Region TextureAtlas<IsColor>::AddGlyph(MdSpan2D auto glyphData) {
  using ElementType = typename decltype(glyphData)::element_type;

  // check if current row is full
  // if so, move to next row
  if (currentPos.x + glyphData.extent(1) > bufferSize.x) {
    currentPos.x = 0;
    currentPos.y += currMaxHeight;
    currMaxHeight = 0;
  }

  // if current row is full and there's not enough space
  if (currentPos.y + glyphData.extent(0) > bufferSize.y) {
    Resize();
  }

  // fill data
  if constexpr (IsColor) {
    for (size_t row = 0; row < glyphData.extent(0); row++) {
      for (size_t col = 0; col < glyphData.extent(1); col++) {
        Pixel& dest = data[currentPos.y + row, currentPos.x + col];
        if constexpr (std::is_same_v<ElementType, uint32_t>) {
          const uint32_t pixel = glyphData[row, col];
          dest.b = pixel & 0xFF;
          dest.g = (pixel >> 8) & 0xFF;
          dest.r = (pixel >> 16) & 0xFF;
          dest.a = (pixel >> 24) & 0xFF;
        } else {
          static_assert(false, "Unsupported glyph data type");
        }
      }
    }

  } else {
    for (size_t row = 0; row < glyphData.extent(0); row++) {
      for (size_t col = 0; col < glyphData.extent(1); col++) {
        Pixel& dest = data[currentPos.y + row, currentPos.x + col];
        if constexpr (std::is_same_v<ElementType, uint8_t>) {
          dest.r = glyphData[row, col];
        } else if constexpr (std::is_same_v<ElementType, uint32_t>) {
          dest.r = (glyphData[row, col] >> 24) & 0xFF;
        } else {
          static_assert(false, "Unsupported glyph data type");
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
