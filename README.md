## Compiling instructions


- `git submodule update --init --recursive`
- `mkdir build && cd build`
- `cmake ..`
- `cmake --build .`

For some reason, CppSockets compiles fine for psn-cpp itself but not in this project, with obscure compiler errors apparently related to clang vs gcc...

For now, comment out lines 36-38 in `libs/CppSockets/UdpServerSocket.hpp` before attempting to compile everything
