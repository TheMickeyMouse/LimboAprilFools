cmake_minimum_required(VERSION 4.0)
project(LimboFools)

set(CMAKE_CXX_STANDARD 20)

set(RESOURCE_FILES
    1.png
    vine-boom.mp3
    trophy.png
    error_sound.mp3
    comp_bg.png
    computer.png
    keymain.png
    LimboMus.mp3
    colorpalette.png
    partyblower.mp3
    mypc.mp3
    party-sheet.png
    incorrect.mp3
    limbokey.png
    losers_background.png
    icons.png
    ready.png
    keyhigh.png
    ur_comp.png
    spotlight.png
    wrong.png
    2.png
    ta-da.mp3
    go.png
    exp_compressed.png
    keyshadow.png
    post.glsl
    glow.png
    windows_shutdown.mp3
    keyoutline.png
    ominous_hands.png
    bsod.png
    3.png
    fake_error.png
    choose.png
    correct.mp3
    correct.png
    you_did_it.png
)

foreach(res ${RESOURCE_FILES})
    list(APPEND RESOURCE_ABSOLUTE "res/${res}")
endforeach()

find_program(ARCHIVE_CMD NAMES archive)

if (NOT ARCHIVE_CMD)
    message(SEND_ERROR "Archive not found. Download at https://github.com/TheMickeyMouse/Archiver.")
endif()

add_custom_command(
    OUTPUT
    "${CMAKE_SOURCE_DIR}/Resources.h"
    "${CMAKE_SOURCE_DIR}/arch.o"
    COMMAND ${ARCHIVE_CMD} -o arch.o -r res -f Resources.h ${RESOURCE_FILES}
    DEPENDS
        ${RESOURCE_ABSOLUTE}
        ${ARCHIVE_CMD}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

add_executable(LimboFools main.cpp
    LimboApp.cpp
    LimboApp.h
    miniaudio/miniaudio.cpp
    Timeline.cpp
    Timeline.h
    System.h
    System.cpp
    PostEffect.h
    PostEffect.cpp
    Resources.h
    arch.o
)

add_subdirectory(OpenGLPort)
add_subdirectory(Quasi)
target_compile_definitions(Quasi PUBLIC Q_NO_IMGUI)

target_link_directories(${PROJECT_NAME} PUBLIC lib)
target_link_libraries(${PROJECT_NAME} PUBLIC Quasi)

# set to 0 or 1.
set(SAFE_MODE 0)

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
    "$<IF:$<CONFIG:Release>,-mwindows,>"
)

target_link_libraries(LimboFools PRIVATE
    -static gcc stdc++ winpthread
)