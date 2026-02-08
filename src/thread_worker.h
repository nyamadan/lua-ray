#pragma once
#include <sol/sol.hpp>
#include <thread>
#include <atomic>
#include <string>
#include "app_data.h"
#include "embree_wrapper.h"

class ThreadWorker {
public:
    struct Bounds {
        int x, y, w, h;
    };

    ThreadWorker(AppData* data, EmbreeScene* scene, Bounds bounds, int thread_id);
    ~ThreadWorker();

    void start(const std::string& script_path, const std::string& scene_type);
    void join();
    void terminate();
    bool is_done() const;
    bool is_cancel_requested() const;
    float get_progress() const;

private:
    void thread_func(std::string script_path, std::string scene_type);

    AppData* m_data;
    EmbreeScene* m_scene;
    Bounds m_bounds;
    int m_thread_id;
    
    std::thread m_thread;
    std::atomic<bool> m_done{true};
    std::atomic<bool> m_cancel_requested{false};
    std::atomic<float> m_progress{0.0f};
};
