#include "./renderer.hpp"
#include "editor/grid.hpp"
#include "editor/highlight.hpp"
#include "editor/window.hpp"
#include "gfx/instance.hpp"
#include "gfx/pipeline.hpp"
#include "gfx/shapes.hpp"
#include "utils/logger.hpp"
#include "utils/region.hpp"
#include "utils/unicode.hpp"
#include "utils/color.hpp"
#include <limits>
#include <utility>
#include <vector>
#include <array>
#include "glm/gtx/string_cast.hpp"

using namespace wgpu;

Renderer::Renderer(const SizeHandler& sizes) {
  timestamp = TimestampHelper(2, false);

  // color stuff
  gammaBuffer = ctx.CreateUniformBuffer(sizeof(float));
  gammaBG = ctx.MakeBindGroup(
    ctx.pipeline.gammaBGL,
    {
      {0, gammaBuffer},
    }
  );

  linearColorBuffer = ctx.CreateUniformBuffer(sizeof(glm::vec4));
  defaultColorBG = ctx.MakeBindGroup(
    ctx.pipeline.defaultColorBGL,
    {
      {0, linearColorBuffer},
    }
  );

  // shared
  camera = Ortho2D(sizes.size);

  CurrFinalRenderTexture() =
    RenderTexture(sizes.uiSize, sizes.dpiScale, TextureFormat::RGBA8UnormSrgb);
  CurrFinalRenderTexture().UpdatePos(sizes.offset);

  // rect
  rectRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
    },
  });
  rectNoClearRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    },
  });

  // text and shapes
  textAndShapesRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    },
  });

  // text mask
  textMaskData.CreateBuffers(1);
  textMaskRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
      .clearValue = {0.0, 0.0, 0.0, 0.0},
    },
  });

  // windows
  auto stencilTextureView =
    ctx.CreateRenderTexture({sizes.uiFbSize, TextureFormat::Stencil8})
    .CreateView();

  windowsRPD = utils::RenderPassDescriptor(
    {
      RenderPassColorAttachment{
        .view = nullptr,             // set later in RenderWindows
        .loadOp = LoadOp::Undefined, // set later in RenderWindows
        .storeOp = StoreOp::Store,
      },
    },
    RenderPassDepthStencilAttachment{
      .view = stencilTextureView,
      .stencilLoadOp = LoadOp::Undefined,
      .stencilStoreOp = StoreOp::Store,
      .stencilClearValue = 0,
    }
  );

  // final texture
  finalRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
    },
  });

  // cursor
  cursorData.CreateBuffers(1);
  cursorRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    },
  });
}

void Renderer::Resize(const SizeHandler& sizes) {
  camera.Resize(sizes.size);

  OtherFinalRenderTexture() =
    RenderTexture(sizes.uiSize, sizes.dpiScale, TextureFormat::RGBA8UnormSrgb);
  OtherFinalRenderTexture().UpdatePos(sizes.offset);
  resize = true;

  auto stencilTextureView =
    ctx.CreateRenderTexture({sizes.uiFbSize, TextureFormat::Stencil8})
    .CreateView();
  windowsRPD.cDepthStencilAttachmentInfo.view = stencilTextureView;
}

void Renderer::SetColors(const glm::vec4& color, float gamma) {
  clearColor = ToWGPUColor(color);
  linearClearColor = ToWGPUColor(ToLinear(color, gamma));
  premultClearColor = ToWGPUColor(
    ToSrgb(PremultiplyAlpha(ToLinear(color, gamma)), gamma)
  );

  if (this->gamma != gamma) {
    ctx.queue.WriteBuffer(gammaBuffer, 0, &gamma, sizeof(gamma));
    this->gamma = gamma;
  }

  auto linearColor = ToLinear(color, gamma);
  if (this->linearColor != linearColor) {
    ctx.queue.WriteBuffer(linearColorBuffer, 0, &linearColor, sizeof(linearColor));
    this->linearColor = linearColor;
  }
}

