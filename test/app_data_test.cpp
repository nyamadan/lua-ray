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
    
    // swap()でバックバッファをフロントに移動
    data.swap();
    
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
    
    // swap()でバックバッファをフロントに移動
    data.swap();
    
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

// ダブルバッファリング機能のテスト（TDD: テストを先に書く）
TEST_F(AppDataTest, GetPixelReturnsCorrectValues) {
    AppData data(10, 10);
    
    // バックバッファに書き込み
    data.set_pixel(3, 3, 100, 150, 200);
    
    // swapしてフロントバッファにする
    data.swap();
    
    // get_pixelでフロントバッファから読み取り
    auto [r, g, b] = data.get_pixel(3, 3);
    EXPECT_EQ(r, 100);
    EXPECT_EQ(g, 150);
    EXPECT_EQ(b, 200);
}

TEST_F(AppDataTest, SwapExchangesBuffers) {
    AppData data(10, 10);
    
    // 最初にバックバッファに赤を書き込み
    data.set_pixel(0, 0, 255, 0, 0);
    
    // swap: バック→フロント
    data.swap();
    
    // 次にバックバッファに緑を書き込み
    data.set_pixel(0, 0, 0, 255, 0);
    
    // フロントバッファ(赤)はまだ維持されている
    auto [r1, g1, b1] = data.get_pixel(0, 0);
    EXPECT_EQ(r1, 255);
    EXPECT_EQ(g1, 0);
    EXPECT_EQ(b1, 0);
    
    // 再度swap: 緑がフロントに来る
    data.swap();
    auto [r2, g2, b2] = data.get_pixel(0, 0);
    EXPECT_EQ(r2, 0);
    EXPECT_EQ(g2, 255);
    EXPECT_EQ(b2, 0);
}

TEST_F(AppDataTest, SetPixelWritesToBackBuffer) {
    AppData data(10, 10);
    
    // バックバッファに書き込み
    data.set_pixel(5, 5, 128, 64, 32);
    
    // swap前はget_pixelで読めない（フロントは初期値）
    auto [r1, g1, b1] = data.get_pixel(5, 5);
    EXPECT_EQ(r1, 0);
    EXPECT_EQ(g1, 0);
    EXPECT_EQ(b1, 0);
    
    // swap後に読める
    data.swap();
    auto [r2, g2, b2] = data.get_pixel(5, 5);
    EXPECT_EQ(r2, 128);
    EXPECT_EQ(g2, 64);
    EXPECT_EQ(b2, 32);
}

TEST_F(AppDataTest, GetPixelOutOfBoundsReturnsZero) {
    AppData data(10, 10);
    
    // 範囲外のピクセル取得は(0, 0, 0)を返す
    auto [r1, g1, b1] = data.get_pixel(-1, 0);
    EXPECT_EQ(r1, 0);
    EXPECT_EQ(g1, 0);
    EXPECT_EQ(b1, 0);
    
    auto [r2, g2, b2] = data.get_pixel(10, 5);
    EXPECT_EQ(r2, 0);
    EXPECT_EQ(g2, 0);
    EXPECT_EQ(b2, 0);
}

// 文字列ストレージ機能のテスト（TDD: テストを先に書く）
TEST_F(AppDataTest, SetStringAndGetString) {
    AppData data(10, 10);
    
    // 文字列を設定して取得
    data.set_string("materials", R"({"red": [1, 0, 0]})");
    
    EXPECT_EQ(data.get_string("materials"), R"({"red": [1, 0, 0]})");
}

TEST_F(AppDataTest, GetStringReturnsEmptyForNonexistentKey) {
    AppData data(10, 10);
    
    // 存在しないキーの読み取りは空文字列を返す
    EXPECT_EQ(data.get_string("nonexistent"), "");
}

TEST_F(AppDataTest, HasStringReturnsTrueForExistingKey) {
    AppData data(10, 10);
    
    // 最初は存在しない
    EXPECT_FALSE(data.has_string("test_key"));
    
    // 設定後は存在する
    data.set_string("test_key", "test_value");
    EXPECT_TRUE(data.has_string("test_key"));
}

