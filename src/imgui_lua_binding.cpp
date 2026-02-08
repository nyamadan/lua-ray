#include "imgui_lua_binding.h"
#include "imgui.h"

void bind_imgui(sol::state& lua) {
    auto imgui = lua.create_named_table("ImGui");

    imgui.set_function("Begin", sol::overload(
        [](const char* name) { return ImGui::Begin(name); },
        [](const char* name, bool* p_open) { return ImGui::Begin(name, p_open); } // Note: sol2 handles bool* as in/out reference if configured? No, sol2 might not handle bool* automatically as return. 
        // Let's stick to simple bindings for now.
    ));
    
    // Simple Begin: ImGui.Begin("Name") -> bool (expanded)
    imgui.set_function("Begin", [](const char* name) -> bool { 
        return ImGui::Begin(name); 
    });

    imgui.set_function("End", &ImGui::End);

    imgui.set_function("BeginDisabled", sol::overload(
        [](bool disabled) { ImGui::BeginDisabled(disabled); },
        []() { ImGui::BeginDisabled(true); }
    ));
    imgui.set_function("EndDisabled", &ImGui::EndDisabled);

    imgui.set_function("Text", [](const char* text) { 
        ImGui::Text("%s", text); 
    });

    imgui.set_function("Button", [](const char* label) -> bool { 
        return ImGui::Button(label); 
    });

    imgui.set_function("SameLine", sol::overload(
        []() { ImGui::SameLine(); },
        [](float offset_from_start_x) { ImGui::SameLine(offset_from_start_x); },
        [](float offset_from_start_x, float spacing) { ImGui::SameLine(offset_from_start_x, spacing); }
    ));
    imgui.set_function("Separator", &ImGui::Separator);

    imgui.set_function("RadioButton", [](const char* label, bool active) -> bool {
        return ImGui::RadioButton(label, active);
    });

    imgui.set_function("ProgressBar", sol::overload(
        [](float fraction) { ImGui::ProgressBar(fraction); },
        [](float fraction, const char* overlay) { ImGui::ProgressBar(fraction, ImVec2(-FLT_MIN, 0), overlay); }
    ));

    // ComboBox用バインディング
    imgui.set_function("BeginCombo", [](const char* label, const char* preview_value) -> bool {
        return ImGui::BeginCombo(label, preview_value);
    });
    imgui.set_function("EndCombo", &ImGui::EndCombo);
    imgui.set_function("Selectable", sol::overload(
        [](const char* label) -> bool { return ImGui::Selectable(label); },
        [](const char* label, bool selected) -> bool { return ImGui::Selectable(label, selected); }
    ));
}
