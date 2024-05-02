#include "locator.hpp"
#include <AppKit/NSFontDescriptor.h>
#include <Foundation/Foundation.h>
#include <CoreText/CoreText.h>

static NSFontWeight WeightToCoreTextWeight(FontWeight weight) {
  NSFontWeight fWeight;
  switch (weight) {
    case FontWeight::Thin: fWeight = NSFontWeightThin; break;
    case FontWeight::ExtraLight: fWeight = NSFontWeightUltraLight; break;
    case FontWeight::Light: fWeight = NSFontWeightLight; break;
    case FontWeight::Normal: fWeight = NSFontWeightRegular; break;
    case FontWeight::Medium: fWeight = NSFontWeightMedium; break;
    case FontWeight::SemiBold: fWeight = NSFontWeightSemibold; break;
    case FontWeight::Bold: fWeight = NSFontWeightBold; break;
    case FontWeight::ExtraBold: fWeight = NSFontWeightHeavy; break;
    case FontWeight::Black: fWeight = NSFontWeightBlack; break;
  }
  return fWeight;
}

static CTFontSymbolicTraits SlantToCoreTextSlant(FontSlant slant) {
  switch (slant) {
    case FontSlant::Italic: return kCTFontItalicTrait;
    case FontSlant::Oblique: return kCTFontItalicTrait;
    default: return 0;
  }
}

std::string GetFontPath(const FontDescriptor& desc) {
  NSString* ctFamily = [NSString stringWithUTF8String:desc.family.c_str()];
  NSFontWeight ctWeight = WeightToCoreTextWeight(desc.weight);
  CTFontSymbolicTraits ctSlant = SlantToCoreTextSlant(desc.slant);

  NSDictionary* traits = @{
    (NSString*)kCTFontWeightTrait : @(ctWeight),
    (NSString*)kCTFontSymbolicTrait : @(ctSlant),
  };

  NSDictionary* attributes = @{
    (NSString*)kCTFontFamilyNameAttribute : ctFamily,
    (NSString*)kCTFontTraitsAttribute : traits,
  };

  CTFontDescriptorRef ctDescriptor =
    CTFontDescriptorCreateWithAttributes((__bridge CFDictionaryRef)attributes);

  // size can be ignored, we just need the font URL
  CTFontRef font = CTFontCreateWithFontDescriptor(ctDescriptor, 0, nullptr);
  CFURLRef fontURL = (CFURLRef)CTFontCopyAttribute(font, kCTFontURLAttribute);
  NSString *fontPath = [NSString stringWithString:[(NSURL *)fontURL path]];

  CFRelease(font);
  CFRelease(ctDescriptor);
  if (fontURL) CFRelease(fontURL);

  return [fontPath UTF8String];
}
