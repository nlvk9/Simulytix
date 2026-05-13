cmake_minimum_required(VERSION 3.16)
project(Simulytix CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network)

set(SOURCES
    src/main.cpp
    src/Canvas.cpp
    src/Component.cpp
    src/Simulator.cpp
    src/MainWindow.cpp
    src/CanvasWidget.cpp
    src/AiTipsPanel.cpp
)

add_executable(Simulytix ${SOURCES})

target_link_libraries(Simulytix PRIVATE Qt5::Core Qt5::Widgets Qt5::Network)

if(APPLE)
    target_link_libraries(Simulytix PRIVATE "-framework AppKit")
endif()

target_compile_options(Simulytix PRIVATE -O2 -Wall)