void Renderer::GetNextTexture() {
  SurfaceTexture surfaceTexture;
  ctx.surface.GetCurrentTexture(&surfaceTexture);
  nextTexture = surfaceTexture.texture;
  nextTextureView = nextTexture.CreateView();
}

void Renderer::Begin() {
  commandEncoder = ctx.device.CreateCommandEncoder();
  timestamp.Begin(commandEncoder);
  timestamp.Write();
}

void Renderer::RenderToWindow(
  Win& win, FontFamily& fontFamily, HlTable& hlTable
) {
  // if for whatever reason (prob nvim events buggy, events not sent or offsync)
  // the grid is not the same size as the window
  if (win.grid.width != win.width || win.grid.height != win.height) {
    // LOG_INFO(
    //   "RenderWindow: grid size not equal to window size for id: {}\n"
    //   "Sizes: grid: {}x{}, window: {}x{}\n"
    //   "IsFloat: {}",
    //   win.id, win.grid.width, win.grid.height, win.width, win.height,
    //   win.IsFloating()
    // );
  }

  // keep track of quad index after each row
  size_t rows = std::min(win.grid.height, win.height);
  size_t cols = std::min(win.grid.width, win.width);
  std::vector<int> rectIntervals; rectIntervals.reserve(rows + 1);
  std::vector<int> textIntervals; textIntervals.reserve(rows + 1);
  std::vector<int> emojiIntervals; emojiIntervals.reserve(rows + 1);
  std::vector<int> shapeIntervals; shapeIntervals.reserve(rows + 1);

  auto& rectData = win.rectData;
  auto& textData = win.textData;
  auto& emojiData = win.emojiData;
  auto& shapeData = win.shapeData;

  rectData.ResetCounts();
  textData.ResetCounts();
  emojiData.ResetCounts();
  shapeData.ResetCounts();

  glm::vec2 textOffset(0, 0);
  const auto& defaultFont = fontFamily.DefaultFont();
  auto defaultBg = GetDefaultBackground(hlTable);

  for (size_t row = 0; row < rows; row++) {
    auto& line = win.grid.lines[row];
    textOffset.x = 0;

    rectIntervals.push_back(rectData.quadCount);
    textIntervals.push_back(textData.quadCount);
    emojiIntervals.push_back(emojiData.quadCount);
    shapeIntervals.push_back(shapeData.quadCount);

    for (size_t col = 0; col < cols; col++) {
      auto& cell = line[col];
      const Highlight& hl = hlTable[cell.hlId];
      auto hlBg = GetBackground(hlTable, hl);
      // don't render background if same as default background
      if (hlBg != defaultBg) {
        auto rectPositions = MakeRegion({0, 0}, defaultFont.charSize);

        auto& quad = rectData.NextQuad();
        for (size_t i = 0; i < 4; i++) {
          quad[i].position = textOffset + rectPositions[i];
          quad[i].color = hlBg;
        }
      }

      if (!cell.text.empty() && cell.text != " ") {
        char32_t charcode = Utf8ToChar32(cell.text);
        const auto& glyphInfo = fontFamily.GetGlyphInfo(charcode, hl.bold, hl.italic);

        glm::vec2 textQuadPos{
          textOffset.x,
          textOffset.y + (glyphInfo.useAscender ? defaultFont.ascender : 0)
        };

        glm::vec4 foreground{};
        QuadRenderData<TextQuadVertex, true>* quadData;
        if (glyphInfo.isEmoji) {
          quadData = &emojiData;
        } else {
          foreground = GetForeground(hlTable, hl);
          quadData = &textData;
        }

        auto& quad = quadData->NextQuad();
        for (size_t i = 0; i < 4; i++) {
          quad[i].position = textQuadPos + glyphInfo.localPoss[i];
          quad[i].regionCoord = glyphInfo.atlasRegion[i];
          quad[i].foreground = foreground;
        }
      }

      if (hl.underline.has_value()) {
        auto underlineType = *hl.underline;

        float thicknessScale = 1.0f;
        if (underlineType == UnderlineType::Undercurl) {
          thicknessScale = 3.5f;
        } else if (underlineType == UnderlineType::Underdouble) {
          thicknessScale = 3.0f;
        } else if (underlineType == UnderlineType::Underdotted) {
          thicknessScale = 1.5f;
        }
        float thickness = defaultFont.underlineThickness * thicknessScale;
        // make sure underdouble is at least 4 physical pixels high
        if (underlineType == UnderlineType::Underdouble) {
          thickness = std::max(thickness, 4.0f / defaultFont.dpiScale);
        }

        Rect quadRect{
          .pos = {
            textOffset.x,
            textOffset.y + defaultFont.ascender - defaultFont.underlinePosition -
              (thickness / 2),
          },
          .size = {
            defaultFont.charSize.x,
            thickness,
          },
        };
        quadRect.RoundToPixel(defaultFont.dpiScale);

        auto underlineColor = GetSpecial(hlTable, hl);
        AddShapeQuad(
          shapeData, quadRect, underlineColor,
          std::to_underlying(underlineType)
        );
      }

      textOffset.x += defaultFont.charSize.x;
    }

    textOffset.y += defaultFont.charSize.y;
  }

  rectIntervals.push_back(rectData.quadCount);
  textIntervals.push_back(textData.quadCount);
  emojiIntervals.push_back(emojiData.quadCount);
  shapeIntervals.push_back(shapeData.quadCount);

  rectData.WriteBuffers();
  textData.WriteBuffers();
  emojiData.WriteBuffers();
  shapeData.WriteBuffers();

  // gpu texture is reallocated if resized
  // old gpu texture is not referenced by texture atlas anymore
  // but still referenced by command encoder if used by other windows
  fontFamily.textureAtlas.Update();
  fontFamily.colorTextureAtlas.Update();

  auto renderInfos = win.sRenderTexture.GetRenderInfos(rows);

  for (auto& [renderTexture, range, clearRegion] : renderInfos) {
    // clear window, and render backgrounds
    int start = rectIntervals[range.start];
    int end = rectIntervals[range.end];
    {
      auto& currRPD = clearRegion.has_value() ? rectNoClearRPD : rectRPD;
      currRPD.cColorAttachments[0].view = renderTexture->textureView;
      currRPD.cColorAttachments[0].clearValue = linearClearColor;
      RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&currRPD);
      passEncoder.SetPipeline(ctx.pipeline.rectRPL);
      passEncoder.SetBindGroup(0, renderTexture->camera.viewProjBG);
      passEncoder.SetBindGroup(1, gammaBG);

      // this will only be ran at most once inside this loop,
      // so it's safe reset and write buffers directly inside
      if (clearRegion.has_value()) {
        auto& clearData = win.sRenderTexture.clearData;
        clearData.ResetCounts();
        auto region = clearRegion->Region();
        auto& quad = clearData.NextQuad();
        for (size_t i = 0; i < 4; i++) {
          quad[i].position = region[i];
          quad[i].color = ToGlmColor(clearColor);
        }
        clearData.WriteBuffers();
        clearData.Render(passEncoder);
      }

      if (start != end) rectData.Render(passEncoder, start, end - start);
      passEncoder.End();
    }

    // render text and shapes
    textAndShapesRPD.cColorAttachments[0].view = renderTexture->textureView;

    start = textIntervals[range.start];
    end = textIntervals[range.end];
    if (start != end) {
      RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&textAndShapesRPD);
      passEncoder.SetPipeline(ctx.pipeline.textRPL);
      passEncoder.SetBindGroup(0, renderTexture->camera.viewProjBG);
      passEncoder.SetBindGroup(1, gammaBG);
      passEncoder.SetBindGroup(2, fontFamily.textureAtlas.textureSizeBG);
      passEncoder.SetBindGroup(3, fontFamily.textureAtlas.renderTexture.textureBG);
      if (start != end) textData.Render(passEncoder, start, end - start);
      passEncoder.End();
    }

    start = emojiIntervals[range.start];
    end = emojiIntervals[range.end];
    if (start != end) {
      RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&textAndShapesRPD);
      passEncoder.SetPipeline(ctx.pipeline.emojiRPL);
      passEncoder.SetBindGroup(0, renderTexture->camera.viewProjBG);
      passEncoder.SetBindGroup(1, gammaBG);
      passEncoder.SetBindGroup(2, fontFamily.colorTextureAtlas.textureSizeBG);
      passEncoder.SetBindGroup(3, fontFamily.colorTextureAtlas.renderTexture.textureBG);
      if (start != end) emojiData.Render(passEncoder, start, end - start);
      passEncoder.End();
    }

    start = shapeIntervals[range.start];
    end = shapeIntervals[range.end];
    if (start != end) {
      RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&textAndShapesRPD);
      passEncoder.SetPipeline(ctx.pipeline.shapesRPL);
      passEncoder.SetBindGroup(0, renderTexture->camera.viewProjBG);
      passEncoder.SetBindGroup(1, gammaBG);
      if (start != end) shapeData.Render(passEncoder, start, end - start);
      passEncoder.End();
    }
  }

  rectRPD.cColorAttachments[0].view = {};
  rectNoClearRPD.cColorAttachments[0].view = {};
  textAndShapesRPD.cColorAttachments[0].view = {};
}

