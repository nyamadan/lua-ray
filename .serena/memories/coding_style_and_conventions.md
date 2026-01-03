# Coding Style and Conventions for lua-ray

## General Guidelines
- **Language**: C++17 is the primary language for the project.
- **Indentation**: Use 4 spaces for indentation. Do not use tabs.
- **Naming Conventions**: Use descriptive and clear names for variables, functions, and classes. Follow camelCase for variables and functions (e.g., `myVariable`, `myFunction`).
- **Error Handling**: Ensure proper error handling, especially for SDL operations. Use exceptions or return codes as appropriate.

## Code Structure
- **Header Files**: Use header files to declare functions, classes, and variables. Include guards to prevent multiple inclusions.
- **Source Files**: Implement functions and classes in source files. Keep implementations concise and focused.
- **Comments**: Use comments to explain complex logic, assumptions, and important details. Avoid redundant comments.

## SDL Operations
- **Initialization**: Always check the return values of SDL initialization functions. Handle errors gracefully.
- **Resource Management**: Use RAII (Resource Acquisition Is Initialization) for managing SDL resources like windows, renderers, and textures.
- **Event Handling**: Implement proper event handling loops for user input and system events.

## Lua Integration
- **Embedding Lua**: Use the Lua C API to embed Lua scripts in the application. Ensure proper error handling for Lua operations.
- **Scripting**: Provide clear documentation for Lua scripts and their integration points with the C++ code.

## Embree Integration
- **Ray Tracing**: Use Embree for high-performance ray tracing. Follow Embree's best practices for scene setup and rendering.
- **Error Handling**: Check for errors in Embree operations and handle them appropriately.

## Testing
- **Unit Testing**: Use Google Test for writing unit tests. Ensure tests are comprehensive and cover edge cases.
- **Test Execution**: Run tests using the `ctest` command in the build directory. Exclude tests as necessary using regex patterns.

## Documentation
- **Code Documentation**: Use Doxygen-style comments for documenting functions, classes, and modules.
- **Project Documentation**: Maintain up-to-date documentation in markdown files (e.g., `AGENTS.md`).

## Version Control
- **Commit Messages**: Use conventional commit messages to describe changes. Reference related issues in commit descriptions.
- **Branching**: Follow a consistent branching strategy. Use feature branches for new features and bug fixes.
- **Pull Requests**: Ensure code compiles successfully and passes all tests before submitting a pull request.

## Code Reviews
- **Review Process**: Conduct thorough code reviews for all changes. Ensure code adheres to the project's style and conventions.
- **Feedback**: Provide constructive feedback during code reviews. Address feedback promptly and thoroughly.

## Continuous Integration
- **CI/CD**: Set up continuous integration and deployment pipelines to automate testing and deployment.
- **Automated Testing**: Run automated tests on every commit to ensure code quality and stability.

## Best Practices
- **Modularity**: Keep code modular and reusable. Avoid tight coupling between components.
- **Performance**: Optimize critical sections of the code. Use profiling tools to identify bottlenecks.
- **Security**: Follow secure coding practices. Sanitize inputs and handle errors securely.
- **Maintainability**: Write code that is easy to understand, maintain, and extend. Avoid overly complex solutions.