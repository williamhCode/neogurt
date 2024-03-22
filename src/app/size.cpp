#include "size.hpp"
#include "glm/common.hpp"

void SizeHandler::UpdateSizes(glm::vec2 _size, float dpiScale, glm::vec2 _charSize) {
  size = _size;
  charSize = _charSize;

  fbSize = size * dpiScale;

  uiWidthHeight = glm::floor(size / charSize);

  uiSize = uiWidthHeight * charSize;
  uiFbSize = uiSize * dpiScale;
}

std::tuple<int, int> SizeHandler::GetUiWidthHeight() {
  return {uiWidthHeight.x, uiWidthHeight.y};
}
