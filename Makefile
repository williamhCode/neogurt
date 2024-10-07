.PHONY: build

# TYPE = release
TYPE = debug

build:
	cmake --build build/$(TYPE) --target neogurt
	cp build/$(TYPE)/compile_commands.json .

build-dawn:
	cmake --build build/$(TYPE) --target dawn-single-lib

build-tint:
	# cmake --build build/release --target tint
	# cp build/release/_deps/dawn-build/tint .

build-setup: build-setup-debug build-setup-release

build-setup-debug:
	cmake . -B build/debug \
		-DCMAKE_BUILD_TYPE=Debug \
		-GNinja \
		-DCMAKE_C_COMPILER=clang \
		-DCMAKE_CXX_COMPILER=clang++ \
		-DCMAKE_C_COMPILER_LAUNCHER=ccache \
		-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
		-DCMAKE_COLOR_DIAGNOSTICS=ON \
		-DSDL_SHARED=ON

build-setup-release:
	cmake . -B build/release \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo \
		-GNinja \
		-DCMAKE_C_COMPILER=clang \
		-DCMAKE_CXX_COMPILER=clang++ \
		-DCMAKE_C_COMPILER_LAUNCHER=ccache \
		-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
		-DCMAKE_COLOR_DIAGNOSTICS=ON \
		-DSDL_SHARED=OFF \
		-DSDL_STATIC=ON

xcode-setup:
	cmake . -B xcode -GXcode

run:
	build/$(TYPE)/neogurt

gen-app:
	mkdir -p gen/out/Neogurt.app
	mkdir -p gen/out/Neogurt.app/Contents/{Resources,MacOS}
	cp res/Info.plist gen/out/Neogurt.app/Contents
	cp -r res/lua gen/out/Neogurt.app/Contents/Resources
	cp -r res/shaders gen/out/Neogurt.app/Contents/Resources
	cp build/release/neogurt gen/out/Neogurt.app/Contents/MacOS
