#pragma once

#include <opencv/opencv/opencv.hpp>
#include <string>
#include <atomic>
#include <thread>
#include <memory>
#include <chrono>

#include "ThreadSafeQueue.hpp"

struct FramePacket {
    int cameraId;
    cv::Mat frame;
    std::chrono::steady_clock::time_point timestamp;
}

class FrameGrabber {
    public:
    FrameGrabber(int camId,
                 const std::string& source,
                 std::shard_ptr<ThreadSafeQueue<FramePacket>> queue);
    ~FrameGrabber();

    void start();
    void stop();

private:
    void run();

    int m_cameraId;
    std::string m_source;
    std::shared_ptr<ThreadSafeQueue<FramePacket>> m_queue;

    std::atomic<bool> m_running(false);
    std::thread m_thread;
}