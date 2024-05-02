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

enum class FontSlant { Normal, Italic, Oblique };

struct FontDescriptor {
  std::string family;
  FontWeight weight = FontWeight::Normal;
  FontSlant slant = FontSlant::Normal;
  int size;
};
