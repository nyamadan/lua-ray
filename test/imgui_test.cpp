#include <gtest/gtest.h>
#include <sol/sol.hpp>
#include "imgui.h"
#include "imgui_lua_binding.h"

class ImGuiTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        // Build atlas
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height); // This builds the atlas
    }

    void TearDown() override {
        ImGui::DestroyContext();
    }
};

TEST_F(ImGuiTest, LuaBindBeginEnd) {
    sol::state lua;
    lua.open_libraries(sol::lib::base);
    bind_imgui(lua);

    // Mock NewFrame just enough so Begin doesn't crash?
    // ImGui::NewFrame() requires Update size, delta time, etc.
    // But we can try to skip backend newframe and just call ImGui::NewFrame with manual IO setup.
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1920, 1080);
    io.DeltaTime = 1.0f / 60.0f;

    ImGui::NewFrame();

    // Run Lua script
    auto result = lua.script(R"(
        local open = ImGui.Begin("Test Window")
        ImGui.Text("Hello World")
        ImGui.Button("Click Me")
        ImGui.End()
        return open
    )");

    EXPECT_TRUE(result.valid());
    bool open = result;
    // Window might be collapsed or not, usually true (expanded) by default.
    EXPECT_TRUE(open); 

    ImGui::Render();
}
