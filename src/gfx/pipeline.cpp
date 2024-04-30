#include "pipeline.hpp"
#include "context.hpp"
#include "webgpu_tools/utils/webgpu.hpp"

using namespace wgpu;

Pipeline::Pipeline(const WGPUContext& ctx) {
  // shared ------------------------------------------------
  viewProjBGL = utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Vertex, BufferBindingType::Uniform},
    }
  );

  // rect pipeline -------------------------------------------
  ShaderModule rectShader =
    utils::LoadShaderModule(ctx.device, ROOT_DIR "/src/gfx/shaders/rect.wgsl");

  rectRPL = utils::MakeRenderPipeline(ctx.device, {
    .vs = rectShader,
    .fs = rectShader,
    .bgls = {viewProjBGL},
    .buffers = {
      {
        .arrayStride = sizeof(RectQuadVertex),
        .attributes = {
          {VertexFormat::Float32x2, offsetof(RectQuadVertex, position)},
          {VertexFormat::Float32x4, offsetof(RectQuadVertex, color)},
        }
      }
    },
    .targets = {{.format = TextureFormat::RGBA8Unorm}},
  });

  // text pipeline -------------------------------------------
  ShaderModule textShader =
    utils::LoadShaderModule(ctx.device, ROOT_DIR "/src/gfx/shaders/text.wgsl");

  fontTextureBGL = utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Vertex, BufferBindingType::Uniform},
      {1, ShaderStage::Fragment, TextureSampleType::UnfilterableFloat},
      {2, ShaderStage::Fragment, SamplerBindingType::NonFiltering},
    }
  );

  textRPL = utils::MakeRenderPipeline(ctx.device, {
    .vs = textShader,
    .fs = textShader,
    .bgls = {viewProjBGL, fontTextureBGL},
    .buffers = {
      {
        sizeof(TextQuadVertex), {
          {VertexFormat::Float32x2, offsetof(TextQuadVertex, position)},
          {VertexFormat::Float32x2, offsetof(TextQuadVertex, regionCoords)},
          {VertexFormat::Float32x4, offsetof(TextQuadVertex, foreground)},
        }
      }
    },
    .targets = {
      {
        .format = TextureFormat::RGBA8Unorm,
        .blend = &utils::BlendState::AlphaBlending,
      },
      {.format = TextureFormat::R8Unorm},
    },
  });

  // texture pipeline ------------------------------------------------
  ShaderModule textureShader =
    utils::LoadShaderModule(ctx.device, ROOT_DIR "/src/gfx/shaders/texture.wgsl");

  utils::VertexBufferLayout textureQuadVBL{
    .arrayStride = sizeof(TextureQuadVertex),
    .attributes = {
      {VertexFormat::Float32x2, offsetof(TextureQuadVertex, position)},
      {VertexFormat::Float32x2, offsetof(TextureQuadVertex, uv)},
    }
  };

  textureBGL = utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Fragment, TextureSampleType::UnfilterableFloat},
      {1, ShaderStage::Fragment, SamplerBindingType::NonFiltering},
    }
  );

  textureRPL = utils::MakeRenderPipeline(ctx.device, {
    .vs = textureShader,
    .fs = textureShader,
    .bgls = {viewProjBGL, textureBGL},
    .buffers = {textureQuadVBL},
    .targets = {
      {
        .format = TextureFormat::RGBA8Unorm,
        .blend = &utils::BlendState::AlphaBlending,
      },
    },
    .depthStencil{
      .format = TextureFormat::Stencil8,
      // textures are clockwise, so backface
      .stencilBack{
        .compare = CompareFunction::NotEqual,
        .failOp = StencilOperation::Keep,
        .passOp = StencilOperation::IncrementClamp,
      },
    },
  });

  finalTextureRPL = utils::MakeRenderPipeline(ctx.device, {
    .vs = textureShader,
    .fs = textureShader,
    .bgls = {viewProjBGL, textureBGL},
    .buffers = {textureQuadVBL},
    .targets = {{.format = TextureFormat::BGRA8Unorm}},
  });

  // cursor pipeline ------------------------------------------------
  ShaderModule cursorShader =
    utils::LoadShaderModule(ctx.device, ROOT_DIR "/src/gfx/shaders/cursor.wgsl");

  utils::VertexBufferLayout cursorQuadVBL{
    sizeof(CursorQuadVertex),
    {
      {VertexFormat::Float32x2, offsetof(CursorQuadVertex, position)},
      {VertexFormat::Float32x4, offsetof(CursorQuadVertex, foreground)},
      {VertexFormat::Float32x4, offsetof(CursorQuadVertex, background)},
    }
  };

  maskBGL = utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Fragment, TextureSampleType::UnfilterableFloat},
      {1, ShaderStage::Fragment, BufferBindingType::Uniform},
    }
  );

  maskOffsetBGL = utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Fragment, BufferBindingType::Uniform},
    }
  );

  cursorRPL = utils::MakeRenderPipeline(ctx.device, {
    .vs = cursorShader,
    .fs = cursorShader,
    .bgls = {viewProjBGL, maskBGL, maskOffsetBGL},
    .buffers = {cursorQuadVBL},
    .targets = {{.format = TextureFormat::BGRA8Unorm}},
  });
}
