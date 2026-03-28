#include "./renderer.hpp"
#include "editor/grid.hpp"
#include "editor/highlight.hpp"
#include "editor/window.hpp"
#include "gfx/instance.hpp"
#include "gfx/pipeline.hpp"
#include "utils/logger.hpp"
#include "utils/region.hpp"
#include "utils/color.hpp"
#include "utils/unicode.hpp"
#include <limits>
#include <utility>
#include <vector>
#include <array>
#include "glm/gtx/string_cast.hpp"
#include "utils/round.hpp"
#include <chrono>
#include <hb.h>
#include <boost/locale/utf.hpp>

static bool IsRTLText(const std::string& text) {
  if (text.empty()) return false;
  char32_t c = Utf8ToChar32(text);
  using namespace boost::locale::utf;
  if (c == incomplete || c == illegal) return false;
  hb_script_t script = hb_unicode_script(hb_unicode_funcs_get_default(), c);
  return hb_script_get_horizontal_direction(script) == HB_DIRECTION_RTL;
}

using namespace wgpu;

Renderer::Renderer() {
  timestamp = TimestampHelper(2, false);

  // color stuff
  defaultBgLinearBuffer = ctx.CreateUniformBuffer(sizeof(glm::vec4));
  defaultBgLinearBG = ctx.MakeBindGroup(
    ctx.pipeline.defaultBgLinearBGL,
    {
      {0, defaultBgLinearBuffer},
    }
  );

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
  textRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    },
  });

  // text mask
  textMaskData.CreateBuffers(2);
  textMaskRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
      .clearValue = {0.0, 0.0, 0.0, 0.0},
    },
  });

  // windows
  windowsRPD = utils::RenderPassDescriptor(
    {
      RenderPassColorAttachment{
        .view = nullptr,             // set later in RenderWindows
        .loadOp = LoadOp::Undefined, // set later in RenderWindows
        .storeOp = StoreOp::Store,
      },
    },
    RenderPassDepthStencilAttachment{
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

  cursorEmojiOverlayData.CreateBuffers(2);
  cursorEmojiOverlayRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    },
  });

  // post-processing
  float initialTime = 0.0f;
  postFxTimeBuffer = ctx.CreateUniformBuffer(sizeof(float), &initialTime);
  postFxTimeBG = ctx.MakeBindGroup(ctx.pipeline.postFxTimeBGL, {{0, postFxTimeBuffer}});
  postFxRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
      .clearValue = {0.0, 0.0, 0.0, 0.0},
    },
  });
}

void Renderer::Resize(const SizeHandler& sizes) {
  camera.Resize(sizes.size);

  OtherFinalRenderTexture() =
    RenderTexture(sizes.uiSize, sizes.dpiScale, TextureFormat::RGBA8UnormSrgb);
  OtherFinalRenderTexture().UpdatePos(sizes.offset);

  preEffectsTexture = RenderTexture(sizes.size, sizes.dpiScale, TextureFormat::BGRA8Unorm);

  resize = true;

  auto stencilTextureView =
    ctx.CreateRenderTexture({sizes.uiFbSize, TextureFormat::Stencil8}).CreateView();
  windowsRPD.cDepthStencilAttachmentInfo.view = stencilTextureView;
}

