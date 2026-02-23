#include <gtest/gtest.h>
#include "gltf_loader.h"
#include "embree_wrapper.h"

// =============================================================
// テストリスト (TDD):
// 1. [x] assets/Box.glb をパースできる
// 2. [x] メッシュ数が1である
// 3. [x] 頂点データを抽出できる（頂点数 > 0）
// 4. [x] インデックスデータを抽出できる（インデックス数 > 0）
// 5. [x] 不正なファイルパスでエラーハンドリングできる
// 6. [x] 抽出したメッシュをEmbreeSceneに追加できる
// 7. [x] 追加したメッシュにレイが交差する
// 8. [ ] BoxTextured.glb をパースできる
// 9. [ ] BoxTextured.glb から UV 座標を抽出できる
// 10. [ ] BoxTextured.glb からテクスチャ画像を取得できる
// 11. [ ] Box.glb（テクスチャなし）からは UV 座標が空
// 12. [ ] intersect の戻り値にバリセントリック座標が含まれる
// =============================================================

// --- テスト1: assets/Box.glb をパースできる ---
TEST(GltfLoaderTest, CanParseBoxGlb) {
    GltfData data;
    bool result = data.load("assets/Box.glb");
    ASSERT_TRUE(result) << "assets/Box.glb のパースに失敗";
    EXPECT_TRUE(data.isLoaded());
}

// --- テスト2: メッシュ数が1である ---
TEST(GltfLoaderTest, BoxGlbHasOneMesh) {
    GltfData data;
    ASSERT_TRUE(data.load("assets/Box.glb"));
    EXPECT_EQ(data.getMeshCount(), 1u);
}

// --- テスト3: 頂点データを抽出できる ---
TEST(GltfLoaderTest, CanExtractVertices) {
    GltfData data;
    ASSERT_TRUE(data.load("assets/Box.glb"));
    auto vertices = data.getVertices(0, 0);
    EXPECT_GT(vertices.size(), 0u) << "頂点データが空";
    // Boxは24頂点 (6面 x 4頂点) x 3成分(xyz) = 72 float
    EXPECT_EQ(vertices.size() % 3, 0u) << "頂点数が3の倍数でない";
}

// --- テスト4: インデックスデータを抽出できる ---
TEST(GltfLoaderTest, CanExtractIndices) {
    GltfData data;
    ASSERT_TRUE(data.load("assets/Box.glb"));
    auto indices = data.getIndices(0, 0);
    EXPECT_GT(indices.size(), 0u) << "インデックスデータが空";
    // 三角形メッシュなのでインデックスは3の倍数
    EXPECT_EQ(indices.size() % 3, 0u) << "インデックス数が3の倍数でない";
}

// --- テスト5: 不正なファイルパスでエラーハンドリング ---
TEST(GltfLoaderTest, InvalidPathReturnsFalse) {
    GltfData data;
    bool result = data.load("nonexistent/file.glb");
    EXPECT_FALSE(result);
    EXPECT_FALSE(data.isLoaded());
}

// --- テスト6: 抽出したメッシュをEmbreeSceneに追加できる ---
TEST(GltfLoaderTest, CanAddMeshToEmbreeScene) {
    GltfData data;
    ASSERT_TRUE(data.load("assets/Box.glb"));

    auto vertices = data.getVertices(0, 0);
    auto indices = data.getIndices(0, 0);
    ASSERT_GT(vertices.size(), 0u);
    ASSERT_GT(indices.size(), 0u);

    EmbreeDevice device;
    EmbreeScene scene(device);
    unsigned int geomID = scene.add_mesh(vertices, indices);
    EXPECT_NE(geomID, RTC_INVALID_GEOMETRY_ID);
    scene.commit();
}

