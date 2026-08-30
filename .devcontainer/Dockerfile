FROM mcr.microsoft.com/devcontainers/base:resolute

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        extrepo \
        lldb \
        build-essential \
        git \
        make \
        ninja-build \
        python3 \
        python3-yaml \
        gdb \
        pkg-config \
        cmake \
        ccache \
        gnome-desktop-testing \
        libasound2-dev \
        libpulse-dev \
        libaudio-dev \
        libfribidi-dev \
        libjack-dev \
        libsndio-dev \
        libx11-dev \
        libxext-dev \
        libxrandr-dev \
        libxcursor-dev \
        libxfixes-dev \
        libxi-dev \
        libxss-dev \
        libxtst-dev \
        libxkbcommon-dev \
        libdrm-dev \
        libgbm-dev \
        libgl1-mesa-dev \
        libgles2-mesa-dev \
        libegl1-mesa-dev \
        libdbus-1-dev \
        libibus-1.0-dev \
        libudev-dev \
        libthai-dev \
        libpipewire-0.3-dev \
        libwayland-dev \
        libdecor-0-dev \
        liburing-dev \
        mingw-w64-x86-64-dev \
        gcc-mingw-w64-x86-64 \
        g++-mingw-w64-x86-64 && \
    extrepo enable mise && \
    apt-get update && \
    apt-get install -y --no-install-recommends mise && \
    rm -rf /var/lib/apt/lists/*
