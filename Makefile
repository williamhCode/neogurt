.PHONY: build

TYPE = debug

build:
	cmake --build build/$(TYPE) --target App
	cp build/$(TYPE)/compile_commands.json .

build-tint:
	cmake --build build/$(TYPE) --target tint
	cp build/$(TYPE)/_deps/dawn-build/tint .

build-setup:
	cmake . -B build/debug -DCMAKE_BUILD_TYPE=RelWithDebInfo -GNinja
	cmake . -B build/release -DCMAKE_BUILD_TYPE=Release -GNinja

xcode-setup:
	cmake . -B xcode -GXcode

run:
	build/$(TYPE)/App

