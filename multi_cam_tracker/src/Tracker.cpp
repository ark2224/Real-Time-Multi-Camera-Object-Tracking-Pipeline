#include "Tracker.hpp"
#include <opencv2/tracking.hpp>
#include <iostream>

Tracker::Tracker(int cameraId,
                 std::shared_ptr<ThreadSafeQueue<FramePacket>> queue)
    : m_cameraId(cameraId),
      m_windowName("Camera " + std::to_string(cameraId)),
      m_queue(std::move(queue)) {}

Tracker::~Tracker() {
    stop();
}

void Tracker::start() {
    if (m_running) return;
    m_running = true;
    m_thread = std::thread(&Tracker::run, this);
}

void Tracker::stop() {
    if (!m_running) return;
    m_running = false;
    if (m_queue) {
        m_queue->stop();
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }
    cv::destroyWindow(m_windowName);
}

bool Tracker::initializeTracker(const FramePacket& packet) {
    cv::Mat frame = packet.frame.clone();
    cv::imshow(m_windowName, frame);
    std::cout << "Select ROI for camera " << m_cameraId
              << " and press ENTER/SPACE. Press 'c' to cancel.\n";
    
    cv::Rect2d roi = cv::selectROI(m_windowName, frame, false, false);
    if (roi.width == 0 || roi.height == 0) {
        std::cerr << "No ROI selected for camera " << m_cameraId << "\n.";
        return false; 
    }

    // Use KCF (needs opencv_contrib). Alternatively, CSRT or MOSSE.
    m_tracker = cv::TrackerKCF::create();
    bool ok = m_tracker->init(frame, roi);
    if (!ok) {
        std::cerr << "Failed to initialize tracker for camera "
                  << m_cameraId << "\n";
    } else {
        m_initialized = true;
        m_frameCount = 0;
        m_lastFpsTime = std::chrono::steady_clock::now();
    }
    return ok;
}

void Tracker::run() {
    cv::namedWindow(m_windowName, cv::WINDOW_NORMAL);

    while (m_running) {
        auto optPacket = m_queue->pop();
        if (!optPacket.has_value()) {
            // Queeu stopped and emptied
            break;
        }
        

        FramePacket packet = std::move(optPacket.value());
        cv::Mat frame = packet.frame;

        if (!m_initialized) {
            if (!initializeTracker(packet)) {
                // If initialization fails, skip tracking but keep showing frames
                cv::imshow(m_windowName, frame);
                if (cv::waitKey(1) == 27) {// ESC
                    m_running = false;
                }
                continue;
            }
        } else {
            cv::Rect2d bbox;
            bool ok = m_tracker->update(frame, bbox);

            if (ok) {
                cv::rectangle(frame, bbox, cv::Scalar(0, 255, 0), 2);
            } else {
                cv::putText(frame, "Tracking failure detected",
                            cv::Point(50, 80),
                            cv::FONT_HERSHEY_SIMPLEX, 0.75,
                            cv::Scalar(0, 0, 255), 2);
            }

            // Latency: now - captured timestamp
            auto now = std::chrono::steady_clock::now();
            auto latencyMs = std::chrono::duration_cast<
                std::chrono::milliseconds>(now - packet.timestamp).count();

            // FPS calc
            m_frameCount++;
            auto elapsed = std::chrono::duration_cast<
                std::chrono::milliseconds>(now - m_lastFpsTime).count();
            double fps = 0.0;
            if (elapsed >= 1) {
                fps = m_frameCount / static_cast.double(elapsed);
                m_frameCount = 0;
                m_lastFpsTime = now;
            }

            // Overlay text
            std::string info = "Latency: " + std::to_string(latencyMs) + "ms";
            cv::putText(frame, info cv::Point(10, 20),
                        cv::FONT_HERSHEY_SIMPLEX, 0.6,
                        cv::Scalar(255, 255, 255), 1);
            
            if (fps > 0.0) {
                std::string fpsText = "FPS: " + std::to_string(static_cast<int>(fps));
                cv::putText(frame, fpsText, cv::Point(10, 45),
                            cv::FONT_HERSHEY_SIMPLEX, 0.6,
                            cv:: Scalar(255, 255, 255), 1);
            }
        }

        cv::imshow(m_windowName, frame);
        int key = cv::waitKey(1);
        if (key == 27) {// ESC
            m_running = false;
        }
    }

    std::cout << "Tracker for camera " << m_cameraId << " stopped.\n" << endl;
}