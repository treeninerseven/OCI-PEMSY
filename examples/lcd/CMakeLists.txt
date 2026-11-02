cmake_minimum_required(VERSION 3.16)

if(NOT PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
set(PICO_BOARD pico CACHE STRING "Course RP2040 Pico")
set(PICO_NO_PICOTOOL 1)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(lcd_logo C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(lcd_logo main.c lcd.c mioty_logo.c)
target_compile_options(lcd_logo PRIVATE -Wall -Wextra -Werror)
target_link_libraries(lcd_logo PRIVATE pico_stdlib hardware_gpio)
# GPIO0 and GPIO1 belong to the LCD, so do not use them for UART.
pico_enable_stdio_uart(lcd_logo 0)
pico_add_extra_outputs(lcd_logo)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_command(TARGET lcd_logo POST_BUILD
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/../../tools/bin_to_uf2.py
        ${CMAKE_CURRENT_BINARY_DIR}/lcd_logo.bin
        ${CMAKE_CURRENT_BINARY_DIR}/lcd_logo.uf2
    VERBATIM)
