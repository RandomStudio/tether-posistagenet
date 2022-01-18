#include <psn_lib.hpp>
#include "UdpServerSocket.hpp"

#include <iostream>

#include <string.h>
#include <unistd.h>
#include <chrono>
#include <sys/time.h>
#include <thread>

#include "tether_agent.h"
#include <msgpack.hpp>

struct TrackedObject {
  int id;
  float x;
  float y;
  float z;
  MSGPACK_DEFINE_MAP(id, x, y, z);
};

struct ProcessedTrackedObject {
  int id;
  float x;
  float y;
  float z;
  unsigned long lastTimeTracked;
};

const short           DEFAULT_PSN_PORT  = 8888;
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

int main(int argc, char *argv[]) {

  std::cout << "Tether PSN Agent starting..." << std::endl;

  char buf[BUFLEN];

  std::string agentId;
  std::string tetherHost;

  //Args: AGENT_ID TETHER_HOST
  if (argc > 1) { // argv[0] is name of program
    agentId = argv[1];
    tetherHost = argv[2];
  } else {
    agentId = DEFAULT_AGENT_ID;
    tetherHost = DEFAULT_TETHER_HOST;
  }


  auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  cout << "ms since epoch: " << millisec_since_epoch << endl;

  std::cout << "Connect type " << DEFAULT_AGENT_TYPE << " with agentId " << agentId << " to MQTT broker at " << tetherHost << ":" << DEFAULT_TETHER_PORT << std::endl;
  TetherAgent agent (DEFAULT_AGENT_TYPE, agentId);

  agent.connect("tcp", tetherHost, DEFAULT_TETHER_PORT);

  Output* outputPlug = agent.createOutput("psn");

  // UDP server (for receiving) and PSN Decoder
  UdpServerSocket server(DEFAULT_PSN_PORT, TIMEOUT_MSEC);
  ::psn::psn_decoder psn_decoder;
  uint8_t last_frame_id = 0 ;
  int skip_cout = 0 ;

    // Main loop
    while ( 1 ) 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1) );

        // printf("Waiting for data...");
        // fflush(stdout);

        memset(buf, 0, BUFLEN);

        *buf = 0;

        server.receiveData(buf, BUFLEN);

        if (*buf) {
            printf("Data: %s\n", buf);
            // server.sendData(buf, strlen(buf));

            // std::cout << "As string: " << msg << " with size " << msg.size() << std::endl;
            
            psn_decoder.decode( buf , BUFLEN ) ;

            // if ( psn_decoder.get_data().header.frame_id != last_frame_id )
            // {
                last_frame_id = psn_decoder.get_data().header.frame_id ;

                const ::psn::tracker_map & recv_trackers = psn_decoder.get_data().trackers ;
                
                // if ( skip_cout++ % 20 == 0 )
                // {
                    ::std::cout << "--------------------PSN CLIENT-----------------" << ::std::endl ;
                    ::std::cout << "System Name: " << psn_decoder.get_info().system_name << ::std::endl ;
                    ::std::cout << "Frame Id: " << (int)last_frame_id << ::std::endl ;
                    ::std::cout << "Frame Timestamp: " << psn_decoder.get_data().header.timestamp_usec << ::std::endl ;
                    ::std::cout << "Tracker count: " << recv_trackers.size() << ::std::endl ;

                    for ( auto it = recv_trackers.begin() ; it != recv_trackers.end() ; ++it )
                    {
                        const ::psn::tracker & tracker = it->second ;

                        ::std::cout << "Tracker - id: " << tracker.get_id() << " | name: " << tracker.get_name() << ::std::endl ;

                        if ( tracker.is_pos_set() )
                            ::std::cout << "    pos: " << tracker.get_pos().x << ", " << 
                                                          tracker.get_pos().y << ", " <<
                                                          tracker.get_pos().z << std::endl ;

                        // double x = static_cast<double>(tracker.get_pos().x);

                        TrackedObject t {
                          tracker.get_id(),
                          tracker.get_pos().x,
                          tracker.get_pos().y,
                          tracker.get_pos().z
                        };
          

                        // Make a buffer, pack data using messagepack...
                        std::stringstream buffer;
                        msgpack::pack(buffer, t);

                        outputPlug->publish(buffer.str());

                        // if ( tracker.is_speed_set() )
                        //     ::std::cout << "    speed: " << tracker.get_speed().x << ", " << 
                        //                                     tracker.get_speed().y << ", " <<
                        //                                     tracker.get_speed().z << std::endl ;

                        // if ( tracker.is_ori_set() )
                        //     ::std::cout << "    ori: " << tracker.get_ori().x << ", " << 
                        //                                   tracker.get_ori().y << ", " <<
                        //                                   tracker.get_ori().z << std::endl ;

                        // if ( tracker.is_status_set() )
                        //     ::std::cout << "    status: " << tracker.get_status() << std::endl ;

                        // if ( tracker.is_accel_set() )
                        //     ::std::cout << "    accel: " << tracker.get_accel().x << ", " << 
                        //                                     tracker.get_accel().y << ", " <<
                        //                                     tracker.get_accel().z << std::endl ;

                        // if ( tracker.is_target_pos_set() )
                        //     ::std::cout << "    target pos: " << tracker.get_target_pos().x << ", " << 
                        //                                          tracker.get_target_pos().y << ", " <<
                        //                                          tracker.get_target_pos().z << std::endl ;

                        // if ( tracker.is_timestamp_set() )
                        //     ::std::cout << "    timestamp: " << tracker.get_timestamp() << std::endl ;
                    }

                    ::std::cout << "-----------------------------------------------" << ::std::endl ;
                //} // skip

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