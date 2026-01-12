#include "app.h"
#include "lua_binding.h"
#include <iostream>

int main(int argc, char* argv[]) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    SDL_Texture* texture = nullptr;
    if (!init_sdl(&window, &renderer)) {
        return 1;
    }
    // Init Lua and run script (capture returned value)
    sol::state lua;
    bind_lua(lua);
    // Expose the SDL_Renderer* to Lua as lightuserdata so scripts can create textures
    lua["renderer"] = static_cast<void*>(renderer);
    sol::object ret = run_script(lua, argc, argv);

    // If the script returned a lightuserdata, use it as texture (no Lua global)
    if (ret.get_type() == sol::type::lightuserdata) {
        texture = static_cast<SDL_Texture*>(ret.as<void*>());
    } else {
        texture = nullptr;
    }

    // Enter main loop (texture may be null)
    main_loop(renderer, texture);

    // Cleanup (destroy texture if created)
    if (texture) SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