TEST_F(AppDataTest, OverwriteExistingString) {
    AppData data(10, 10);
    
    // 文字列を設定
    data.set_string("key", "value1");
    EXPECT_EQ(data.get_string("key"), "value1");
    
    // 上書き
    data.set_string("key", "value2");
    EXPECT_EQ(data.get_string("key"), "value2");
}

// ========================================
// pop_next_index テスト（TDD Red Phase）
// ========================================

// テスト: 初期値0から開始
TEST_F(AppDataTest, PopNextIndexStartsFromZero) {
    AppData data(10, 10);
    
    // 最初の呼び出しは0を返す
    int index = data.pop_next_index("counter");
    EXPECT_EQ(index, 0);
}

// テスト: 連続呼び出しでインクリメント
TEST_F(AppDataTest, PopNextIndexIncrementsEachCall) {
    AppData data(10, 10);
    
    EXPECT_EQ(data.pop_next_index("counter"), 0);
    EXPECT_EQ(data.pop_next_index("counter"), 1);
    EXPECT_EQ(data.pop_next_index("counter"), 2);
    EXPECT_EQ(data.pop_next_index("counter"), 3);
}

// テスト: 異なるキーは独立してインクリメント
TEST_F(AppDataTest, PopNextIndexDifferentKeysAreIndependent) {
    AppData data(10, 10);
    
    EXPECT_EQ(data.pop_next_index("key_a"), 0);
    EXPECT_EQ(data.pop_next_index("key_b"), 0);
    EXPECT_EQ(data.pop_next_index("key_a"), 1);
    EXPECT_EQ(data.pop_next_index("key_b"), 1);
}

// テスト: set_stringで事前設定された値から開始
TEST_F(AppDataTest, PopNextIndexRespectsPresetValue) {
    AppData data(10, 10);
    
    // 事前に値を設定
    data.set_string("preset_counter", "10");
    
    // 10から開始してインクリメント
    EXPECT_EQ(data.pop_next_index("preset_counter"), 10);
    EXPECT_EQ(data.pop_next_index("preset_counter"), 11);
}

// ========================================
// copy_front_to_back テスト（TDD）
// ========================================

TEST_F(AppDataTest, CopyFrontToBackCopiesData) {
    AppData data(10, 10);
    
    // バックバッファに書き込み
    data.set_pixel(1, 1, 255, 0, 0);
    
    // swapでフロントに移動
    data.swap();
    
    // 新しいバックバッファに別の色を書き込み (元の状態と区別するため)
    data.set_pixel(1, 1, 0, 255, 0);
    
    // フロントからバックへコピー
    data.copy_front_to_back();
    
    // 再度swapして、コピーされたバックバッファの内容をフロントとして確認
    data.swap();
    
    // get_pixelでコピーされた値(赤)が取得できるはず
    auto [r, g, b] = data.get_pixel(1, 1);
    EXPECT_EQ(r, 255);
    EXPECT_EQ(g, 0);
    EXPECT_EQ(b, 0);
}

// ========================================
// copy_back_to_front テスト（TDD）
// ========================================

TEST_F(AppDataTest, CopyBackToFrontCopiesData) {
    AppData data(10, 10);
    
    // バックバッファに書き込み
    data.set_pixel(2, 2, 0, 0, 255);
    
    // swap前（フロントにある初期値の色、真っ黒であることを確認）
    auto [r1, g1, b1] = data.get_pixel(2, 2);
    EXPECT_EQ(r1, 0);
    EXPECT_EQ(g1, 0);
    EXPECT_EQ(b1, 0);
    
    // バックからフロントへコピー
    data.copy_back_to_front();
    
    // get_pixelでフロントをチェック、コピーされた値(青)が取得できるはず
    auto [r2, g2, b2] = data.get_pixel(2, 2);
    EXPECT_EQ(r2, 0);
    EXPECT_EQ(g2, 0);
    EXPECT_EQ(b2, 255);
}

