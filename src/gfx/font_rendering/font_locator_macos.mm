#include "./font_locator.hpp"
#include "app/path.hpp"
#include "utils/logger.hpp"
#include "utils/unicode.hpp"
#import <AppKit/NSFontDescriptor.h>
#import <Foundation/Foundation.h>
#import <CoreText/CoreText.h>

// clang-format off
static NSFontWeight FontWeightToNSFontWeight(FontWeight weight) {
  switch (weight) {
    case FontWeight::Thin: return NSFontWeightUltraLight;
    case FontWeight::ExtraLight: return NSFontWeightThin;
    case FontWeight::Light: return NSFontWeightLight;
    case FontWeight::Normal: return NSFontWeightRegular;
    case FontWeight::Medium: return NSFontWeightMedium;
    case FontWeight::SemiBold: return NSFontWeightSemibold;
    case FontWeight::Bold: return NSFontWeightBold;
    case FontWeight::ExtraBold: return NSFontWeightHeavy;
    case FontWeight::Black: return NSFontWeightBlack;
  }
}

static CTFontSymbolicTraits SlantToCoreTextSlant(FontSlant slant) {
  switch (slant) {
    case FontSlant::Italic: return kCTFontItalicTrait;
    case FontSlant::Oblique: return kCTFontItalicTrait;
    default: return 0;
  }
}

static NSFontWeight NSFontWeightBolder(NSFontWeight weight) {
  if (weight <= NSFontWeightLight) return NSFontWeightRegular;
  if (weight <= NSFontWeightMedium) return NSFontWeightBold;
  return NSFontWeightBlack;
}
// clang-format on

std::string GetFontPathFromName(const FontDescriptorWithName& desc) {
  // Register default fonts from resourcesDir/fonts
  static bool fontsRegistered = false;
  if (!fontsRegistered) {
    NSString* fontsPath =
      [NSString stringWithUTF8String:(resourcesDir / "fonts").c_str()];
    NSArray* files =
      [[NSFileManager defaultManager] contentsOfDirectoryAtPath:fontsPath error:nil];

    for (NSString* file in files) {
      NSString* fullPath = [fontsPath stringByAppendingPathComponent:file];
      NSURL* fontURL = [NSURL fileURLWithPath:fullPath];
      CTFontManagerRegisterFontsForURL((CFURLRef)fontURL, kCTFontManagerScopeProcess, nullptr);
    }

    fontsRegistered = true;
  }

  NSString* ctName = @(desc.name.c_str());
  CTFontDescriptorRef ctDescriptor =
    CTFontDescriptorCreateWithNameAndSize((CFStringRef)ctName, 0);
  CFURLRef fontUrl =
    (CFURLRef)CTFontDescriptorCopyAttribute(ctDescriptor, kCTFontURLAttribute);
  if (fontUrl == nullptr) {
    CFRelease(ctDescriptor);
    return "";
  }

  // alter original font traits,
  // and create new font desc with original family but with new traits
  auto traits =
    (NSDictionary*)CTFontDescriptorCopyAttribute(ctDescriptor, kCTFontTraitsAttribute);
  if (!traits) {
    CFRelease(ctDescriptor);
    CFRelease(fontUrl);
    return "";
  }

  NSMutableDictionary* newTraits = [NSMutableDictionary dictionary];

  auto currWeight = (NSNumber*)traits[(NSString*)kCTFontWeightTrait];
  if (desc.bold) {
    NSNumber* newWeight = @(NSFontWeightBolder([currWeight floatValue]));
    newTraits[(NSString*)kCTFontWeightTrait] = newWeight;
  } else {
    newTraits[(NSString*)kCTFontWeightTrait] = currWeight;
  }

  auto currSlant = (NSNumber*)traits[(NSString*)kCTFontSlantTrait];
  if (desc.italic) {
    newTraits[(NSString*)kCTFontSlantTrait] = @(kCTFontItalicTrait);
  } else {
    newTraits[(NSString*)kCTFontSlantTrait] = currSlant;
  }

  auto family =
    (NSString*)CTFontDescriptorCopyAttribute(ctDescriptor, kCTFontFamilyNameAttribute);
  if (!family) {
    CFRelease(ctDescriptor);
    CFRelease(fontUrl);
    CFRelease(traits);
    return "";
  }

  NSDictionary* newAttributes = @{
    (NSString*)kCTFontFamilyNameAttribute : family,
    (NSString*)kCTFontTraitsAttribute : newTraits,
  };
  CTFontDescriptorRef newDescriptor =
    CTFontDescriptorCreateWithAttributes((CFDictionaryRef)newAttributes);

  std::string path;
  CFURLRef newFontUrl =
    (CFURLRef)CTFontDescriptorCopyAttribute(newDescriptor, kCTFontURLAttribute);
  if (newFontUrl) {
    path = [[(NSURL*)newFontUrl path] UTF8String];
    CFRelease(newFontUrl);
  }

  CFRelease(ctDescriptor);
  CFRelease(fontUrl);
  CFRelease(traits);
  CFRelease(family);
  CFRelease(newDescriptor);

  return path;
}

std::string FindFallbackFontForCharacter(const std::string& text) {
  std::u32string u32text = Utf8ToUtf32(text);
  if (u32text.empty()) return "";

  CTFontRef baseFont = CTFontCreateWithName(CFSTR("SF Mono"), 12.0, nullptr);
  CFStringRef charString =
    CFStringCreateWithCString(kCFAllocatorDefault, text.c_str(), kCFStringEncodingUTF8);
  if (!charString) return "";

  CTFontRef fallbackFont = CTFontCreateForString(
    baseFont, charString, CFRangeMake(0, CFStringGetLength(charString))
  );
  CFRelease(baseFont);
  CFRelease(charString);
  if (!fallbackFont) return "";

  CFStringRef fontName = CTFontCopyPostScriptName(fallbackFont);
  CFRelease(fallbackFont);
  if (!fontName) return "";

  std::string result = [(NSString*)fontName UTF8String];
  CFRelease(fontName);

  return result;
}
