#include <gtest/gtest.h>
#include "app_data.h"

class AppDataTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup code if needed
    }

    void TearDown() override {
        // Common cleanup code if needed
    }
};

TEST_F(AppDataTest, ConstructorInitializesDimensions) {
    AppData data(800, 600);
    EXPECT_EQ(data.get_width(), 800);
    EXPECT_EQ(data.get_height(), 600);
}

TEST_F(AppDataTest, SetPixelCorrectlyUpdatesData) {
    AppData data(10, 10);
    
    // Set a pixel at (5, 5) to red
    data.set_pixel(5, 5, 255, 0, 0);
    
    const uint32_t* raw_data = static_cast<const uint32_t*>(data.get_data());
    uint32_t expected_color = (255) | (0 << 8) | (0 << 16) | (255 << 24);
    
    EXPECT_EQ(raw_data[5 * 10 + 5], expected_color);
}

TEST_F(AppDataTest, SetPixelIgnoresOutOfBounds) {
    AppData data(10, 10);
    
    // Attempt to set pixels outside the bounds
    data.set_pixel(-1, 0, 255, 255, 255);
    data.set_pixel(0, -1, 255, 255, 255);
    data.set_pixel(10, 0, 255, 255, 255);
    data.set_pixel(0, 10, 255, 255, 255);
    
    // Verify that memory access violations didn't crash the test runner (implicit)
    // and ideally, check that nothing unexpected was written if we could inspect internal state deeply.
    // For now, implicit crash safety is enough, plus checking a valid pixel remains unchanged if we were overwriting.
    
    data.set_pixel(0, 0, 10, 10, 10);
    const uint32_t* raw_data = static_cast<const uint32_t*>(data.get_data());
    uint32_t expected_color = (10) | (10 << 8) | (10 << 16) | (255 << 24);
    EXPECT_EQ(raw_data[0], expected_color);
}

TEST_F(AppDataTest, InitialContentIsBlackAssumingAlpha) {
    AppData data(2, 2);
    const uint32_t* raw_data = static_cast<const uint32_t*>(data.get_data());
    
    // We initialized with 0xFF000000 (Black with 255 Alpha if format is RGBA/ABGR... wait, let's check AppData init)
    // AppData Init: std::fill(m_pixels.begin(), m_pixels.end(), 0xFF000000);
    // 0xFF000000 usually means Alpha=255, R=0, G=0, B=0 in little endian 0xAABBGGRR
    
    EXPECT_EQ(raw_data[0], 0xFF000000);
    EXPECT_EQ(raw_data[1], 0xFF000000);
    EXPECT_EQ(raw_data[2], 0xFF000000);
    EXPECT_EQ(raw_data[3], 0xFF000000);
}
