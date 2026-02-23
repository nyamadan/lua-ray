#pragma once
#include <vector>
#include <cstdint>
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <string>
#include <mutex>
#include <memory>
#include "gltf_loader.h"

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

    // バックバッファをフロントバッファにコピー
    void copy_back_to_front() {
        std::copy(m_back_buffer.begin(), m_back_buffer.end(), m_front_buffer.begin());
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

    // バックバッファのみをクリア
    void clear_back_buffer() {
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

    // ================================================================
    // GltfData キャッシュ（スレッド間 readonly 共有）
    // ================================================================

    // glTFファイルをロードしてキャッシュに格納（既にロード済みの場合はスキップ）
    // @return true: ロード成功（または既にロード済み）, false: ロード失敗
    bool load_gltf(const std::string& name, const std::string& path) {
        std::lock_guard<std::mutex> lock(m_resource_mutex);
        // 既にロード済みならスキップ
        if (m_gltf_cache.find(name) != m_gltf_cache.end()) {
            return true;
        }
        auto gltf = std::make_shared<GltfData>();
        if (!gltf->load(path)) {
            return false;
        }
        m_gltf_cache[name] = std::move(gltf);
        return true;
    }

    // キャッシュされた GltfData を取得（readonly）
    // @return 見つからない場合は nullptr
    std::shared_ptr<GltfData> get_gltf(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_resource_mutex);
        auto it = m_gltf_cache.find(name);
        if (it != m_gltf_cache.end()) {
            return it->second;
        }
        return nullptr;
    }

    // ================================================================
    // TextureImage キャッシュ（スレッド間 readonly 共有）
    // ================================================================

    // GltfData からテクスチャ画像をデコードしてキャッシュに格納
    // @param name キャッシュキー
    // @param gltf_name GltfDataキャッシュのキー
    // @param texture_index テクスチャインデックス
    // @return true: 成功（または既にロード済み）, false: 失敗
    bool load_texture_image(const std::string& name, const std::string& gltf_name, size_t texture_index) {
        std::lock_guard<std::mutex> lock(m_resource_mutex);
        // 既にロード済みならスキップ
        if (m_texture_cache.find(name) != m_texture_cache.end()) {
            return true;
        }
        auto gltf_it = m_gltf_cache.find(gltf_name);
        if (gltf_it == m_gltf_cache.end() || !gltf_it->second) {
            return false;
        }
        auto image = std::make_shared<TextureImage>(gltf_it->second->getTextureImage(texture_index));
        if (image->width == 0 || image->height == 0) {
            return false;
        }
        m_texture_cache[name] = std::move(image);
        return true;
    }

    // キャッシュされた TextureImage を取得（readonly）
    std::shared_ptr<TextureImage> get_texture_image(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_resource_mutex);
        auto it = m_texture_cache.find(name);
        if (it != m_texture_cache.end()) {
            return it->second;
        }
        return nullptr;
    }

private:
    int m_width;
    int m_height;
    std::vector<uint32_t> m_front_buffer;
    std::vector<uint32_t> m_back_buffer;
    
    // 文字列ストレージ（スレッド間データ共有用）
    std::unordered_map<std::string, std::string> m_string_storage;
    mutable std::mutex m_string_mutex;

    // リソースキャッシュ（スレッド間 readonly 共有用）
    std::unordered_map<std::string, std::shared_ptr<GltfData>> m_gltf_cache;
    std::unordered_map<std::string, std::shared_ptr<TextureImage>> m_texture_cache;
    mutable std::mutex m_resource_mutex;
};


