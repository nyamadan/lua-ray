#pragma once
#include <string>
#include <vector>

// 前方宣言 (cgltf の型はcppで使用)
struct cgltf_data;

/// glTFファイルのRAIIラッパー
class GltfData {
public:
    GltfData();
    ~GltfData();

    // コピー禁止
    GltfData(const GltfData&) = delete;
    GltfData& operator=(const GltfData&) = delete;

    // ムーブ許可
    GltfData(GltfData&& other) noexcept;
    GltfData& operator=(GltfData&& other) noexcept;

    /// glTF/GLBファイルを読み込む
    bool load(const std::string& path);

    /// データが読み込まれているか
    bool isLoaded() const;

    /// メッシュ数を取得
    size_t getMeshCount() const;

    /// 指定メッシュ・プリミティブの頂点座標を取得 (x,y,zのフラット配列)
    std::vector<float> getVertices(size_t meshIndex, size_t primitiveIndex) const;

    /// 指定メッシュ・プリミティブのインデックスを取得
    std::vector<unsigned int> getIndices(size_t meshIndex, size_t primitiveIndex) const;

    /// リソースを解放する
    void release();

private:
    cgltf_data* data_;
};
