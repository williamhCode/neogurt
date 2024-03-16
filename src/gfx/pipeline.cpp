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
      {1, ShaderStage::Fragment, TextureSampleType::Float},
      {2, ShaderStage::Fragment, SamplerBindingType::Filtering},
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
        // {.format = TextureFormat::R8Unorm},
      }),
    }),
  }));
  
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

  textureRPL = ctx.device.CreateRenderPipeline(ToPtr(RenderPipelineDescriptor{
    .layout = utils::MakePipelineLayout(
      ctx.device,
      {
        viewProjBGL,
        textureBGL,
      }
    ),
    .vertex = VertexState{
      .module = textureShader,
      .entryPoint = "vs_main",
      .bufferCount = 1,
      .buffers = &textureQuadVBL,
    },
    .fragment = ToPtr(FragmentState{
      .module = textureShader,
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
    }
  );

  cursorRPL = ctx.device.CreateRenderPipeline(ToPtr(RenderPipelineDescriptor{
    .layout = utils::MakePipelineLayout(
      ctx.device,
      {
        viewProjBGL,
        maskBGL,
      }
    ),
    .vertex = VertexState{
      .module = cursorShader,
      .entryPoint = "vs_main",
      .bufferCount = 1,
      .buffers = &cursorQuadVBL,
    },
    .fragment = ToPtr(FragmentState{
      .module = cursorShader,
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
