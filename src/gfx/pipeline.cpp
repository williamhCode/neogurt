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
  utils::VertexBufferLayout rectQuadVBL{
    sizeof(RectQuadVertex),
    {
      {VertexFormat::Float32x2, offsetof(RectQuadVertex, position), 0},
      {VertexFormat::Float32x4, offsetof(RectQuadVertex, color), 1},
    }
  };

  ShaderModule rectShader =
    utils::LoadShaderModule(ctx.device, ROOT_DIR "/src/shaders/rect.wgsl");

  rectRPL = utils::RenderPipelineMaker{
    .layout = utils::MakePipelineLayout(ctx.device, {viewProjBGL}),
    .module = rectShader,
    .buffers{rectQuadVBL},
    .targets{{.format = TextureFormat::RGBA8Unorm}},
  }.Make(ctx.device);

  // text pipeline -------------------------------------------
  utils::VertexBufferLayout textQuadVBL{
    sizeof(TextQuadVertex),
    {
      {VertexFormat::Float32x2, offsetof(TextQuadVertex, position), 0},
      {VertexFormat::Float32x2, offsetof(TextQuadVertex, regionCoords), 1},
      {VertexFormat::Float32x4, offsetof(TextQuadVertex, foreground), 2},
    }
  };

  fontTextureBGL = utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Vertex, BufferBindingType::Uniform},
      {1, ShaderStage::Fragment, TextureSampleType::UnfilterableFloat},
      {2, ShaderStage::Fragment, SamplerBindingType::NonFiltering},
    }
  );

  ShaderModule textShader =
    utils::LoadShaderModule(ctx.device, ROOT_DIR "/src/shaders/text.wgsl");

  textRPL = utils::RenderPipelineMaker{
    .layout = utils::MakePipelineLayout(ctx.device, {viewProjBGL, fontTextureBGL}),
    .module = textShader,
    .buffers{textQuadVBL},
    .targets{
      {
        .format = TextureFormat::RGBA8Unorm,
        .blend = &utils::BlendState::AlphaBlending,
      },
      {.format = TextureFormat::R8Unorm},
    },
  }.Make(ctx.device);

  // texture pipeline ------------------------------------------------
  utils::VertexBufferLayout textureQuadVBL{
    sizeof(TextureQuadVertex),
    {
      {VertexFormat::Float32x2, offsetof(TextureQuadVertex, position), 0},
      {VertexFormat::Float32x2, offsetof(TextureQuadVertex, uv), 1},
    }
  };

  ShaderModule textureShader =
    utils::LoadShaderModule(ctx.device, ROOT_DIR "/src/shaders/texture.wgsl");

  textureBGL = utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Fragment, TextureSampleType::UnfilterableFloat},
      {1, ShaderStage::Fragment, SamplerBindingType::NonFiltering},
    }
  );

  auto textureLayout = utils::MakePipelineLayout(ctx.device, {viewProjBGL, textureBGL});

  textureRPL = utils::RenderPipelineMaker{
    .layout = textureLayout,
    .module = textureShader,
    .buffers{textureQuadVBL},
    .depthStencil{
      .format = TextureFormat::Stencil8,
      // textures are clockwise, so backface
      .stencilBack = {
        .compare = CompareFunction::NotEqual,
        .failOp = StencilOperation::Keep,
        .passOp = StencilOperation::IncrementClamp,
      },
    },
    .targets{
      {
        .format = TextureFormat::RGBA8Unorm,
        .blend = &utils::BlendState::AlphaBlending,
      },
    },
  }.Make(ctx.device);


  finalTextureRPL = utils::RenderPipelineMaker{
    .layout = textureLayout,
    .module = textureShader,
    .buffers{textureQuadVBL},
    .targets{{.format = TextureFormat::BGRA8Unorm}},
  }.Make(ctx.device);

  // cursor pipeline ------------------------------------------------
  utils::VertexBufferLayout cursorQuadVBL{
    sizeof(CursorQuadVertex),
    {
      {VertexFormat::Float32x2, offsetof(CursorQuadVertex, position), 0},
      {VertexFormat::Float32x4, offsetof(CursorQuadVertex, foreground), 1},
      {VertexFormat::Float32x4, offsetof(CursorQuadVertex, background), 2},
    }
  };

  ShaderModule cursorShader =
    utils::LoadShaderModule(ctx.device, ROOT_DIR "/src/shaders/cursor.wgsl");

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

  cursorRPL = utils::RenderPipelineMaker{
    .layout = utils::MakePipelineLayout(
      ctx.device,
      {
        viewProjBGL,
        maskBGL,
        maskOffsetBGL,
      }
    ),
    .module = cursorShader,
    .buffers{cursorQuadVBL},
    .targets{
      {.format = TextureFormat::BGRA8Unorm},
    },
  }.Make(ctx.device);
}
