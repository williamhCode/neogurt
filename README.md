# Neogui
Blazingly fast neovim gui written in C++20 and WebGPU.  

### Key Features
- Blazingly fast and energy efficient
- Smooth scrolling and cursor out of the box
- Detachable sessions (similiar to tmux)

### Build Instructions
- Make sure Boost is installed and its headers are locatable
- All other depedencies are bundled
- Edit Makefile to switch between debug and release mode
```sh
make build-setup
make build-dawn  # only needs to be run once
make build
make run
```
