#include <iostream>
#include <vector>
#include <memory>
#include <string>

#include "FrameGrabber.hpp"
#include "Tracker.hpp"


int main(int argc, char** argv) {
    if (argc > 2) {
        std::cout << "Usage: " << argv[0]
            << " <source0> [<source1> ...]\n";
        std::cout << "Examples:\n"
                << "  " << argv[0] << " 0\n"
                << "  " << argv[0] << " 0 1\n"
                << "  " << argv[0] << " data/cam1.mp4 data/cam2.mp4\n" 
                << std::endl;
        
        return 0;
    }

    int numCams = argc - 1;
    std::vector<std::shared_ptr<ThreadSafeQueue<FramePacket>>> queues;
    std::vector<std::unique_ptr<FrameGrabber>> grabbers;
    std::vector<std::unique_ptr<Tracker>> trackers;


    queues.reserve(numCams);
    grabbers.reserve(numCams);
    trackers.reserve(numCams);

    for (int i = 0; i < numCams; ++i) {
        std::string source = argv[i + 1];
        auto queue = std::make_shared<ThreadSafeQueue<FramePacket>>();
        queues.push_back(queue);

        grabbers.emplace_back(
            std::make_unique<FrameGrabber>(i, source, queue)
        );

        trackers.emplace_back(
            std::make_unique<Tracker>(i, queue)
        );
    }

    // Start everything
    for (auto& g : grabbers) {
        g->start();
    }
    for (auto& t : trackers) {
        t->start();
    }

    // Simple wait loop: when one tracker stops (ESC), signal all
    std::cout << "Press Ctrl_C in terminal to force exit if needed.\n" << std::endl;

    // Busy wait is simple but fine for v1; could be improved with signals.
    while (true) {
        bool anyRunning = false;
        for (const auto& t : trackers) {
            // Didn't expose a running() getter to keep it simple.
            // Will just wait until user closes windows / ESC.
        }
        // Set a sleep timer to keep main thread running.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Cleanup (won't usually reach here, but let's be explicit)
    for (auto& g : grabbers) {
        g->stop();
    }
    for (auto& t : trackers) {
        t->stop();
    }

    return 0;
}