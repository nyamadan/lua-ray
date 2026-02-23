#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include "gltf_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

// ----------------------------------------------------------------
// GltfData
// ----------------------------------------------------------------

GltfData::GltfData() : data_(nullptr) {}

GltfData::~GltfData() {
    release();
}

GltfData::GltfData(GltfData&& other) noexcept : data_(other.data_) {
    other.data_ = nullptr;
}

GltfData& GltfData::operator=(GltfData&& other) noexcept {
    if (this != &other) {
        release();
        data_ = other.data_;
        other.data_ = nullptr;
    }
    return *this;
}

bool GltfData::load(const std::string& path) {
    // 既存データがあれば解放
    release();

    cgltf_options options = {};
    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data_);
    if (result != cgltf_result_success) {
        std::cerr << "glTFパースエラー: " << path << std::endl;
        data_ = nullptr;
        return false;
    }

    // バッファデータをロード
    result = cgltf_load_buffers(&options, data_, path.c_str());
    if (result != cgltf_result_success) {
        std::cerr << "glTFバッファロードエラー: " << path << std::endl;
        cgltf_free(data_);
        data_ = nullptr;
        return false;
    }

    return true;
}

bool GltfData::isLoaded() const {
    return data_ != nullptr;
}

size_t GltfData::getMeshCount() const {
    if (!data_) return 0;
    return data_->meshes_count;
}

std::vector<float> GltfData::getVertices(size_t meshIndex, size_t primitiveIndex) const {
    if (!data_ || meshIndex >= data_->meshes_count) return {};

    const cgltf_mesh& mesh = data_->meshes[meshIndex];
    if (primitiveIndex >= mesh.primitives_count) return {};

    const cgltf_primitive& prim = mesh.primitives[primitiveIndex];

    // POSITION属性を探す
    for (size_t i = 0; i < prim.attributes_count; ++i) {
        if (prim.attributes[i].type == cgltf_attribute_type_position) {
            const cgltf_accessor* accessor = prim.attributes[i].data;
            size_t floatCount = cgltf_accessor_unpack_floats(accessor, nullptr, 0);
            std::vector<float> vertices(floatCount);
            cgltf_accessor_unpack_floats(accessor, vertices.data(), floatCount);
            return vertices;
        }
    }
    return {};
}

std::vector<unsigned int> GltfData::getIndices(size_t meshIndex, size_t primitiveIndex) const {
    if (!data_ || meshIndex >= data_->meshes_count) return {};

    const cgltf_mesh& mesh = data_->meshes[meshIndex];
    if (primitiveIndex >= mesh.primitives_count) return {};

    const cgltf_primitive& prim = mesh.primitives[primitiveIndex];
    if (!prim.indices) return {};

    const cgltf_accessor* accessor = prim.indices;
    size_t indexCount = accessor->count;
    std::vector<unsigned int> indices(indexCount);
    cgltf_accessor_unpack_indices(accessor, indices.data(), sizeof(unsigned int), indexCount);
    return indices;
}

std::vector<float> GltfData::getTexCoords(size_t meshIndex, size_t primitiveIndex) const {
    if (!data_ || meshIndex >= data_->meshes_count) return {};

    const cgltf_mesh& mesh = data_->meshes[meshIndex];
    if (primitiveIndex >= mesh.primitives_count) return {};

    const cgltf_primitive& prim = mesh.primitives[primitiveIndex];

    // TEXCOORD_0 属性を探す
    for (size_t i = 0; i < prim.attributes_count; ++i) {
        if (prim.attributes[i].type == cgltf_attribute_type_texcoord &&
            prim.attributes[i].index == 0) {
            const cgltf_accessor* accessor = prim.attributes[i].data;
            size_t floatCount = cgltf_accessor_unpack_floats(accessor, nullptr, 0);
            std::vector<float> texcoords(floatCount);
            cgltf_accessor_unpack_floats(accessor, texcoords.data(), floatCount);
            return texcoords;
        }
    }
    return {};
}

TextureImage GltfData::getTextureImage(size_t textureIndex) const {
    TextureImage result;
    if (!data_) return result;

    // テクスチャ配列のインデックスチェック
    if (textureIndex >= data_->textures_count) return result;

    const cgltf_texture& texture = data_->textures[textureIndex];
    if (!texture.image) return result;

    const cgltf_image& image = *texture.image;

    // バッファビューから画像データを取得（GLB埋め込み画像の場合）
    if (image.buffer_view) {
        const cgltf_buffer_view& bv = *image.buffer_view;
        const unsigned char* rawData = static_cast<const unsigned char*>(bv.buffer->data) + bv.offset;
        size_t rawSize = bv.size;

        // stb_imageでデコード
        int w, h, ch;
        unsigned char* decoded = stbi_load_from_memory(rawData, static_cast<int>(rawSize), &w, &h, &ch, 0);
        if (decoded) {
            result.width = w;
            result.height = h;
            result.channels = ch;
            result.pixels.assign(decoded, decoded + w * h * ch);
            stbi_image_free(decoded);
        }
    }

    return result;
}

void GltfData::release() {
    if (data_) {
        cgltf_free(data_);
        data_ = nullptr;
    }
}
