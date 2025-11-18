cmake_minimum_required(VERSION 3.10)
project(MultiCamTracker LANGUAGE CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable more warnings
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(OpenCV REQUIRED)

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(multi_cam_tracker
    src/main.cpp
    src/FrameGrabber.cpp
    src/Tracker.cpp
)

target_link_libraries(multi_cam_tracker
    ${OpenCV_LIBS}
    pthread             # ignored if not on Linux
)