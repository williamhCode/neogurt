#include "box_drawing.hpp"
#include "utils/mdspan.hpp"

using namespace box;

static auto boxChars = [] {
  std::unordered_map<char32_t, DrawDesc> c{};

  using std::tuple;

  auto _ = None;
  auto L = Light;
  auto H = Heavy;
  auto D = Double;

  auto t = true;
  auto f = false;

  // solid lines (0x2500 - 0x2503)
  c[0x2500] = HLine{};
  c[0x2501] = HLine{Heavy};
  c[0x2502] = VLine{};
  c[0x2503] = VLine{Heavy};

  // dashed lines (0x2504 - 0x250b)
  c[0x2504] = HDash{3};
  c[0x2505] = HDash{3, Heavy};
  c[0x2506] = VDash{3};
  c[0x2507] = VDash{3, Heavy};
  c[0x2508] = HDash{4};
  c[0x2509] = HDash{4, Heavy};
  c[0x250a] = VDash{4};
  c[0x250b] = VDash{4, Heavy};

  // line box components (0x250C - 0x254B)
  auto Corner = [&](char32_t start, Side vert, Side hori) {
    for (auto [vWeight, hWeight] : {tuple{L, L}, {L, H}, {H, L}, {H, H}}) {
      Cross cross{};
      cross[vert] = vWeight;
      cross[hori] = hWeight;
      c[start++] = cross;
    }
  };
  Corner(0x250C, Bottom, Right);
  Corner(0x2510, Bottom, Left);
  Corner(0x2514, Top, Right);
  Corner(0x2518, Top, Left);

  auto VertT = [&](char32_t start, Side side) {
    for (auto [tWeight, bWeight, sWeight] :
         {tuple{L, L, L}, {L, L, H}, {H, L, L}, {L, H, L}, {H, H, L}, {H, L, H}, {L, H, H}, {H, H, H}}) {
      Cross cross{};
      cross[Top] = tWeight;
      cross[Bottom] = bWeight;
      cross[side] = sWeight;
      c[start++] = cross;
    }
  };
  VertT(0x251C, Right);
  VertT(0x2524, Left);

  auto HoriT = [&](char32_t start, Side side) {
    for (auto [lWeight, rWeight, sWeight] :
         {tuple{L, L, L}, {H, L, L}, {L, H, L}, {H, H, L}, {L, L, H}, {H, L, H}, {L, H, H}, {H, H, H}}) {
      Cross cross{};
      cross[Left] = lWeight;
      cross[Right] = rWeight;
      cross[side] = sWeight;
      c[start++] = cross;
    }
  };
  HoriT(0x252C, Bottom);
  HoriT(0x2534, Top);

  char32_t start = 0x253C;
  for (auto [tWeight, bWeight, lWeight, rWeight] :
       {tuple{L, L, L, L}, {L, L, H, L}, {L, L, L, H}, {L, L, H, H},
             {H, L, L, L}, {L, H, L, L}, {H, H, L, L}, {H, L, H, L},
             {H, L, L, H}, {L, H, H, L}, {L, H, L, H}, {H, L, H, H},
             {L, H, H, H}, {H, H, H, L}, {H, H, L, H}, {H, H, H, H}}) {
    Cross cross{tWeight, bWeight, lWeight, rWeight};
    c[start++] = cross;
  }

  // dashed lines (0x254c - 0x254f)
  c[0x254c] = HDash{2};
  c[0x254d] = HDash{2, Heavy};
  c[0x254e] = VDash{2};
  c[0x254f] = VDash{2, Heavy};

  // double lines (0x2550 - 0x2551)
  c[0x2550] = HLine{Double};
  c[0x2551] = VLine{Double};

  // double line box components (0x2552 - 0x256c)
  auto DoubleCorner = [&](char32_t start, Side vert, Side hori) {
    for (auto [vWeight, hWeight] : {tuple{L, D}, {D, L}, {D, D}}) {
      DoubleCross cross{};
      cross[vert] = vWeight;
      cross[hori] = hWeight;
      c[start++] = cross;
    }
  };
  DoubleCorner(0x2552, Bottom, Right);
  DoubleCorner(0x2555, Bottom, Left);
  DoubleCorner(0x2558, Top, Right);
  DoubleCorner(0x255b, Top, Left);

  c[0x255e] = DoubleCross{L, L, _, D};
  c[0x255f] = DoubleCross{D, D, _, L};
  c[0x2560] = DoubleCross{D, D, _, D};
  c[0x2561] = DoubleCross{L, L, D, _};
  c[0x2562] = DoubleCross{D, D, L, _};
  c[0x2563] = DoubleCross{D, D, D, _};

  c[0x2564] = DoubleCross{_, L, D, D};
  c[0x2565] = DoubleCross{_, D, L, L};
  c[0x2566] = DoubleCross{_, D, D, D};
  c[0x2567] = DoubleCross{L, _, D, D};
  c[0x2568] = DoubleCross{D, _, L, L};
  c[0x2569] = DoubleCross{D, _, D, D};

  c[0x256a] = DoubleCross{L, L, D, D};
  c[0x256b] = DoubleCross{D, D, L, L};
  c[0x256c] = DoubleCross{D, D, D, D};

  // half lines (0x2574 - 0x257b)
  c[0x2574] = HalfLine{.left = Light};
  c[0x2575] = HalfLine{.top = Light};
  c[0x2576] = HalfLine{.right = Light};
  c[0x2577] = HalfLine{.bottom = Light};
  c[0x2578] = HalfLine{.left = Heavy};
  c[0x2579] = HalfLine{.top = Heavy};
  c[0x257a] = HalfLine{.right = Heavy};
  c[0x257b] = HalfLine{.bottom = Heavy};

  // mixed lines (0x257c - 0x257f)
  c[0x257c] = HalfLine{.left = Light, .right = Heavy};
  c[0x257d] = HalfLine{.top = Light, .bottom = Heavy};
  c[0x257e] = HalfLine{.left = Heavy, .right = Light};
  c[0x257f] = HalfLine{.top = Heavy, .bottom = Light};

  // block elements (0x2580 - 0x2590)
  c[0x2580] = UpperBlock{1/2.};
  c[0x2581] = LowerBlock{1/8.};
  c[0x2582] = LowerBlock{1/4.};
  c[0x2583] = LowerBlock{3/8.};
  c[0x2584] = LowerBlock{1/2.};
  c[0x2585] = LowerBlock{5/8.};
  c[0x2586] = LowerBlock{3/4.};
  c[0x2587] = LowerBlock{7/8.};
  c[0x2588] = LowerBlock{1};
  c[0x2589] = LeftBlock{7/8.};
  c[0x258a] = LeftBlock{3/4.};
  c[0x258b] = LeftBlock{5/8.};
  c[0x258c] = LeftBlock{1/2.};
  c[0x258d] = LeftBlock{3/8.};
  c[0x258e] = LeftBlock{1/4.};
  c[0x258f] = LeftBlock{1/8.};
  c[0x2590] = RightBlock{1/2.};

  // block elements (0x2594 - 0x2595)
  c[0x2594] = UpperBlock{1/8.};
  c[0x2595] = RightBlock{1/8.};

  // terminal graphic characters (0x2596 - 0x259f)
  c[0x2596] = Quadrant{f, f, t, f};
  c[0x2597] = Quadrant{f, f, f, t};
  c[0x2598] = Quadrant{t, f, f, f};
  c[0x2599] = Quadrant{t, f, t, t};
  c[0x259a] = Quadrant{t, f, f, t};
  c[0x259b] = Quadrant{t, t, t, f};
  c[0x259c] = Quadrant{t, t, f, t};
  c[0x259d] = Quadrant{f, t, f, f};
  c[0x259e] = Quadrant{f, t, t, f};
  c[0x259f] = Quadrant{f, t, t, t};

  return c;
}();

