#include "thread_worker.h"
#include "lua_binding.h"
#include <iostream>

// Defined in lua_binding.cpp, but we need to declare it here or in lua_binding.h
// We will update lua_binding.h later to include this.
void bind_worker_lua(sol::state& lua);

ThreadWorker::ThreadWorker(AppData* data, EmbreeScene* scene, Bounds bounds, int thread_id)
    : m_data(data), m_scene(scene), m_bounds(bounds), m_thread_id(thread_id) {}

ThreadWorker::~ThreadWorker() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void ThreadWorker::start(const std::string& script_path, const std::string& scene_type) {
    if (m_thread.joinable()) {
        m_thread.join(); // Ensure previous run is finished
    }
    m_done = false;
    m_progress = 0.0f;
    m_thread = std::thread(&ThreadWorker::thread_func, this, script_path, scene_type);
}

void ThreadWorker::join() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

bool ThreadWorker::is_done() const {
    return m_done;
}

float ThreadWorker::get_progress() const {
    return m_progress;
}

void ThreadWorker::thread_func(std::string script_path, std::string scene_type) {
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::string, sol::lib::table, sol::lib::coroutine, sol::lib::os);

    // Bind strict subset of functionality
    bind_worker_lua(lua);

    // Inject shared data
    // Note: We are passing pointers to C++ objects.
    // The userdata in this new lua state will point to the SAME C++ objects.
    // Thread safety of AppData and EmbreeScene is crucial.
    // AppData: set_pixel is thread-safe if x,y are disjoint.
    // EmbreeScene: intersect is thread-safe (const method basically).
    lua["_app_data"] = m_data; // sol will verify this type matches the bind
    lua["_scene"] = m_scene;
    
    lua["_bounds"] = lua.create_table_with(
        "x", m_bounds.x,
        "y", m_bounds.y,
        "w", m_bounds.w,
        "h", m_bounds.h
    );
    
    lua["_scene_type"] = scene_type;
    lua["_thread_id"] = m_thread_id;

    // Execute the worker script
    // The worker script is expected to require the scene and run the loop
    sol::protected_function_result result = lua.script_file(script_path, sol::script_pass_on_error);
    
    if (!result.valid()) {
        sol::error err = result;
        std::cerr << "Thread " << m_thread_id << " Lua Error: " << err.what() << std::endl;
    }

    m_done = true;
    m_progress = 1.0f;
}
