#include "./camera.hpp"

#include "gfx/instance.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"

using namespace wgpu;

Ortho2D::Ortho2D() {
  viewProjBuffer = ctx.CreateUniformBuffer(sizeof(glm::mat4));
  viewProjBG = ctx.MakeBindGroup(
    ctx.pipeline.viewProjBGL,
    {
      {0, viewProjBuffer},
    }
  );
}

Ortho2D::Ortho2D(glm::vec2 size) : Ortho2D() {
  Resize(size);
}

void Ortho2D::Resize(glm::vec2 size, glm::vec2 pos) {
  float left = pos.x;
  float right = pos.x + size.x;
  float top = pos.y;
  float bottom = pos.y + size.y;
  glm::mat4 view = glm::ortho<float>(left, right, bottom, top, -1, 1);
  ctx.queue.WriteBuffer(viewProjBuffer, 0, &view, sizeof(glm::mat4));
}
