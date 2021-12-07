cmake_minimum_required(VERSION 3.13)

include(pico-sdk/pico_sdk_init.cmake)
include(toolchains/gcc-arm-none-eabi_toolchain.cmake)

project(picos C CXX ASM)
set(CMAKE_C_STANDARD 11)

set(default_build_type "Debug")
set(PICO_NO_UF2 on)

pico_sdk_init()

# Find all project relevant files
file(GLOB SRC_FOLDER CONFIGURE_DEPENDS "src/*.h" "src/*.c" "src/*.s")
# Create our main executable
add_executable(main
    ${SRC_FOLDER}
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(main pico_stdlib hardware_irq hardware_timer pico_multicore)

# Add USB serial output
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)