void Renderer::SetColors(const glm::vec4& color, float gamma) {
  clearColor = ToWGPUColor(color);
  linearClearColor = ToWGPUColor(ToLinear(color, gamma));
  premultClearColor =
    ToWGPUColor(ToSrgb(PremultiplyAlpha(ToLinear(color, gamma)), gamma));

  auto linearColor = ToLinear(color, gamma);
  if (this->defaultBgLinear != linearColor) {
    ctx.queue.WriteBuffer(defaultBgLinearBuffer, 0, &linearColor, sizeof(linearColor));
    this->defaultBgLinear = linearColor;
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

void Renderer::RenderToWindow(Win& win, FontFamily& fontFamily, HlManager& hlManager) {
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

textureReset:
  // keep track of quad index after each row
  size_t rows = std::min(win.grid.height, win.height);
  size_t cols = std::min(win.grid.width, win.width);
  std::vector<int> rectIntervals; rectIntervals.reserve(rows + 1);
  std::vector<int> textIntervals; textIntervals.reserve(rows + 1);
  std::vector<int> emojiIntervals; emojiIntervals.reserve(rows + 1);

  auto& rectData = win.rectData;
  auto& textData = win.textData;
  auto& emojiData = win.emojiData;

  rectData.ResetCounts();
  textData.ResetCounts();
  emojiData.ResetCounts();

  glm::vec2 textOffset(0, 0);
  const auto defaultBg = hlManager.GetDefaultBackground();
  const glm::vec2 charSize = fontFamily.GetCharSize();
  const float ascender = fontFamily.GetAscender();
  const float underlinePosition = fontFamily.DefaultFont().underlinePosition;
  const float strikeoutPosition = fontFamily.DefaultFont().strikeoutPosition;
  const float dpiScale = fontFamily.dpiScale;

  struct RunData {
    FontHandle font;
    int hlId = -1;
    size_t startCol = 0;
    std::string text;

    bool empty() const {
      return text.empty();
    }

    void reset() {
      hlId = -1;
      font = nullptr;
      text.clear();
    }
  } run;

  auto addTextGlyph = [&](const GlyphInfo& glyphInfo, glm::vec2 offset, const Highlight& hl) {
    glm::vec2 quadPos{
      offset.x,
      offset.y + (glyphInfo.useAscender ? ascender : 0),
    };
    quadPos = RoundToPixel(quadPos, dpiScale);

    glm::vec4 foreground{};
    QuadRenderData<TextQuadVertex, true>* quadData;
    if (glyphInfo.isEmoji) {
      quadData = &emojiData;
    } else {
      foreground = hlManager.GetForeground(hl);
      quadData = &textData;
    }

    auto& quad = quadData->NextQuad();
    for (size_t i = 0; i < 4; i++) {
      quad[i].position = quadPos + glyphInfo.localPoss[i];
      quad[i].regionCoord = glyphInfo.atlasRegion[i];
      quad[i].foreground = foreground;
    }
  };

  auto flushRun = [&]() {
    if (run.empty()) return;
    float startX = run.startCol * charSize.x;
    float cellAdvance = 0;
    Highlight& hl = hlManager.hlTable[run.hlId];

    for (ShapedGlyph& sg : fontFamily.ShapeText(run.text, run.font)) {
      if (sg.glyphInfo) {
        float xOffset = sg.glyphInfo->isEmoji ? 0 : sg.xOffset;
        if (xOffset > 0) {
          LOG_INFO("xOffset {} for text: '{}'", xOffset, run.text);
        }
        glm::vec2 offset{
          startX + cellAdvance + xOffset,
          textOffset.y,
        };
        addTextGlyph(*sg.glyphInfo, offset, hl);
      }
      cellAdvance += sg.numCells * charSize.x;
    }
    run.reset();
  };

  for (size_t row = 0; row < rows; row++) {
    auto& line = win.grid.lines[row];
    textOffset.x = 0;

    rectIntervals.push_back(rectData.quadCount);
    textIntervals.push_back(textData.quadCount);
    emojiIntervals.push_back(emojiData.quadCount);

    for (size_t col = 0; col < cols; col++) {
      auto& cell = line[col];
      const Highlight& hl = hlManager.hlTable[cell.hlId];
      auto hlBg = hlManager.GetBackground(hl);
      // don't render background if same as default background
      if (hlBg != defaultBg) {
        auto rectPositions = MakeRegion({0, 0}, charSize);

        auto& quad = rectData.NextQuad();
        for (size_t i = 0; i < 4; i++) {
          quad[i].position = textOffset + rectPositions[i];
          quad[i].color = hlBg;
        }
      }

      try {
        auto [kind, font] = fontFamily.ResolveFont(cell.text, hl.bold, hl.italic);
        using Kind = FontFamily::ResolvedFont::Kind;

        if (kind == Kind::Regular) {
          // RTL cells: Neovim pre-shapes and sends in visual order.
          // Shape per-cell so HarfBuzz handles base + combining diacritics correctly.
          if (IsRTLText(cell.text)) {
            flushRun();
            for (auto& sg : fontFamily.ShapeText(cell.text, font)) {
              if (sg.glyphInfo) {
                float xOffset = sg.glyphInfo->isEmoji ? 0 : sg.xOffset;
                addTextGlyph(*sg.glyphInfo, {textOffset.x + xOffset, textOffset.y}, hl);
              }
            }

          } else if (font != run.font || cell.hlId != run.hlId) {
            flushRun();
            run = RunData{.font = font, .hlId = cell.hlId, .startCol = col};
            run.text += cell.text;

          } else {
            run.text += cell.text;
          }

        } else if (kind == Kind::ShapeDrawing) {
          flushRun();
          if (const auto* glyphInfo = fontFamily.GetGlyphInfo(cell.text)) {
            addTextGlyph(*glyphInfo, textOffset, hl);
          }

        } else {
          flushRun(); // Skip: empty/space — flush pending run, nothing to render
        }

        auto addDecoration = [&](const GlyphInfo* glyphInfo, float centerPos, glm::vec4 color) {
          if (!glyphInfo) return;

          const auto& region = glyphInfo->localPoss;                                                                                                                                                                 
          float thickness = region[3].y - region[0].y;                                                                                                                                                               

          float relPos = centerPos - thickness / 2;                                                                                                                                               
          relPos = std::min(relPos, charSize.y - thickness); // don't go below the cell                                                                                                                              

          glm::vec2 quadPos{                                                                                                                                                                                         
            textOffset.x,                                                                                                                                                                                            
            textOffset.y + relPos,                                                                                                                                                                                   
          };                                                                                                                                                                                                         
          quadPos = RoundToPixel(quadPos, dpiScale);

          auto& quad = textData.NextQuad();
          for (size_t i = 0; i < 4; i++) {
            quad[i].position = quadPos + glyphInfo->localPoss[i];
            quad[i].regionCoord = glyphInfo->atlasRegion[i];
            quad[i].foreground = color;
          }
        };

        if (hl.strikethrough) {
          addDecoration(
            fontFamily.GetGlyphInfo(StrikethroughTag{}),
            ascender - strikeoutPosition,
            hlManager.GetForeground(hl)
          );
        }

        if (hl.underline) {
          addDecoration(
            fontFamily.GetGlyphInfo(*hl.underline),
            ascender - underlinePosition,
            hlManager.GetSpecial(hl)
          );
        }

      } catch (TextureResizeError e) {
        LOG_INFO("Texture reset, re-rendering font glyphs for window: {}", win.id);

        fontFamily.ResetTextureAtlas(e);

        // redo entire rendering of current window if texture reset
        // use goto cuz it makes the code cleaner
        goto textureReset;
      }

      textOffset.x += charSize.x;
    }

    flushRun(); // flush any run pending at end of row

    textOffset.y += charSize.y;
  }

  rectIntervals.push_back(rectData.quadCount);
  textIntervals.push_back(textData.quadCount);
  emojiIntervals.push_back(emojiData.quadCount);

  rectData.WriteBuffers();
  textData.WriteBuffers();
  emojiData.WriteBuffers();

  // gpu texture is reallocated if resized.
  // old gpu texture is not referenced by texture atlas anymore, but still
  // referenced by command encoder if used by windows previously rendered to.
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
    textRPD.cColorAttachments[0].view = renderTexture->textureView;

    start = textIntervals[range.start];
    end = textIntervals[range.end];
    if (start != end) {
      RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&textRPD);
      passEncoder.SetPipeline(ctx.pipeline.textRPL);
      passEncoder.SetBindGroup(0, renderTexture->camera.viewProjBG);
      passEncoder.SetBindGroup(1, fontFamily.textureAtlas.textureSizeBG);
      passEncoder.SetBindGroup(2, fontFamily.textureAtlas.renderTexture.textureBG);
      if (start != end) textData.Render(passEncoder, start, end - start);
      passEncoder.End();
    }

    start = emojiIntervals[range.start];
    end = emojiIntervals[range.end];
    if (start != end) {
      RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&textRPD);
      passEncoder.SetPipeline(ctx.pipeline.emojiRPL);
      passEncoder.SetBindGroup(0, renderTexture->camera.viewProjBG);
      passEncoder.SetBindGroup(1, fontFamily.colorTextureAtlas.textureSizeBG);
      passEncoder.SetBindGroup(2, fontFamily.colorTextureAtlas.renderTexture.textureBG);
      if (start != end) emojiData.Render(passEncoder, start, end - start);
      passEncoder.End();
    }
  }

  rectRPD.cColorAttachments[0].view = {};
  rectNoClearRPD.cColorAttachments[0].view = {};
  textRPD.cColorAttachments[0].view = {};
}