void Renderer::RenderCursorMask(
  const Win& win, const Cursor& cursor, FontFamily& fontFamily, HlTable& hlTable
) {
  auto& cell = win.grid.lines[cursor.row][cursor.col];

  // if (!cell.text.empty() && cell.text != " ") {
  char32_t charcode = Utf8ToChar32(cell.text);
  const auto& hl = hlTable[cell.hlId];
  const auto& glyphInfo = fontFamily.GetGlyphInfo(charcode, hl.bold, hl.italic);

  glm::vec2 textQuadPos{
    0, glyphInfo.useAscender ? fontFamily.DefaultFont().ascender : 0
  };

  textMaskData.ResetCounts();
  auto& quad = textMaskData.NextQuad();
  for (size_t i = 0; i < 4; i++) {
    quad[i].position = textQuadPos + glyphInfo.localPoss[i];
    quad[i].regionCoord = glyphInfo.atlasRegion[i];
  }
  textMaskData.WriteBuffers();

  textMaskRPD.cColorAttachments[0].view = cursor.maskRenderTexture.textureView;
  RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&textMaskRPD);
  passEncoder.SetPipeline(ctx.pipeline.textMaskRPL);
  passEncoder.SetBindGroup(0, cursor.maskRenderTexture.camera.viewProjBG);
  passEncoder.SetBindGroup(1, fontFamily.textureAtlas.textureSizeBG);
  passEncoder.SetBindGroup(2, fontFamily.textureAtlas.renderTexture.textureBG);
  textMaskData.Render(passEncoder);
  passEncoder.End();

  // TODO: for some reason this only clears half of the mask for
  // certain font sizes

  // } else {
  //   // just clear the texture if there's nothing
  //   textMaskRPD.cColorAttachments[0].view = cursor.maskRenderTexture.textureView;
  //   RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&textMaskRPD);
  //   passEncoder.End();
  // }

  textMaskRPD.cColorAttachments[0].view = {};
}

