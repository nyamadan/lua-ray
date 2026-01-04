#pragma once
#include "context.h"
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

// Global context reference needed for draw_pixel
extern Context g_ctx;

int api_draw_pixel(lua_State* L);

void register_lua_api(lua_State* L);
lua_State* init_lua();
void run_script(lua_State* L, int argc, char* argv[]);
