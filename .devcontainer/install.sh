#!/bin/bash
set -eux

DIR="$(cd -- "$(dirname "$0")" && pwd)"
ROOT_DIR="$DIR/.."

# Define versions
EMSCRIPTEN_VERSION=4.0.22

# Dependencies list
DEPS=(
    build-essential
    git
    make
    ninja-build
    python3
    gdb
    lldb
    pkg-config
    cmake
    ccache
    gnome-desktop-testing
    libasound2-dev
    libpulse-dev
    libaudio-dev
    libfribidi-dev
    libjack-dev
    libsndio-dev
    libx11-dev
    libxext-dev
    libxrandr-dev
    libxcursor-dev
    libxfixes-dev
    libxi-dev
    libxss-dev
    libxtst-dev
    libxkbcommon-dev
    libdrm-dev
    libgbm-dev
    libgl1-mesa-dev
    libgles2-mesa-dev
    libegl1-mesa-dev
    libdbus-1-dev
    libibus-1.0-dev
    libudev-dev
    libthai-dev
    libpipewire-0.3-dev
    libwayland-dev
    libdecor-0-dev
    liburing-dev
    gcc-mingw-w64-ucrt64
    g++-mingw-w64-ucrt64
)

# Install dependencies
sudo apt-get update
sudo apt-get -y install "${DEPS[@]}"

# emsdk install
# Setup emsdk in the project root
(
    cd "$ROOT_DIR"
    if [ ! -d ".emsdk" ]; then
        # Assuming .emsdk dir might be created/managed elsewhere or cloned here.
        # But based on original script, it expects ./.emsdk/emsdk to exist relative to .. of script dir.
        # Original: cd $DIR/.. -> ./.emsdk/emsdk
        echo "Warning: .emsdk directory expected but not checked in script logic thoroughly."
    fi
    ./.emsdk/emsdk install $EMSCRIPTEN_VERSION
    ./.emsdk/emsdk activate $EMSCRIPTEN_VERSION
)

# npm install
# Run npm install in the project root
(
    cd "$ROOT_DIR"
    sudo chown -R "$(id -u)":"$(id -g)" .pnpm-store 2>/dev/null || true
    sudo chown -R "$(id -u)":"$(id -g)" node_modules 2>/dev/null || true
    sudo chown -R "$(id -u)":"$(id -g)" .ccache 2>/dev/null || true
    pnpm install
)

# if install.local.sh exists, run it
if [ -f "$DIR/install.local.sh" ]; then
    bash "$DIR/install.local.sh"
fi

