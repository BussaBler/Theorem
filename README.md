# Axiom Game Engine

## Introduction

Axiom is a game engine project with an editor application called Theorem. The idea behind this project is to use minimal external libraries for development, such as ImGui, GLM, GLFW and others (currently only using external libs for font rendering, MSDFGen and shader compilation, shaderc | spirv-cross); everything will be done internally within this repository or other submodule repositories. This project is for recreational purposes only and is currently in early development stages.

## Build System

The project uses CMake as its build system (minimum version 3.20). The build script automatically detects your host operating system and routes compiled executables and libraries cleanly into `build/Bin/Runtime`, `build/Bin/Shared`, and `build/Bin/Static`.

It is highly recommended to use the **Ninja** build system generator for maximum compilation speed on macOS and Linux, and **Visual Studio** for Windows.

### Available Configurations

You can customize your build by passing the following arguments to the `cmake` generation command using the `-D` flag:

* **`CMAKE_BUILD_TYPE`**: Build configuration. Allowed values: `Debug`, `Release`. *(Default: `Debug`)*
* **`AX_RENDERER`**: Graphics API backend. Allowed values: `vulkan`, `dx12`, `metal`. *(Default: `vulkan`)*
* **`AX_COMPILER`**: Specify the compiler toolchain. Allowed values: `msvc`, `gcc`, `g++`, `clang`. *(Note: CMake automatically detects your host's native compiler by default).*

### Build Flags

* **`AX_VERBOSE`**: Enable verbose output to see raw command lines during compilation (`-DAX_VERBOSE=ON`).
* **`AX_NO_COLOR`**: Disable colored terminal output (`-DAX_NO_COLOR=ON`).

> [!NOTE]  
> Currently the font renderer only supports fonts that do not contain self-intersecting outlines. If you notice rendering issues on fonts downloaded from websites such as Google Fonts, try downloading the font from the font author's official website instead.

> [!NOTE]  
> To enable Vulkan Validation Layers in Debug builds, ensure the Vulkan SDK is installed on your system. These layers provide essential error checking and debugging information that is not present in the standard driver.

### Example Build Commands

The standard CMake workflow is a two-step process: **Configure** (generate the build files) and **Build** (compile the code).

**macOS (Using Ninja):**
Build a Debug version using the Metal backend.
```bash
# 1. Configure the project
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DAX_RENDERER=metal

# 2. Compile the engine
cmake --build build
