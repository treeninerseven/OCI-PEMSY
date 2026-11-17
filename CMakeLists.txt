cmake_minimum_required(VERSION 3.16)

option(PEMSY_HOST_TESTS "Build portable tests and terminal simulator" OFF)
if(PEMSY_HOST_TESTS)
    project(pemsy_host C CXX)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    enable_testing()
    add_compile_options(-Wall -Wextra -Werror -pedantic -UNDEBUG)
    add_library(pemsy_app src/app.c)
    target_include_directories(pemsy_app PUBLIC include)
    add_executable(test_app tests/test_app.c)
    target_link_libraries(test_app PRIVATE pemsy_app)
    add_test(NAME application COMMAND test_app)
    add_executable(pemsy_simulator tools/simulator.c)
    target_link_libraries(pemsy_simulator PRIVATE pemsy_app)
    add_executable(test_counter tests/test_counter.c src/counter.c)
    target_include_directories(test_counter PRIVATE include)
    add_test(NAME persistent_counter COMMAND test_counter)
    add_executable(test_counter_limits tests/test_counter_limits.c src/counter.c)
    target_include_directories(test_counter_limits PRIVATE include)
    add_test(NAME counter_limits COMMAND test_counter_limits)
    foreach(driver encoder temperature lcd)
        add_executable(test_${driver} tests/test_${driver}.c src/${driver}.c)
        target_include_directories(test_${driver} PRIVATE tests/peripheral_stubs include)
        add_test(NAME ${driver} COMMAND test_${driver})
    endforeach()
    target_link_libraries(test_temperature PRIVATE m)
    add_executable(test_radio tests/test_radio.c src/radio.c)
    target_include_directories(test_radio PRIVATE tests/radio_stubs include)
    add_test(NAME radio COMMAND test_radio)
    add_executable(test_mioty tests/test_mioty.cpp src/mioty.cpp)
    target_include_directories(test_mioty PRIVATE tests/radio_stubs include config)
    target_include_directories(test_mioty SYSTEM PRIVATE TS-UNB-Lib-Pico)
    target_compile_definitions(test_mioty PRIVATE PEMSY_ENABLE_MIOTY=1
        PEMSY_MIOTY_CONFIG_HEADER="mioty_credentials.example.h")
    add_test(NAME mioty_adapter COMMAND test_mioty)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME uf2_format COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_uf2.py)
    return()
endif()

