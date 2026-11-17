cmake_minimum_required(VERSION 3.16)

if(NOT PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
set(PICO_BOARD pico CACHE STRING "Course RP2040 Pico")
set(PICO_NO_PICOTOOL 1)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pwm_encoder C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

add_executable(pwm_encoder main.c encoder.c ../lcd/lcd.c ../led_beacon/led.c)
target_include_directories(pwm_encoder PRIVATE ../lcd ../led_beacon)
target_compile_options(pwm_encoder PRIVATE -Wall -Wextra -Werror)
target_link_libraries(pwm_encoder PRIVATE pico_stdlib hardware_pwm)
pico_enable_stdio_usb(pwm_encoder 1)
pico_enable_stdio_uart(pwm_encoder 0)
pico_add_extra_outputs(pwm_encoder)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_command(TARGET pwm_encoder POST_BUILD
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/../../tools/bin_to_uf2.py
        ${CMAKE_CURRENT_BINARY_DIR}/pwm_encoder.bin
        ${CMAKE_CURRENT_BINARY_DIR}/pwm_encoder.uf2
    VERBATIM)
