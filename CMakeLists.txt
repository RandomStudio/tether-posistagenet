cmake_minimum_required(VERSION 3.19)

project(PSN_TetherAgent)

add_executable(psn-tether-agent src/psn-tether-agent.cpp)
set_property(TARGET psn-tether-agent PROPERTY CXX_STANDARD 17)

add_subdirectory(./libs/tether/base_agent/cpp)
add_subdirectory(./libs/psn-cpp)
add_subdirectory(./libs/kissnet)

target_include_directories(psn-tether-agent PUBLIC ./libs/tether/base_agent/cpp/src)
target_include_directories(psn-tether-agent PUBLIC ./libs/kissnet)

target_link_libraries(psn-tether-agent PUBLIC TetherAgent msgpackc-cxx psnlib kissnet)