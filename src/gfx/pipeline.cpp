#include "pipeline.hpp"
#include "context.hpp"
#include "dawn/utils/WGPUHelpers.h"

using namespace wgpu;

Pipeline::Pipeline(const WGPUContext& ctx) {
    auto cameraBGL = dawn::utils::MakeBindGroupLayout(
    ctx.device,
    {
      {0, ShaderStage::Vertex | ShaderStage::Fragment, BufferBindingType::Uniform},
      {1, ShaderStage::Vertex | ShaderStage::Fragment, BufferBindingType::Uniform},
      {2, ShaderStage::Vertex | ShaderStage::Fragment, BufferBindingType::Uniform},
    }
  );

}
