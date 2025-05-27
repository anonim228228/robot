cmake_minimum_required(VERSION 3.10)
project(Robot)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Mosquitto
find_package(PkgConfig REQUIRED)
pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)
include_directories(${MOSQUITTO_INCLUDE_DIRS})

# Find wiringPi
find_library(WIRINGPI_LIB wiringPi)
if(NOT WIRINGPI_LIB)
    message(FATAL_ERROR "wiringPi library not found. Install it using: sudo apt-get install wiringpi or manual installation for 64-bit systems")
endif()

# Add executable
add_executable(robot robot.cpp)

# Link libraries
target_link_libraries(robot ${MOSQUITTO_LIBRARIES} ${WIRINGPI_LIB})