void Renderer::RenderCursorMask(
  const Win& win, Cursor& cursor, FontFamily& fontFamily, HlManager& hlManager
) {
  if (!win.grid.ValidCoords(cursor.row, cursor.col)) return;
  auto& cell = win.grid.lines[cursor.row][cursor.col];
  const auto& hl = hlManager.hlTable[cell.hlId];
  const float ascender = fontFamily.GetAscender();

  const GlyphInfo* glyphInfo = nullptr;
  auto [kind, font] = fontFamily.ResolveFont(cell.text, hl.bold, hl.italic);
  using Kind = FontFamily::ResolvedFont::Kind;

  if (kind == Kind::Regular) {
    for (auto& sg : fontFamily.ShapeText(cell.text, font)) {
      if (sg.glyphInfo) { glyphInfo = sg.glyphInfo; break; }
    }
  } else if (kind == Kind::ShapeDrawing) {
    glyphInfo = fontFamily.GetGlyphInfo(cell.text);
  }

  cursor.onEmoji = glyphInfo && glyphInfo->isEmoji;

  // emoji cursor is rendered directly on top via RenderCursorEmoji — no mask needed
  if (cursor.onEmoji) return;

  textMaskRPD.cColorAttachments[0].view = cursor.maskRenderTexture.textureView;
  RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&textMaskRPD);

  if (glyphInfo) {
    glm::vec2 textQuadPos{0, glyphInfo->useAscender ? ascender : 0};
    textMaskData.ResetCounts();
    auto& quad = textMaskData.NextQuad();
    for (size_t i = 0; i < 4; i++) {
      quad[i].position = textQuadPos + glyphInfo->localPoss[i];
      quad[i].regionCoord = glyphInfo->atlasRegion[i];
    }
    textMaskData.WriteBuffers();

    passEncoder.SetPipeline(ctx.pipeline.textMaskRPL);
    passEncoder.SetBindGroup(0, cursor.maskRenderTexture.camera.viewProjBG);
    passEncoder.SetBindGroup(1, fontFamily.textureAtlas.textureSizeBG);
    passEncoder.SetBindGroup(2, fontFamily.textureAtlas.renderTexture.textureBG);
    textMaskData.Render(passEncoder);
  }

  passEncoder.End();
  textMaskRPD.cColorAttachments[0].view = {};
}

