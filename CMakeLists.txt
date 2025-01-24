# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

set(PICO_BOARD CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(led_matrix C CXX ASM)

pico_sdk_init()

# Primeiro, adicione o executável
add_executable(led_matrix 
    src/led_matrix.c
)

# Depois, gere o header do arquivo PIO
pico_generate_pio_header(led_matrix ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio)

pico_set_program_name(led_matrix "led_matrix")
pico_set_program_version(led_matrix "0.1")

pico_enable_stdio_uart(led_matrix 0)
pico_enable_stdio_usb(led_matrix 1)

target_link_libraries(led_matrix
    pico_stdlib
    hardware_pio
    hardware_clocks
)

target_include_directories(led_matrix PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_BINARY_DIR}
)

pico_add_extra_outputs(led_matrix)
