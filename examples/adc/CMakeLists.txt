cmake_minimum_required(VERSION 3.16)

if(NOT PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
set(PICO_BOARD pico CACHE STRING "Course RP2040 Pico")
set(PICO_NO_PICOTOOL 1)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(adc_temperature C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

add_executable(adc_temperature main.c adc.c ../lcd/lcd.c)
target_include_directories(adc_temperature PRIVATE ../lcd)
target_compile_options(adc_temperature PRIVATE -Wall -Wextra -Werror)
target_link_libraries(adc_temperature PRIVATE pico_stdlib hardware_adc hardware_pwm)
pico_enable_stdio_usb(adc_temperature 1)
pico_enable_stdio_uart(adc_temperature 0) # GPIO0/1 belong to the LCD.
pico_add_extra_outputs(adc_temperature)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_command(TARGET adc_temperature POST_BUILD
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/../../tools/bin_to_uf2.py
        ${CMAKE_CURRENT_BINARY_DIR}/adc_temperature.bin
        ${CMAKE_CURRENT_BINARY_DIR}/adc_temperature.uf2
    VERBATIM)
