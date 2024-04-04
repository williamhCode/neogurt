#include "render_texture.hpp"

#include "webgpu_tools/utils/webgpu.hpp"
#include "utils/region.hpp"
#include "gfx/instance.hpp"

using namespace wgpu;

RenderTexture::RenderTexture(
  glm::vec2 pos, glm::vec2 size, float dpiScale, wgpu::TextureFormat format
) {
  camera = Ortho2D(size);

  Extent3D textureSize{
    static_cast<uint32_t>(size.x * dpiScale), static_cast<uint32_t>(size.y * dpiScale)
  };
  texture = utils::CreateRenderTexture(ctx.device, textureSize, format);
  textureView = texture.CreateView();

  auto textureSampler = ctx.device.CreateSampler(ToPtr(SamplerDescriptor{
    .addressModeU = AddressMode::ClampToEdge,
    .addressModeV = AddressMode::ClampToEdge,
    .magFilter = FilterMode::Nearest,
    .minFilter = FilterMode::Nearest,
  }));

  textureBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.textureBGL,
    {
      {0, textureView},
      {1, textureSampler},
    }
  );

  renderData.CreateBuffers(1);
  UpdateRegion(pos, size);
}

void RenderTexture::Update(glm::vec2 pos, glm::vec2 size, float dpiScale) {
  camera.Resize(size);

  Extent3D textureSize{
    static_cast<uint32_t>(size.x * dpiScale), static_cast<uint32_t>(size.y * dpiScale)
  };
  texture = utils::CreateRenderTexture(ctx.device, textureSize, texture.GetFormat());
  textureView = texture.CreateView();

  auto textureSampler = ctx.device.CreateSampler(ToPtr(SamplerDescriptor{
    .addressModeU = AddressMode::ClampToEdge,
    .addressModeV = AddressMode::ClampToEdge,
    .magFilter = FilterMode::Nearest,
    .minFilter = FilterMode::Nearest,
  }));

  textureBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.textureBGL,
    {
      {0, textureView},
      {1, textureSampler},
    }
  );

  UpdateRegion(pos, size);
}

void RenderTexture::UpdateRegion(glm::vec2 pos, glm::vec2 size) {
  renderData.ResetCounts();
  auto positions = MakeRegion(pos, size);
  auto uvs = MakeRegion(0, 0, 1, 1);
  for (size_t i = 0; i < 4; i++) {
    auto& vertex = renderData.CurrQuad()[i];
    vertex.position = positions[i];
    vertex.uv = uvs[i];
  }
  renderData.Increment();
  renderData.WriteBuffers();
}
