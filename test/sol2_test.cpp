#include <gtest/gtest.h>
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

TEST(Sol2Test, BasicBinding) {
    sol::state lua;
    lua.open_libraries(sol::lib::base);

    lua["answer"] = 42;
    int answer = lua["answer"];
    EXPECT_EQ(answer, 42);
}

TEST(Sol2Test, FunctionBinding) {
    sol::state lua;
    lua.open_libraries(sol::lib::base);

    lua.set_function("add", [](int a, int b) {
        return a + b;
    });

    int result = lua["add"](10, 20);
    EXPECT_EQ(result, 30);
}

