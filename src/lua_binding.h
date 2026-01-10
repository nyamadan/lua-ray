#pragma once
#include "context.h"
#include <sol/sol.hpp>

// Global context reference needed for draw_pixel
extern Context g_ctx;

void bind_lua(sol::state& lua);
void run_script(sol::state& lua, int argc, char* argv[]);
