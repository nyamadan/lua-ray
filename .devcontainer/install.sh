#!/bin/bash
set -eux

DIR="$(cd -- "$(dirname "$0")" && pwd)"
ROOT_DIR="$DIR/.."

# Define versions
EMSCRIPTEN_VERSION=4.0.22

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
    echo 'eval "$(mise activate bash)"' >> ~/.bashrc
    mise install
    sudo chown -R "$(id -u)":"$(id -g)" .pnpm-store 2>/dev/null || true
    sudo chown -R "$(id -u)":"$(id -g)" node_modules 2>/dev/null || true
    sudo chown -R "$(id -u)":"$(id -g)" .ccache 2>/dev/null || true
    mise x -- pnpm install
)

# if install.local.sh exists, run it
if [ -f "$DIR/install.local.sh" ]; then
    bash "$DIR/install.local.sh"
fi

