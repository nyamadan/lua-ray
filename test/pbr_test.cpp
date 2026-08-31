#include <gtest/gtest.h>
#include <sol/sol.hpp>

class PbrTest : public ::testing::Test {
protected:
    sol::state lua;

    void SetUp() override {
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math,
                           sol::lib::string, sol::lib::table);
        lua["package"]["path"] = "?.lua;?/init.lua;lib/?.lua";
    }
};

TEST_F(PbrTest, ConvertsSrgbAndToneMapsToFiniteRange) {
    auto result = lua.safe_script(R"(
        local PBR = require('lib.PBR')
        local linear = PBR.srgb_to_linear(0.5)
        assert(math.abs(linear - 0.214041) < 0.0001)
        local mapped = PBR.aces_tonemap(4.0)
        assert(mapped >= 0.0 and mapped <= 1.0)
        return true
    )", sol::script_pass_on_error);
    ASSERT_TRUE(result.valid()) << sol::error(result).what();
}

TEST_F(PbrTest, EvaluatesMetallicRoughnessBrdf) {
    auto result = lua.safe_script(R"(
        local PBR = require('lib.PBR')
        local r, g, b = PBR.evaluate_direct(
            {0.8, 0.2, 0.1}, 0.75, 0.35,
            {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {5, 5, 5})
        for _, value in ipairs({r, g, b}) do
            assert(value == value and value >= 0 and value < math.huge)
        end
        return true
    )", sol::script_pass_on_error);
    ASSERT_TRUE(result.valid()) << sol::error(result).what();
}
