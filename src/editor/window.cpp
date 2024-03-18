#include "window.hpp"
#include "gfx/instance.hpp"
#include "glm/ext/vector_float2.hpp"
#include "utils/region.hpp"
#include "webgpu_utils/webgpu.hpp"

using namespace wgpu;

void WinManager::Pos(WinPos& e) {
  auto [it, first] = windows.try_emplace(e.grid);
  auto& win = it->second;

  if (first) {
    win.grid = &gridManager->grids.at(e.grid);
    win.grid->win = &win;
  }

  win.startRow = e.startRow;
  win.startCol = e.startCol;

  win.width = e.width;
  win.height = e.height;

  win.hidden = false;

  // rendering data
  if (first) {
    auto textureSampler = ctx.device.CreateSampler(ToPtr(SamplerDescriptor{
      .addressModeU = AddressMode::ClampToEdge,
      .addressModeV = AddressMode::ClampToEdge,
      .magFilter = FilterMode::Nearest,
      .minFilter = FilterMode::Nearest,
    }));

    win.textureBG = utils::MakeBindGroup(
      ctx.device, ctx.pipeline.textureBGL,
      {
        {0, win.grid->textureView},
        {1, textureSampler},
      }
    );
  }

  if (first) {
    win.renderData.CreateBuffers(1);
    win.renderData.ReserveVectors(1);
  }

  win.renderData.ResetCounts();

  auto positions = MakeRegion(e.startCol, e.startRow, e.width, e.height);
  auto uvs = MakeRegion(0, 0, 1, 1);

  for (size_t i = 0; i < 4; i++) {
    auto& vertex = win.renderData.quads[0][i];
    vertex.position = positions[i] * gridSize;
    vertex.uv = uvs[i];
  }

  win.renderData.SetIndices();
  win.renderData.IncrementCounts();

  win.renderData.WriteBuffers();
}

void WinManager::FloatPos(WinFloatPos& e) {
}

void WinManager::ExternalPos(WinExternalPos& e) {
}

void WinManager::Hide(WinHide& e) {
  // NOTE: temporary to fix nvim issue
  if (auto it = windows.find(e.grid); it != windows.end()) {
    auto& win = it->second;
    win.hidden = true;
  } else {

  }
}

void WinManager::Close(WinClose& e) {
  windows.erase(e.grid);
}

void WinManager::Viewport(WinViewport& e) {
}

void WinManager::Extmark(WinExtmark& e) {
}
