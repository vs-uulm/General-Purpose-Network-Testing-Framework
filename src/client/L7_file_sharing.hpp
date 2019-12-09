/*
Copyright (C) 2019 Leonard Bradatsch
Contact: leonard.bradatsch@uni-ulm.de
*/

#ifndef L7_FILE_SHARING_HPP
#define L7_FILE_SHARING_HPP

#define SERVER_ANSWER_PACKET_SIZE 1400

#include "L7.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

template <typename L4_object>
class File_Sharing : public L7_object<L4_object>
{
    public:
        File_Sharing(std::vector<std::shared_ptr<L4_object>>& object, bool _logging, std::string _path, unsigned int _id, 
            unsigned int _session_length, unsigned int _idle_time_ms, std::vector<std::string> _ips, std::vector<std::string> _ports)
            : L7_object<L4_object>{object, _logging, _path}, id{_id}, session_length{_session_length}, idle_time_ms{_idle_time_ms},
            generator{RandomEngine()}, request{}, ips{_ips}, ports{_ports}
        {
            #ifdef DEBUG
            print_status(id, "INFO", "L7 File Sharing Constructor");
            #endif

            request.append(66, 'r');
        }

        ~File_Sharing()
        {
            ;
        }

        File_Sharing(File_Sharing&& orig) = delete;
        File_Sharing& operator= (File_Sharing) = delete;

        void start_session()
        {
            std::thread threads[L7_object<L4_object>::sockets.size()];
            for(unsigned int i = 0; i < L7_object<L4_object>::sockets.size(); ++i) {
                threads[i] = std::thread(&File_Sharing::run, this, i, session_length);
            }

            for(unsigned int i = 0; i < L7_object<L4_object>::sockets.size(); ++i) {
                threads[i].join();
            }
        }
    
    private:
        void
        run(int connection, int nrolls)
        {   
            for(int i = 0; i < nrolls; ++i) {
                int error_setup, attempt = 0;
                /* --- Time Measurement Starts Here --- */
                std::chrono::high_resolution_clock::time_point start;
                if(L7_object<L4_object>::logging) start = std::chrono::high_resolution_clock::now();

                do {
                    error_setup = L7_object<L4_object>::sockets[connection]->setup_socket();
                    if(error_setup == -2) {
                        ++attempt;
                        print_status(id, "INFO", "Could not assign requested address. Retrying... Attempt", std::to_string(attempt));
                    }
                } while (error_setup == -2);
                if(error_setup < 0) {
                    print_status(id, "ERROR", "Could not set up socket", strerror(errno));
                    continue;
                }


                L7_object<L4_object>::sockets[connection]->_send(1, request);
                response_handler(connection);

                if(L7_object<L4_object>::sockets[connection]->_close() < 0) {
                    print_status(id, "ERROR", "Could not close socket correctly");
                }
                
                if(L7_object<L4_object>::logging) {
                    auto finish = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<long long, std::nano> elapsed = finish - start;

                    #ifdef DEBUG
                    print_status(id, "INFO", "Flow duration", std::to_string(elapsed.count()));
                    #endif
                }
                /* --- Time Measurement Ends Here  --- */
                
                poll(0,0,idle_time_ms); 
            }
        }

        int
        response_handler(int connection)
        {
            char buf[BUFSIZ];
            std::string small_pkt;
            small_pkt.append(66, 'a');
            int result, response_size;

            if((result = L7_object<L4_object>::sockets[connection]->_recv(buf, 66)) <= 0) {
                if(!result) {
                    print_status(id, "INFO", "Remote side closed connection");
                } else {
                    print_status(id, "ERROR", "Could not extract object size. Skipping Chunk...");
                }
                return 0;
            }

            response_size = atoi(buf);
            //std::cout << "CHUNK SIZE: " << response_size << std::endl;
            //std::cout << "RECVD: " << result << std::endl;

            #ifdef DEBUG
            print_status(id, "INFO", "Response Size", std::to_string(response_size));
            #endif

            response_size-=result;

            while(response_size > 0) {
                result = L7_object<L4_object>::sockets[connection]->_recv(buf, SERVER_ANSWER_PACKET_SIZE);
                //std::cout << "RECVD: " << result << std::endl;
                if(result == 0) { 
                    print_status(id, "INFO", "Remote side closed connection");
                    return 0;
                }
                if(result == -1) {
                    print_status(id, "ERROR", "Error on receiving server response");
                    return -1;
                }
                if(buf[0] == 'y' || buf[1] == 'y' || buf[2] == 'y') {
                    L7_object<L4_object>::sockets[connection]->_send(1, small_pkt);
                }
                response_size-=result;
//                std::cout << "BYTES LEFT: " << response_size << std::endl;
            }
//            std::cout << "DONE" << std::endl;
            return 1;
        }

        unsigned int id, session_length, idle_time_ms;
        // std::shared_ptr<Distribution_Function> idle_time_dist; <-- Maybe later
        std::default_random_engine generator;
        std::string request;
        std::vector<std::string> ips, ports;
};

#endif
