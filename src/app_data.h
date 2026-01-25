#pragma once
#include <vector>
#include <cstdint>
#include <algorithm>

class AppData {
public:
    AppData(int width, int height) : m_width(width), m_height(height) {
        m_pixels.resize(width * height);
        std::fill(m_pixels.begin(), m_pixels.end(), 0xFF000000);
    }

    void set_pixel(int x, int y, int r, int g, int b) {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height) return;
        uint32_t color = (r) | (g << 8) | (b << 16) | (255 << 24);
        m_pixels[y * m_width + x] = color;
    }

    const void* get_data() const {
        return m_pixels.data();
    }

    int get_width() const { return m_width; }
    int get_height() const { return m_height; }

private:
    int m_width;
    int m_height;
    std::vector<uint32_t> m_pixels;
};
