#include <gtest/gtest.h>
#include <embree4/rtcore.h>

TEST(EmbreeLinkTest, CanCreateDevice) {
    RTCDevice device = rtcNewDevice(nullptr);
    ASSERT_NE(device, nullptr) << "rtcNewDevice failed";
    rtcReleaseDevice(device);
}

TEST(EmbreeLinkTest, CanCreateScene) {
    RTCDevice device = rtcNewDevice(nullptr);
    ASSERT_NE(device, nullptr) << "rtcNewDevice failed";
    
    RTCScene scene = rtcNewScene(device);
    ASSERT_NE(scene, nullptr) << "rtcNewScene failed";
    
    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
}

TEST(EmbreeLinkTest, CanCreateGeometry) {
    RTCDevice device = rtcNewDevice(nullptr);
    ASSERT_NE(device, nullptr) << "rtcNewDevice failed";
    
    RTCScene scene = rtcNewScene(device);
    ASSERT_NE(scene, nullptr) << "rtcNewScene failed";
    
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    ASSERT_NE(geom, nullptr) << "rtcNewGeometry failed";
    
    rtcReleaseGeometry(geom);
    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
}
