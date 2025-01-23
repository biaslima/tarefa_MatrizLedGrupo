# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialize pico_sdk
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

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pio_matrix C CXX ASM)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable
add_executable(pio_matrix
    pio_matrix.c
)

pico_set_program_name(pio_matrix "pio_matrix")
pico_set_program_version(pio_matrix "0.1")

# Enable USB output
pico_enable_stdio_uart(pio_matrix 0)
pico_enable_stdio_usb(pio_matrix 1)

# Generate PIO header
pico_generate_pio_header(pio_matrix ${CMAKE_CURRENT_LIST_DIR}/pio_matrix.pio)

# Add libraries
target_link_libraries(pio_matrix PRIVATE
    pico_stdlib
    hardware_pio
    hardware_adc
    hardware_clocks
    pico_bootrom
)

# Include directories (at root)
target_include_directories(pio_matrix PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Add extra outputs
pico_add_extra_outputs(pio_matrix)

