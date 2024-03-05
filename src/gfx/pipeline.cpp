#include "pipeline.hpp"
#include "context.hpp"
#include "webgpu_utils/webgpu.hpp"

using namespace wgpu;

Pipeline::Pipeline(const WGPUContext& ctx) {
  // shared ------------------------------------------------
  viewProjBGL = utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Vertex, BufferBindingType::Uniform},
    }
  );

  // text pipeline -------------------------------------------
  utils::VertexBufferLayout textQuadVBL{
    sizeof(TextQuadVertex),
    {
      {VertexFormat::Float32x2, offsetof(TextQuadVertex, position), 0},
      {VertexFormat::Float32x2, offsetof(TextQuadVertex, uv), 1},
      {VertexFormat::Float32x4, offsetof(TextQuadVertex, foreground), 2},
    }
  };

  fontTextureBGL = utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Fragment, TextureSampleType::Float},
      {1, ShaderStage::Fragment, SamplerBindingType::Filtering},
    }
  );

  ShaderModule textShader =
    utils::LoadShaderModule(ctx.device, ROOT_DIR "/src/shaders/text.wgsl");

  textRPL = ctx.device.CreateRenderPipeline(ToPtr(RenderPipelineDescriptor{
    .layout = utils::MakePipelineLayout(
      ctx.device,
      {
        viewProjBGL,
        fontTextureBGL,
      }
    ),
    .vertex = VertexState{
      .module = textShader,
      .entryPoint = "vs_main",
      .bufferCount = 1,
      .buffers = &textQuadVBL,
    },
    .fragment = ToPtr(FragmentState{
      .module = textShader,
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

  rectRPL = ctx.device.CreateRenderPipeline(ToPtr(RenderPipelineDescriptor{
    .layout = utils::MakePipelineLayout(
      ctx.device,
      {
        viewProjBGL,
      }
    ),
    .vertex = VertexState{
      .module = rectShader,
      .entryPoint = "vs_main",
      .bufferCount = 1,
      .buffers = &rectQuadVBL,
    },
    .fragment = ToPtr(FragmentState{
      .module = rectShader,
      .entryPoint = "fs_main",
      .targetCount = 1,
      .targets = ToPtr<ColorTargetState>({
        {
          .format = TextureFormat::BGRA8Unorm,
          // .blend = &utils::BlendState::AlphaBlending,
        },
      }),
    }),
  }));
}
