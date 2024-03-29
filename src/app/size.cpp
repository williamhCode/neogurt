#include "size.hpp"
#include "glm/common.hpp"

void SizeHandler::UpdateSizes(glm::vec2 _size, float _dpiScale, glm::vec2 _charSize) {
  size = _size;
  dpiScale = _dpiScale;
  charSize = _charSize;

  fbSize = size * dpiScale;

  auto uiWidthHeight = glm::floor(size / charSize);
  uiWidth = uiWidthHeight.x;
  uiHeight = uiWidthHeight.y;

  uiSize = uiWidthHeight * charSize;
  uiFbSize = uiSize * dpiScale;

  offset = (size - uiSize) / 2.0f;
  offset = glm::round(offset * dpiScale) / dpiScale;
  fbOffset = offset * dpiScale;
}
