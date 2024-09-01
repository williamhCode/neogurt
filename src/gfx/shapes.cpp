#include "shapes.hpp"
#include "gfx/instance.hpp"
#include "webgpu_tools/utils/webgpu.hpp"

using namespace wgpu;

ShapesManager::ShapesManager(glm::vec2 charSize, float dpiScale) {
  glm::vec2 textureSize{
    charSize.x * 5,
    charSize.y * 1.5,
  };

  textureSizeBuffer =
    utils::CreateUniformBuffer(ctx.device, sizeof(glm::vec2), &textureSize);

  textureSizeBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.textureSizeBGL,
    {
      {0, textureSizeBuffer},
    }
  );

  renderTexture = RenderTexture(textureSize, dpiScale, TextureFormat::RGBA8Unorm);

  // extra height if underline goes below the grid cell
  auto extraCharSize = glm::vec2{charSize.x, charSize.y * 1.5};
  for (int i = 0; i < 5; i++) {
    infoArray[i].localPoss = MakeRegion({0, 0}, extraCharSize);
    infoArray[i].atlasRegion = MakeRegion({charSize.x * i, 0}, extraCharSize);
  }
}
