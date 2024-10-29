# Neogurt
Blazingly fast Neovim GUI written in C++23 and WebGPU.

<img width="840" alt="Neogurt screenshot" src="https://github.com/user-attachments/assets/6affed8f-88fe-4e4c-8698-9a543c0f9f57">

## Features
- Blazingly fast and energy efficient
- Smooth scrolling and cursor out of the box
- Session support, similar to tmux (not detachable yet, wip)
- Supported on macOS, Windows and Linux WIP

## Build Instructions
```
git clone https://github.com/williamhCode/neogurt.git --recurse-submodules
git submodule update  # if submodules become out of sync
```

- Install the latest version of LLVM Clang
- Make sure Boost is installed and its headers are locatable
- All other depedencies are bundled
- Edit Makefile to switch between debug and release mode
```sh
make build-setup
make build

make run  # runs executable

make package  # builds .dmg file in build/release
```
