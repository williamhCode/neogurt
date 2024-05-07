#include "locator.hpp"
#include "utils/logger.hpp"
#import <AppKit/NSFontDescriptor.h>
#include <format>
#include <CoreText/CTFontDescriptor.h>
#import <Foundation/NSString.h>
#import <Foundation/NSDictionary.h>
#import <CoreText/CoreText.h>

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

std::string GetFontPathFromName(const FontDescriptorWithName& desc) {
  NSString* ctName = @(desc.name.c_str());
  CTFontDescriptorRef ctDescriptor =
    CTFontDescriptorCreateWithNameAndSize((CFStringRef)ctName, 0);
  CFURLRef fontUrl =
    (CFURLRef)CTFontDescriptorCopyAttribute(ctDescriptor, kCTFontURLAttribute);
  if (fontUrl == nullptr) {
    return "";
  }
  
  CTFontDescriptorRef newDescriptor;

  // alter original font traits, and create new font desc with original family but new traits
  if (desc.bold || desc.italic) {
    NSMutableDictionary* newTraits = [NSMutableDictionary dictionary];

    auto traits = (NSDictionary *)CTFontDescriptorCopyAttribute(ctDescriptor, kCTFontTraitsAttribute);
    auto currWeight = (NSNumber *)traits[(NSString *)kCTFontWeightTrait];

    if (desc.bold) {
      NSNumber* newWeight = @(NSFontWeightBolder([currWeight floatValue]));
      newTraits[(NSString *)kCTFontWeightTrait] = newWeight;
    } else {
      newTraits[(NSString *)kCTFontWeightTrait] = currWeight;
    }

    if (desc.italic) {
      newTraits[(NSString *)kCTFontSlantTrait] = @(kCTFontItalicTrait);
    }

    auto family = (NSString *)CTFontDescriptorCopyAttribute(ctDescriptor, kCTFontFamilyNameAttribute);
    NSDictionary* newAttributes = @{
      (NSString*)kCTFontFamilyNameAttribute : family,
      (NSString*)kCTFontTraitsAttribute : newTraits,
    };
    newDescriptor =
      CTFontDescriptorCreateWithAttributes((CFDictionaryRef)newAttributes);

  } else {
    newDescriptor = ctDescriptor;
  }

  CFURLRef newFontUrl =
    (CFURLRef)CTFontDescriptorCopyAttribute(newDescriptor, kCTFontURLAttribute);

  std::string path;
  if (newFontUrl) {
    path = [[(NSURL*)newFontUrl path] UTF8String];
    CFRelease(newFontUrl);
  }
  CFRelease(newDescriptor);

  return path;
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
