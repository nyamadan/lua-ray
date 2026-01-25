#include "app.h"
#include "lua_binding.h"
#include <iostream>

int main(int argc, char* argv[]) {
    // 1. Init Lua and run script to load definitions and config
    sol::state lua;
    AppContext ctx;
    bind_lua(lua, ctx);
    
    // Run the script. We expect it to define variables and functions, not necessarily return the texture yet.
    sol::object script_ret = run_script(lua, argc, argv);
    // Note: run_script already prints errors.

    // 2. Read configuration from Lua via app.configure (already handled during script execution)
    // ctx object is populated by bind_lua -> app.configure callback which also initializes SDL.
    
    // 3. Check if initialization happened
    if (!ctx.window || !ctx.renderer || !ctx.texture) {
        std::cerr << "Error: App framework was not initialized by Lua script. Did you call app.configure()?" << std::endl;
        return 1;
    }
    
    SDL_Window* window = ctx.window;
    SDL_Renderer* renderer = ctx.renderer;
    SDL_Texture* texture = ctx.texture;

    // 4. Pass renderer to Lua - No longer needed as Lua called configure to create it.
    // However, if other parts of API need it via get_sdl_renderer:
    lua["app"]["get_sdl_renderer"] = [renderer]() -> void* { return static_cast<void*>(renderer); };

    // 5. Call app_init - DEPRECATED / REMOVED
    // Initialization logic should have happened in script top-level or via app.configure return value.

    // 6. Setup on_frame callback
    std::function<void()> on_frame_callback = nullptr;
    sol::protected_function on_frame = lua["app"]["on_frame"];
    if (on_frame.valid()) {
        on_frame_callback = [&on_frame]() {
            sol::protected_function_result result = on_frame();
            if (!result.valid()) {
                sol::error err = result;
                std::cerr << "Lua Error in app.on_frame: " << err.what() << std::endl;
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
