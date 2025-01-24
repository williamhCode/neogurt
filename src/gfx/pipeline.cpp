#include "./pipeline.hpp"
#include "webgpu_utils/blend.hpp"
#include "gfx/context.hpp"
#include "app/path.hpp"

using namespace wgpu;

Pipeline::Pipeline(const WGPUContext& ctx) {
  // shared ------------------------------------------------
  viewProjBGL = ctx.MakeBindGroupLayout({
    {0, ShaderStage::Vertex | ShaderStage::Fragment, BufferBindingType::Uniform},
  });

  textureBGL = ctx.MakeBindGroupLayout({
    {0, ShaderStage::Fragment, TextureSampleType::UnfilterableFloat},
    {1, ShaderStage::Fragment, SamplerBindingType::NonFiltering},
  });

  gammaBGL = ctx.MakeBindGroupLayout({
    {0, ShaderStage::Vertex | ShaderStage::Fragment, BufferBindingType::Uniform},
  });

  // shapes pipeline ---------------------------------------------
  ShaderModule shapesShader = ctx.LoadShaderModule(resourcesDir + "/shaders/shapes.wgsl");

  shapesRPL = ctx.MakeRenderPipeline({
    .vs = shapesShader,
    .fs = shapesShader,
    .bgls = {viewProjBGL, gammaBGL},
    .buffers = {
      {
        sizeof(ShapeQuadVertex),
        {
          {VertexFormat::Float32x2, offsetof(ShapeQuadVertex, position)},
          {VertexFormat::Float32x2, offsetof(ShapeQuadVertex, size)},
          {VertexFormat::Float32x2, offsetof(ShapeQuadVertex, coord)},
          {VertexFormat::Float32x4, offsetof(ShapeQuadVertex, color)},
          {VertexFormat::Uint32, offsetof(ShapeQuadVertex, shapeType)},
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

  // rect pipeline -------------------------------------------
  ShaderModule rectShader = ctx.LoadShaderModule(resourcesDir + "/shaders/rect.wgsl");

  rectRPL = ctx.MakeRenderPipeline({
    .vs = rectShader,
    .fs = rectShader,
    .bgls = {viewProjBGL, gammaBGL},
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
  ShaderModule textShader = ctx.LoadShaderModule(resourcesDir + "/shaders/text.wgsl");

  textureSizeBGL = ctx.MakeBindGroupLayout({
    {0, ShaderStage::Vertex, BufferBindingType::Uniform},
  });

  textRPL = ctx.MakeRenderPipeline({
    .vs = textShader,
    .fs = textShader,
    .bgls = {viewProjBGL, gammaBGL, textureSizeBGL, textureBGL},
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
  ShaderModule textMaskShader = ctx.LoadShaderModule(resourcesDir + "/shaders/text_mask.wgsl");

  textMaskRPL = ctx.MakeRenderPipeline({
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
  ShaderModule textureShader = ctx.LoadShaderModule(resourcesDir + "/shaders/texture.wgsl");

  utils::VertexBufferLayout textureQuadVBL{
    .arrayStride = sizeof(TextureQuadVertex),
    .attributes = {
      {VertexFormat::Float32x2, offsetof(TextureQuadVertex, position)},
      {VertexFormat::Float32x2, offsetof(TextureQuadVertex, uv)},
    }
  };

  defaultColorBGL = ctx.MakeBindGroupLayout({
    {0, ShaderStage::Fragment, BufferBindingType::Uniform},
  });

  textureNoBlendRPL = ctx.MakeRenderPipeline({
    .vs = textureShader,
    .fs = textureShader,
    .bgls = {viewProjBGL, gammaBGL, defaultColorBGL, textureBGL},
    .buffers = {textureQuadVBL},
    .targets =
      {
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
        .passOp = StencilOperation::Replace,
      },
      .stencilReadMask = 0b001,
      .stencilWriteMask = 0b011,
    },
  });

  textureRPL = ctx.MakeRenderPipeline({
    .vs = textureShader,
    .fs = textureShader,
    .bgls = {viewProjBGL, gammaBGL, defaultColorBGL, textureBGL},
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
        .passOp = StencilOperation::Replace,
      },
      .stencilReadMask = 0b110, 
      .stencilWriteMask = 0b010,
    },
  });

  // final texture pipeline -------------------------------------
  ShaderModule textureFinalShader = ctx.LoadShaderModule(resourcesDir + "/shaders/texture_final.wgsl");

  textureFinalRPL = ctx.MakeRenderPipeline({
    .vs = textureFinalShader,
    .fs = textureFinalShader,
    .bgls = {viewProjBGL, gammaBGL, textureBGL},
    .buffers = {textureQuadVBL},
    .targets = {{.format = TextureFormat::BGRA8Unorm}},
  });

  // cursor pipeline ------------------------------------------------
  ShaderModule cursorShader = ctx.LoadShaderModule(resourcesDir + "/shaders/cursor.wgsl");

  utils::VertexBufferLayout cursorQuadVBL{
    sizeof(CursorQuadVertex),
    {
      {VertexFormat::Float32x2, offsetof(CursorQuadVertex, position)},
      {VertexFormat::Float32x4, offsetof(CursorQuadVertex, foreground)},
      {VertexFormat::Float32x4, offsetof(CursorQuadVertex, background)},
    }
  };

  cursorMaskPosBGL = ctx.MakeBindGroupLayout({
    {0, ShaderStage::Vertex, BufferBindingType::Uniform},
  });

  cursorRPL = ctx.MakeRenderPipeline({
    .vs = cursorShader,
    .fs = cursorShader,
    .bgls = {viewProjBGL, viewProjBGL, cursorMaskPosBGL, textureBGL},
    .buffers = {cursorQuadVBL},
    .targets = {{.format = TextureFormat::BGRA8Unorm}},
  });
}
