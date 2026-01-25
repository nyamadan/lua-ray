#include "app.h"
#include "lua_binding.h"
#include <iostream>

int main(int argc, char* argv[]) {
    // 1. Init Lua and run script to load definitions and config
    sol::state lua;
    bind_lua(lua);
    
    // Run the script. We expect it to define variables and functions, not necessarily return the texture yet.
    sol::object script_ret = run_script(lua, argc, argv);
    // Note: run_script already prints errors.

    // 2. Read configuration from Lua via get_config function
    int width = 800;
    int height = 600;
    std::string title_str = "Lua Ray Tracing";

    sol::protected_function get_config = lua["get_config"];
    if (get_config.valid()) {
        sol::protected_function_result result = get_config();
        if (result.valid()) {
            sol::table config = result;
            width = config["width"].get_or(800);
            height = config["height"].get_or(600);
            title_str = config["title"].get_or(std::string("Lua Ray Tracing"));
        } else {
            sol::error err = result;
            std::cerr << "Lua Error in get_config: " << err.what() << std::endl;
        }
    } else {
        std::cerr << "Warning: 'get_config' function not found. Using defaults." << std::endl;
    }
    
    // 3. Initialize SDL with config
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!init_sdl(&window, &renderer, width, height, title_str.c_str())) {
        return 1;
    }

    // 4. Pass renderer to Lua
    lua["renderer"] = static_cast<void*>(renderer);

    // 5. Call app_init to create texture and setup scene
    SDL_Texture* texture = nullptr;
    sol::protected_function app_init = lua["app_init"];
    if (app_init.valid()) {
        sol::protected_function_result result = app_init();
        if (result.valid()) {
            sol::object ret = result;
            if (ret.get_type() == sol::type::lightuserdata) {
                texture = static_cast<SDL_Texture*>(ret.as<void*>());
            }
        } else {
            sol::error err = result;
            std::cerr << "Lua Error in app_init: " << err.what() << std::endl;
        }
    } else {
        // Fallback or explicit warning
        // If the script returned a texture (old style), we can use it, but better to enforce new structure.
        if (script_ret.is<void*>()) { 
             texture = static_cast<SDL_Texture*>(script_ret.as<void*>());
        }
        if (!texture) {
            std::cerr << "Warning: 'app_init' function not found and script did not return a texture." << std::endl;
        }
    }

    // 6. Setup on_frame callback
    std::function<void()> on_frame_callback = nullptr;
    sol::protected_function on_frame = lua["on_frame"];
    if (on_frame.valid()) {
        on_frame_callback = [&on_frame]() {
            sol::protected_function_result result = on_frame();
            if (!result.valid()) {
                sol::error err = result;
                std::cerr << "Lua Error in on_frame: " << err.what() << std::endl;
            }
        };
    }

    // 7. Enter main loop
    main_loop(window, renderer, texture, on_frame_callback);

    // Cleanup
    if (texture) SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