void Renderer::RenderWindows(
  std::span<const Win*> windows, std::span<const Win*> floatWindows
) {
  if (resize) {
    // reset texture, since we're rendering to a new texture
    CurrFinalRenderTexture() = {};
    SwapFinalRenderTexture();
    resize = false;
  }

  // windows are rendered front to back with stencil buffer a layer
  // there are two layers, the normal layer and the floating layer

  // this is the window ordering priority:
  // - normal windows covers normal windows
  // - floating window covers normal + floating windows

  // we assign priority numbers to each type of window,
  // and use CompareFunction::Greater for replacement
  // - normal: 1
  // - floating: 2

  windowsRPD.cColorAttachments[0].view = CurrFinalRenderTexture().textureView;
  windowsRPD.cColorAttachments[0].clearValue = linearClearColor;
  windowsRPD.cColorAttachments[0].loadOp = LoadOp::Clear;
  windowsRPD.cDepthStencilAttachmentInfo.stencilLoadOp = LoadOp::Clear;
  {
    auto passEncoder = commandEncoder.BeginRenderPass(&windowsRPD);
    passEncoder.SetPipeline(ctx.pipeline.textureNoBlendRPL);
    passEncoder.SetBindGroup(0, CurrFinalRenderTexture().camera.viewProjBG);
    passEncoder.SetBindGroup(1, defaultBgLinearBG);

    passEncoder.SetStencilReference(1);
    for (const Win* win : windows) {
      win->sRenderTexture.Render(passEncoder, 2);
    }

    passEncoder.End();
  }

  windowsRPD.cColorAttachments[0].loadOp = LoadOp::Load;
  windowsRPD.cDepthStencilAttachmentInfo.stencilLoadOp = LoadOp::Load;
  {
    auto passEncoder = commandEncoder.BeginRenderPass(&windowsRPD);
    passEncoder.SetPipeline(ctx.pipeline.textureRPL);
    passEncoder.SetBindGroup(0, CurrFinalRenderTexture().camera.viewProjBG);
    passEncoder.SetBindGroup(1, defaultBgLinearBG);
    
    passEncoder.SetStencilReference(2);
    for (const Win* win : floatWindows) {
      win->sRenderTexture.Render(passEncoder, 2);
    }

    passEncoder.End();
  }

  windowsRPD.cColorAttachments[0].view = nullptr;
}

void Renderer::RenderFinalTexture() {
  finalRPD.cColorAttachments[0].view = EffectsTarget();
  finalRPD.cColorAttachments[0].clearValue = premultClearColor;

  auto passEncoder = commandEncoder.BeginRenderPass(&finalRPD);
  passEncoder.SetPipeline(ctx.pipeline.textureFinalRPL);
  passEncoder.SetBindGroup(0, camera.viewProjBG);
  passEncoder.SetBindGroup(1, CurrFinalRenderTexture().textureBG);
  CurrFinalRenderTexture().renderData.Render(passEncoder);

  passEncoder.End();
  finalRPD.cColorAttachments[0].view = {};
}

