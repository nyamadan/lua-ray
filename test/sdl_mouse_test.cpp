#include <gtest/gtest.h>
#include "../src/lua_binding.h"
#include <sol/sol.hpp>

class SdlMouseTest : public ::testing::Test {
protected:
    void SetUp() override {
        AppContext ctx;
        bind_lua(lua, ctx);
    }

    sol::state lua;
};

TEST_F(SdlMouseTest, GetMouseStateReturnsTable) {
    auto result = lua.safe_script(R"(
        local state = app.get_mouse_state()
        assert(type(state) == "table", "get_mouse_state should return a table")
        return true
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
}

TEST_F(SdlMouseTest, MouseStateContainsExpectedFields) {
    auto result = lua.safe_script(R"(
        local state = app.get_mouse_state()
        assert(type(state.x) == "number", "x should be a number")
        assert(type(state.y) == "number", "y should be a number")
        assert(type(state.rel_x) == "number", "rel_x should be a number")
        assert(type(state.rel_y) == "number", "rel_y should be a number")
        assert(type(state.left) == "boolean", "left boolean required")
        assert(type(state.middle) == "boolean", "middle boolean required")
        assert(type(state.right) == "boolean", "right boolean required")
        return true
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
}
