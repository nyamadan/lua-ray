cmake_minimum_required(VERSION 3.14)
project(lua-ray VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
# Embree (FetchContent版)
# ---------------------------------------------------------
# Embree自体のオプションを事前に設定
set(EMBREE_TUTORIALS OFF CACHE BOOL "" FORCE)
set(EMBREE_RAY_MASK ON CACHE BOOL "" FORCE)
set(EMBREE_TASKING_SYSTEM "INTERNAL" CACHE STRING "" FORCE)
set(EMBREE_STATIC_LIB ON CACHE BOOL "" FORCE)
set(EMBREE_ISPC_SUPPORT OFF CACHE BOOL "" FORCE)
set(EMBREE_ISA_AVX OFF CACHE BOOL "" FORCE)
set(EMBREE_ISA_AVX2 OFF CACHE BOOL "" FORCE)
set(EMBREE_ISA_AVX512 OFF CACHE BOOL "" FORCE)
set(EMBREE_ISA_SSE42 OFF CACHE BOOL "" FORCE)
set(FLAGS_SSE2 "-msse -msse2")

set(EMBREE_ISA_SSE2 ON CACHE BOOL "" FORCE) # Wasm SIMD用

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
# ---------------------------------------------------------
# Main executable
# ---------------------------------------------------------
# Gather source files
set(SOURCES
    src/main.cpp
    src/app.cpp
    src/lua_binding.cpp
    src/embree_wrapper.cpp
)

add_executable(lua-ray ${SOURCES})

# Emscripten固有の設定
if(EMSCRIPTEN)
    set_target_properties(lua-ray PROPERTIES SUFFIX ".html")
    target_link_options(lua-ray PRIVATE
        "-sUSE_SDL=0"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sWASM=1"
        "-msse"
        "-msse2"
    )
endif()

target_link_libraries(lua-ray PRIVATE
    SDL3::SDL3-static
    lua
    embree
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
    add_executable(unit-tests test/lua_binding_test.cpp src/lua_binding.cpp src/embree_wrapper.cpp)
    target_link_libraries(unit-tests PRIVATE gtest_main lua embree)
    add_test(NAME unit-tests COMMAND unit-tests)
endif()
