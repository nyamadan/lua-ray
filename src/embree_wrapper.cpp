#include "embree_wrapper.h"
#include <iostream>
#include <limits>
#include <cmath>

// ----------------------------------------------------------------
// EmbreeDevice
// ----------------------------------------------------------------

EmbreeDevice::EmbreeDevice() : device(nullptr) {
    device = rtcNewDevice(NULL);
    if (!device) {
        std::cerr << "Failed to create Embree device" << std::endl;
        // In a real app, throw exception or handle error
    }
}

EmbreeDevice::~EmbreeDevice() {
    release();
}

void EmbreeDevice::release() {
    if (device) {
        rtcReleaseDevice(device);
        device = nullptr;
    }
}

EmbreeDevice::EmbreeDevice(EmbreeDevice&& other) noexcept : device(other.device) {
    other.device = nullptr;
}

EmbreeDevice& EmbreeDevice::operator=(EmbreeDevice&& other) noexcept {
    if (this != &other) {
        release();
        device = other.device;
        other.device = nullptr;
    }
    return *this;
}

// ----------------------------------------------------------------
// EmbreeScene
// ----------------------------------------------------------------

EmbreeScene::EmbreeScene(EmbreeDevice& dev) : device(dev.get()), scene(nullptr) {
    if (device) {
        scene = rtcNewScene(device);
    }
}

EmbreeScene::~EmbreeScene() {
    release();
}

void EmbreeScene::release() {
    if (scene) {
        rtcReleaseScene(scene);
        scene = nullptr;
    }
}

void EmbreeScene::add_sphere(float cx, float cy, float cz, float r) {
    if (!device || !scene) return;

    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_SPHERE_POINT);
    float* buffer = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT4, sizeof(float) * 4, 1);
    buffer[0] = cx;
    buffer[1] = cy;
    buffer[2] = cz;
    buffer[3] = r;

    rtcCommitGeometry(geom);
    rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);
}

void EmbreeScene::add_triangle(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3) {
    if (!device || !scene) return;

    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    float* vertices = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), 3);
    vertices[0] = x1; vertices[1] = y1; vertices[2] = z1;
    vertices[3] = x2; vertices[4] = y2; vertices[5] = z2;
    vertices[6] = x3; vertices[7] = y3; vertices[8] = z3;

    unsigned int* indices = (unsigned int*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), 1);
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;

    rtcCommitGeometry(geom);
    rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);
}

void EmbreeScene::commit() {
    if (scene) {
        rtcCommitScene(scene);
    }
}

std::tuple<bool, float, float, float, float> EmbreeScene::intersect(float ox, float oy, float oz, float dx, float dy, float dz) {
    if (!scene) return {false, 0.0f, 0.0f, 0.0f, 0.0f};

    RTCRayHit rayhit;
    rayhit.ray.org_x = ox; rayhit.ray.org_y = oy; rayhit.ray.org_z = oz;
    rayhit.ray.dir_x = dx; rayhit.ray.dir_y = dy; rayhit.ray.dir_z = dz;
    rayhit.ray.tnear = 0.0f;
    rayhit.ray.tfar = std::numeric_limits<float>::infinity();
    rayhit.ray.mask = -1;
    rayhit.ray.flags = 0;
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    rtcIntersect1(scene, &rayhit);

    if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
        // Normalize normal
        float nx = rayhit.hit.Ng_x;
        float ny = rayhit.hit.Ng_y;
        float nz = rayhit.hit.Ng_z;
        float len = std::sqrt(nx*nx + ny*ny + nz*nz);
        if (len > 0) {
            nx /= len; ny /= len; nz /= len;
        }
        return {true, rayhit.ray.tfar, nx, ny, nz};
    } else {
        return {false, 0.0f, 0.0f, 0.0f, 0.0f};
    }
}

