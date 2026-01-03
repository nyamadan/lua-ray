# Project Overview: lua-ray

## Purpose
The `lua-ray` project is a C++/WebAssembly application that integrates Lua scripting with ray tracing capabilities using Embree. It leverages SDL3 for windowing and input handling, making it suitable for both native and web-based environments.

## Tech Stack
- **C++17**: Core language for the application.
- **SDL3**: Used for windowing and input handling.
- **Lua 5.4.6**: Embedded scripting language for extensibility.
- **Embree 4.4.0**: High-performance ray tracing library.
- **Emscripten**: For compiling to WebAssembly (WASM) to enable web deployment.
- **CMake**: Build system for managing dependencies and compilation.
- **Node.js/pnpm**: For managing JavaScript dependencies and tooling.
- **Google Test**: For unit testing (excluded in WebAssembly builds).

## Project Structure
- `main.cpp`: Entry point for the application.
- `CMakeLists.txt`: Build configuration for CMake.
- `package.json`: Node.js package management.
- `.emsdk/`: Emscripten SDK for WebAssembly compilation.
- `build/`: Output directory for CMake builds.
- `test/`: Contains test files for Lua and Embree integration.

## Key Features
- **Cross-platform**: Supports both native and web deployment via WebAssembly.
- **Scripting**: Embedded Lua for runtime scripting and extensibility.
- **Ray Tracing**: Uses Embree for high-performance ray tracing.
- **Testing**: Google Test framework for unit testing (native builds only).

## Build and Development Commands
- **Build**: `cmake --build ./build/gcc-debug --parallel 4 --config Debug --target all`
- **Run**: `./build/gcc-debug/lua-ray`
- **Test**: `ctest --test-dir ./build/gcc-debug --exclude-regex "^prim"`
- **Install Dependencies**: `pnpm install`

## Coding Style
- **Indentation**: 4 spaces.
- **Naming**: Descriptive and clear variable and function names in camelCase.
- **Error Handling**: Proper error handling for SDL operations.

## Testing Guidelines
- **Framework**: CTest with Google Test.
- **Execution**: Run tests using `ctest` in the build directory.

## Commit Guidelines
- Use conventional commit messages.
- Reference related issues in commit descriptions.
- Ensure code compiles successfully before submitting a PR.