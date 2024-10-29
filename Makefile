.PHONY: build

# TYPE = debug
TYPE = release

build:
	cmake --build build/$(TYPE) --target neogurt
	cp build/$(TYPE)/compile_commands.json .
	if [ "$(TYPE)" = "release" ]; then \
		cp build/release/neogurt.app/Contents/Macos/neogurt build/release; \
	fi

build-setup: build-setup-debug build-setup-release

build-setup-debug:
	cmake . -B build/debug \
		-D CMAKE_BUILD_TYPE=Debug \
		-GNinja \
		-D CMAKE_C_COMPILER=clang \
		-D CMAKE_CXX_COMPILER=clang++ \
		-D CMAKE_C_COMPILER_LAUNCHER=ccache \
		-D CMAKE_CXX_COMPILER_LAUNCHER=ccache \
		-D CMAKE_COLOR_DIAGNOSTICS=ON \
		-D SDL_SHARED=ON

build-setup-release:
	cmake . -B build/release \
		-D CMAKE_BUILD_TYPE=RelWithDebInfo \
		-GNinja \
		-D CMAKE_C_COMPILER=clang \
		-D CMAKE_CXX_COMPILER=clang++ \
		-D CMAKE_C_COMPILER_LAUNCHER=ccache \
		-D CMAKE_CXX_COMPILER_LAUNCHER=ccache \
		-D CMAKE_COLOR_DIAGNOSTICS=ON \
		-D SDL_SHARED=OFF \
		-D SDL_STATIC=ON \
		-D BLEND2D_STATIC=ON

xcode-setup:
	cmake . -B xcode -GXcode

run:
	build/$(TYPE)/neogurt

package:
	cmake --build build/release --target package
