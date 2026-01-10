#include "app.h"
#include "lua_binding.h"
#include <iostream>

int main(int argc, char* argv[]) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!init_sdl(&window, &renderer)) {
        return 1;
    }

    // Lock texture for writing
    SDL_LockTexture(g_ctx.texture, NULL, &g_ctx.pixels, &g_ctx.pitch);

    // Init Lua and run script
    sol::state lua;
    bind_lua(lua);
    run_script(lua, argc, argv);
    
    // Unlock texture
    SDL_UnlockTexture(g_ctx.texture);

    // Enter main loop
    main_loop(renderer);

    // Cleanup
    SDL_DestroyTexture(g_ctx.texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
