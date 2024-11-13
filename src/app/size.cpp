#include "size.hpp"
#include "glm/common.hpp"
#include "app/options.hpp"

void SizeHandler::UpdateSizes(
  glm::vec2 _size, float _dpiScale, glm::vec2 _charSize, const Options& options
) {
  size = _size;
  dpiScale = _dpiScale;
  charSize = _charSize;

  fbSize = size * dpiScale;
  charFbSize = charSize * dpiScale;

  glm::vec2 innerSize{
    size.x - options.marginLeft - options.marginRight,
    size.y - options.marginTop - options.marginBottom,
  };
  auto uiWidthHeight = glm::floor(innerSize / charSize);
  uiWidth = uiWidthHeight.x;
  uiHeight = uiWidthHeight.y;

  uiSize = uiWidthHeight * charSize;
  uiFbSize = uiSize * dpiScale;

  // offset = (size - uiSize) / 2.0f;
  offset = {options.marginLeft, options.marginTop};
  offset = glm::round(offset * dpiScale) / dpiScale;
  fbOffset = offset * dpiScale;
}
