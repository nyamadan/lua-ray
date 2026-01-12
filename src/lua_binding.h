#pragma once
#include <sol/sol.hpp>

// Bind Lua functions. No Context concept — texture is provided to Lua as lightuserdata.
void bind_lua(sol::state& lua);
// Run script and return the script's return value (if any) as a sol::object.
sol::object run_script(sol::state& lua, int argc, char* argv[]);
