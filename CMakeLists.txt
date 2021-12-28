cmake_minimum_required(VERSION 3.10)
project(PSN_TetherAgent)

add_executable(psn-tether-agent src/psn-tether-agent.cpp)
target_include_directories(psn-tether-agent PUBLIC ./libs/tether/base_agent/cpp/src)