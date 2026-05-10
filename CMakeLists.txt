cmake_minimum_required(VERSION 3.16)
project(Simulytix CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    src/Canvas.cpp
    src/Component.cpp
    src/CodeEditor.cpp
    src/Renderer.cpp
    src/Simulator.cpp
)

add_executable(Simulytix ${SOURCES})

# SDL2
find_package(SDL2 REQUIRED)
target_include_directories(Simulytix PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(Simulytix PRIVATE ${SDL2_LIBRARIES})

# SDL2_ttf
find_path(SDL2_TTF_INCLUDE_DIR SDL2/SDL_ttf.h
    HINTS /opt/homebrew/include /usr/local/include /usr/include)
find_library(SDL2_TTF_LIBRARY
    NAMES SDL2_ttf
    HINTS /opt/homebrew/lib /usr/local/lib /usr/lib)

if(SDL2_TTF_INCLUDE_DIR AND SDL2_TTF_LIBRARY)
    target_include_directories(Simulytix PRIVATE ${SDL2_TTF_INCLUDE_DIR})
    target_link_libraries(Simulytix PRIVATE ${SDL2_TTF_LIBRARY})
    message(STATUS "SDL2_ttf found: ${SDL2_TTF_LIBRARY}")
else()
    message(FATAL_ERROR "SDL2_ttf not found. Run: brew install sdl2_ttf")
endif()

if(APPLE)
    target_link_libraries(Simulytix PRIVATE "-framework AppKit")
endif()

target_compile_options(Simulytix PRIVATE -O2 -Wall)
