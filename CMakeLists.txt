cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(CPAP_PI_firmware)

target_include_directories(app PRIVATE 
    src
    src/arduino_compat
    src/sensors/max30101/EmotiBit_MAX30101/src
)

target_sources(app PRIVATE 
    src/main.c
    src/arduino_compat/Wire.cpp
    src/sensors/mux_init.c
    src/sensors/sensor_manager.c
    src/sensors/max30101/max30101_reader.cpp
    src/sensors/max30101/EmotiBit_MAX30101/src/MAX30105.cpp
    src/comm/comm_manager.c
)

target_compile_definitions(app PRIVATE ARDUINO=100)
