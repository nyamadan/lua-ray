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
        -- すべてのキーがboolean型であることを確認
        assert(type(state.w) == "boolean", "w key should be boolean")
        assert(type(state.a) == "boolean", "a key should be boolean")
        assert(type(state.s) == "boolean", "s key should be boolean")
        assert(type(state.d) == "boolean", "d key should be boolean")
        assert(type(state.q) == "boolean", "q key should be boolean")
        assert(type(state.e) == "boolean", "e key should be boolean")
        return true
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
}

TEST_F(SdlKeyboardTest, KeyboardStateDefaultsToFalse) {
    // テスト環境ではキーが押されていないのですべてfalse
    auto result = lua.safe_script(R"(
        local state = app.get_keyboard_state()
        assert(state.w == false, "w should be false by default")
        assert(state.a == false, "a should be false by default")
        assert(state.s == false, "s should be false by default")
        assert(state.d == false, "d should be false by default")
        assert(state.q == false, "q should be false by default")
        assert(state.e == false, "e should be false by default")
        return true
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
}
