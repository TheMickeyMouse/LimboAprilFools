cmake_minimum_required(VERSION 4.0)
project(LimboFools)

set(CMAKE_CXX_STANDARD 20)

add_executable(LimboFools main.cpp
        LimboApp.cpp
        LimboApp.h
        miniaudio/miniaudio.cpp
        Timeline.cpp
        Timeline.h)

add_subdirectory(OpenGLPort)
add_subdirectory(Quasi)

target_link_directories(${PROJECT_NAME} PUBLIC lib)

target_link_libraries(${PROJECT_NAME} PUBLIC Quasi)