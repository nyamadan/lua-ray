# Suggested Commands for lua-ray Development

## Build Commands
- **Build the Project**:
  ```bash
  cmake --build ./build/gcc-debug --parallel 4 --config Debug --target all
  ```
- **Run the Application**:
  ```bash
  ./build/gcc-debug/lua-ray
  ```
- **Run Tests**:
  ```bash
  ctest --test-dir ./build/gcc-debug --exclude-regex "^prim"
  ```

## Dependency Management
- **Install Node.js Dependencies**:
  ```bash
  pnpm install
  ```

## Utility Commands
- **List Files**:
  ```bash
  ls
  ```
- **Change Directory**:
  ```bash
  cd <directory>
  ```
- **Search for Files**:
  ```bash
  find . -name "filename"
  ```
- **Search for Patterns in Files**:
  ```bash
  grep "pattern" <file>
  ```
- **Git Commands**:
  ```bash
  git status
  git add <file>
  git commit -m "message"
  git push
  ```

## Testing and Debugging
- **Run Specific Test**:
  ```bash
  ctest -R <test_name>
  ```
- **Debug Build**:
  ```bash
  cmake --build ./build/gcc-debug --config Debug
  ```

## Code Style and Formatting
- **Indentation**: Use 4 spaces for C++ code.
- **Naming**: Use camelCase for variables and functions.
- **Error Handling**: Ensure proper error handling for SDL operations.

## Version Control
- **Check Git Status**:
  ```bash
  git status
  ```
- **Add Files to Git**:
  ```bash
  git add <file>
  ```
- **Commit Changes**:
  ```bash
  git commit -m "message"
  ```
- **Push Changes**:
  ```bash
  git push
  ```

## System Commands
- **Check Current Directory**:
  ```bash
  pwd
  ```
- **List Directory Contents**:
  ```bash
  ls -la
  ```
- **Remove Files/Directories**:
  ```bash
  rm -rf <file/directory>
  ```
- **Create Directory**:
  ```bash
  mkdir <directory>
  ```