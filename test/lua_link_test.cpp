#include <gtest/gtest.h>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

TEST(LuaLinkTest, CanCreateStateAndLoadStandardLibraries) {
    lua_State* L = luaL_newstate();
    ASSERT_NE(L, nullptr) << "luaL_newstate failed";
    // Load standard libraries
    luaL_openlibs(L);
    // Simple Lua script to add two numbers
    const char* script = "return 1 + 1";
    int status = luaL_loadstring(L, script);
    EXPECT_EQ(status, LUA_OK) << "luaL_loadstring failed";
    if (status == LUA_OK) {
        status = lua_pcall(L, 0, LUA_MULTRET, 0);
        EXPECT_EQ(status, LUA_OK) << "lua_pcall failed";
        if (status == LUA_OK) {
            int isnum = 0;
            lua_Number result = lua_tonumberx(L, -1, &isnum);
            EXPECT_TRUE(isnum);
            EXPECT_EQ(static_cast<int>(result), 2);
        }
    }
    lua_close(L);
}

TEST(LuaLinkTest, VersionMacroIsPresent) {
    // Just verify that compiling linked to Lua headers succeeds and we can read version
    // LUA_VERSION_NUM is defined in lua.h
    EXPECT_GE(LUA_VERSION_NUM, 500);
}
