#include "box_drawing.hpp"

using namespace box;

static std::unordered_map<char32_t, DrawDesc> boxChars = {
  // solid lines
  {0x2500, HLine{}},
  {0x2501, HLine{Heavy}},
  {0x2502, VLine{}},
  {0x2503, VLine{Heavy}},

  // dashed lines
  {0x2504, HDash{3}},
  {0x2505, HDash{3, Heavy}},
  {0x2506, VDash{3}},
  {0x2507, VDash{3, Heavy}},

  {0x2508, HDash{4}},
  {0x2509, HDash{4, Heavy}},
  {0x250a, VDash{4}},
  {0x250b, VDash{4, Heavy}},

  {0x254c, HDash{2}},
  {0x254d, HDash{2, Heavy}},
  {0x254e, VDash{2}},
  {0x254f, VDash{2, Heavy}},

  // double lines
};

void PopulateBoxChars() {
  // line box components (0x250C - 0x254B)
  enum Side { Top, Right, Bottom, Left };
  auto SetSide = [](Cross& cross, Side side, Weight weight) {
    switch (side) {
      case Top: cross.top = weight; break;
      case Right: cross.right = weight; break;
      case Bottom: cross.bottom = weight; break;
      case Left: cross.left = weight; break;
    }
  };
  auto L = Light;
  auto H = Heavy;

  auto Corner = [&](char32_t start, Side vert, Side hori) {
    for (auto [vWeight, hWeight] : {std::tuple{L, L}, {L, H}, {H, L}, {H, H}}) {
      Cross cross{};
      SetSide(cross, vert, vWeight);
      SetSide(cross, hori, hWeight);
      boxChars[start++] = cross;
    }
  };
  Corner(0x250C, Bottom, Right);
  Corner(0x2510, Bottom, Left);
  Corner(0x2514, Top, Right);
  Corner(0x2518, Top, Left);

  auto VertT = [&](char32_t start, Side side) {
    for (auto [tWeight, bWeight, sWeight] :
         {std::tuple{L, L, L}, {L, L, H}, {H, L, L}, {L, H, L}, {H, H, L}, {H, L, H}, {L, H, H}, {H, H, H}}) {
      Cross cross{.top = tWeight, .bottom = bWeight};
      SetSide(cross, side, sWeight);
      boxChars[start++] = cross;
    }
  };
  VertT(0x251C, Right);
  VertT(0x2524, Left);

  auto HoriT = [&](char32_t start, Side side) {
    for (auto [tWeight, bWeight, sWeight] :
         {std::tuple{L, L, L}, {H, L, L}, {L, H, L}, {H, H, L}, {L, L, H}, {H, L, H}, {L, H, H}, {H, H, H}}) {
      Cross cross{.left = tWeight, .right = bWeight};
      SetSide(cross, side, sWeight);
      boxChars[start++] = cross;
    }
  };
  HoriT(0x252C, Bottom);
  HoriT(0x2534, Top);

  char32_t start = 0x253C;
  for (auto [tWeight, bWeight, lWeight, rWeight] :
       {std::tuple{L, L, L, L}, {L, L, H, L}, {L, L, L, H}, {L, L, H, H},
                  {H, L, L, L}, {L, H, L, L}, {H, H, L, L}, {H, L, H, L},
                  {H, L, L, H}, {L, H, H, L}, {L, H, L, H}, {H, L, H, H},
                  {L, H, H, H}, {H, H, H, L}, {H, H, L, H}, {H, H, H, H}}) {
    Cross cross{tWeight, bWeight, lWeight, rWeight};
    boxChars[start++] = cross;
  }
}

BoxDrawing::BoxDrawing(glm::vec2 _size, float _dpiScale)
    : size(_size), dpiScale(_dpiScale) {
  int width = size.x * dpiScale;
  int height = size.y * dpiScale;

  dataRaw.resize(width * height);
  data = std::mdspan(dataRaw.data(), std::dextents<size_t, 2>{height, width});

  pen.SetData(data, dpiScale);
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

  std::ranges::fill(dataRaw, 0);
  pen.Draw(boxCharIt->second);

  auto region = textureAtlas.AddGlyph(data);

  auto pair = glyphInfoMap.emplace(
    charcode,
    GlyphInfo{
      .boxDrawing = true,
      .localPoss = MakeRegion({0, 0}, size),
      .atlasRegion = region,
    }
  );

  return &(pair.first->second);
}
