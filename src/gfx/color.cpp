#include "color.hpp"
#include "webgpu_tools/utils/webgpu.hpp"
#include "gfx/instance.hpp"

using namespace wgpu;

void ColorState::Init() {
  gammaBuffer = utils::CreateUniformBuffer(ctx.device, sizeof(float), &gamma);
  gammaBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.gammaBGL,
    {
      {0, gammaBuffer},
    }
  );
}

void ColorState::SetGamma(float value) {
  if (gamma == value) return;
  gamma = value;
  ctx.queue.WriteBuffer(gammaBuffer, 0, &gamma, sizeof(float));
}