void Renderer::RenderCursor(const Cursor& cursor, HlManager& hlManager) {
  auto attrId = cursor.cursorMode->attrId;
  const auto& hl = hlManager.hlTable[attrId];
  auto foreground = hlManager.GetForeground(hl);
  auto background = hlManager.GetBackground(hl);
  if (attrId == 0) std::swap(foreground, background);

  cursorData.ResetCounts();
  auto& quad = cursorData.NextQuad();
  for (size_t i = 0; i < 4; i++) {
    quad[i].position = cursor.corners[i];
    quad[i].foreground = foreground;
    quad[i].background = background;
  }
  cursorData.WriteBuffers();

  cursorRPD.cColorAttachments[0].view = EffectsTarget();
  RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&cursorRPD);
  passEncoder.SetPipeline(
    cursor.onEmoji ? ctx.pipeline.cursorEmojiRPL : ctx.pipeline.cursorRPL
  );
  passEncoder.SetBindGroup(0, camera.viewProjBG);
  passEncoder.SetBindGroup(1, cursor.maskRenderTexture.camera.viewProjBG);
  passEncoder.SetBindGroup(2, cursor.maskPosBG);
  passEncoder.SetBindGroup(3, cursor.maskRenderTexture.textureBG);
  cursorData.Render(passEncoder);
  passEncoder.End();
  cursorRPD.cColorAttachments[0].view = {};
}

void Renderer::RenderCursorEmoji(
  const Win& win, const Cursor& cursor, FontFamily& fontFamily, HlManager& hlManager
) {
  if (!cursor.onEmoji) return;
  if (!win.grid.ValidCoords(cursor.row, cursor.col)) return;

  auto& cell = win.grid.lines[cursor.row][cursor.col];
  const auto& hl = hlManager.hlTable[cell.hlId];
  auto [kind, font] = fontFamily.ResolveFont(cell.text, hl.bold, hl.italic);
  using Kind = FontFamily::ResolvedFont::Kind;
  if (kind != Kind::Regular) return;

  const float ascender = fontFamily.GetAscender();

  cursorEmojiOverlayData.ResetCounts();
  for (ShapedGlyph& sg : fontFamily.ShapeText(cell.text, font)) {
    if (!sg.glyphInfo || !sg.glyphInfo->isEmoji) continue;
    glm::vec2 quadPos{
      cursor.maskPos.x,
      cursor.maskPos.y + (sg.glyphInfo->useAscender ? ascender : 0),
    };
    auto& quad = cursorEmojiOverlayData.NextQuad();
    for (size_t i = 0; i < 4; i++) {
      quad[i].position = quadPos + sg.glyphInfo->localPoss[i];
      quad[i].regionCoord = sg.glyphInfo->atlasRegion[i];
      quad[i].foreground = {};
    }
  }
  if (cursorEmojiOverlayData.quadCount == 0) return;
  cursorEmojiOverlayData.WriteBuffers();

  cursorEmojiOverlayRPD.cColorAttachments[0].view = EffectsTarget();
  auto passEncoder = commandEncoder.BeginRenderPass(&cursorEmojiOverlayRPD);
  passEncoder.SetPipeline(ctx.pipeline.cursorEmojiOverlayRPL);
  passEncoder.SetBindGroup(0, camera.viewProjBG);
  passEncoder.SetBindGroup(1, fontFamily.colorTextureAtlas.textureSizeBG);
  passEncoder.SetBindGroup(2, fontFamily.colorTextureAtlas.renderTexture.textureBG);
  cursorEmojiOverlayData.Render(passEncoder);
  passEncoder.End();
  cursorEmojiOverlayRPD.cColorAttachments[0].view = {};
}

void Renderer::RenderPostFx() {
  using namespace std::chrono;
  static auto startTime = steady_clock::now();
  float t = duration<float>(steady_clock::now() - startTime).count();
  ctx.queue.WriteBuffer(postFxTimeBuffer, 0, &t, sizeof(float));

  postFxRPD.cColorAttachments[0].view = nextTextureView;

  auto passEncoder = commandEncoder.BeginRenderPass(&postFxRPD);
  passEncoder.SetPipeline(ctx.pipeline.postFxRPL);
  passEncoder.SetBindGroup(0, camera.viewProjBG);
  passEncoder.SetBindGroup(1, preEffectsTexture.textureBG);
  passEncoder.SetBindGroup(2, postFxTimeBG);
  preEffectsTexture.renderData.Render(passEncoder);

  passEncoder.End();
  postFxRPD.cColorAttachments[0].view = {};
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
