cmake_minimum_required(VERSION 3.19)

project(PSN_TetherAgent)

add_executable(psn-tether-agent src/psn-tether-agent.cpp)
set_property(TARGET psn-tether-agent PROPERTY CXX_STANDARD 11)

add_library(cppsockets INTERFACE)
set_property(TARGET cppsockets PROPERTY CXX_STANDARD 11)
target_include_directories(cppsockets INTERFACE ./libs/CppSockets)

add_subdirectory(./libs/tether/base_agent/cpp)
add_subdirectory(./libs/psn-cpp)

target_include_directories(psn-tether-agent PUBLIC ./libs/tether/base_agent/cpp/src)

target_link_libraries(psn-tether-agent PUBLIC TetherAgent msgpackc-cxx psnlib cppsockets)