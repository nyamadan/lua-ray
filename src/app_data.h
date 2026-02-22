#pragma once
#include <vector>
#include <cstdint>
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <string>
#include <mutex>

class AppData {
public:
    AppData(int width, int height) : m_width(width), m_height(height) {
        m_front_buffer.resize(width * height);
        m_back_buffer.resize(width * height);
        std::fill(m_front_buffer.begin(), m_front_buffer.end(), 0xFF000000);
        std::fill(m_back_buffer.begin(), m_back_buffer.end(), 0xFF000000);
    }

    // バックバッファに書き込み
    void set_pixel(int x, int y, int r, int g, int b) {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height) return;
        uint32_t color = (r) | (g << 8) | (b << 16) | (255 << 24);
        m_back_buffer[y * m_width + x] = color;
    }

    // フロントバッファから読み取り
    std::tuple<int, int, int> get_pixel(int x, int y) const {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
            return std::make_tuple(0, 0, 0);
        }
        uint32_t color = m_front_buffer[y * m_width + x];
        int r = color & 0xFF;
        int g = (color >> 8) & 0xFF;
        int b = (color >> 16) & 0xFF;
        return std::make_tuple(r, g, b);
    }

    // フロントとバックを交換
    void swap() {
        std::swap(m_front_buffer, m_back_buffer);
    }

    // フロントバッファをバックバッファにコピー
    void copy_front_to_back() {
        std::copy(m_front_buffer.begin(), m_front_buffer.end(), m_back_buffer.begin());
    }

    // フロントバッファのデータ取得（テクスチャ更新用）
    const void* get_data() const {
        return m_front_buffer.data();
    }

    // バックバッファのデータ取得（レンダリング途中表示用）
    const void* get_back_data() const {
        return m_back_buffer.data();
    }

    int get_width() const { return m_width; }
    int get_height() const { return m_height; }

    // 両バッファをクリア
    void clear() {
        std::fill(m_front_buffer.begin(), m_front_buffer.end(), 0xFF000000);
        std::fill(m_back_buffer.begin(), m_back_buffer.end(), 0xFF000000);
    }

    // 文字列ストレージ（排他制御付き）
    void set_string(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(m_string_mutex);
        m_string_storage[key] = value;
    }

    std::string get_string(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_string_mutex);
        auto it = m_string_storage.find(key);
        if (it != m_string_storage.end()) {
            return it->second;
        }
        return "";
    }

    bool has_string(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_string_mutex);
        return m_string_storage.find(key) != m_string_storage.end();
    }

    // 指定キーのカウンタをアトミックにインクリメントし、前の値を返す
    // スレッド間で排他的にインデックスを取得するために使用
    int pop_next_index(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_string_mutex);
        auto it = m_string_storage.find(key);
        int current = 0;
        if (it != m_string_storage.end()) {
            current = std::stoi(it->second);
        }
        m_string_storage[key] = std::to_string(current + 1);
        return current;
    }

private:
    int m_width;
    int m_height;
    std::vector<uint32_t> m_front_buffer;
    std::vector<uint32_t> m_back_buffer;
    
    // 文字列ストレージ（スレッド間データ共有用）
    std::unordered_map<std::string, std::string> m_string_storage;
    mutable std::mutex m_string_mutex;
};

