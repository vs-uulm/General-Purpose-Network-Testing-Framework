/*
Copyright (C) 2019 Leonard Bradatsch
Contact: leonard.bradatsch@uni-ulm.de
*/

#ifndef L7_WEB_BROWSING_HPP
#define L7_WEB_BROWSING_HPP

#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include "L7.hpp"

template <typename L4_object>
class Web_Browsing : public L7_object<L4_object> 
{
    public:
        Web_Browsing(std::vector<std::shared_ptr<L4_object>>& _object, bool _logging, std::string _path, unsigned int _id, 
            unsigned int _request_size, unsigned int _session_length, std::shared_ptr<Distribution_Function> _view_time_dist, 
            std::shared_ptr<Distribution_Function> _nof_inline_objects_dist)
            : L7_object<L4_object>{_object, _logging, _path}, initial_request{}, request{}, id{_id}, request_size{_request_size}, 
            session_length{_session_length}, generator{RandomEngine()},
            view_time_dist{_view_time_dist}, nof_inline_objects_dist{_nof_inline_objects_dist}
        {
            #ifdef DEBUG
            std::cout << "L7 Web Browsing Constructor" << std::endl;
            #endif

            initial_request.append(request_size, 'g');
            request.append(request_size, 'h');
        }

        ~Web_Browsing()
        {
            ;
        }

        Web_Browsing(Web_Browsing&& orig) = delete;
        Web_Browsing& operator= (Web_Browsing) = delete;
     
        void
        start_session() {
            std::ofstream output{};
            if(L7_object<L4_object>::logging) output = open_file();

            std::thread threads[L7_object<L4_object>::sockets.size()];
            for(unsigned int i = 0; i < session_length; i++) {
                int objects_per_session = round(nof_inline_objects_dist->operator()(generator));
                int objects_per_connection = objects_per_session / L7_object<L4_object>::sockets.size();
                if(objects_per_connection == 0) objects_per_connection = 1;

                /*
                for(unsigned int i = 0; i < L7_object<L4_object>::sockets.size(); ++i) {
                    L7_object<L4_object>::sockets[i]->setup_socket("","");
                }
                */

                /* --- Time Measurement Starts Here  --- */
                std::chrono::high_resolution_clock::time_point start;
                if(L7_object<L4_object>::logging) start = std::chrono::high_resolution_clock::now(); 

                for(unsigned int i = 0; i < L7_object<L4_object>::sockets.size(); ++i) {
                    threads[i] = std::thread(&Web_Browsing::run, this, i, objects_per_connection);
                }

                for(unsigned int i = 0; i < L7_object<L4_object>::sockets.size(); ++i) {
                    threads[i].join();
                }
                
                /*
                 for(unsigned int i = 0; i < L7_object<L4_object>::sockets.size(); ++i) {
                    L7_object<L4_object>::sockets[i]->_close();
                }
                */

                if(L7_object<L4_object>::logging) {
                    auto finish = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<long long, std::nano> elapsed = finish - start;
                    output << elapsed.count() << '\n';

                    #ifdef DEBUG
                    print_status(id, "INFO", "Flow duration", std::to_string(elapsed.count()));
                    #endif
                }
                /* --- Time Measurement Ends Here --- */ 

                wait_reading_time();
            }

            if(L7_object<L4_object>::logging) output.close();
        }

    private:
        void
        run(int connection, int nrolls)
        {
            int setup_error, attempt = 0;
            do {
                setup_error = L7_object<L4_object>::sockets[connection]->setup_socket();
                if(setup_error == -2) {
                    ++attempt;
                    std::cout << "> [" << id << "] " << "Could not assign requested address. Retrying... Attempt "
                        << attempt << std::endl;
                }
                poll(0,0,1);
            } while (setup_error == -2);
            if(setup_error < 0) {
                print_status(id, "ERROR", "critical error while setting up socket. Returning from run() ...");
                return;
            }

            if(!connection) {
                L7_object<L4_object>::sockets[connection]->_send(1, initial_request);
                if(response_handler(connection) != 1) {
                    std::cerr << "Error on receiving response" << std::endl;
                }
            }

            for(int i = 0; i < nrolls; ++i) {
                L7_object<L4_object>::sockets[connection]->_send(1, request);
                if(response_handler(connection) != 1) {
                    std::cerr << "Error on receiving response" << std::endl;
                }
            }

            L7_object<L4_object>::sockets[connection]->_close();
        }

        int
        response_handler(int connection)
        {
            char buf[BUFSIZ];
            int result, response_size;

            if((result = L7_object<L4_object>::sockets[connection]->_recv(buf, sizeof buf)) <= 0) {
                if(!result) {
                    std::cerr << "Remote side closed connection" << std::endl;
                } else {
                    std::cerr << "Could not extract object size" << std::endl;
                }
                return 0;    
            }

            response_size = atoi(buf);

            #ifdef DEBUG
            print_status(id, "INFO", "Response Size", std::to_string(response_size));
            #endif

            response_size-=result;

            while(response_size > 0) {
                result = L7_object<L4_object>::sockets[connection]->_recv(buf, sizeof buf);
                if(result == 0) return 0;
                if(result == -1) return -1;
                response_size-=result;
            }
            return 1;
        }

        void
        wait_reading_time()
        {
            unsigned int reading_time = round(1000*view_time_dist->operator()(generator));
            // if(reading_time > 60000) reading_time = 60000;
            if(reading_time < 0) reading_time = 0;
            // unsigned int reading_time = 10;
            poll(0,0,reading_time);
        }
        
        std::ofstream
        open_file()
        {
            std::string filename = L7_object<L4_object>::path + std::to_string(id) + "_page_loading_time.csv";
            return std::ofstream{filename, std::ofstream::trunc | std::ofstream::out};
        }

        std::string initial_request, request;
        unsigned id, request_size, session_length;
        std::default_random_engine generator;
        std::shared_ptr<Distribution_Function> view_time_dist;
        std::shared_ptr<Distribution_Function> nof_inline_objects_dist;
};

#endif
