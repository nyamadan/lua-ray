#!/bin/bash
set -eux

DIR="$(cd -- "$(dirname "$0")" && pwd)"

cd $DIR/..

# Install dependencies
sudo apt-get update
sudo apt-get -y install build-essential git make ninja-build python3 gdb \
    pkg-config cmake ninja-build gnome-desktop-testing libasound2-dev libpulse-dev \
    libaudio-dev libfribidi-dev libjack-dev libsndio-dev libx11-dev libxext-dev \
    libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev \
    libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev \
    libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev libthai-dev \
    libpipewire-0.3-dev libwayland-dev libdecor-0-dev liburing-dev

# emsdk install
EMSCRIPTEN_VERSION=4.0.22
./.emsdk/emsdk install $EMSCRIPTEN_VERSION
./.emsdk/emsdk activate $EMSCRIPTEN_VERSION

# npm install
sudo chown -R "$(id -u)":"$(id -g)" .pnpm-store
sudo chown -R "$(id -u)":"$(id -g)" node_modules
pnpm install

cd -

# if install.local.sh exists, run it
if [ -f "$DIR/install.local.sh" ]; then
    bash "$DIR/install.local.sh"
fi

