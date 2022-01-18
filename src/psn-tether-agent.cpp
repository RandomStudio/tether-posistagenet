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
  MSGPACK_DEFINE_MAP(id, x, y);
};

struct ProcessedTrackedObject {
  int id;
  float x;
  float y;
  int64_t lastTimeTracked;
};

const short           DEFAULT_PSN_PORT  = 8888;
const std::string     DEFAULT_TETHER_HOST = "127.0.0.1";
const short           DEFAULT_TETHER_PORT = 1883;
static const short    BUFLEN       = 1024;
static const uint32_t TIMEOUT_MSEC = 1000;
const std::string DEFAULT_AGENT_ID = "test-psn-bridge";
const std::string DEFAULT_AGENT_TYPE = "psn-bridge";

bool PROCESS_OBJECTS = false;
const float DEFAULT_MERGE_RADIUS = 10;

using std::cout; using std::endl;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;

float getDistance (float x1, float y1, float x2, float y2) {
  return std::sqrtf( std::pow((x2 - x1),2) + std::pow((y2 - y1),2) );
}

int main(int argc, char *argv[]) {

  std::cout << "Tether PSN Agent starting..." << std::endl;

  char buf[BUFLEN];
  std::vector<ProcessedTrackedObject*> processedObjects;

  std::string agentId;
  std::string tetherHost;
  bool shouldProcess;
  float mergeRadius;

  //Args: AGENT_ID TETHER_HOST SHOULD_PROCESS ("true"/"false") MERGE_RADIUS
  if (argc > 1) { // argv[0] is name of program
    if (argc != 5) {
      cout << "WARNING: unexpected number of arguments: " << argc << endl;
    }
    agentId = argv[1];
    tetherHost = argv[2];
    shouldProcess = std::strcmp(argv[3], "true") == 0 ? true : false;
    mergeRadius = std::atof(argv[4]);
  } else {
    agentId = DEFAULT_AGENT_ID;
    tetherHost = DEFAULT_TETHER_HOST;
    shouldProcess = false;
    mergeRadius = DEFAULT_MERGE_RADIUS;
  }


  std::cout << "Connect type " << DEFAULT_AGENT_TYPE << " with agentId " << agentId << " to MQTT broker at " << tetherHost << ":" << DEFAULT_TETHER_PORT << std::endl;
  if (shouldProcess) {
    cout << "Will send processed tracked objects with merge radius = " << mergeRadius << endl;
  } else {
    cout << "Will NOT send processed tracked objects (disabled)" << endl;
  }
  TetherAgent agent (DEFAULT_AGENT_TYPE, agentId);

  agent.connect("tcp", tetherHost, DEFAULT_TETHER_PORT);

  Output* rawTrackedObjectsPlug = agent.createOutput("trackedObjects");
  Output* processedTrackedObjectsPlug = agent.createOutput("processedTrackedObjects");

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
                        };
          

                        // Make a buffer, pack data using messagepack, send via Tether Output Plug...
                        {
                          std::stringstream buffer;
                          msgpack::pack(buffer, t);
                          rawTrackedObjectsPlug->publish(buffer.str());
                        }

                        if (shouldProcess) {
                          int64_t timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  
                          if (processedObjects.size() == 0) {
                            cout << "Size 0; push new tracked object...";
                            // Zero objects being tracked, always add
                            ProcessedTrackedObject * obj = new ProcessedTrackedObject {
                              static_cast<int>(processedObjects.size()),
                              tracker.get_pos().x,
                              tracker.get_pos().y,
                              timestamp
                            };
                            processedObjects.push_back(obj);
                            cout << "OK" << endl;
                          } else {
                            // Check if this tracking point falls within radius of known
                            // processed tracked objects - if so, update them
                            ProcessedTrackedObject * match = nullptr;

                            for (auto obj: processedObjects) {
                              float distance = getDistance(t.x, t.y, obj->x, obj->y);
                              if (distance <= mergeRadius) {
                                cout << "Match found" << endl;
                                match = obj;
                              }
                            }
                            
                            if (match == nullptr) {
                              // No match, create new processedTrackedObject

                              cout << "No match for tracker, create new processed tracked object...";
                              ProcessedTrackedObject * obj = new ProcessedTrackedObject{
                                static_cast<int>(processedObjects.size()),
                                tracker.get_pos().x,
                                tracker.get_pos().y,
                                timestamp
                              };
                              processedObjects.push_back(obj);
                              match = obj;
                              cout << "OK" << endl;
                              cout << "New list size: " << processedObjects.size() << endl;
                            } else {
                              cout << "Had a match; update...";
                              match->lastTimeTracked = timestamp;
                              match->x = t.x;
                              match->y = t.y;
                              cout << "OK" << endl;
                            }
                            if (match != nullptr) {
                              // If we got a match (or created one),
                              // send it via Tether Output Plug

                              cout << "Send processed tracked object via Tether" << endl;

                              TrackedObject obj {
                                  match->id,
                                  match->x,
                                  match->y
                              };

                              {
                                std::stringstream buffer;
                                msgpack::pack(buffer, obj);
                                processedTrackedObjectsPlug->publish(buffer.str());
                              }
                            }
                          }
                        }

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