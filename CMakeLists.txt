cmake_minimum_required(VERSION 3.14)
project(lua-ray VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------
# Enable ccache if available
# ---------------------------------------------------------
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# ---------------------------------------------------------
# MinGW static runtime linking
# ---------------------------------------------------------
if (MINGW)
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()

include(FetchContent)

# ---------------------------------------------------------
# SDL3
# ---------------------------------------------------------
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.0
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_X11_XTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

# ---------------------------------------------------------
# Lua
# ---------------------------------------------------------
FetchContent_Declare(
    lua_src
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG v5.4.6
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(lua_src)

# Luaのビルドルール定義 (FetchContent_MakeAvailable後、ソースディレクトリが確定する)
FetchContent_GetProperties(lua_src)
file(GLOB LUA_CORE_SRC "${lua_src_SOURCE_DIR}/*.c")
list(FILTER LUA_CORE_SRC EXCLUDE REGEX "lua\\.c|luac\\.c")

add_library(lua STATIC ${LUA_CORE_SRC})
target_include_directories(lua PUBLIC ${lua_src_SOURCE_DIR})
target_compile_definitions(lua PUBLIC MAKE_LIB)

# ---------------------------------------------------------
# lbuffer
# ---------------------------------------------------------
FetchContent_Declare(
    lbuffer_src
    GIT_REPOSITORY https://github.com/starwing/lbuffer.git
    GIT_TAG 1031755e5bd1cbf64f0761c2d5c402a678226768
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

# Populate lbuffer and apply patch
FetchContent_GetProperties(lbuffer_src)
if(NOT lbuffer_src_POPULATED)
    FetchContent_Populate(lbuffer_src)
    # Apply patch to fix little-endian bug
    execute_process(
        COMMAND git apply --ignore-whitespace --reject ${CMAKE_SOURCE_DIR}/patches/lbuffer-fix-little-endian.patch
        WORKING_DIRECTORY ${lbuffer_src_SOURCE_DIR}
        RESULT_VARIABLE PATCH_RESULT
        ERROR_QUIET
    )
    # Apply patch to fix LUA_QS undefined error
    execute_process(
        COMMAND git apply --ignore-whitespace --reject ${CMAKE_SOURCE_DIR}/patches/lbuffer-fix-lua-qs.patch
        WORKING_DIRECTORY ${lbuffer_src_SOURCE_DIR}
        RESULT_VARIABLE PATCH_RESULT2
        ERROR_QUIET
    )
endif()

add_library(lbuffer STATIC
    ${lbuffer_src_SOURCE_DIR}/lbuffer.c
    ${lbuffer_src_SOURCE_DIR}/lbufflib.c
)
target_compile_options(lbuffer PRIVATE -w -fpermissive -Wno-error=incompatible-pointer-types)
target_include_directories(lbuffer PUBLIC ${lbuffer_src_SOURCE_DIR})
# lua headers are needed for lbuffer
target_link_libraries(lbuffer PRIVATE lua)

# ---------------------------------------------------------
# sol2 (v3)
# ---------------------------------------------------------
FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_GetProperties(sol2)
if(NOT sol2_POPULATED)
    FetchContent_Populate(sol2)
    add_library(sol2 INTERFACE)
    target_include_directories(sol2 INTERFACE ${sol2_SOURCE_DIR}/include)
    target_link_libraries(sol2 INTERFACE lua)
    # sol2 requires C++17, which is already set globally, but we can ensure it here
    target_compile_features(sol2 INTERFACE cxx_std_17)
endif()

# ---------------------------------------------------------
# Dear ImGui
# ---------------------------------------------------------
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.5
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

# Create ImGui library
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
# ImGui needs SDL3 headers
target_link_libraries(imgui PUBLIC SDL3::SDL3-static)



# ---------------------------------------------------------
# Embree (FetchContent版)
# ---------------------------------------------------------
# Embree自体のオプションを事前に設定
set(EMBREE_TUTORIALS OFF CACHE BOOL "" FORCE)
set(EMBREE_RAY_MASK ON CACHE BOOL "" FORCE)
set(EMBREE_TASKING_SYSTEM "INTERNAL" CACHE STRING "" FORCE)
set(EMBREE_STATIC_LIB ON CACHE BOOL "" FORCE)
set(EMBREE_ISPC_SUPPORT OFF CACHE BOOL "" FORCE)

# if(EMSCRIPTEN)
    set(EMBREE_ISA_AVX OFF CACHE BOOL "" FORCE)
    set(EMBREE_ISA_AVX2 OFF CACHE BOOL "" FORCE)
    set(EMBREE_ISA_AVX512 OFF CACHE BOOL "" FORCE)
    set(EMBREE_ISA_SSE42 OFF CACHE BOOL "" FORCE)
    set(EMBREE_ISA_SSE2 ON CACHE BOOL "" FORCE)
    set(FLAGS_SSE2 "-msse -msse2")
# endif()

FetchContent_Declare(
    embree
    GIT_REPOSITORY https://github.com/embree/embree.git
    GIT_TAG v4.4.0 # 適宜バージョンは調整してください
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

# Embreeは内部で複雑な依存関係を持つため、Populateしてからadd_subdirectory
FetchContent_MakeAvailable(embree)

# ---------------------------------------------------------
# Main executable
# ---------------------------------------------------------
# Gather source files
set(SOURCES
    src/main.cpp
    src/app.cpp
    src/lua_binding.cpp
    src/imgui_lua_binding.cpp
    src/embree_wrapper.cpp
)

add_executable(lua-ray ${SOURCES})

# Emscripten固有の設定
if(EMSCRIPTEN)
    set_target_properties(lua-ray PROPERTIES SUFFIX ".html")
    target_compile_options(lua-ray PRIVATE "-fexceptions")

    target_link_options(lua-ray PRIVATE
        "-fexceptions"
        "-sDISABLE_EXCEPTION_CATCHING=0"
        "-sUSE_SDL=0"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sWASM=1"
        "-msse"
        "-msse2"
        # Bundle main.lua and scenes
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/main.lua@/main.lua"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/scenes@/scenes"
    )
endif()

target_link_libraries(lua-ray PRIVATE
    SDL3::SDL3-static
    lua
    embree
    lbuffer
    sol2
    imgui
)

# ---------------------------------------------------------
# Testing
# ---------------------------------------------------------
if(NOT EMSCRIPTEN)
    include(CTest)
    enable_testing()

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # Unit Tests
    add_executable(lua-ray-tests test/lua_binding_test.cpp test/lbuffer_test.cpp test/sol2_test.cpp test/imgui_test.cpp test/app_data_test.cpp src/lua_binding.cpp src/imgui_lua_binding.cpp src/embree_wrapper.cpp)
    target_include_directories(lua-ray-tests PRIVATE src)
    # SDL3 headers are required by context.h, so link the static SDL3 library
    target_link_libraries(lua-ray-tests PRIVATE gtest_main lua embree lbuffer sol2 imgui SDL3::SDL3-static)
    add_test(NAME lua-ray-tests COMMAND lua-ray-tests)
endif()
