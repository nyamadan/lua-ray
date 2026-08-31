#include <gtest/gtest.h>
#include <sol/sol.hpp>

class MonteCarloTest : public ::testing::Test {
protected:
    sol::state lua;
    void SetUp() override {
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math,
                           sol::lib::string, sol::lib::table);
        lua["package"]["path"] = "?.lua;?/init.lua;lib/?.lua";
    }
};

TEST_F(MonteCarloTest, RngIsDeterministicPerPixelAndPass) {
    auto result = lua.safe_script(R"(
        local RNG = require('lib.RNG')
        local a, b = RNG.for_pixel(10, 20, 3), RNG.for_pixel(10, 20, 3)
        local c = RNG.for_pixel(10, 20, 4)
        local a1, a2, b1, b2, c1 = a:next(), a:next(), b:next(), b:next(), c:next()
        assert(a1 == b1 and a2 == b2)
        assert(a1 ~= c1)
        assert(a1 >= 0 and a1 < 1 and a2 >= 0 and a2 < 1)
        return true
    )", sol::script_pass_on_error);
    ASSERT_TRUE(result.valid()) << sol::error(result).what();
}

TEST_F(MonteCarloTest, BsdfAndPowerHeuristicAreFinite) {
    auto result = lua.safe_script(R"(
        local BSDF = require('lib.BSDF')
        local RNG = require('lib.RNG')
        local material = {base_color={0.8,0.3,0.1}, metallic=0.4, roughness=0.35}
        local n, wo = {0,0,1}, {0,0,1}
        local wi, f, pdf, delta = BSDF.sample(material, n, wo, RNG.for_pixel(1,2,3))
        assert(not delta and pdf > 0 and pdf < math.huge)
        for i=1,3 do assert(f[i] >= 0 and f[i] < math.huge) end
        local queried = BSDF.pdf(material, n, wo, wi)
        assert(queried > 0 and math.abs(queried-pdf) < 1e-8)
        assert(BSDF.power_heuristic(0, 0) == 0)
        assert(math.abs(BSDF.power_heuristic(1, 1)-0.5) < 1e-8)
        return true
    )", sol::script_pass_on_error);
    ASSERT_TRUE(result.valid()) << sol::error(result).what();
}

TEST_F(MonteCarloTest, NeeMisIntegratorIsDeterministicAndAddsDirectLight) {
    auto result = lua.safe_script(R"(
        local PathTracer = require('lib.PathTracer')
        local RNG = require('lib.RNG')
        local Vec3 = require('lib.Vec3')
        local Ray = require('lib.Ray')
        local function render()
            local calls = 0
            local scene = {intersect=function()
                calls = calls + 1
                if calls == 1 then return true, 1, 0, 0, 1, 0, 0, 0, 0 end
                return false, 0, 0, 0, 0, 0, 0, 0, 0
            end}
            local callbacks = {
                surface=function(hit) return {
                    material={base_color={0.8,0.8,0.8},metallic=0,roughness=1},
                    normal={0,0,1}, emission={0,0,0}}
                end,
                sample_light=function() return {
                    direction={0,0,1},distance=2,radiance={4,4,4},pdf=1}
                end,
                background=function() return {0,0,0} end
            }
            return PathTracer.trace_mis(
                Ray.new(Vec3.new(0,0,1),Vec3.new(0,0,-1)), scene, callbacks,
                RNG.for_pixel(0,0,1), {max_depth=2,rr_depth=4})
        end
        local a, b = render(), render()
        assert(a[1] > 0 and a[1] == b[1] and a[2] == b[2] and a[3] == b[3])
        return true
    )", sol::script_pass_on_error);
    ASSERT_TRUE(result.valid()) << sol::error(result).what();
}

TEST_F(MonteCarloTest, NeeMisIntegratorRejectsOccludedLight) {
    auto result = lua.safe_script(R"(
        local PathTracer=require('lib.PathTracer')
        local RNG=require('lib.RNG')
        local Vec3=require('lib.Vec3')
        local Ray=require('lib.Ray')
        local calls=0
        local scene={intersect=function()
            calls=calls+1
            if calls==1 then return true,1,0,0,1,0,0,0,0 end
            if calls==2 then return true,0.5,0,0,-1,9,0,0,0 end
            return false,0,0,0,0,0,0,0,0
        end}
        local color=PathTracer.trace_mis(
            Ray.new(Vec3.new(0,0,1),Vec3.new(0,0,-1)),scene,{
                surface=function() return {material={base_color={1,1,1},metallic=0,roughness=1},normal={0,0,1},emission={0,0,0}} end,
                sample_light=function() return {direction={0,0,1},distance=2,radiance={10,10,10},pdf=1} end,
                background=function() return {0,0,0} end
            },RNG.for_pixel(0,0,1),{max_depth=2,rr_depth=4})
        assert(color[1]==0 and color[2]==0 and color[3]==0)
        return true
    )",sol::script_pass_on_error);
    ASSERT_TRUE(result.valid()) << sol::error(result).what();
}

TEST_F(MonteCarloTest, NeeMisIntegratorReturnsEmissionOnPrimaryHit) {
    auto result = lua.safe_script(R"(
        local PathTracer=require('lib.PathTracer')
        local RNG=require('lib.RNG')
        local Vec3=require('lib.Vec3')
        local Ray=require('lib.Ray')
        local scene={intersect=function() return true,1,0,0,1,0,0,0,0 end}
        local color=PathTracer.trace_mis(
            Ray.new(Vec3.new(0,0,1),Vec3.new(0,0,-1)),scene,{
                surface=function() return {normal={0,0,1},emission={2,3,4}} end
            },RNG.for_pixel(0,0,1),{max_depth=1})
        assert(color[1]==2 and color[2]==3 and color[3]==4)
        return true
    )",sol::script_pass_on_error);
    ASSERT_TRUE(result.valid()) << sol::error(result).what();
}