BoxDrawing::BoxDrawing(glm::vec2 _size, float _dpiScale)
    : size(_size), dpiScale(_dpiScale) {
  int width = size.x * dpiScale;
  int height = size.y * dpiScale;

  canvasRaw.resize(width * height);
  canvas = std::mdspan(canvasRaw.data(), height, width);
}

const GlyphInfo*
BoxDrawing::GetGlyphInfo(char32_t charcode, TextureAtlas& textureAtlas) {
  auto glyphIt = glyphInfoMap.find(charcode);
  if (glyphIt != glyphInfoMap.end()) {
    return &(glyphIt->second);
  }

  auto boxCharIt = boxChars.find(charcode);
  if (boxCharIt == boxChars.end()) {
    return nullptr;
  }

  std::ranges::fill(canvasRaw, 0);
  pen.SetCanvas(canvas, dpiScale);
  pen.Draw(boxCharIt->second);

  // memory optimization
  // find the bounds of data and create span within the bounds
  size_t xmin = canvas.extent(1);
  size_t ymin = canvas.extent(0);
  size_t xmax = 0;
  size_t ymax = 0;

  for (size_t y = 0; y < canvas.extent(0); y++) {
    for (size_t x = 0; x < canvas.extent(1); x++) {
      if (canvas[y, x] != 0) {
        xmin = std::min(xmin, x);
        ymin = std::min(ymin, y);
        xmax = std::max(xmax, x);
        ymax = std::max(ymax, y);
      }
    }
  }

  size_t width = xmax - xmin + 1;
  size_t height = ymax - ymin + 1;

  // NOTE: use submdspan for c++26
  auto subCanvas = SubMdspan2d(canvas, {ymin, xmin}, {height, width});

  auto region = textureAtlas.AddGlyph(subCanvas);

  auto pair = glyphInfoMap.emplace(
    charcode,
    GlyphInfo{
      .boxDrawing = true,
      .localPoss = MakeRegion(
        glm::vec2(xmin, ymin) / dpiScale,
        glm::vec2(width, height) / dpiScale
      ),
      .atlasRegion = region,
    }
  );

  return &(pair.first->second);
}
