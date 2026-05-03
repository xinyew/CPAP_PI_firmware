cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(CPAP_PI_firmware)

target_include_directories(app PRIVATE src)

target_sources(app PRIVATE 
    src/main.c
    src/sensors/mux_init.c
    src/sensors/sensor_manager.c
    src/sensors/max30101_reader.c
    src/comm/comm_manager.c
)
