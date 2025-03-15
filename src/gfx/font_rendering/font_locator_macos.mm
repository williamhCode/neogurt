#include "./font_locator.hpp"
#include "utils/logger.hpp"
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

std::string FindFallbackFontForCharacter(uint32_t unicodeChar) {
  // Convert the Unicode character to a CFStringRef
  auto uniChar = static_cast<UniChar>(unicodeChar);
  CFStringRef charString =
    CFStringCreateWithCharacters(kCFAllocatorDefault, &uniChar, 1);

  // Ask CoreText to find a suitable font
  CTFontRef fallbackFont =
    CTFontCreateForString(nullptr, charString, CFRangeMake(0, 1));
  CFRelease(charString);

  if (!fallbackFont) {
    return ""; // No suitable font found
  }

  // Get the font's PostScript name
  CFStringRef fontName = CTFontCopyPostScriptName(fallbackFont);
  CFRelease(fallbackFont);

  if (!fontName) {
    return "";
  }

  // Convert CFStringRef to std::string
  std::string result = [(NSString*)fontName UTF8String];
  CFRelease(fontName);

  return result;
}

// std::string GetFontPathFromFamilyAndStyle(const FontDescriptor& desc) {

//   NSString* ctName = @(desc.name.c_str());
//   NSFontWeight ctWeight = FontWeightToNSFontWeight(desc.weight);
//   CTFontSymbolicTraits ctSlant = SlantToCoreTextSlant(desc.slant);

//   NSDictionary* traits = @{
//     (NSString*)kCTFontWeightTrait : @(ctWeight),
//     (NSString*)kCTFontSlantTrait : @(ctSlant),
//   };

//   NSDictionary* attributes = @{
//     (NSString*)kCTFontNameAttribute : ctName,
//     (NSString*)kCTFontTraitsAttribute : traits,
//   };

//   CTFontDescriptorRef ctDescriptor =
//     CTFontDescriptorCreateWithAttributes((__bridge CFDictionaryRef)attributes);
//   CFURLRef fontUrl =
//     (CFURLRef)CTFontDescriptorCopyAttribute(ctDescriptor, kCTFontURLAttribute);

//   std::string path;
//   if (fontUrl) {
//     path = [[(NSURL*)fontUrl path] UTF8String];
//     CFRelease(fontUrl);
//   }
//   CFRelease(ctDescriptor);

//   return path;
// }
