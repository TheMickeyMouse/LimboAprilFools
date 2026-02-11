cmake_minimum_required(VERSION 4.0)
project(LimboFools)

set(CMAKE_CXX_STANDARD 20)

add_executable(LimboFools main.cpp
        LimboApp.cpp
        LimboApp.h
        miniaudio/miniaudio.cpp
        Timeline.cpp
        Timeline.h
        System.h
        System.cpp)

add_subdirectory(OpenGLPort)
add_subdirectory(Quasi)

target_link_directories(${PROJECT_NAME} PUBLIC lib)

target_link_libraries(${PROJECT_NAME} PUBLIC Quasi)

# set to 0 or 1.
set(SAFE_MODE 1)

if (SAFE_MODE)
    message(NOTICE "Safe Mode is currently on. No system level trolls will take place. "
                   "To change this, go to CMakeLists.txt and turn off SAFE_MODE.")
    target_compile_definitions(${PROJECT_NAME} PUBLIC -DSAFE_MODE)
else ()
    message(WARNING "Safe Mode is not on. All System level trolls are active and will take place. "
                    "To change this, go to CMakeLists.txt and turn on SAFE_MODE.")
endif ()


target_compile_options(LimboFools PRIVATE
    "$<IF:$<CONFIG:Release>,-s,>"
)