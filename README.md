## Compiling instructions
- `git submodule update --init --recursive`
- `mkdir build && cd build`
- `cmake ..`
- `cmake --build .`

Do NOT include `using namespace std` anywhere if you can help it! Because otherwise CppSockets will almost certainly break (see https://stackoverflow.com/questions/65102143/socket-bind-error-invalid-operands-to-binary-expression)