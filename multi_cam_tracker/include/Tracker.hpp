#pragma once

// #include <opencv/opencv/opencv.hpp>
#include <opencv2/opencv.hpp>
#include <atomic>
#include <thread>
#include <memory>
#include <string>
#include <chrono>

#include "ThreadSafeQueue.hpp"
#include "FrameGrabber.hpp"

class Tracker {
public:
    Tracker(int cameraId,
            std::shared_ptr<ThreadSafeQueue<FramePacket>> queue);
    ~Tracker();

    void start();
    void stop();

private:
    void run();
    bool initializeTracker(const FramePacket& packet);

    int m_cameraId;
    std::string m_windowName;
    std::shared_ptr<ThreadSafeQueue<FramePacket>> m_queue;

    std::atomic<bool> m_running{false};
    std::thread m_thread;

    cv::Ptr<cv::Tracker> m_tracker;
    bool m_initialized{false};

    int m_frameCount{0};
    std::chrono::steady_clock::time_point m_lastFpsTime;
};