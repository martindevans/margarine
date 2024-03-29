cmake_minimum_required(VERSION 3.12)

set(PROJECT_NAME margarine)

set(PROJECT_SOURCES
    main.cpp

    render/dda.cpp
    render/hud.cpp
    render/planes.cpp
    render/sprite.cpp
    render/texture.cpp
    render/camera.cpp

    profiler/profiler.cpp

    multithreading/multithreading.cpp

    content/maps/test_map/test_map.cpp
)

# --- Here Be Boilerplate ---

# Pull in PICO SDK
include(pico_sdk_import.cmake)

# set up project
project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

# Find the PicoSystem library
include(picosystem_sdk_import.cmake)

# Create the output target
picosystem_executable(
  ${PROJECT_NAME}
  ${PROJECT_SOURCES}
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(${PROJECT_NAME}
    pico_multicore
)

# --- End Of Boilerplate ---

# Set your build options here

# disable_startup_logo(${PROJECT_NAME})  
# no_font(${PROJECT_NAME})         
no_spritesheet(${PROJECT_NAME})

target_compile_definitions(margarine PRIVATE
    # Disable Pico alarm functions to save some binary space (https://raspberrypi.github.io/pico-sdk-doxygen/group__alarm.html)
    PICO_TIME_DEFAULT_ALARM_POOL_DISABLED=0

    # Disable checking if this is running on simulated Pico hardware to save some binary space
    PICO_NO_FPGA_CHECK=1

    # Enable profiling
    ENABLE_PROFILER=1

    # Enable drawing of the depth buffer to the screen
    DEBUG_DRAW_DEPTH_BUFFER=0
)