void Renderer::RenderWindows(
  const Win* msgWin, std::span<const Win*> windows, std::span<const Win*> floatWindows
) {
  if (resize) {
    // reset texture, since we're rendering to a new texture
    CurrFinalRenderTexture() = {};
    SwapFinalRenderTexture();
    resize = false;
  }

  // - msg window covers normal + floating win
  // - normal window covers normal win
  // - ime window covers floating + msg win
  // - floating window covers floating win

  windowsRPD.cColorAttachments[0].view = CurrFinalRenderTexture().textureView;
  windowsRPD.cColorAttachments[0].clearValue = linearClearColor;
  windowsRPD.cColorAttachments[0].loadOp = LoadOp::Clear;
  windowsRPD.cDepthStencilAttachmentInfo.stencilLoadOp = LoadOp::Clear;
  {
    auto passEncoder = commandEncoder.BeginRenderPass(&windowsRPD);
    passEncoder.SetPipeline(ctx.pipeline.textureNoBlendRPL);
    passEncoder.SetBindGroup(0, CurrFinalRenderTexture().camera.viewProjBG);
    passEncoder.SetBindGroup(1, gammaBG);
    passEncoder.SetBindGroup(2, defaultColorBG);

    if (msgWin) {
      // writes for both floating and normal win
      passEncoder.SetStencilReference(0b011);
      msgWin->sRenderTexture.Render(passEncoder, 3);
    }

    // writes for normal win
    passEncoder.SetStencilReference(0b001);
    for (const Win* win : windows) {
      win->sRenderTexture.Render(passEncoder, 3);
    }

    passEncoder.End();
  }

  windowsRPD.cColorAttachments[0].loadOp = LoadOp::Load;
  windowsRPD.cDepthStencilAttachmentInfo.stencilLoadOp = LoadOp::Load;
  {
    auto passEncoder = commandEncoder.BeginRenderPass(&windowsRPD);
    passEncoder.SetPipeline(ctx.pipeline.textureRPL);
    passEncoder.SetBindGroup(0, CurrFinalRenderTexture().camera.viewProjBG);
    passEncoder.SetBindGroup(1, gammaBG);
    passEncoder.SetBindGroup(2, defaultColorBG);
    for (const Win* win : floatWindows) {
      if (win->floatData->zindex == std::numeric_limits<int>::max()) {
        // writes for only floating win (0b010),
        // but reads 0b110 so not blocked by msg win
        passEncoder.SetStencilReference(0b110);
      } else {
        // writes for floating win
        passEncoder.SetStencilReference(0b010);
      }
      win->sRenderTexture.Render(passEncoder, 3);
    }
    passEncoder.End();
  }

  windowsRPD.cColorAttachments[0].view = nullptr;
}

