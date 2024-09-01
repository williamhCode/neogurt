#include "pipeline.hpp"
#include "context.hpp"
#include "utils/logger.hpp"
#include "webgpu_tools/utils/webgpu.hpp"
#include "app/path.hpp"

using namespace wgpu;

Pipeline::Pipeline(const WGPUContext& ctx) {
  // shared ------------------------------------------------
  viewProjBGL = utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Vertex | ShaderStage::Fragment, BufferBindingType::Uniform},
    }
  );

  textureBGL = utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Fragment, TextureSampleType::UnfilterableFloat},
      {1, ShaderStage::Fragment, SamplerBindingType::NonFiltering},
    }
  );

  // shapes pipeline ---------------------------------------------
  ShaderModule shapesShader =
    utils::LoadShaderModule(ctx.device, resourcesDir + "/shaders/shapes.wgsl");

  shapesRPL = utils::MakeRenderPipeline(ctx.device, {
    .vs = shapesShader,
    .fs = shapesShader,
    .bgls = {viewProjBGL},
    .buffers = {
      {
        sizeof(ShapeQuadVertex),
        {
          {VertexFormat::Float32x2, offsetof(ShapeQuadVertex, position)},
          {VertexFormat::Float32x2, offsetof(ShapeQuadVertex, coord)},
          {VertexFormat::Uint32, offsetof(ShapeQuadVertex, shapeType)},
        }
      }
    },
    .targets = {{.format = TextureFormat::RGBA8Unorm}},
  });

  // rect pipeline -------------------------------------------
  ShaderModule rectShader =
    utils::LoadShaderModule(ctx.device, resourcesDir + "/shaders/rect.wgsl");

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
    .targets = {{.format = TextureFormat::RGBA8UnormSrgb}},
  });

  // text pipeline -------------------------------------------
  ShaderModule textShader =
    utils::LoadShaderModule(ctx.device, resourcesDir + "/shaders/text.wgsl");

  textureSizeBGL = utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Vertex, BufferBindingType::Uniform},
    }
  );

  textRPL = utils::MakeRenderPipeline(ctx.device, {
    .vs = textShader,
    .fs = textShader,
    .bgls = {viewProjBGL, textureSizeBGL, textureBGL},
    .buffers = {
      {
        sizeof(TextQuadVertex),
        {
          {VertexFormat::Float32x2, offsetof(TextQuadVertex, position)},
          {VertexFormat::Float32x2, offsetof(TextQuadVertex, regionCoord)},
          {VertexFormat::Float32x4, offsetof(TextQuadVertex, foreground)},
        }
      }
    },
    .targets = {
      {
        .format = TextureFormat::RGBA8UnormSrgb,
        .blend = &utils::BlendState::AlphaBlending,
      },
    },
  });

  // mask
  ShaderModule textMaskShader =
    utils::LoadShaderModule(ctx.device, resourcesDir + "/shaders/text_mask.wgsl");

  textMaskRPL = utils::MakeRenderPipeline(ctx.device, {
    .vs = textMaskShader,
    .fs = textMaskShader,
    .bgls = {viewProjBGL, textureSizeBGL, textureBGL},
    .buffers = {
      {
        sizeof(TextMaskQuadVertex),
        {
          {VertexFormat::Float32x2, offsetof(TextMaskQuadVertex, position)},
          {VertexFormat::Float32x2, offsetof(TextMaskQuadVertex, regionCoord)},
        }
      }
    },
    .targets = {
      { .format = TextureFormat::R8Unorm },
    },
  });

  // texture pipeline ------------------------------------------------
  ShaderModule textureShader =
    utils::LoadShaderModule(ctx.device, resourcesDir + "/shaders/texture.wgsl");

  utils::VertexBufferLayout textureQuadVBL{
    .arrayStride = sizeof(TextureQuadVertex),
    .attributes = {
      {VertexFormat::Float32x2, offsetof(TextureQuadVertex, position)},
      {VertexFormat::Float32x2, offsetof(TextureQuadVertex, uv)},
    }
  };

  defaultColorBGL = utils::MakeBindGroupLayout(
    ctx.device, {{0, ShaderStage::Fragment, BufferBindingType::Uniform}}
  );

  textureNoBlendRPL = utils::MakeRenderPipeline(ctx.device, {
    .vs = textureShader,
    .fs = textureShader,
    .bgls = {viewProjBGL, textureBGL, defaultColorBGL},
    .buffers = {textureQuadVBL},
    .targets = {
      {
        .format = TextureFormat::RGBA8UnormSrgb,
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

  textureRPL = utils::MakeRenderPipeline(ctx.device, {
    .vs = textureShader,
    .fs = textureShader,
    .bgls = {viewProjBGL, textureBGL, defaultColorBGL},
    .buffers = {textureQuadVBL},
    .targets = {
      {
        .format = TextureFormat::RGBA8UnormSrgb,
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

  // final texture pipeline -------------------------------------
  ShaderModule textureFinalShader = utils::LoadShaderModule(
    ctx.device, resourcesDir + "/shaders/texture_final.wgsl"
  );

  textureFinalRPL = utils::MakeRenderPipeline(ctx.device, {
    .vs = textureFinalShader,
    .fs = textureFinalShader,
    .bgls = {viewProjBGL, textureBGL},
    .buffers = {textureQuadVBL},
    .targets = {{.format = TextureFormat::BGRA8Unorm}},
  });

  // ShaderModule textureFinalBorderlessShader = utils::LoadShaderModule(
  //   ctx.device, resourcesDir + "/shaders/texture_final_borderless.wgsl"
  // );

  // textureFinalBorderlessRPL = utils::MakeRenderPipeline(ctx.device, {
  //   .vs = textureFinalBorderlessShader,
  //   .fs = textureFinalBorderlessShader,
  //   .bgls = {viewProjBGL, textureBGL},
  //   .buffers = {textureQuadVBL},
  //   .targets = {{.format = TextureFormat::BGRA8Unorm}},
  // });

  // cursor pipeline ------------------------------------------------
  ShaderModule cursorShader =
    utils::LoadShaderModule(ctx.device, resourcesDir + "/shaders/cursor.wgsl");

  utils::VertexBufferLayout cursorQuadVBL{
    sizeof(CursorQuadVertex),
    {
      {VertexFormat::Float32x2, offsetof(CursorQuadVertex, position)},
      {VertexFormat::Float32x4, offsetof(CursorQuadVertex, foreground)},
      {VertexFormat::Float32x4, offsetof(CursorQuadVertex, background)},
    }
  };

  cursorMaskPosBGL = utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Vertex, BufferBindingType::Uniform},
    }
  );

  cursorRPL = utils::MakeRenderPipeline(ctx.device, {
    .vs = cursorShader,
    .fs = cursorShader,
    .bgls = {viewProjBGL, viewProjBGL, cursorMaskPosBGL, textureBGL},
    .buffers = {cursorQuadVBL},
    .targets = {{.format = TextureFormat::BGRA8Unorm}},
  });

  // post process pipeline ------------------------------------------------
  // ShaderModule postProcessShader =
  //   utils::LoadShaderModule(ctx.device, resourcesDir + "/shaders/post_process.wgsl");

  // postProcessRPL = utils::MakeRenderPipeline(ctx.device, {
  //   .vs = postProcessShader,
  //   .fs = postProcessShader,
  //   .bgls = {viewProjBGL, textureBGL},
  //   .buffers = {textureQuadVBL},
  //   .targets = {{.format = TextureFormat::BGRA8Unorm}},
  // });
}
