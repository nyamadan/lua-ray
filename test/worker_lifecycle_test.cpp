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
