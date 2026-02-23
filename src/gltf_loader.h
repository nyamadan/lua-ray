#pragma once
#include <string>
#include <vector>

// 前方宣言 (cgltf の型はcppで使用)
struct cgltf_data;

/// テクスチャ画像データ
struct TextureImage {
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<unsigned char> pixels;
};

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

    /// 指定メッシュ・プリミティブのUV座標 (TEXCOORD_0) を取得 (u,vのフラット配列)
    std::vector<float> getTexCoords(size_t meshIndex, size_t primitiveIndex) const;

    /// 指定インデックスのテクスチャ画像を取得（デコード済み）
    TextureImage getTextureImage(size_t textureIndex) const;

    /// リソースを解放する
    void release();

private:
    cgltf_data* data_;
};

