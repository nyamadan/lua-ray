---
description: Build and Run the Lua Ray Tracer
---

1. Configure the project with CMake

```bash
cmake -S . -B build/gcc-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

// turbo 2. Build the project

```bash
cmake --build build/gcc-debug --parallel 4 --config Debug --target all
```

3. Run the application

```bash
./build/gcc-debug/lua-ray
```
