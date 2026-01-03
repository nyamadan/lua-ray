# Task Completion Checklist for lua-ray

## Before Starting a Task
1. **Understand the Task**: Ensure you fully understand the task requirements and objectives.
2. **Review Documentation**: Check the project documentation (`AGENTS.md`, `README.md`, etc.) for relevant guidelines.
3. **Check Dependencies**: Ensure all dependencies are installed and up-to-date.

## During Task Execution
1. **Follow Coding Style**: Adhere to the project's coding style and conventions.
2. **Write Tests**: Write unit tests for new features or bug fixes. Ensure tests cover edge cases.
3. **Document Changes**: Update documentation to reflect any changes made to the codebase.
4. **Handle Errors**: Implement proper error handling for all operations, especially SDL and Lua interactions.

## After Completing a Task
1. **Run Tests**: Execute the test suite to ensure all tests pass.
   ```bash
   ctest --test-dir ./build/gcc-debug --exclude-regex "^prim"
   ```
2. **Build the Project**: Ensure the project builds successfully without errors.
   ```bash
   cmake --build ./build/gcc-debug --parallel 4 --config Debug --target all
   ```
3. **Run the Application**: Verify that the application runs as expected.
   ```bash
   ./build/gcc-debug/lua-ray
   ```
4. **Check for Warnings**: Address any compiler warnings or static analysis issues.
5. **Review Changes**: Conduct a self-review of the changes to ensure they meet the project's standards.

## Before Submitting a Pull Request
1. **Update Documentation**: Ensure all documentation is up-to-date and reflects the changes made.
2. **Commit Messages**: Use conventional commit messages to describe changes. Reference related issues.
3. **Push Changes**: Push the changes to the remote repository.
   ```bash
   git push
   ```
4. **Create Pull Request**: Submit a pull request with a clear description of the changes and their purpose.
5. **Address Feedback**: Respond to any feedback from code reviews promptly and thoroughly.

## Additional Checks
- **Performance**: Ensure the changes do not negatively impact performance. Use profiling tools if necessary.
- **Security**: Verify that the changes do not introduce security vulnerabilities.
- **Compatibility**: Ensure the changes are compatible with the project's target platforms (native and WebAssembly).
- **Code Quality**: Use static analysis tools to check for code quality issues.

## Post-Submission
1. **Monitor CI/CD**: Check the continuous integration and deployment pipelines for any issues.
2. **Address Issues**: Fix any issues identified during the CI/CD process.
3. **Update Documentation**: Update any additional documentation based on feedback or changes.

By following this checklist, you can ensure that your tasks are completed thoroughly and meet the project's standards.