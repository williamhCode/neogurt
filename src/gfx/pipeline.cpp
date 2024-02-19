#include "pipeline.hpp"
#include "context.hpp"
#include "dawn/utils/WGPUHelpers.h"
#include "webgpu_utils/webgpu.hpp"

using namespace wgpu;

Pipeline::Pipeline(const WGPUContext& ctx) {
  // font vbo layout
  std::vector<VertexAttribute> textQuadAttribs{
    {VertexFormat::Float32x2, offsetof(TextQuadVertex, position), 0},
    {VertexFormat::Float32x2, offsetof(TextQuadVertex, uv), 1},
    {VertexFormat::Float32x4, offsetof(TextQuadVertex, foreground), 2},
    {VertexFormat::Float32x4, offsetof(TextQuadVertex, background), 3},
  };
  VertexBufferLayout textQuadVBL{
    .arrayStride = sizeof(TextQuadVertex),
    .attributeCount = textQuadAttribs.size(),
    .attributes = textQuadAttribs.data(),
  };

  viewProjBGL = dawn::utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Vertex, BufferBindingType::Uniform},
    }
  );

  fontTextureBGL = dawn::utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Fragment, TextureSampleType::Float},
      {1, ShaderStage::Fragment, SamplerBindingType::Filtering},
    }
  );

  ShaderModule textVert =
    utils::LoadShaderModule(ctx.device, ROOT_DIR "/src/shaders/text.vert.wgsl");

  ShaderModule textFrag =
    utils::LoadShaderModule(ctx.device, ROOT_DIR "/src/shaders/text.frag.wgsl");

  textRPL = ctx.device.CreateRenderPipeline(ToPtr(RenderPipelineDescriptor{
    .layout = dawn::utils::MakePipelineLayout(
      ctx.device,
      {
        viewProjBGL,
        fontTextureBGL,
      }
    ),
    .vertex =
      VertexState{
        .module = textVert,
        .entryPoint = "vs_main",
        .bufferCount = 1,
        .buffers = &textQuadVBL,
      },
    .fragment = ToPtr(FragmentState{
      .module = textFrag,
      .entryPoint = "fs_main",
      .targetCount = 1,
      .targets = ToPtr<ColorTargetState>({
        {
          .format = TextureFormat::BGRA8Unorm,
          .blend = &utils::BlendState::AlphaBlending,
        },
      }),
    }),
  }));
}
