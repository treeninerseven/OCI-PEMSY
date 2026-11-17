cmake_minimum_required(VERSION 3.16)

if(NOT PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
set(PICO_BOARD pico CACHE STRING "Course RP2040 Pico")
# Reuse this repository's UF2 converter, without downloading picotool.
set(PICO_NO_PICOTOOL 1)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(led_beacon C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
if(NOT PICO_BOARD STREQUAL "pico")
    message(FATAL_ERROR "This example requires the course RP2040 Pico board")
endif()

add_executable(led_beacon main.c led.c)
target_compile_options(led_beacon PRIVATE -Wall -Wextra -Werror)
target_link_libraries(led_beacon PRIVATE pico_stdlib hardware_gpio)
pico_add_extra_outputs(led_beacon)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_command(TARGET led_beacon POST_BUILD
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/../../tools/bin_to_uf2.py
        ${CMAKE_CURRENT_BINARY_DIR}/led_beacon.bin
        ${CMAKE_CURRENT_BINARY_DIR}/led_beacon.uf2
    VERBATIM)
