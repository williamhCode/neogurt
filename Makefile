.PHONY: build

# flags
REL ?= 0
TARGET ?= neogurt

# variables
IS_DEBUG = $(filter 0,$(REL))

BUILD_TYPE = $(if $(IS_DEBUG),Debug,RelWithDebInfo)

TYPE = $(if $(IS_DEBUG),debug,release)
BUILD_DIR = build/$(TYPE)

# targets
build:
	cmake --build $(BUILD_DIR) --target $(TARGET)
	cp $(BUILD_DIR)/compile_commands.json .
	if [ "$(REL)" = "1" ]; then \
		cp build/release/neogurt.app/Contents/MacOS/neogurt build/release; \
	fi

build-setup:
	cmake . -B $(BUILD_DIR) \
		-D CMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-G Ninja \
		-D CMAKE_C_COMPILER=clang \
		-D CMAKE_CXX_COMPILER=clang++ \
		-D CMAKE_C_COMPILER_LAUNCHER=ccache \
		-D CMAKE_CXX_COMPILER_LAUNCHER=ccache \
		-D CMAKE_COLOR_DIAGNOSTICS=ON

run:
ifeq ($(TARGET),tests)
	ctest --test-dir $(BUILD_DIR) --output-on-failure -V
else
	$(BUILD_DIR)/$(TARGET)
endif

package:
	cmake --build build/release --target package

xcode-setup:
	cmake . -B xcode -GXcode
