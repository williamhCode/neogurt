#include "render_texture.hpp"

#include "webgpu_tools/utils/webgpu.hpp"
#include "utils/region.hpp"
#include "gfx/instance.hpp"

using namespace wgpu;

RenderTexture::RenderTexture(
  glm::vec2 _size, float dpiScale, wgpu::TextureFormat format
)
    : size(_size) {
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
}

void RenderTexture::UpdateRegion(glm::vec2 pos, std::optional<RegionHandle> region) {
  renderData.ResetCounts();

  Region positions;
  Region uvs;

  if (region == std::nullopt) {
    positions = MakeRegion({0, 0}, size);
    uvs = MakeRegion(0, 0, 1, 1);
  } else {
    positions = region->Get();
    region->pos /= size, region->size /= size;
    uvs = region->Get();
  }

  for (size_t i = 0; i < 4; i++) {
    auto& vertex = renderData.CurrQuad()[i];
    vertex.position = pos + positions[i];
    vertex.uv = uvs[i];
  }
  renderData.Increment();
  renderData.WriteBuffers();
}