// --- テスト7: 追加したメッシュにレイが交差し、バリセントリック座標も返る ---
TEST(GltfLoaderTest, RayIntersectsBoxMesh) {
    GltfData data;
    ASSERT_TRUE(data.load("assets/Box.glb"));

    auto vertices = data.getVertices(0, 0);
    auto indices = data.getIndices(0, 0);

    EmbreeDevice device;
    EmbreeScene scene(device);
    scene.add_mesh(vertices, indices);
    scene.commit();

    // Boxの中心に向かってレイを飛ばす (Box.glbは原点付近に配置されるはず)
    auto [hit, t, nx, ny, nz, geomID, primID, baryU, baryV] = scene.intersect(0.0f, 0.0f, 5.0f, 0.0f, 0.0f, -1.0f);
    EXPECT_TRUE(hit) << "ボックスメッシュにレイが交差しなかった";
    EXPECT_GT(t, 0.0f) << "交差距離が正でない";
    // バリセントリック座標は [0, 1] の範囲で、u + v <= 1
    EXPECT_GE(baryU, 0.0f);
    EXPECT_LE(baryU, 1.0f);
    EXPECT_GE(baryV, 0.0f);
    EXPECT_LE(baryV, 1.0f);
    EXPECT_LE(baryU + baryV, 1.0f + 1e-6f);
}

// --- テスト8: BoxTextured.glb をパースできる ---
TEST(GltfLoaderTest, CanParseBoxTexturedGlb) {
    GltfData data;
    bool result = data.load("assets/BoxTextured.glb");
    ASSERT_TRUE(result) << "assets/BoxTextured.glb のパースに失敗";
    EXPECT_TRUE(data.isLoaded());
    EXPECT_EQ(data.getMeshCount(), 1u);
}

// --- テスト9: BoxTextured.glb から UV 座標を抽出できる ---
TEST(GltfLoaderTest, CanExtractTexCoords) {
    GltfData data;
    ASSERT_TRUE(data.load("assets/BoxTextured.glb"));
    auto texcoords = data.getTexCoords(0, 0);
    EXPECT_GT(texcoords.size(), 0u) << "UV座標データが空";
    // UV座標は2成分(u, v)のフラット配列なので2の倍数
    EXPECT_EQ(texcoords.size() % 2, 0u) << "UV座標数が2の倍数でない";

    // 頂点数と一致するか確認
    auto vertices = data.getVertices(0, 0);
    EXPECT_EQ(texcoords.size() / 2, vertices.size() / 3) << "UV座標数と頂点数が一致しない";
}

// --- テスト10: BoxTextured.glb からテクスチャ画像を取得できる ---
TEST(GltfLoaderTest, CanExtractTextureImage) {
    GltfData data;
    ASSERT_TRUE(data.load("assets/BoxTextured.glb"));
    auto image = data.getTextureImage(0);
    EXPECT_GT(image.width, 0) << "テクスチャ幅が0";
    EXPECT_GT(image.height, 0) << "テクスチャ高さが0";
    EXPECT_GT(image.channels, 0) << "テクスチャチャンネル数が0";
    EXPECT_FALSE(image.pixels.empty()) << "テクスチャピクセルデータが空";
    // ピクセルデータサイズ = 幅 × 高さ × チャンネル数
    EXPECT_EQ(image.pixels.size(), static_cast<size_t>(image.width * image.height * image.channels));
}

// --- テスト11: Box.glb（テクスチャなし）からは UV 座標が空 ---
TEST(GltfLoaderTest, BoxGlbHasNoTexCoords) {
    GltfData data;
    ASSERT_TRUE(data.load("assets/Box.glb"));
    auto texcoords = data.getTexCoords(0, 0);
    // Box.glb にはテクスチャ座標がない可能性がある（あるかもしれないが検証）
    // glTF仕様上、UVがない場合は空を返す
    // 注: Box.glb にもUVがある場合があるので、このテストは実際のデータに依存
    // → 実装後に結果を確認して調整
}

// --- テスト12: テクスチャなしファイルからテクスチャ画像取得は空を返す ---
TEST(GltfLoaderTest, NoTextureReturnsEmptyImage) {
    GltfData data;
    ASSERT_TRUE(data.load("assets/Box.glb"));
    auto image = data.getTextureImage(0);
    // Box.glb にはテクスチャが埋め込まれていないはず
    EXPECT_EQ(image.width, 0);
    EXPECT_EQ(image.height, 0);
    EXPECT_TRUE(image.pixels.empty());
}
