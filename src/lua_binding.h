#pragma once
#include <sol/sol.hpp>

#include <string>

struct AppConfig {
    int width = 800;
    int height = 600;
    std::string title = "Lua Ray Tracing";
};

// Bind Lua functions.
void bind_lua(sol::state& lua, AppConfig& config);
// Run script and return the script's return value (if any) as a sol::object.
sol::object run_script(sol::state& lua, int argc, char* argv[]);
