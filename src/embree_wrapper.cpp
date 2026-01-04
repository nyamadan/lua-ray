#include "embree_wrapper.h"
#include <iostream>
#include <limits>
#include <cmath>

int api_embree_new_device(lua_State* L) {
    RTCDevice device = rtcNewDevice(NULL);
    if (!device) {
        std::cerr << "Failed to create Embree device" << std::endl;
        return 0;
    }
    lua_pushlightuserdata(L, device);
    return 1;
}

int api_embree_new_scene(lua_State* L) {
    RTCDevice device = (RTCDevice)lua_touserdata(L, 1);
    RTCScene scene = rtcNewScene(device);
    lua_pushlightuserdata(L, scene);
    return 1;
}

int api_embree_add_sphere(lua_State* L) {
    RTCDevice device = (RTCDevice)lua_touserdata(L, 1);
    RTCScene scene = (RTCScene)lua_touserdata(L, 2);
    float cx = luaL_checknumber(L, 3);
    float cy = luaL_checknumber(L, 4);
    float cz = luaL_checknumber(L, 5);
    float r = luaL_checknumber(L, 6);

    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_SPHERE_POINT);
    float* buffer = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT4, sizeof(float) * 4, 1);
    buffer[0] = cx;
    buffer[1] = cy;
    buffer[2] = cz;
    buffer[3] = r;

    rtcCommitGeometry(geom);
    rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);
    return 0;
}

int api_embree_commit_scene(lua_State* L) {
    RTCScene scene = (RTCScene)lua_touserdata(L, 1);
    rtcCommitScene(scene);
    return 0;
}

int api_embree_intersect(lua_State* L) {
    RTCScene scene = (RTCScene)lua_touserdata(L, 1);
    float ox = luaL_checknumber(L, 2);
    float oy = luaL_checknumber(L, 3);
    float oz = luaL_checknumber(L, 4);
    float dx = luaL_checknumber(L, 5);
    float dy = luaL_checknumber(L, 6);
    float dz = luaL_checknumber(L, 7);

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
        lua_pushboolean(L, true);
        lua_pushnumber(L, rayhit.ray.tfar); // t
        
        // Normalize normal
        float nx = rayhit.hit.Ng_x;
        float ny = rayhit.hit.Ng_y;
        float nz = rayhit.hit.Ng_z;
        float len = sqrt(nx*nx + ny*ny + nz*nz);
        lua_pushnumber(L, nx/len);
        lua_pushnumber(L, ny/len);
        lua_pushnumber(L, nz/len);
        return 5;
    } else {
        lua_pushboolean(L, false);
        return 1;
    }
}

int api_embree_release_scene(lua_State* L) {
    RTCScene scene = (RTCScene)lua_touserdata(L, 1);
    rtcReleaseScene(scene);
    return 0;
}

int api_embree_release_device(lua_State* L) {
    RTCDevice device = (RTCDevice)lua_touserdata(L, 1);
    rtcReleaseDevice(device);
    return 0;
}
