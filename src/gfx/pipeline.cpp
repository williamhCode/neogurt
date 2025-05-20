#include "./pipeline.hpp"
#include "webgpu_utils/blend.hpp"
#include "gfx/context.hpp"
#include <string>
#include <vector>

using namespace wgpu;

Pipeline::Pipeline(const WGPUContext& ctx, SlangContext& slang, float gamma) {
  // slang stuff ---------------
  auto loadShaderModule = [&](
    const std::string& moduleName,
    const std::vector<slang::PreprocessorMacroDesc>& macros = {}
  ) {
    auto source = slang.GetModuleSource(moduleName, macros);
    return ctx.LoadShaderModuleSource(source);
  };

  // compile utils
  slang.ClearModuleFiles();
  slang.CompileModuleObject("utils", {{"GAMMA", std::to_string(gamma).c_str()}});

  // shared ------------------------------------------------
  viewProjBGL = ctx.MakeBindGroupLayout({
    {0, ShaderStage::Vertex | ShaderStage::Fragment, BufferBindingType::Uniform},
  });

  textureBGL = ctx.MakeBindGroupLayout({
    {0, ShaderStage::Fragment, TextureSampleType::Float},
    {1, ShaderStage::Fragment, SamplerBindingType::Filtering},
  });

  // rect pipeline -------------------------------------------
  ShaderModule rectShader = loadShaderModule("rect");

  rectRPL = ctx.MakeRenderPipeline({
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
  ShaderModule textShader = loadShaderModule("text");

  textureSizeBGL = ctx.MakeBindGroupLayout({
    {0, ShaderStage::Vertex, BufferBindingType::Uniform},
  });

  utils::RenderPipelineDescriptor textRPLDesc{
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
  };

  textRPL = ctx.MakeRenderPipeline(textRPLDesc);

  // emoji pipeline
  ShaderModule emojiShader = loadShaderModule("text", {{"EMOJI"}});

  textRPLDesc.vs = emojiShader;
  textRPLDesc.fs = emojiShader;

  emojiRPL = ctx.MakeRenderPipeline(textRPLDesc);

  // mask pipeline -------------------------------------------
  ShaderModule textMaskShader = loadShaderModule("text_mask");

  utils::RenderPipelineDescriptor textMaskRPLDesc{
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
  };

  textMaskRPL = ctx.MakeRenderPipeline(textMaskRPLDesc);

  // emoji mask pipeline
  ShaderModule emojiMaskShader = loadShaderModule("text_mask", {{"EMOJI"}});

  textMaskRPLDesc.vs = emojiMaskShader;
  textMaskRPLDesc.fs = emojiMaskShader;

  emojiMaskRPL = ctx.MakeRenderPipeline(textMaskRPLDesc);

  // texture pipeline ------------------------------------------------
  ShaderModule textureShader = loadShaderModule("texture");

  utils::VertexBufferLayout textureQuadVBL{
    .arrayStride = sizeof(TextureQuadVertex),
    .attributes = {
      {VertexFormat::Float32x2, offsetof(TextureQuadVertex, position)},
      {VertexFormat::Float32x2, offsetof(TextureQuadVertex, uv)},
    }
  };

  defaultBgLinearBGL = ctx.MakeBindGroupLayout({
    {0, ShaderStage::Fragment, BufferBindingType::Uniform},
  });

  textureNoBlendRPL = ctx.MakeRenderPipeline({
    .vs = textureShader,
    .fs = textureShader,
    .bgls = {viewProjBGL, defaultBgLinearBGL, textureBGL},
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
        .compare = CompareFunction::Greater,
        .failOp = StencilOperation::Keep,
        .passOp = StencilOperation::Replace,
      },
    },
  });

  textureRPL = ctx.MakeRenderPipeline({
    .vs = textureShader,
    .fs = textureShader,
    .bgls = {viewProjBGL, defaultBgLinearBGL, textureBGL},
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
        .compare = CompareFunction::Greater,
        .failOp = StencilOperation::Keep,
        .passOp = StencilOperation::Replace,
      },
    },
  });

  // final texture pipeline -------------------------------------
  ShaderModule textureFinalShader = loadShaderModule("texture", {{"FINAL"}});

  textureFinalRPL = ctx.MakeRenderPipeline({
    .vs = textureFinalShader,
    .fs = textureFinalShader,
    .bgls = {viewProjBGL, textureBGL},
    .buffers = {textureQuadVBL},
    .targets = {{.format = TextureFormat::BGRA8Unorm}},
  });

  // cursor pipeline ------------------------------------------------
  ShaderModule cursorShader = loadShaderModule("cursor");

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

  utils::RenderPipelineDescriptor cursorRPLDesc{
    .vs = cursorShader,
    .fs = cursorShader,
    .bgls = {viewProjBGL, viewProjBGL, cursorMaskPosBGL, textureBGL},
    .buffers = {cursorQuadVBL},
    .targets = {{.format = TextureFormat::BGRA8Unorm}},
  };

  cursorRPL = ctx.MakeRenderPipeline(cursorRPLDesc);

  // curosr emoji
  ShaderModule cursorEmojiShader = loadShaderModule("cursor", {{"EMOJI"}});

  cursorRPLDesc.vs = cursorEmojiShader;
  cursorRPLDesc.fs = cursorEmojiShader;

  cursorEmojiRPL = ctx.MakeRenderPipeline(cursorRPLDesc);
}
