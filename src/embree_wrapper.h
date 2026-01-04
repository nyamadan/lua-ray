#pragma once
#include <embree4/rtcore.h>
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

// Embree Wrappers
int api_embree_new_device(lua_State* L);
int api_embree_new_scene(lua_State* L);
int api_embree_add_sphere(lua_State* L);
int api_embree_commit_scene(lua_State* L);
int api_embree_intersect(lua_State* L);
int api_embree_release_scene(lua_State* L);
int api_embree_release_device(lua_State* L);
