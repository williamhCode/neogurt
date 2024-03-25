#include "render_texture.hpp"

#include "utils/region.hpp"
#include "webgpu_utils/webgpu.hpp"
#include "gfx/instance.hpp"

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_clip_space.hpp"

using namespace wgpu;

RenderTexture::RenderTexture(glm::uvec2 size, wgpu::TextureFormat format) {
  auto view = glm::ortho<float>(0, size.x, size.y, 0, -1, 1);
  viewProjBuffer = utils::CreateUniformBuffer(ctx.device, sizeof(view), &view);
  viewProjBG =
    utils::MakeBindGroup(ctx.device, ctx.pipeline.viewProjBGL, {{0, viewProjBuffer}});

  texture = utils::CreateRenderTexture(ctx.device, {size.x, size.y}, format);
  textureView = texture.CreateView();

  textureData.CreateBuffers(1);
  textureData.ResetCounts();
  auto positions = MakeRegion(0, 0, size.x, size.y);
  auto uvs = MakeRegion(0, 0, 1, 1);
  for (size_t i = 0; i < 4; i++) {
    auto& vertex = textureData.quads[0][i];
    vertex.position = positions[i];
    vertex.uv = uvs[i];
  }
  textureData.Increment();
  textureData.WriteBuffers();

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
}

void RenderTexture::Resize(glm::uvec2 size) {
  textureView =
    utils::CreateRenderTexture(ctx.device, {size.x, size.y}, texture.GetFormat())
      .CreateView();
}
