---
trigger: model_decision
description: when build or test the app. References package.json scripts.
---

1. Install dependencies (if needed)

```bash
pnpm install
```

2. Build the project

```bash
pnpm run build
```

3. Run the tests

```bash
pnpm run test
```

4. Run the application

```bash
./build/gcc-debug/lua-ray
```
