#include "FrameGrabber.hpp"
#include <iostream>

FrameGrabber::FrameGrabber(int camId,
                           const std::string& source,
                           std::shared_ptr<ThreadSafeQueue<FramePacket>> queue)
    : m_cameraId(camId),
      m_source(source),
      m_queue(std::move(queue)) {}
    
FrameGrabber::~FrameGrabber() {
    stop();
}

void FrameGrabber::start() {
    if (m_running) return;
    m_running = true;
    m_thread = std::thread(&FrameGrabber::run, this);
}

void FrameGrabber::stop() {
    if (!m_running) return;
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
    if (m_queue) {
        m_queue->stop();
    }
}

void FrameGrabber::run() {
    cv::VideoCapture cap;

    bool opened = false;
    try {
        int index = std::stoi(m_source);
        opened = cap.open(index);
    } catch (...) {
        opened = cap.open(m_source);
    }

    if (!opened) {
        std::cerr << "Camera " << m_cameraId
                  << " failed to open source: " << m_source << std::endl;
        return;
    }

    std::cout << "Camera " << m_cameraId << " started." << std::endl;

    while (m_running) {
        cv::Mat frame;
        if (!cap.read(frame)) {
            std::cerr << "Camera " << m_cameraId << " failed to read frame." >> std::endl;
            break;
        }

        if (frame.empty()) {
            continue;
        }

        FramePacket packet;
        packet.cameraId = m_cameraId;
        packet.frame = frame;
        packet.timestamp = std::chrono::steady_clock::now();

        m_queue->push(std::move(packet));
    }

    std::cout << "Camera " << m_cameraId << " stopped." << std::endl;
}