#!/bin/bash

mkdir -p build/resources/shaders/metal/main
xcrun -sdk macosx metal resources/shaders/metal/main/main.metal -c -o build/resources/shaders/metal/main/main.air
xcrun -sdk macosx metallib \
  build/resources/shaders/metal/main/main.air \
  -o build/resources/shaders/metal/lib.metallib

# ./dui/build.sh
clang++ \
  -g -std=c++17 -fno-exceptions \
  -framework Foundation -framework Metal -framework Quartzcore -framework IOKit -framework Cocoa -framework OpenGL \
  src/editor_main.cpp \
  -o ./build/editor.exe \
  -I ./src/ -I ./ -I ./third_party/ \
  -I ./third_party/glm/ \
  -I ./third_party/metal-cpp  ./src/gpu/metal/glfw_bridge.mm ./src/gpu/metal/metal_implementation.cpp \
  -I ./third_party/glfw/include ./third_party/glfw/build/src/libglfw3.a \
  -I ./third_party/freetype/include ./third_party/freetype/build/libfreetype.a \
  # -I "C:\Users\Asad\VulkanSdk\1.3.231.1\Include" "C:\Users\Asad\VulkanSdk\1.3.231.1\Lib\vulkan-1.lib"  \
  # -I ./third_party/assimp/include   ./third_party/assimp/lib/assimp-vc141-mt.lib \
