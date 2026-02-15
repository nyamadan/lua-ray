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
// 6. [ ] 抽出したメッシュをEmbreeSceneに追加できる
// 7. [ ] 追加したメッシュにレイが交差する
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

// --- テスト7: 追加したメッシュにレイが交差する ---
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
    auto [hit, t, nx, ny, nz, geomID, primID] = scene.intersect(0.0f, 0.0f, 5.0f, 0.0f, 0.0f, -1.0f);
    EXPECT_TRUE(hit) << "ボックスメッシュにレイが交差しなかった";
    EXPECT_GT(t, 0.0f) << "交差距離が正でない";
}
