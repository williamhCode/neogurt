#include "shape_drawing.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "utils/unicode.hpp"

using namespace shape;

static auto shapeDescMap = [] {
  std::unordered_map<char32_t, DrawDesc> c{};

  using std::tuple;

  auto _ = None;
  auto L = Light;
  auto H = Heavy;
  auto D = Double;

  auto UL = UpLeft;
  auto UR = UpRight;
  auto DL = DownLeft;
  auto DR = DownRight;

  // solid lines (0x2500 - 0x2503)
  // ─ ━ │ ┃
  c[0x2500] = HLine{};
  c[0x2501] = HLine{Heavy};
  c[0x2502] = VLine{};
  c[0x2503] = VLine{Heavy};

  // dashed lines (0x2504 - 0x250b)
  // ┄ ┅ ┆ ┇ ┈ ┉ ┊ ┋
  c[0x2504] = HDash{3};
  c[0x2505] = HDash{3, Heavy};
  c[0x2506] = VDash{3};
  c[0x2507] = VDash{3, Heavy};
  c[0x2508] = HDash{4};
  c[0x2509] = HDash{4, Heavy};
  c[0x250a] = VDash{4};
  c[0x250b] = VDash{4, Heavy};

  // line box components (0x250C - 0x254B)
  // ┌ ┍ ┎ ┏ ┐ ┑ ┒ ┓ └ ┕ ┖ ┗ ┘ ┙ ┚ ┛
  auto Corner = [&](char32_t start, Side vert, Side hori) {
    for (auto [vWeight, hWeight] : {tuple{L, L}, {L, H}, {H, L}, {H, H}}) {
      Cross cross{};
      cross[vert] = vWeight;
      cross[hori] = hWeight;
      c[start++] = cross;
    }
  };
  Corner(0x250C, Down, Right);
  Corner(0x2510, Down, Left);
  Corner(0x2514, Up, Right);
  Corner(0x2518, Up, Left);

  // ├ ┝ ┞ ┟ ┠ ┡ ┢ ┣ ┤ ┥ ┦ ┧ ┨ ┩ ┪ ┫
  auto VertT = [&](char32_t start, Side side) {
    for (auto [tWeight, bWeight, sWeight] :
         {tuple{L, L, L}, {L, L, H}, {H, L, L}, {L, H, L}, {H, H, L}, {H, L, H}, {L, H, H}, {H, H, H}}) {
      Cross cross{};
      cross[Up] = tWeight;
      cross[Down] = bWeight;
      cross[side] = sWeight;
      c[start++] = cross;
    }
  };
  VertT(0x251C, Right);
  VertT(0x2524, Left);

  // ┬ ┭ ┮ ┯ ┰ ┱ ┲ ┳ ┴ ┵ ┶ ┷ ┸ ┹ ┺ ┻
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
  HoriT(0x252C, Down);
  HoriT(0x2534, Up);

  // ┼ ┽ ┾ ┿ ╀ ╁ ╂ ╃ ╄ ╅ ╆ ╇ ╈ ╉ ╊ ╋
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
  // ╌ ╍ ╎ ╏
  c[0x254c] = HDash{2};
  c[0x254d] = HDash{2, Heavy};
  c[0x254e] = VDash{2};
  c[0x254f] = VDash{2, Heavy};

  // double lines (0x2550 - 0x2551)
  // ═ ║
  c[0x2550] = HLine{Double};
  c[0x2551] = VLine{Double};

  // double line box components (0x2552 - 0x256c)
  // ╒ ╓ ╔ ╕ ╖ ╗ ╘ ╙ ╚ ╛ ╜ ╝
  auto DoubleCorner = [&](char32_t start, Side vert, Side hori) {
    for (auto [vWeight, hWeight] : {tuple{L, D}, {D, L}, {D, D}}) {
      DoubleCross cross{};
      cross[vert] = vWeight;
      cross[hori] = hWeight;
      c[start++] = cross;
    }
  };
  DoubleCorner(0x2552, Down, Right);
  DoubleCorner(0x2555, Down, Left);
  DoubleCorner(0x2558, Up, Right);
  DoubleCorner(0x255b, Up, Left);

  // ╞ ╟ ╠ ╡ ╢ ╣
  c[0x255e] = DoubleCross{L, L, _, D};
  c[0x255f] = DoubleCross{D, D, _, L};
  c[0x2560] = DoubleCross{D, D, _, D};
  c[0x2561] = DoubleCross{L, L, D, _};
  c[0x2562] = DoubleCross{D, D, L, _};
  c[0x2563] = DoubleCross{D, D, D, _};

  // ╤ ╥ ╦ ╧ ╨ ╩
  c[0x2564] = DoubleCross{_, L, D, D};
  c[0x2565] = DoubleCross{_, D, L, L};
  c[0x2566] = DoubleCross{_, D, D, D};
  c[0x2567] = DoubleCross{L, _, D, D};
  c[0x2568] = DoubleCross{D, _, L, L};
  c[0x2569] = DoubleCross{D, _, D, D};

  // ╪ ╫ ╬ 
  c[0x256a] = DoubleCross{L, L, D, D};
  c[0x256b] = DoubleCross{D, D, L, L};
  c[0x256c] = DoubleCross{D, D, D, D};

  // character cell arcs (0x256d - 0x2570)
  // ╭ ╮ ╯ ╰
  c[0x256d] = Arc{DownRight};
  c[0x256e] = Arc{DownLeft};
  c[0x256f] = Arc{UpLeft};
  c[0x2570] = Arc{UpRight};

  // character cell diagonals (0x2571 - 0x2573)
  // ╱ ╲ ╳
  c[0x2571] = Diagonal{.forward = true};
  c[0x2572] = Diagonal{.back = true};
  c[0x2573] = Diagonal{true, true};

  // half lines (0x2574 - 0x257b)
  // ╴ ╵ ╶ ╷ ╸ ╹ ╺ ╻
  c[0x2574] = HalfLine{.left = Light};
  c[0x2575] = HalfLine{.up = Light};
  c[0x2576] = HalfLine{.right = Light};
  c[0x2577] = HalfLine{.down = Light};
  c[0x2578] = HalfLine{.left = Heavy};
  c[0x2579] = HalfLine{.up = Heavy};
  c[0x257a] = HalfLine{.right = Heavy};
  c[0x257b] = HalfLine{.down = Heavy};

  // mixed lines (0x257c - 0x257f)
  // ╼ ╽ ╾ ╿
  c[0x257c] = HalfLine{.left = Light, .right = Heavy};
  c[0x257d] = HalfLine{.up = Light, .down = Heavy};
  c[0x257e] = HalfLine{.left = Heavy, .right = Light};
  c[0x257f] = HalfLine{.up = Heavy, .down = Light};

  // block elements (0x2580 - 0x2590)
  // ▀ ▁ ▂ ▃ ▄ ▅ ▆ ▇ █ ▉ ▊ ▋ ▌ ▍ ▎ ▏▐
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

  // shade characters (0x2591 - 0x2593)
  // ░ ▒ ▓
  c[0x2591] = Shade{SLight};
  c[0x2592] = Shade{SMedium};
  c[0x2593] = Shade{SDark};

  // block elements (0x2594 - 0x2595)
  // ▔ ▕
  c[0x2594] = UpperBlock{1/8.};
  c[0x2595] = RightBlock{1/8.};

  // terminal graphic characters (0x2596 - 0x259f)
  // ▖ ▗ ▘ ▙ ▚ ▛ ▜ ▝ ▞ ▟
  c[0x2596] = Quadrant{DL};
  c[0x2597] = Quadrant{DR};
  c[0x2598] = Quadrant{UL};
  c[0x2599] = Quadrant{UL, DL, DR};
  c[0x259a] = Quadrant{UL, DR};
  c[0x259b] = Quadrant{UL, UR, DL};
  c[0x259c] = Quadrant{UL, UR, DR};
  c[0x259d] = Quadrant{UR};
  c[0x259e] = Quadrant{UR, DL};
  c[0x259f] = Quadrant{UR, DL, DR};

  // braille characters
  for (char32_t charcode = 0x2801; charcode <= 0x28FF; charcode++) {
    c[charcode] = Braille{charcode};
  }

  return c;
}();

