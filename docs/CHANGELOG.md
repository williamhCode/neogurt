# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Add icon to app
- Add session_edit command

### Fixed
- FontFamily::UpdateLinespace not setting topLinespace.
- Bundle SF Mono (default font) so no crashing if system doesn't have SF Mono.
- Fix dpiScale not updating for fonts
- Proper TextureAtlas resizing (wasn't clearing all the glyph info maps before leading
to rendering artifacts)

## [0.2.2] - 2025-05-08

### Fixed
- `Client::DoRead()` crashing due to use after free error. (0.2.1 was incorrect fix)

## [0.2.1] - 2025-05-05 [YANKED]

### Fixed
- `Client::DoRead()` crashing due to bad conversion

## [0.2.0] - 2025-05-05

### Added
- Emoji Rendering for joined emojis.
- Options can be set dynamically.

### Changed
- Rewrote startup code to be async.

### Fixed
- Texture atlas blowing up
- A lot of stuff

> Previous changes before 0.2.0 were untracked in this changelog.
