#include <iostream>

#include <string.h>
#include <unistd.h>
#include <chrono>
#include <thread>

#include "tether_agent.h"
#include <msgpack.hpp>
#include <psn_lib.hpp>
#include "UdpServerSocket.hpp"

// struct HelloData {
// 	std::string agentType;
//   std::string agentID;
// 	MSGPACK_DEFINE_MAP(agentType, agentID);
// };

const short           PORT         = 8888;
static const short    BUFLEN       = 1024;
static const uint32_t TIMEOUT_MSEC = 1000;

using namespace std; 

int main() {

  char buf[BUFLEN];

  const string agentType = "psn-bridge";
  const string agentID = "test-psn-bridge";

  // cout << "Tether PSN Agent starting..." << endl;

  // TetherAgent agent (agentType, agentID);

  // agent.connect("tcp", "127.0.0.1", 1883);

  // Output* outputPlug = agent.createOutput("hello");

  // HelloData h {
  //   agentType, agentID
  // };

  // // Make a buffer, pack data using messagepack...
  // std::stringstream buffer;
  // msgpack::pack(buffer, h);
  // const std::string& tmp = buffer.str();   
  // const char* payload = tmp.c_str();

  // outputPlug->publish(payload);

  // UDP server (for receiving) and PSN Decoder
  UdpServerSocket server(PORT, TIMEOUT_MSEC);
  ::psn::psn_decoder psn_decoder;
  uint8_t last_frame_id = 0 ;
  int skip_cout = 0 ;

    // Main loop
    while ( 1 ) 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1) );

        printf("Waiting for data...");
        fflush(stdout);

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

                        if ( tracker.is_speed_set() )
                            ::std::cout << "    speed: " << tracker.get_speed().x << ", " << 
                                                            tracker.get_speed().y << ", " <<
                                                            tracker.get_speed().z << std::endl ;

                        if ( tracker.is_ori_set() )
                            ::std::cout << "    ori: " << tracker.get_ori().x << ", " << 
                                                          tracker.get_ori().y << ", " <<
                                                          tracker.get_ori().z << std::endl ;

                        if ( tracker.is_status_set() )
                            ::std::cout << "    status: " << tracker.get_status() << std::endl ;

                        if ( tracker.is_accel_set() )
                            ::std::cout << "    accel: " << tracker.get_accel().x << ", " << 
                                                            tracker.get_accel().y << ", " <<
                                                            tracker.get_accel().z << std::endl ;

                        if ( tracker.is_target_pos_set() )
                            ::std::cout << "    target pos: " << tracker.get_target_pos().x << ", " << 
                                                                 tracker.get_target_pos().y << ", " <<
                                                                 tracker.get_target_pos().z << std::endl ;

                        if ( tracker.is_timestamp_set() )
                            ::std::cout << "    timestamp: " << tracker.get_timestamp() << std::endl ;
                    }

                    ::std::cout << "-----------------------------------------------" << ::std::endl ;
                //} // skip

        }

        else {
            printf("\n");
            // sleep(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(1) );
        }

    }

  // cout << "Disconnecting Tether Agent..." << endl;
  // agent.disconnect();

  return 0;
}