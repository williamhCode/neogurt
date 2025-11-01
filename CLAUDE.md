# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Neogurt is a fast Neovim GUI written in C++23 and WebGPU. It's currently supported only on macOS. The application provides smooth scrolling, cursor animations, multiple session management, and accurate font rendering using modern graphics APIs.

## Build System

### Prerequisites
- LLVM Clang (latest version)
- Ninja build system
- CMake 3.5+
- Dependencies: Boost, Freetype, Harfbuzz (install via homebrew)

### Build Commands

**Initial setup:**
```sh
make build-setup          # Configure CMake with Ninja generator
```

**Building:**
```sh
make build                # Build debug version (default)
make build REL=1          # Build release version
make build TARGET=tests   # Build test executables
```

**Running:**
```sh
make run                  # Run the neogurt executable
make run TARGET=tests     # Run tests via ctest
```

**Additional:**
```sh
make package              # Create .dmg installer (release builds only)
make xcode-setup          # Generate Xcode project
```

Build artifacts go to `build/debug/` or `build/release/` depending on `REL` flag. The Makefile automatically copies `compile_commands.json` to the project root for LSP support.

### Running Tests

To run a specific test executable directly:
```sh
./build/debug/font_test --log_level=message
./build/debug/harfbuzz_test
./build/debug/blend2d_test
```

Tests are managed through CTest. Run all tests with `make run TARGET=tests`.

## Architecture

### Core Components

**Session Management (`src/session/`):**
- `SessionManager` handles multiple Neovim instances
- Each session owns its own Nvim process, UI state, and event processing
- Sessions can be created, switched, killed, and restarted independently
- Session switching is managed centrally through `SessionSwitchInternal()`

**Event System (`src/event/`):**
- `EventManager` coordinates events from Neovim UI events to render thread
- `ui_parse.cpp/hpp` - Parses msgpack-rpc responses from Neovim into C++ structs
- `ui_process.cpp/hpp` - Processes parsed UI events and updates editor state
- `neogurt_cmd.cpp/hpp` - Handles custom Neogurt commands (session management, font size, etc.)
- Uses task queue pattern with `AddTask()` to dispatch work to render thread

**Neovim Integration (`src/nvim/`):**
- `Nvim` wraps the RPC client for communicating with Neovim processes
- `msgpack_rpc/client.cpp` - Implements msgpack-rpc protocol
- Supports both stdio and TCP connection modes
- Connections are asynchronous; use futures for TCP connections

**Editor State (`src/editor/`):**
- `Grid` - Text grid storage with cells containing text, highlight IDs, and double-width flags
- `Window` - Neovim window with grid, position, size, scroll state, and floating window data
- `Cursor` - Cursor state including position, shape, blend mode, and animations
- `Font` - Font family management using FreeType and HarfBuzz
- `Highlight` - Manages highlight groups (colors, styles) from Neovim

**Graphics/Rendering (`src/gfx/`):**
- `Renderer` orchestrates the entire render pipeline
- `Context` manages WebGPU device and surface
- `Pipeline` compiles Slang shaders and manages render pipelines
- Multi-pass rendering: backgrounds → text → text masks → windows → cursor → final composite
- `RenderTexture` manages intermediate framebuffers
- Font rendering (`gfx/font_rendering/`):
  - `font.cpp` - FreeType font loading and glyph extraction
  - `shape_drawing.cpp` - Rasterizes glyph outlines using Blend2D
  - `texture_atlas.cpp` - Packs rendered glyphs into GPU textures
  - `shape_pen.cpp` - Converts FreeType outlines to Blend2D paths

**Application Layer (`src/app/`):**
- `sdl_window.cpp/hpp` - SDL3 window creation and management
- `sdl_event.cpp/hpp` - SDL event polling and dispatching
- `input.cpp/hpp` - Keyboard and mouse input handling, converts SDL events to Neovim format
- `ime.cpp/hpp` - Input method editor support for international text input
- `options.cpp/hpp` - Application-wide options (titlebar, blur, gamma, vsync, margins, etc.)
- `size.cpp/hpp` - Window and grid size calculations
- `path.cpp/hpp` - Resource path resolution

**Utilities (`src/utils/`):**
- `thread.hpp` - Thread synchronization primitives (`Sync<T>`)
- `logger.cpp/hpp` - Logging utilities
- `unicode.cpp/hpp` - UTF-8 string handling
- `timer.cpp/hpp` - Frame timing
- `clock.cpp/hpp` - High-resolution time measurements

### Rendering Pipeline

The renderer uses a multi-pass approach:

1. **Rect pass** - Renders background rectangles for grid cells
2. **Text pass** - Renders text glyphs from texture atlas
3. **Text mask pass** - Applies underline/undercurl decorations
4. **Windows pass** - Composites all windows with proper blending
5. **Cursor pass** - Renders cursor with smooth animations
6. **Final pass** - Presents to screen with gamma correction

Shaders are written in Slang (located in `res/shaders/`) and compiled at runtime. The pipeline compiles separate pipelines for rects, text, text masks, windows, cursor, and final presentation.

### Threading Model

- **Main/Render Thread**: Runs SDL event loop, processes events, renders frames
- **Event Processing**: Tasks are queued via `EventManager::AddTask()` and executed on render thread
- **Nvim Threads**: Each session's Nvim instance runs msgpack-rpc communication in background threads
- Use `Sync<T>` wrapper (in `utils/thread.hpp`) for thread-safe access to shared data

### Resource Loading

Resources are embedded into the app bundle for release builds:
- Shaders: `res/shaders/*.slang`
- Fonts: `res/fonts/` (fallback fonts)
- Lua scripts: `res/lua/` (initialization scripts)

Use `PathHelper` to resolve resource paths correctly in both development and bundled builds.

## Code Style

- C++23 with modules support (some files use modules, see `CXXModules.json`)
- Use modern C++ features: `std::span`, ranges, concepts where appropriate
- Headers use `#pragma once`
- Compile flags: `-Wall -Wextra -Wshadow -pedantic` with specific suppressions
- Formatting: `.clang-format` is configured (LLVM style)
- Experimental: `-fexperimental-library` needed for `std::jthread` and other C++23 features

## Important Notes

- **macOS-specific code**: `*.mm` files contain Objective-C++ for macOS window management and font location
- **WebGPU backend**: Uses Dawn (Google's WebGPU implementation), fetched via CMake FetchContent
- **Shader language**: Slang shaders compile to Metal on macOS
- **Boost usage**: Process spawning, Unicode handling, program options
- **ccache**: Build uses ccache for faster recompilation (configured in `build-setup`)
- **Code signing**: Release builds are ad-hoc signed with `codesign --force --deep --sign -`

## Neogurt API

The application exposes commands to Neovim via `vim.g.neogurt_cmd()`. Key commands:
- `session_new`, `session_kill`, `session_switch`, `session_restart` - Session management
- `session_list`, `session_info`, `session_select` - Session queries
- `option_set` - Update runtime options (opacity, margins, vsync, etc.)
- `font_size_change`, `font_size_reset` - Font size adjustments

Commands are parsed in `event/neogurt_cmd.cpp` and dispatched to `SessionManager`.
