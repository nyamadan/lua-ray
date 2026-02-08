// thread_worker_test.cpp
// TDD Red Phase: ThreadWorkerのterminate機能のテスト

#include <gtest/gtest.h>
#include "../src/thread_worker.h"
#include "../src/app_data.h"
#include "../src/embree_wrapper.h"
#include <chrono>
#include <thread>

class ThreadWorkerTest : public ::testing::Test {
protected:
    void SetUp() override {
        data = std::make_unique<AppData>(100, 100);
        device = std::make_unique<EmbreeDevice>();
        scene = std::make_unique<EmbreeScene>(*device);
    }

    void TearDown() override {
        scene.reset();
        device.reset();
        data.reset();
    }

    std::unique_ptr<AppData> data;
    std::unique_ptr<EmbreeDevice> device;
    std::unique_ptr<EmbreeScene> scene;
};

// テスト1: 初期状態ではキャンセル要求されていない
TEST_F(ThreadWorkerTest, InitialCancelRequestedIsFalse) {
    ThreadWorker worker(data.get(), scene.get(), {0, 0, 100, 100}, 0);
    ASSERT_FALSE(worker.is_cancel_requested());
}

// テスト2: terminate()でキャンセルフラグが立つ
TEST_F(ThreadWorkerTest, TerminateSetsCancelFlag) {
    ThreadWorker worker(data.get(), scene.get(), {0, 0, 100, 100}, 0);
    ASSERT_FALSE(worker.is_cancel_requested());
    
    worker.terminate();
    ASSERT_TRUE(worker.is_cancel_requested());
}

// テスト3: terminate()を複数回呼んでも安全
TEST_F(ThreadWorkerTest, MultipleTerminateCallsAreSafe) {
    ThreadWorker worker(data.get(), scene.get(), {0, 0, 100, 100}, 0);
    
    // 複数回terminateを呼んでもクラッシュしない
    worker.terminate();
    worker.terminate();
    worker.terminate();
    
    ASSERT_TRUE(worker.is_cancel_requested());
}

// テスト4: 未開始ワーカーのterminateは安全
TEST_F(ThreadWorkerTest, TerminateBeforeStartIsSafe) {
    ThreadWorker worker(data.get(), scene.get(), {0, 0, 100, 100}, 0);
    
    // start()を呼ばずにterminateを呼んでも安全
    worker.terminate();
    ASSERT_TRUE(worker.is_cancel_requested());
    ASSERT_TRUE(worker.is_done()); // 未開始なのでdoneはtrue
}
