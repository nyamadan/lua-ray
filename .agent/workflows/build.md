---
description: Build and Run the Lua Ray Tracer
---

1. Build the project

```bash
cmake --build build/gcc-debug
```

2. Ran the tests

```bash
ctest --test-dir ./build/gcc-debug --exclude-regex "^prim_"
```

3. Run the application

```bash
./build/gcc-debug/lua-ray
```
