#include <iostream>

#include "tether_agent.h"
#include <msgpack.hpp>



struct HelloData {
	std::string agentType;
  std::string agentID;
	MSGPACK_DEFINE_MAP(agentType, agentID);
};

using namespace std; 

int main() {

  const string agentType = "psn-bridge";
  const string agentID = "test-psn-bridge";

  cout << "Tether PSN Agent starting..." << endl;

  TetherAgent agent (agentType, agentID);

  agent.connect("tcp", "127.0.0.1", 1883);

  Output* outputPlug = agent.createOutput("hello");

  HelloData h {
    agentType, agentID
  };

  // Make a buffer, pack data using messagepack...
  std::stringstream buffer;
  msgpack::pack(buffer, h);
  const std::string& tmp = buffer.str();   
  const char* payload = tmp.c_str();

  outputPlug->publish(payload);

  agent.disconnect();

  return 0;
}