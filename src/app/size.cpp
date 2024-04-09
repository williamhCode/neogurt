#include "size.hpp"
#include "glm/common.hpp"
#include "app/options.hpp"

void SizeHandler::UpdateSizes(glm::vec2 _size, float _dpiScale, glm::vec2 _charSize) {
  size = _size;
  dpiScale = _dpiScale;
  charSize = _charSize;

  fbSize = size * dpiScale;

  glm::vec2 innerSize(
    size.x - appOpts.windowMargins.left - appOpts.windowMargins.right,
    size.y - appOpts.windowMargins.top - appOpts.windowMargins.bottom
  );
  auto uiWidthHeight = glm::floor(innerSize / charSize);
  uiWidth = uiWidthHeight.x;
  uiHeight = uiWidthHeight.y;

  uiSize = uiWidthHeight * charSize;
  uiFbSize = uiSize * dpiScale;

  offset = (size - uiSize) / 2.0f;
  offset = glm::round(offset * dpiScale) / dpiScale;
  fbOffset = offset * dpiScale;
}