if(NOT PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
if(NOT EXISTS "${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
    message(FATAL_ERROR "Set PICO_SDK_PATH to a Pico SDK checkout (tested with 2.1.1), or use -DPEMSY_HOST_TESTS=ON")
endif()
set(PICO_BOARD pico CACHE STRING "Course hardware: RP2040 Pico")
set(PICO_NO_PICOTOOL 1) # Python converter avoids downloading host build tools.
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(pemsy C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
pico_sdk_init()
if(NOT PICO_BOARD STREQUAL "pico")
    message(FATAL_ERROR "This board pinout and reserved flash layout require PICO_BOARD=pico")
endif()

set(PEMSY_NODE_ADDRESS 1 CACHE STRING "Local peer address (1..254)")
set(PEMSY_DEFAULT_RECIPIENT 2 CACHE STRING "Initial peer recipient (1..254 or 255 broadcast)")
set(PEMSY_GROUP 1 CACHE STRING "Course frequency group (1..6)")
set(PEMSY_ENCODER_EDGES_PER_DETENT 4 CACHE STRING "Encoder edges per detent (2 or 4)")
if(NOT PEMSY_ENCODER_EDGES_PER_DETENT MATCHES "^[24]$")
    message(FATAL_ERROR "PEMSY_ENCODER_EDGES_PER_DETENT must be 2 or 4")
endif()
option(PEMSY_RFM69_HIGH_POWER "Use RFM69HW instead of RFM69W" OFF)
option(PEMSY_ENABLE_MIOTY "Compile mioty with local endpoint credentials" OFF)
set(PEMSY_MIOTY_CONFIG_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/config/mioty_credentials.h" CACHE FILEPATH "Local mioty configuration header")
foreach(value PEMSY_NODE_ADDRESS PEMSY_DEFAULT_RECIPIENT PEMSY_GROUP)
    if(NOT "${${value}}" MATCHES "^[0-9]+$")
        message(FATAL_ERROR "${value} must be an integer")
    endif()
endforeach()
if(PEMSY_NODE_ADDRESS LESS 1 OR PEMSY_NODE_ADDRESS GREATER 254 OR
   PEMSY_DEFAULT_RECIPIENT LESS 1 OR PEMSY_DEFAULT_RECIPIENT GREATER 255 OR
   PEMSY_GROUP LESS 1 OR PEMSY_GROUP GREATER 6)
    message(FATAL_ERROR "Invalid node/recipient/group setting")
endif()
if(PEMSY_NODE_ADDRESS EQUAL PEMSY_DEFAULT_RECIPIENT)
    message(FATAL_ERROR "Local and recipient node addresses must differ")
endif()
if(PEMSY_ENABLE_MIOTY AND NOT EXISTS "${PEMSY_MIOTY_CONFIG_HEADER}")
    message(FATAL_ERROR "Copy config/mioty_credentials.example.h to config/mioty_credentials.h and configure the registered endpoint")
endif()
if(PEMSY_ENABLE_MIOTY AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/TS-UNB-Lib-Pico/TsUnb/SimpleNode.h")
    message(FATAL_ERROR "Initialize the library: git submodule update --init")
endif()
math(EXPR PEMSY_RADIO_FREQUENCY_HZ "869800000 + (${PEMSY_GROUP} - 1) * 20000")
add_executable(pemsy src/main.c src/app.c src/lcd.c src/encoder.c src/temperature.c
    src/SPI.c src/radio.c src/mioty.cpp src/counter.c src/counter_flash.c)
# Index the local LCD exercise when present. It is intentionally not built with the firmware.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/lcd/lcd.c")
    add_library(lcd_exercise_for_editor OBJECT EXCLUDE_FROM_ALL examples/lcd/lcd.c)
    target_link_libraries(lcd_exercise_for_editor PRIVATE pico_stdlib)
endif()
target_include_directories(pemsy PRIVATE include)
target_include_directories(pemsy SYSTEM PRIVATE TS-UNB-Lib-Pico)
target_compile_options(pemsy PRIVATE -Wall -Wextra)
target_compile_definitions(pemsy PRIVATE
    PEMSY_NODE_ADDRESS=${PEMSY_NODE_ADDRESS}
    PEMSY_DEFAULT_RECIPIENT=${PEMSY_DEFAULT_RECIPIENT}
    PEMSY_RADIO_FREQUENCY_HZ=${PEMSY_RADIO_FREQUENCY_HZ}
    PEMSY_ENCODER_EDGES_PER_DETENT=${PEMSY_ENCODER_EDGES_PER_DETENT}
    PEMSY_RFM69_HIGH_POWER=$<BOOL:${PEMSY_RFM69_HIGH_POWER}>
    PEMSY_ENABLE_MIOTY=$<BOOL:${PEMSY_ENABLE_MIOTY}>
    PEMSY_MIOTY_CONFIG_HEADER="${PEMSY_MIOTY_CONFIG_HEADER}"
    PICO_FLASH_ASSERT_ON_UNSAFE=0)
target_link_libraries(pemsy PRIVATE pico_stdlib pico_multicore pico_flash
    hardware_gpio hardware_spi hardware_pwm hardware_adc hardware_clocks
    hardware_flash hardware_sync hardware_watchdog)
pico_enable_stdio_usb(pemsy 1)
pico_enable_stdio_uart(pemsy 0) # UART GPIO0/1 are occupied by the LCD.

# Reserve the last two sectors of the physical 2 MiB flash for counter records.
file(READ "${PICO_SDK_PATH}/src/rp2_common/pico_crt0/rp2040/memmap_default.ld" PEMSY_LINKER_SCRIPT)
string(REPLACE "INCLUDE \"pico_flash_region.ld\""
    "FLASH(rx) : ORIGIN = 0x10000000, LENGTH = 2040k" PEMSY_LINKER_SCRIPT "${PEMSY_LINKER_SCRIPT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/pemsy_memmap.ld" "${PEMSY_LINKER_SCRIPT}")
pico_set_linker_script(pemsy "${CMAKE_CURRENT_BINARY_DIR}/pemsy_memmap.ld")
pico_add_extra_outputs(pemsy)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_command(TARGET pemsy POST_BUILD
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/bin_to_uf2.py
        ${CMAKE_CURRENT_BINARY_DIR}/pemsy.bin ${CMAKE_CURRENT_BINARY_DIR}/pemsy.uf2
    VERBATIM)
message(STATUS "PEMSY node=${PEMSY_NODE_ADDRESS}, recipient=${PEMSY_DEFAULT_RECIPIENT}, frequency=${PEMSY_RADIO_FREQUENCY_HZ}, mioty=${PEMSY_ENABLE_MIOTY}")
