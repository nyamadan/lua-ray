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
    lua_State* L = init_lua();
    run_script(L, argc, argv);
    lua_close(L);
    
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
