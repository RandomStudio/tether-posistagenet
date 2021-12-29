# PosiStageNet to Tether
Currently, listens for incoming PSN messages and, for each Tracker decoded, sends the ID and position (x, y, z) in a msgpack buffer via the CPP Tether Agent.

TODO:
- [ ] Provide easier configuration (e.g. via command-line args) for both PSN and Tether settings (e.g. IP addresses and ports)
- [ ] Include more of the PSN data (not just position)
- [ ] Should be able to work the other way, i.e. receive Tether messages and output PosiStageNet

## Compiling instructions
- `git submodule update --init --recursive`
- `mkdir build && cd build`
- `cmake ..`
- `cmake --build .`

Then run the binary `./psn_tether-agent`

Do NOT include `using namespace std` anywhere if you can help it! Because otherwise CppSockets will almost certainly break (see https://stackoverflow.com/questions/65102143/socket-bind-error-invalid-operands-to-binary-expression)