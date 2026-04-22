.PHONY: build run release clean test configure

BUILD_DIR := build/default
RELEASE_DIR := build/release

configure:
	cmake --preset default

build: | $(BUILD_DIR)
	cmake --build $(BUILD_DIR)

run: build
	$(BUILD_DIR)/screencopy

release:
	cmake --preset release
	cmake --build $(RELEASE_DIR)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	rm -rf build

$(BUILD_DIR):
	cmake --preset default