void Renderer::RenderFinalTexture() {
  finalRPD.cColorAttachments[0].view = nextTextureView;
  finalRPD.cColorAttachments[0].clearValue = premultClearColor;

  auto passEncoder = commandEncoder.BeginRenderPass(&finalRPD);
  passEncoder.SetPipeline(ctx.pipeline.textureFinalRPL);
  passEncoder.SetBindGroup(0, camera.viewProjBG);
  passEncoder.SetBindGroup(1, gammaBG);

  passEncoder.SetBindGroup(2, CurrFinalRenderTexture().textureBG);
  CurrFinalRenderTexture().renderData.Render(passEncoder);

  passEncoder.End();
  finalRPD.cColorAttachments[0].view = {};
}

void Renderer::RenderCursor(const Cursor& cursor, HlTable& hlTable) {
  auto attrId = cursor.cursorMode->attrId;
  const auto& hl = hlTable[attrId];
  auto foreground = GetForeground(hlTable, hl);
  auto background = GetBackground(hlTable, hl);
  if (attrId == 0) std::swap(foreground, background);

  cursorData.ResetCounts();
  auto& quad = cursorData.NextQuad();
  for (size_t i = 0; i < 4; i++) {
    quad[i].position = cursor.corners[i];
    quad[i].foreground = foreground;
    quad[i].background = background;
  }
  cursorData.WriteBuffers();

  cursorRPD.cColorAttachments[0].view = nextTextureView;
  RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&cursorRPD);
  passEncoder.SetPipeline(ctx.pipeline.cursorRPL);
  passEncoder.SetBindGroup(0, camera.viewProjBG);
  passEncoder.SetBindGroup(1, cursor.maskRenderTexture.camera.viewProjBG);
  passEncoder.SetBindGroup(2, cursor.maskPosBG);
  passEncoder.SetBindGroup(3, cursor.maskRenderTexture.textureBG);
  cursorData.Render(passEncoder);
  passEncoder.End();
  cursorRPD.cColorAttachments[0].view = {};
}

void Renderer::End() {
  timestamp.Write();
  timestamp.Resolve();
  timestamp.ReadBuffer();

  auto commandBuffer = commandEncoder.Finish();
  ctx.queue.Submit(1, &commandBuffer);
  nextTexture = {};
  nextTextureView = {};
}
