#include "./size.hpp"
#include "glm/common.hpp"

void SizeHandler::UpdateSizes(
  const sdl::Window& window, glm::vec2 _charSize, const GlobalOptions& globalOpts
) {
  size = window.size;
  dpiScale = window.dpiScale;
  charSize = _charSize;
  int marginTop = globalOpts.marginTop + window.titlebarHeight;
  int marginBottom = globalOpts.marginBottom;
  int marginLeft = globalOpts.marginLeft;
  int marginRight = globalOpts.marginRight;

  fbSize = size * dpiScale;
  charFbSize = charSize * dpiScale;

  glm::vec2 innerSize{
    size.x - marginLeft - marginRight,
    size.y - marginTop - marginBottom,
  };
  auto uiWidthHeight = glm::floor(innerSize / charSize);
  uiWidth = uiWidthHeight.x;
  uiHeight = uiWidthHeight.y;

  uiSize = uiWidthHeight * charSize;
  uiFbSize = uiSize * dpiScale;

  // offset = (size - uiSize) / 2.0f;
  offset = {marginLeft, marginTop};
  offset = glm::round(offset * dpiScale) / dpiScale;
  fbOffset = offset * dpiScale;
}
