#include "locator.hpp"
#include <AppKit/AppKit.h>
#include <CoreText/CoreText.h>
#include <Foundation/Foundation.h>
#include <iomanip>
#include <iostream>

static CGFloat WeightToCoreTextWeight(FontWeight weight) {
  CGFloat fWeight;
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
    case FontSlant::Oblique:
      return kCTFontItalicTrait; // Typically treated the same as italic
    default: return 0;
  }
}

std::string GetFontPath(const FontDescriptor& desc) {
  CFStringRef cfFamily =
    CFStringCreateWithCString(NULL, desc.family.c_str(), kCFStringEncodingUTF8);
  CGFloat ctWeight = WeightToCoreTextWeight(desc.weight);
  CTFontSymbolicTraits ctSlant = SlantToCoreTextSlant(desc.slant);

  NSDictionary* traits = @{
    (NSString*)kCTFontWeightTrait : @(ctWeight),
    (NSString*)kCTFontSymbolicTrait : @(ctSlant),
  };

  NSDictionary* attributes = @{
    (NSString*)kCTFontFamilyNameAttribute : (__bridge NSString*)cfFamily,
    (NSString*)kCTFontTraitsAttribute : traits,
  };

  CTFontDescriptorRef ctDescriptor =
    CTFontDescriptorCreateWithAttributes((__bridge CFDictionaryRef)attributes);

  CTFontRef font = CTFontCreateWithFontDescriptor(ctDescriptor, desc.size, NULL);
  CFURLRef fontURL = (CFURLRef)CTFontCopyAttribute(font, kCTFontURLAttribute);

  std::string path;
  char cpath[1024];
  if (CFURLGetFileSystemRepresentation(fontURL, true, (UInt8*)cpath, sizeof(cpath))) {
    path = cpath;
  }
  CFRelease(font);
  CFRelease(ctDescriptor);
  CFRelease(cfFamily);
  if (fontURL) CFRelease(fontURL);

  return path;
}
