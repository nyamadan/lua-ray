#pragma once
#include <embree4/rtcore.h>
#include <vector>
#include <tuple>
#include <memory>

// RAII Wrapper for Embree Device
class EmbreeDevice {
public:
    EmbreeDevice();
    ~EmbreeDevice();

    // Prevent copying
    EmbreeDevice(const EmbreeDevice&) = delete;
    EmbreeDevice& operator=(const EmbreeDevice&) = delete;

    // Allow moving
    EmbreeDevice(EmbreeDevice&& other) noexcept;
    EmbreeDevice& operator=(EmbreeDevice&& other) noexcept;

    RTCDevice get() const { return device; }

private:
    RTCDevice device;
};

// RAII Wrapper for Embree Scene
class EmbreeScene {
public:
    EmbreeScene(EmbreeDevice& device); // Keep reference to device wrapper to ensure ordering? Or just raw device 
    // Actually, rtcNewScene takes a device, so we need the raw device. 
    // Ideally we keep the device alive, but for now let's assume usage pattern is correct or use shared_ptr if needed.
    // For simplicity in this step, we take the raw pointer or reference. 
    // Let's take EmbreeDevice& to imply dependency.

    ~EmbreeScene();

    // Prevent copying
    EmbreeScene(const EmbreeScene&) = delete;
    EmbreeScene& operator=(const EmbreeScene&) = delete;

    // Allow moving (if needed) - simplified for now

    void add_sphere(float cx, float cy, float cz, float r);
    void commit();
    
    // Return hit, t, nx, ny, nz
    std::tuple<bool, float, float, float, float> intersect(float ox, float oy, float oz, float dx, float dy, float dz);

private:
    RTCDevice device; // We might need to store device if we create geometries later, but add_sphere uses it.
    RTCScene scene;
};
