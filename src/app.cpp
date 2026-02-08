#include "app.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// Emscripten用のグローバルコンテキスト構造体
struct MainLoopContext {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    std::function<void()> on_frame;
    std::function<void()> on_quit;
    bool running;
};

// グローバルコンテキスト（Emscriptenのコールバック用）
static MainLoopContext* g_loop_context = nullptr;

bool init_sdl(SDL_Window** window, SDL_Renderer** renderer, int width, int height, const char* title) {
    if (SDL_Init(SDL_INIT_VIDEO) == false) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    *window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    *renderer = SDL_CreateRenderer(*window, NULL);

    if (!*window || !*renderer) {
        std::cerr << "SDL Setup failed" << std::endl;
        return false;
    }
    return true;
}

// 実行中のメインループのテクスチャを動的に更新する
void set_active_texture(SDL_Texture* texture) {
    if (g_loop_context) {
        g_loop_context->texture = texture;
    }
}

// アスペクト比を維持したフィット矩形を計算
static SDL_FRect calculate_fit_rect(int tex_w, int tex_h, int win_w, int win_h) {
    float tex_aspect = static_cast<float>(tex_w) / static_cast<float>(tex_h);
    float win_aspect = static_cast<float>(win_w) / static_cast<float>(win_h);
    
    float fit_w, fit_h;
    
    if (tex_aspect > win_aspect) {
        // テクスチャが横長: 幅をウィンドウに合わせる
        fit_w = static_cast<float>(win_w);
        fit_h = fit_w / tex_aspect;
    } else {
        // テクスチャが縦長または同じ: 高さをウィンドウに合わせる
        fit_h = static_cast<float>(win_h);
        fit_w = fit_h * tex_aspect;
    }
    
    // 中央揃え
    float x = (static_cast<float>(win_w) - fit_w) / 2.0f;
    float y = (static_cast<float>(win_h) - fit_h) / 2.0f;
    
    return SDL_FRect{ x, y, fit_w, fit_h };
}

// 1フレームの処理を行う関数
static void main_loop_iteration() {
    if (!g_loop_context || !g_loop_context->running) {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        return;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        
        // 終了イベントを処理（1回だけon_quitを呼ぶ）
        bool should_quit = false;
        if (event.type == SDL_EVENT_QUIT) {
            should_quit = true;
        }
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && 
            event.window.windowID == SDL_GetWindowID(g_loop_context->window)) {
            should_quit = true;
        }
        
        if (should_quit && g_loop_context->running) {
            if (g_loop_context->on_quit) {
                g_loop_context->on_quit();
            }
            g_loop_context->running = false;
        }
    }

    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    
    if (g_loop_context->on_frame) {
        g_loop_context->on_frame();
    }

    // Rendering
    // 背景を黒でクリア（余白部分）
    SDL_SetRenderDrawColor(g_loop_context->renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_loop_context->renderer);
    
    if (g_loop_context->texture) {
        // ウィンドウサイズを取得
        int win_w, win_h;
        SDL_GetWindowSize(g_loop_context->window, &win_w, &win_h);
        
        // テクスチャサイズを取得
        float tex_w_f, tex_h_f;
        SDL_GetTextureSize(g_loop_context->texture, &tex_w_f, &tex_h_f);
        int tex_w = static_cast<int>(tex_w_f);
        int tex_h = static_cast<int>(tex_h_f);
        
        // フィット矩形を計算
        SDL_FRect dst_rect = calculate_fit_rect(tex_w, tex_h, win_w, win_h);
        
        // アスペクト比を維持してテクスチャを描画
        SDL_RenderTexture(g_loop_context->renderer, g_loop_context->texture, NULL, &dst_rect);
    }
    
    // Render ImGui
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), g_loop_context->renderer);

    SDL_RenderPresent(g_loop_context->renderer);
}

void main_loop(SDL_Window* window, SDL_Renderer* renderer, SDL_Texture* texture, std::function<void()> on_frame, std::function<void()> on_quit) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // コンテキストを設定
    static MainLoopContext context;
    context.window = window;
    context.renderer = renderer;
    context.texture = texture;
    context.on_frame = on_frame;
    context.on_quit = on_quit;
    context.running = true;
    g_loop_context = &context;

#ifdef __EMSCRIPTEN__
    // Emscripten: ブラウザのイベントループを使用
    // 第2引数: FPS (0 = requestAnimationFrameを使用)
    // 第3引数: simulate_infinite_loop (1 = 無限ループをシミュレート)
    emscripten_set_main_loop(main_loop_iteration, 0, 1);
#else
    // ネイティブ: 従来のwhileループ
    while (context.running) {
        main_loop_iteration();
    }
#endif

    // Cleanup
    g_loop_context = nullptr;
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}
