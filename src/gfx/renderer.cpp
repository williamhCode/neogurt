#include "renderer.hpp"
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
#include "webgpu_tools/utils/webgpu.hpp"
#include <utility>
#include <vector>
#include <array>
#include "glm/gtx/string_cast.hpp"

using namespace wgpu;

Renderer::Renderer(const SizeHandler& sizes) {
  timestamp = TimestampHelper(10, true);

  // color stuff
  gammaBuffer = utils::CreateUniformBuffer(ctx.device, sizeof(float));
  gammaBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.gammaBGL,
    {
      {0, gammaBuffer},
    }
  );

  linearColorBuffer = utils::CreateUniformBuffer(ctx.device, sizeof(glm::vec4));
  defaultColorBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.defaultColorBGL,
    {
      {0, linearColorBuffer},
    }
  );

  // shared
  camera = Ortho2D(sizes.size);

  finalRenderTexture =
    RenderTexture(sizes.uiSize, sizes.dpiScale, TextureFormat::RGBA8UnormSrgb);
  finalRenderTexture.UpdatePos(sizes.offset);

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

  // text
  textRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    },
  });

  // shapes
  shapesRPD = utils::RenderPassDescriptor({
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
    utils::CreateRenderTexture(ctx.device, {sizes.uiFbSize, TextureFormat::Stencil8})
    .CreateView();

  windowsRPD = utils::RenderPassDescriptor(
    {
      RenderPassColorAttachment{
        .view = finalRenderTexture.textureView,
        .loadOp = LoadOp::Undefined, // set later in RenderWindows
        .storeOp = StoreOp::Store,
      },
    },
    RenderPassDepthStencilAttachment{
      .view = stencilTextureView,
      .stencilLoadOp = LoadOp::Clear,
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

  if (!prevFinalRenderTexture.texture) {
    prevFinalRenderTexture = std::move(finalRenderTexture);
  }

  finalRenderTexture =
    RenderTexture(sizes.uiSize, sizes.dpiScale, TextureFormat::RGBA8UnormSrgb);
  finalRenderTexture.UpdatePos(sizes.offset);
  windowsRPD.cColorAttachments[0].view = finalRenderTexture.textureView;

  auto stencilTextureView =
    utils::CreateRenderTexture(ctx.device, {sizes.uiFbSize, TextureFormat::Stencil8})
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

void Renderer::Begin() {
  commandEncoder = ctx.device.CreateCommandEncoder();
  SurfaceTexture surfaceTexture;
  ctx.surface.GetCurrentTexture(&surfaceTexture);
  nextTexture = surfaceTexture.texture;
  nextTextureView = nextTexture.CreateView();

  timestamp.Begin(commandEncoder);
}

void Renderer::RenderToWindow(
  Win& win, FontFamily& fontFamily, HlTable& hlTable
) {
  // if for whatever reason (prob nvim events buggy, events not sent or offsync)
  // the grid is not the same size as the window
  if (win.grid.width != win.width || win.grid.height != win.height) {
    LOG_WARN(
      "RenderWindow: grid size not equal to window size for id: {}\n"
      "Sizes: grid: {}x{}, window: {}x{}\n"
      "IsFloat: {}",
      win.id, win.grid.width, win.grid.height, win.width, win.height,
      win.IsFloating()
    );

    // if (win.grid.width != win.width) return;
  }

  // keep track of quad index after each row
  size_t rows = std::min(win.grid.height, win.height);
  size_t cols = std::min(win.grid.width, win.width);
  std::vector<int> rectIntervals; rectIntervals.reserve(rows + 1);
  std::vector<int> textIntervals; textIntervals.reserve(rows + 1);
  std::vector<int> shapeIntervals; shapeIntervals.reserve(rows + 1);

  auto& rectData = win.rectData;
  auto& textData = win.textData;
  auto& shapeData = win.shapeData;

  rectData.ResetCounts();
  textData.ResetCounts();
  shapeData.ResetCounts();

  glm::vec2 textOffset(0, 0);
  const auto& defaultFont = fontFamily.DefaultFont();
  auto defaultBG = GetDefaultBackground(hlTable);

  for (size_t row = 0; row < rows; row++) {
    auto& line = win.grid.lines[row];
    textOffset.x = 0;

    rectIntervals.push_back(rectData.quadCount);
    textIntervals.push_back(textData.quadCount);
    shapeIntervals.push_back(shapeData.quadCount);

    for (size_t col = 0; col < cols; col++) {
      auto& cell = line[col];
      const Highlight& hl = hlTable[cell.hlId];
      // don't render background if default
      if (cell.hlId != 0 && hl.background.has_value() && hl.background != defaultBG) {
        auto rectPositions = MakeRegion({0, 0}, defaultFont.charSize);

        auto background = *hl.background;
        background.a = hl.bgAlpha;
        auto& quad = rectData.NextQuad();
        for (size_t i = 0; i < 4; i++) {
          quad[i].position = textOffset + rectPositions[i];
          quad[i].color = background;
        }
      }

      if (!cell.text.empty() && cell.text != " ") {
        glm::vec4 foreground = GetForeground(hlTable, hl);
        char32_t charcode = UTF8ToChar32(cell.text);

        if (charcode >= 0x2800 && charcode <= 0x28FF) { // braille characters
          // order, hex value
          // 0 3    1 8
          // 1 4    2 10
          // 2 5    4 20
          // 6 7    8 80

          uint32_t hexVal = charcode - 0x2800; 

          std::span<const glm::vec2> brailleOffsets;
          int numDots = 0;

          if (charcode < 0x2840) { // 6 dots
            static const glm::vec2 sixDotBrailleOffsets[6] = {
              {1/4., 1/6.},
              {1/4., 3/6.},
              {1/4., 5/6.},
              {3/4., 1/6.},
              {3/4., 3/6.},
              {3/4., 5/6.},
            };
            brailleOffsets = sixDotBrailleOffsets;
            numDots = 6;
            
          } else { // 8 dots
            static const glm::vec2 eightDotBrailleOffsets[8] = {
              {1/4., 1/8.},
              {1/4., 3/8.},
              {1/4., 5/8.},
              {3/4., 1/8.},
              {3/4., 3/8.},
              {3/4., 5/8.},
              {1/4., 7/8.},
              {3/4., 7/8.},
            };
            brailleOffsets = eightDotBrailleOffsets;
            numDots = 8;
          }

          for (int dotIndex = 0; dotIndex < numDots; dotIndex++) {
            if (!(hexVal & (1 << dotIndex))) continue;

            auto charSize = defaultFont.charSize;
            auto centerPos = brailleOffsets[dotIndex] * charSize;

            int xNumDots = 2;
            int yNumDots = numDots / 2;
            float radius =
              std::min(charSize.x / xNumDots, charSize.y / yNumDots) / 2;
            radius *= 0.6; // add some padding

            Rect quadRect{
              .pos = textOffset + (centerPos - glm::vec2(radius, radius)),
              .size = glm::vec2(radius * 2, radius * 2),
            };
            static uint32_t brailleShapeId = 5;
            AddShapeQuad(shapeData, quadRect, foreground, brailleShapeId);
          }

        } else {
          const auto& glyphInfo = fontFamily.GetGlyphInfo(charcode, hl.bold, hl.italic);

          glm::vec2 textQuadPos{
            textOffset.x,
            textOffset.y + (glyphInfo.boxDrawing ? 0 : defaultFont.ascender)
          };

          auto& quad = textData.NextQuad();
          for (size_t i = 0; i < 4; i++) {
            quad[i].position = textQuadPos + glyphInfo.localPoss[i];
            quad[i].regionCoord = glyphInfo.atlasRegion[i];
            quad[i].foreground = foreground;
          }
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
  shapeIntervals.push_back(shapeData.quadCount);

  rectData.WriteBuffers();
  textData.WriteBuffers();
  shapeData.WriteBuffers();

  // gpu texture is reallocated if resized
  // old gpu texture is not referenced by texture atlas anymore
  // but still referenced by command encoder if used by previous windows
  fontFamily.textureAtlas.Update();

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

    // render text
    start = textIntervals[range.start];
    end = textIntervals[range.end];
    if (start != end) {
      textRPD.cColorAttachments[0].view = renderTexture->textureView;
      RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&textRPD);
      passEncoder.SetPipeline(ctx.pipeline.textRPL);
      passEncoder.SetBindGroup(0, renderTexture->camera.viewProjBG);
      passEncoder.SetBindGroup(1, gammaBG);
      passEncoder.SetBindGroup(2, fontFamily.textureAtlas.textureSizeBG);
      passEncoder.SetBindGroup(3, fontFamily.textureAtlas.renderTexture.textureBG);
      if (start != end) textData.Render(passEncoder, start, end - start);
      passEncoder.End();
    }

    // render braille
    start = shapeIntervals[range.start];
    end = shapeIntervals[range.end];
    if (start != end) {
      shapesRPD.cColorAttachments[0].view = renderTexture->textureView;
      RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&shapesRPD);
      passEncoder.SetPipeline(ctx.pipeline.shapesRPL);
      passEncoder.SetBindGroup(0, renderTexture->camera.viewProjBG);
      passEncoder.SetBindGroup(1, gammaBG);
      if (start != end) shapeData.Render(passEncoder, start, end - start);
      passEncoder.End();
    }
  }

  rectRPD.cColorAttachments[0].view = {};
  textRPD.cColorAttachments[0].view = {};
  shapesRPD.cColorAttachments[0].view = {};
}

void Renderer::RenderCursorMask(
  const Win& win, const Cursor& cursor, FontFamily& fontFamily, HlTable& hlTable
) {
  auto& cell = win.grid.lines[cursor.row][cursor.col];

  // if (!cell.text.empty() && cell.text != " ") {
  char32_t charcode = UTF8ToChar32(cell.text);
  const auto& hl = hlTable[cell.hlId];
  const auto& glyphInfo = fontFamily.GetGlyphInfo(charcode, hl.bold, hl.italic);

  glm::vec2 textQuadPos{
    0, glyphInfo.boxDrawing ? 0 : fontFamily.DefaultFont().ascender
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
  std::span<const Win*> windows, std::span<const Win*> floatWindows
) {
  windowsRPD.cColorAttachments[0].clearValue = linearClearColor;
  windowsRPD.cColorAttachments[0].loadOp = LoadOp::Clear;
  {
    auto passEncoder = commandEncoder.BeginRenderPass(&windowsRPD);
    passEncoder.SetPipeline(ctx.pipeline.textureNoBlendRPL);
    passEncoder.SetStencilReference(1);
    passEncoder.SetBindGroup(0, finalRenderTexture.camera.viewProjBG);
    passEncoder.SetBindGroup(1, gammaBG);
    passEncoder.SetBindGroup(2, defaultColorBG);
    for (const Win* win : windows) {
      win->sRenderTexture.Render(passEncoder, 3);
    }
    passEncoder.End();
  }
  windowsRPD.cColorAttachments[0].loadOp = LoadOp::Load;
  {
    auto passEncoder = commandEncoder.BeginRenderPass(&windowsRPD);
    passEncoder.SetPipeline(ctx.pipeline.textureRPL);
    passEncoder.SetStencilReference(1);
    passEncoder.SetBindGroup(0, finalRenderTexture.camera.viewProjBG);
    passEncoder.SetBindGroup(1, gammaBG);
    passEncoder.SetBindGroup(2, defaultColorBG);
    for (const Win* win : floatWindows) {
      win->sRenderTexture.Render(passEncoder, 3);
    }
    passEncoder.End();
  }
}

void Renderer::RenderFinalTexture() {
  finalRPD.cColorAttachments[0].view = nextTextureView;
  finalRPD.cColorAttachments[0].clearValue = premultClearColor;

  auto passEncoder = commandEncoder.BeginRenderPass(&finalRPD);
  passEncoder.SetPipeline(ctx.pipeline.textureFinalRPL);
  passEncoder.SetBindGroup(0, camera.viewProjBG);
  passEncoder.SetBindGroup(1, gammaBG);

  if (prevFinalRenderTexture.texture) {
    passEncoder.SetBindGroup(2, prevFinalRenderTexture.textureBG);
    prevFinalRenderTexture.renderData.Render(passEncoder);
  } else {
    passEncoder.SetBindGroup(2, finalRenderTexture.textureBG);
    finalRenderTexture.renderData.Render(passEncoder);
  }

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
    quad[i].position = cursor.pos + cursor.corners[i];
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
  timestamp.Resolve();
  timestamp.ReadBuffer();

  auto commandBuffer = commandEncoder.Finish();
  ctx.queue.Submit(1, &commandBuffer);
  nextTexture = {};
  nextTextureView = {};
}
