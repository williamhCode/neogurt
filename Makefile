.PHONY: build

TYPE = release
# TYPE = debug

build:
	cmake --build build/$(TYPE) --target neogui
	cp build/$(TYPE)/compile_commands.json .

# build-tint:
# 	cmake --build build/release --target tint
# 	cp build/release/_deps/dawn-build/tint .

# https://cmake.org/cmake/help/latest/variable/CMAKE_LINKER_TYPE.html
# mold linker not working
build-setup:
	cmake . -B build/debug \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_C_FLAGS_DEBUG="-g -O1" \
		-DCMAKE_CXX_FLAGS_DEBUG="-g -O1" \
		-GNinja \
		-DCMAKE_C_COMPILER=clang \
		-DCMAKE_CXX_COMPILER=clang++ \
		-DCMAKE_C_COMPILER_LAUNCHER=ccache \
		-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
		-DCMAKE_COLOR_DIAGNOSTICS=ON
	cmake . -B build/release \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo \
		-GNinja \
		-DCMAKE_C_COMPILER=clang \
		-DCMAKE_CXX_COMPILER=clang++ \
		-DCMAKE_C_COMPILER_LAUNCHER=ccache \
		-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
		-DCMAKE_COLOR_DIAGNOSTICS=ON

# TODO: use ccache for xcode
xcode-setup:
	cmake . -B xcode -GXcode \
		-DCMAKE_C_COMPILER_LAUNCHER=ccache \
		-DCMAKE_CXX_COMPILER_LAUNCHER=ccache

run:
	build/$(TYPE)/neogui

