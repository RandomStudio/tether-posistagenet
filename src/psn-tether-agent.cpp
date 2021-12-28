#include <iostream>

#include "tether_agent.h"
// #include <msgpack.hpp>


// struct helloData {
// 	std::string agentType;
//   std::string agentID;
// 	MSGPACK_DEFINE_MAP(agentType, agentID);
// };

using namespace std; 

int main() {

  const string agentType = "psn-bridge";
  const string agentID = "test-psn-bridge";

  cout << "Tether PSN Agent starting..." << endl;

  TetherAgent agent (agentType, agentID);

  return 0;
}