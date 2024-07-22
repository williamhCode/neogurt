#pragma once
#include <string>

enum class FontWeight {
  Thin,
  ExtraLight,
  Light,
  Normal,
  Medium,
  SemiBold,
  Bold,
  ExtraBold,
  Black
};

inline float FontWeightToFloat(FontWeight weight) {
  switch (weight) {
    case FontWeight::Thin: return 100;
    case FontWeight::ExtraLight: return 200;
    case FontWeight::Light: return 300;
    case FontWeight::Normal: return 400;
    case FontWeight::Medium: return 500;
    case FontWeight::SemiBold: return 600;
    case FontWeight::Bold: return 700;
    case FontWeight::ExtraBold: return 800;
    case FontWeight::Black: return 900;
  }
}

inline FontWeight FontWeightBolder(FontWeight weight) {
  switch (weight) {
    case FontWeight::Thin:
    case FontWeight::ExtraLight:
    case FontWeight::Light:
      return FontWeight::Normal;

    case FontWeight::Normal:
    case FontWeight::Medium:
      return FontWeight::Bold;

    case FontWeight::SemiBold:
    case FontWeight::Bold:
    case FontWeight::ExtraBold:
    case FontWeight::Black:
      return FontWeight::Black;
  }
}

enum class FontSlant { Normal, Italic, Oblique };

struct FontDescriptorWithName {
  std::string name;
  float height = 12;
  float width = 0;
  bool bold = false;
  bool italic = false;
};

struct FontDescriptorWithFamily {
  std::string family;
  FontWeight weight = FontWeight::Normal;
  FontSlant slant = FontSlant::Normal;
  float height = 12;
  float width = 0;
  bool bold = false;
  bool italic = false;
};
