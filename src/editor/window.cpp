#include "window.hpp"
#include "gfx/instance.hpp"
#include "glm/ext/vector_float2.hpp"
#include "webgpu_utils/webgpu.hpp"

using namespace wgpu;

void WindowManager::Pos(WinPos& e) {
  auto& win = windows[e.grid];

  win.grid = &gridManager->grids.at(e.grid);
  win.grid->win = &win;

  win.startRow = e.startRow;
  win.startCol = e.startCol;
  win.width = e.width;
  win.height = e.height;
  win.hidden = false;

  // rendering
  auto textureSampler = ctx.device.CreateSampler(
    ToPtr(SamplerDescriptor{
      .addressModeU = AddressMode::ClampToEdge,
      .addressModeV = AddressMode::ClampToEdge,
      .magFilter = FilterMode::Nearest,
      .minFilter = FilterMode::Nearest,
    })
  );

  win.textureBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.textureBGL,
    {
      {0, win.grid->textureView},
      {1, textureSampler},
    }
  );

  win.renderData.CreateBuffers(1);
  win.renderData.ReserveVectors(1);

  win.renderData.ResetCounts();

  glm::vec2 offset(e.startCol, e.startRow);
  std::array<glm::vec2, 4> positions{
    glm::vec2(0, 0),
    glm::vec2(win.width, 0),
    glm::vec2(win.width, win.height),
    glm::vec2(0, win.height),
  };

  std::array<glm::vec2, 4> uvs{
    glm::vec2(0, 0),
    glm::vec2(1, 0),
    glm::vec2(1, 1),
    glm::vec2(0, 1),
  };

  for (size_t i = 0; i < 4; i++) {
    auto& vertex = win.renderData.quads[0][i];
    vertex.position = (offset + positions[i]) * gridSize;
    vertex.uv = uvs[i];
  }

  win.renderData.SetIndices();
  win.renderData.IncrementCounts();

  win.renderData.WriteBuffers();
}

void WindowManager::FloatPos(WinFloatPos& e) {
}

void WindowManager::ExternalPos(WinExternalPos& e) {
}

void WindowManager::Hide(WinHide& e) {
  windows.at(e.grid).hidden = true;
}

void WindowManager::Close(WinClose& e) {
  windows.erase(e.grid);
}

void WindowManager::Viewport(WinViewport& e) {
}

void WindowManager::Extmark(WinExtmark& e) {
}
