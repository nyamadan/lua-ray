#include <gtest/gtest.h>
#include "../src/thread_worker.h"
#include <thread>
#include <chrono>
#include <iostream>

class WorkerLifecycleTest : public ::testing::Test {
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

TEST_F(WorkerLifecycleTest, StopCalledOnTerminate) {
    ThreadWorker worker(data.get(), scene.get(), {0, 0, 100, 100}, 0);
    
    // Capture stdout
    testing::internal::CaptureStdout();
    
    // Start the worker with the test scene
    worker.start("workers/ray_worker.lua", "test_lifecycle");
    
    // Let it run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Terminate the worker
    worker.terminate();
    
    std::string output = testing::internal::GetCapturedStdout();
    
    // Check if the output contains "stop called"
    EXPECT_NE(output.find("stop called"), std::string::npos) << "Output: " << output;
}

TEST_F(WorkerLifecycleTest, PostEffectWorkerStopCalledOnTerminate) {
    ThreadWorker worker(data.get(), scene.get(), {0, 0, 100, 100}, 0);
    
    // Capture stdout
    testing::internal::CaptureStdout();
    
    // Start the worker with the test scene using posteffect script
    worker.start("workers/posteffect_worker.lua", "test_lifecycle");
    
    // Let it run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Terminate the worker
    worker.terminate();
    
    std::string output = testing::internal::GetCapturedStdout();
    
    // Check if the output contains "stop called"
    EXPECT_NE(output.find("stop called"), std::string::npos) << "Output: " << output;
}

// reset_workers: マルチスレッドでterminate後にstartを再呼び出しできることを検証
TEST_F(WorkerLifecycleTest, ResetWorkers_MultiThread) {
    ThreadWorker worker(data.get(), scene.get(), {0, 0, 100, 100}, 0);
    
    // Capture stdout
    testing::internal::CaptureStdout();
    
    // 1回目: start → terminate
    worker.start("workers/ray_worker.lua", "test_lifecycle");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    worker.terminate();
    
    // 2回目: start → terminate (reset_workersのシミュレーション)
    worker.start("workers/ray_worker.lua", "test_lifecycle");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    worker.terminate();
    
    std::string output = testing::internal::GetCapturedStdout();
    
    // start calledが2回出現することを検証
    size_t first_start = output.find("start called");
    ASSERT_NE(first_start, std::string::npos) << "Output: " << output;
    size_t second_start = output.find("start called", first_start + 1);
    EXPECT_NE(second_start, std::string::npos) << "start called should appear twice. Output: " << output;
    
    // stop calledが2回出現することを検証
    size_t first_stop = output.find("stop called");
    ASSERT_NE(first_stop, std::string::npos) << "Output: " << output;
    size_t second_stop = output.find("stop called", first_stop + 1);
    EXPECT_NE(second_stop, std::string::npos) << "stop called should appear twice. Output: " << output;
}

// reset_workers: シングルスレッド（同一ワーカー再利用）でも正常動作することを検証
TEST_F(WorkerLifecycleTest, ResetWorkers_SingleThread) {
    ThreadWorker worker(data.get(), scene.get(), {0, 0, 100, 100}, 0);
    
    // Capture stdout
    testing::internal::CaptureStdout();
    
    // 1回目: start → terminate (完了を待つ)
    worker.start("workers/ray_worker.lua", "test_lifecycle");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    worker.terminate();
    
    // 2回目: start → terminate (再起動)
    worker.start("workers/ray_worker.lua", "test_lifecycle");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    worker.terminate();
    
    std::string output = testing::internal::GetCapturedStdout();
    
    // startが2回呼ばれていることを検証
    size_t first_start = output.find("start called");
    ASSERT_NE(first_start, std::string::npos) << "Output: " << output;
    size_t second_start = output.find("start called", first_start + 1);
    EXPECT_NE(second_start, std::string::npos) << "start called should appear twice. Output: " << output;
    
    // stopが2回呼ばれていることを検証
    size_t first_stop = output.find("stop called");
    ASSERT_NE(first_stop, std::string::npos) << "Output: " << output;
    size_t second_stop = output.find("stop called", first_stop + 1);
    EXPECT_NE(second_stop, std::string::npos) << "stop called should appear twice. Output: " << output;
}
