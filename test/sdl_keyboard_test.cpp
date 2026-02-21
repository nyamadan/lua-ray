#include <gtest/gtest.h>
#include "../src/lua_binding.h"
#include <sol/sol.hpp>

class SdlKeyboardTest : public ::testing::Test {
protected:
    void SetUp() override {
        AppContext ctx;
        bind_lua(lua, ctx);
    }

    sol::state lua;
};

TEST_F(SdlKeyboardTest, GetKeyboardStateReturnsTable) {
    auto result = lua.safe_script(R"(
        local state = app.get_keyboard_state()
        assert(type(state) == "table", "get_keyboard_state should return a table")
        return true
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
}

TEST_F(SdlKeyboardTest, KeyboardStateContainsExpectedKeys) {
    auto result = lua.safe_script(R"(
        local state = app.get_keyboard_state()
        -- 汎用化: w,a,s,d や space, up などのキーが含まれるか確認
        assert(type(state["w"]) == "boolean", "w key should be boolean")
        assert(type(state["space"]) == "boolean", "space key should be boolean")
        assert(type(state["up"]) == "boolean", "up key should be boolean")
        assert(type(state["escape"]) == "boolean", "escape key should be boolean")
        return true
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
}

TEST_F(SdlKeyboardTest, KeyboardStateDefaultsToFalse) {
    // テスト環境ではキーが押されていないのですべてfalse
    auto result = lua.safe_script(R"(
        local state = app.get_keyboard_state()
        assert(state["w"] == false, "w should be false by default")
        assert(state["space"] == false, "space should be false by default")
        assert(state["up"] == false, "up should be false by default")
        assert(state["escape"] == false, "escape should be false by default")
        return true
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
}
