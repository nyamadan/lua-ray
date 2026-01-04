#include <gtest/gtest.h>
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
int luaopen_buffer(lua_State* L);
}

class LbufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        L = luaL_newstate();
        luaL_openlibs(L);
        // Register lbuffer
        luaL_requiref(L, "buffer", luaopen_buffer, 1);
        lua_pop(L, 1); // Remove buffer module from stack
    }

    void TearDown() override {
        lua_close(L);
    }

    lua_State* L;
};

TEST_F(LbufferTest, CreateBuffer) {
    const char* script = R"(
        local B = require 'buffer'
        local b = B.new()
        assert(B.isbuffer(b))
        assert(#b == 0)
    )";
    ASSERT_EQ(luaL_dostring(L, script), LUA_OK) << lua_tostring(L, -1);
}

TEST_F(LbufferTest, WriteAndReadString) {
    const char* script = R"(
        local B = require 'buffer'
        local b = B.new()
        b:set("Hello World")
        assert(#b == 11)
        assert(tostring(b) == "Hello World")
    )";
    ASSERT_EQ(luaL_dostring(L, script), LUA_OK) << lua_tostring(L, -1);
}

TEST_F(LbufferTest, PackAndUnpack) {
    const char* script = R"(
        local B = require 'buffer'
        local b = B.new()
        -- Unpack
        local i, s = b:unpack(1, "<is")
        assert(i == 42, "Integer mismatch")
        assert(s == "lua", "String mismatch")
    )";
    ASSERT_EQ(luaL_dostring(L, script), LUA_OK) << lua_tostring(L, -1);
}
