#include "camera.hpp"

#include "gfx/instance.hpp"
#include "webgpu_tools/utils/webgpu.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"

using namespace wgpu;

Ortho2D::Ortho2D(glm::vec2 size) {
  auto view = glm::ortho<float>(0, size.x, size.y, 0, -1, 1);
  viewProjBuffer = utils::CreateUniformBuffer(ctx.device, sizeof(glm::mat4), &view);

  viewProjBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.viewProjBGL,
    {
      {0, viewProjBuffer},
    }
  );
}

void Ortho2D::Resize(glm::vec2 size, glm::vec2 pos) {
  float left = pos.x;
  float right = pos.x + size.x;
  float top = pos.y;
  float bottom = pos.y + size.y;
  auto view = glm::ortho<float>(left, right, bottom, top, -1, 1);
  ctx.queue.WriteBuffer(viewProjBuffer, 0, &view, sizeof(glm::mat4));
}
