.PHONY: build

TYPE = release

build:
	cmake --build build/$(TYPE) --target App
	cp build/$(TYPE)/compile_commands.json .

build-tint:
	cmake --build build/$(TYPE) --target tint
	cp build/$(TYPE)/_deps/dawn-build/tint .

build-setup:
	cmake . -B build/debug -DCMAKE_BUILD_TYPE=RelWithDebInfo -GNinja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
	cmake . -B build/release -DCMAKE_BUILD_TYPE=Release -GNinja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

xcode-setup:
	cmake . -B xcode -GXcode

run:
	build/$(TYPE)/App

