#include <psn_lib.hpp>
#include <kissnet.hpp>

#include <iostream>

#include <string.h>
#include <unistd.h>
#include <chrono>
#include <sys/time.h>
#include <thread>

#include "tether_agent.h"
#include <msgpack.hpp>

struct Position {
  float x;
  float y;
  MSGPACK_DEFINE_MAP(x, y);
};
struct TrackedObject {
  int id;
  Position position;
  MSGPACK_DEFINE_MAP(id, position);
};

struct ProcessedTrackedObject {
  int id;
  float x;
  float y;
  int64_t lastTimeTracked;
};

const int           DEFAULT_PSN_PORT  = 56565;
const std::string     DEFAULT_PSN_MULTICAST_GROUP = "236.10.10.10";
const std::string     DEFAULT_TETHER_HOST = "127.0.0.1";
const short           DEFAULT_TETHER_PORT = 1883;
static const short    BUFLEN       = 1024;
static const uint32_t TIMEOUT_MSEC = 1000;
const std::string DEFAULT_AGENT_ID = "test-psn-bridge";
const std::string DEFAULT_AGENT_TYPE = "psn-bridge";

using std::cout; using std::endl;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;

float getDistance (float x1, float y1, float x2, float y2) {
  return std::sqrtf( std::pow((x2 - x1),2) + std::pow((y2 - y1),2) );
}

int main(int argc, char *argv[]) {

  std::cout << "Tether PSN Agent starting..." << std::endl;

  char char_buf[BUFLEN];
  std::vector<ProcessedTrackedObject*> processedObjects;

  std::string agentId;
  std::string tetherHost;
  std::string interfaceIp;
  bool ignoreZeroPoints;
  bool verbose;

  //Args: AGENT_ID TETHER_HOST INTERFACE_IP IGNORE_ZERO_POINTS ("true"/"false") VERBOSE ("true"/"false")
  if (argc > 1) { // argv[0] is name of program
    if (argc != 6) {
      cout << "ERROR: unexpected number of arguments: " << argc << endl;
      return -1;
    }
    agentId = argv[1];
    tetherHost = argv[2];
    interfaceIp = argv[3];
    ignoreZeroPoints = std::strcmp(argv[4], "true") == 0 ? true : false;
    verbose = std::strcmp(argv[5], "true") == 0 ? true : false;
  } else {
    agentId = DEFAULT_AGENT_ID;
    tetherHost = DEFAULT_TETHER_HOST;
    interfaceIp = "";
    ignoreZeroPoints = false;
    verbose = false;
  }


  std::cout << "Connect type " << DEFAULT_AGENT_TYPE << " with agentId " << agentId << " to MQTT broker at " << tetherHost << ":" << DEFAULT_TETHER_PORT << std::endl;
  if (ignoreZeroPoints) {
    cout << "ignoreZeroPoints == true; Will ignore points with [0,0,0] coordinates" << endl;
  } else {
    cout << "ignoreZeroPoints == false; Will NOT filter [0,0,0] points" << endl;
  }
  TetherAgent agent (DEFAULT_AGENT_TYPE, agentId);

  agent.connect("tcp", tetherHost, DEFAULT_TETHER_PORT);

  Output* rawTrackedObjectsPlug = agent.createOutput("trackedPoints");

  // UDP socket (for receiving) and PSN Decoder
  auto mcast_listen_socket = kissnet::udp_socket();
	mcast_listen_socket.join(kissnet::endpoint(DEFAULT_PSN_MULTICAST_GROUP, DEFAULT_PSN_PORT), interfaceIp);
  kissnet::buffer<1024> recv_buff;

  ::psn::psn_decoder psn_decoder;
  uint8_t last_frame_id = 0 ;
  int skip_cout = 0 ;

    // Main loop
    while ( 1 ) 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(16) );

        // printf("Waiting for data...");
        // fflush(stdout);

        memset(char_buf, 0, BUFLEN);

        *char_buf = 0;

        auto [received_bytes, status] = mcast_listen_socket.recv(recv_buff);
        const auto from = mcast_listen_socket.get_recv_endpoint();

        std::memcpy(char_buf, recv_buff.data(), received_bytes);

        if (*char_buf) {
          if (verbose) {
            printf("Data: %s\n", char_buf);
          }
            
          psn_decoder.decode( char_buf , BUFLEN ) ;

          last_frame_id = psn_decoder.get_data().header.frame_id ;

          const ::psn::tracker_map & recv_trackers = psn_decoder.get_data().trackers ;

          if (verbose) {
            ::std::cout << "--------------------PSN CLIENT-----------------" << ::std::endl ;
            ::std::cout << "System Name: " << psn_decoder.get_info().system_name << ::std::endl ;
            ::std::cout << "Frame Id: " << (int)last_frame_id << ::std::endl ;
            ::std::cout << "Frame Timestamp: " << psn_decoder.get_data().header.timestamp_usec << ::std::endl ;
            ::std::cout << "Tracker count: " << recv_trackers.size() << ::std::endl ;
          }
          
          for ( auto it = recv_trackers.begin() ; it != recv_trackers.end() ; ++it ) {
            const ::psn::tracker & tracker = it->second ;

            if (verbose) {
              ::std::cout << "Tracker - id: " << tracker.get_id() << " | name: " << tracker.get_name() << ::std::endl ;
            }


            if ( tracker.is_pos_set() ) {
              if (verbose) {
                ::std::cout << "    pos: " << tracker.get_pos().x << ", " << 
                                              tracker.get_pos().y << ", " <<
                                              tracker.get_pos().z << std::endl ;
              }

              if (ignoreZeroPoints && tracker.get_pos().x == 0 && tracker.get_pos().y == 0 && tracker.get_pos().z == 0) {
                // ignore all-zero tracking points!
              } else {
                Position p {
                  tracker.get_pos().x,
                  tracker.get_pos().y,
                };
                TrackedObject t {
                  tracker.get_id(),
                  p
                };          

                // Make a buffer, pack data using messagepack, send via Tether Output Plug...
                {
                  std::stringstream buffer;
                  msgpack::pack(buffer, t);
                  rawTrackedObjectsPlug->publish(buffer.str());
                }
              }
            }
                                      
          }

          if (verbose) {
            ::std::cout << "-----------------------------------------------" << ::std::endl ;
          }

        } else {
            // printf(" nothing received \n");
            // sleep(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(1) );
        }

    }

  std::cout << "Disconnecting Tether Agent..." << std::endl;
  agent.disconnect();

  return 0;
}