# Theorem Editor

Theorem is the primary editor and visual application built on top of the **Axiom Game Engine**.

Designed for early-stage development, Theorem serves as the front-end interface, while delegating all the back end work such as windowing, cross-platform rendering (Vulkan, DX12, Metal), and shader compilation to the Axiom engine

## Getting Started

Because the underlying Axiom engine and its dependencies are bundled as Git submodules, you **must** clone this repository recursively to download all the required files:

```bash
git clone --recursive git@github.com:BussaBler/Theorem.git
cd Theorem
```

*If you already cloned the repository without the `--recursive` flag, you can fetch the submodules manually:*
```bash
git submodule update --init --recursive
```

## Build System

Theorem uses **CMake** (minimum version 3.20) to manage the build process. The script will automatically detect your host operating system, configure the Axiom submodule, and route compiled binaries cleanly into `build/Bin/Runtime`.

### Configuration Options
You can customize the build using the following CMake arguments (`-D`):

* **`CMAKE_BUILD_TYPE`**: `Debug` or `Release`. *(Default: Debug)*
* **`AX_RENDERER`**: Graphics API backend. `vulkan`, `dx12`, or `metal`. *(Default: vulkan)*

### Example Build Commands

**macOS & Linux (Using Ninja):**
```bash
# Generate build files (e.g., Debug with Metal)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DAX_RENDERER=metal

# Compile the project
cmake --build build
```

**Windows (Visual Studio):**
```cmd
:: Generate Visual Studio solution (e.g., with DirectX 12)
cmake -B build -G "Visual Studio 17 2022" -DAX_RENDERER=dx12

:: Compile the project
cmake --build build --config Debug
```

## Running the Editor

Once compiled, you can launch the editor directly from the runtime output directory.

```bash
./build/Bin/Runtime/Theorem
```