ShapeDrawing::ShapeDrawing(glm::vec2 charSize, float dpiScale) {
  int width = charSize.x * dpiScale;
  int height = charSize.y * dpiScale;
  pen = Pen(width, height, dpiScale);
}

const GlyphInfo*
ShapeDrawing::GetGlyphInfo(const std::string& text, TextureAtlas<false>& textureAtlas) {
  char32_t charcode = Utf8ToChar32(text);
  bool isShape = (charcode >= 0x2500 && charcode <= 0x259F) ||
                 (charcode >= 0x2801 && charcode <= 0x28FF);
  if (!isShape) {
    return nullptr;
  }

  // return cached
  auto glyphIt = glyphInfoMap.find(charcode);
  if (glyphIt != glyphInfoMap.end()) {
    return &(glyphIt->second);
  }

  // if not implemented, return nullptr
  auto shapeDescIt = shapeDescMap.find(charcode);
  if (shapeDescIt == shapeDescMap.end()) {
    return nullptr;
  }

  // using namespace std::chrono;
  // auto start = TimeNow();
  auto [data, localPoss] = pen.Draw(shapeDescIt->second);
  // auto end = TimeNow();
  // totalTime += end - start;

  if (data.empty()) {
    LOG_ERR("BoxDrawing::GetGlyphInfo: empty data for charcode: 0x{:x}", (uint32_t)charcode);
    return nullptr;
  }

  auto [region, atlasWasReset] = textureAtlas.AddGlyph(data);

  if (atlasWasReset) glyphInfoMap = {};

  auto pair = glyphInfoMap.emplace(
    charcode,
    GlyphInfo{
      .localPoss = localPoss,
      .atlasRegion = region,
      .useAscender = false,
    }
  );

  return &(pair.first->second);
}