// ========================================
// clear_back_buffer テスト（TDD）
// ========================================

TEST_F(AppDataTest, ClearBackBufferClearsOnlyBackBuffer) {
    AppData data(10, 10);
    
    // フロントとバックを異なる色で塗りつぶし
    data.set_pixel(1, 1, 255, 0, 0); // バックを赤に
    data.swap();
    // ここでフロントの(1,1)が赤になる
    
    data.set_pixel(1, 1, 0, 255, 0); // 新しいバックを緑に
    
    // バックバッファのみクリア
    data.clear_back_buffer();
    
    // 現在のフロントは赤のままのはず
    auto [rF, gF, bF] = data.get_pixel(1, 1);
    EXPECT_EQ(rF, 255);
    EXPECT_EQ(gF, 0);
    EXPECT_EQ(bF, 0);
    
    // swapして元バックだったバッファを確認
    data.swap();
    auto [rB, gB, bB] = data.get_pixel(1, 1);
    
    // AppData初期化のクリアカラーは(0,0,0) [Alphaは255]と想定されるため
    EXPECT_EQ(rB, 0);
    EXPECT_EQ(gB, 0);
    EXPECT_EQ(bB, 0);
}

// ========================================
// GltfData キャッシュテスト（TDD）
// ========================================

TEST_F(AppDataTest, LoadGltfAndGetGltf) {
    AppData data(10, 10);

    // glTFファイルをロード
    EXPECT_TRUE(data.load_gltf("box", "assets/Box.glb"));

    // キャッシュから取得
    auto gltf = data.get_gltf("box");
    ASSERT_NE(gltf, nullptr);
    EXPECT_TRUE(gltf->isLoaded());
    EXPECT_EQ(gltf->getMeshCount(), 1u);
}

TEST_F(AppDataTest, LoadGltfCachesAndSkipsDuplicate) {
    AppData data(10, 10);

    // 同じ名前で2回ロード → 2回目はスキップ（成功を返す）
    EXPECT_TRUE(data.load_gltf("box", "assets/Box.glb"));
    EXPECT_TRUE(data.load_gltf("box", "assets/Box.glb"));

    auto gltf = data.get_gltf("box");
    ASSERT_NE(gltf, nullptr);
}

TEST_F(AppDataTest, GetGltfReturnsNullForUnknownKey) {
    AppData data(10, 10);
    EXPECT_EQ(data.get_gltf("nonexistent"), nullptr);
}

TEST_F(AppDataTest, LoadGltfFailsForInvalidPath) {
    AppData data(10, 10);
    EXPECT_FALSE(data.load_gltf("bad", "nonexistent.glb"));
    EXPECT_EQ(data.get_gltf("bad"), nullptr);
}

// ========================================
// TextureImage キャッシュテスト（TDD）
// ========================================

TEST_F(AppDataTest, LoadTextureImageFromGltf) {
    AppData data(10, 10);

    // まず glTF をロード
    ASSERT_TRUE(data.load_gltf("boxtex", "assets/BoxTextured.glb"));

    // テクスチャ画像をロード
    EXPECT_TRUE(data.load_texture_image("boxtex_0", "boxtex", 0));

    // キャッシュから取得
    auto image = data.get_texture_image("boxtex_0");
    ASSERT_NE(image, nullptr);
    EXPECT_GT(image->width, 0);
    EXPECT_GT(image->height, 0);
    EXPECT_FALSE(image->pixels.empty());
}

TEST_F(AppDataTest, GetTextureImageReturnsNullForUnknownKey) {
    AppData data(10, 10);
    EXPECT_EQ(data.get_texture_image("nonexistent"), nullptr);
}

TEST_F(AppDataTest, LoadTextureImageFailsWithoutGltf) {
    AppData data(10, 10);

    // GltfData がロードされていない場合は失敗
    EXPECT_FALSE(data.load_texture_image("tex", "missing_gltf", 0));
}
