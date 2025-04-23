.PHONY: build

# flags
REL ?= 0
TEST ?= 0

# variables
IS_DEBUG = $(filter 0,$(REL))

TYPE = $(if $(IS_DEBUG),debug,release)
BUILD_TYPE = $(if $(IS_DEBUG),Debug,RelWithDebInfo)

ifeq ($(TEST),1)
BUILD_DIR = build/tests/$(TYPE)
TARGET = tests
else
BUILD_DIR = build/$(TYPE)
TARGET = neogurt
endif

# targets
build:
	cmake --build $(BUILD_DIR) --target $(TARGET)
ifeq ($(TEST),0)
	cp $(BUILD_DIR)/compile_commands.json .
	if [ "$(REL)" = "1" ]; then \
		cp build/release/neogurt.app/Contents/MacOS/neogurt build/release; \
	fi
endif

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
	$(BUILD_DIR)/$(TARGET)

package:
	cmake --build build/release --target package

xcode-setup:
	cmake . -B xcode -GXcode
