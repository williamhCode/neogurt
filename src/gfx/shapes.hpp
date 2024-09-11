#pragma once

#include "gfx/pipeline.hpp"
#include "gfx/quad.hpp"
#include "utils/region.hpp"

// little helper to add a quad to the shapes render data
template <bool Dynamic>
inline void AddShapeQuad(
  QuadRenderData<ShapeQuadVertex, Dynamic>& data,
  const Rect& rect,
  const glm::vec4& color,
  uint32_t shapeType
) {
  auto quadPoss = rect.Region();
  auto quadSize = rect.size;
  auto coords = MakeRegion({0, 0}, quadSize);

  auto& quad = data.NextQuad();
  for (size_t i = 0; i < 4; i++) {
    quad[i].position = quadPoss[i];
    quad[i].size = quadSize;
    quad[i].coord = coords[i];
    quad[i].color = color;
    quad[i].shapeType = shapeType;
  }
}
