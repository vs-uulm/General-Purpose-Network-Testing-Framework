/*
Copyright (C) 2019 Leonard Bradatsch
Contact: leonard.bradatsch@uni-ulm.de
*/

#ifndef L7_FILE_SHARING_HPP
#define L7_FILE_SHARING_HPP

#include "L7.hpp"
#include <poll.h>

#include <fstream>

template <typename Mice_Flow_Dist, typename Buffalo_Flow_Dist, typename Elephant_Flow_Dist>
class File_Sharing : public L7
{
    public:
        File_Sharing(unsigned int _id, std::shared_ptr<Mice_Flow_Dist> _mice_flow_dist, 
            std::shared_ptr<Buffalo_Flow_Dist> _buffalo_flow_dist, std::shared_ptr<Elephant_Flow_Dist> _elephant_flow_dist)
            : id{_id}, lower_bound{0}, upper_bound{100}, generator{}, unif{lower_bound, upper_bound}, mice_flow_dist{_mice_flow_dist}, 
            buffalo_flow_dist{_buffalo_flow_dist}, elephant_flow_dist{_elephant_flow_dist}
        {
            //mice_flow_dist{1.36, 0.81}, buffalo_flow_dist{0.005, 0.35}, elephant_flow_dist{400, 1.42}
            ;
        }

        int
        handle(int confd, int (*f)(int, void*, const char*, size_t), int (*r)(int, void*, char*, size_t),
            void* send_details, std::string protocol, const char* request)
        {   
            static int initialized = 0;
            if(!initialized) {
                generator = RandomEngine();
                initialized = 1;
            }

            //generator = std::default_random_engine{0}; ????

            if(protocol == "udp") {
                if(request[0] == 'a' || request[1] == 'a' || request[2] == 'a') return 0;
            }
    
            std::string first_response_with_size;
            std::string answer_expected;
            std::string no_answer_expected;

            
            /* --- --- */
            std::ofstream myfile;
            myfile.open("luzi.csv", std::ios::app);

            unsigned int chunk_size = chunk_size_calculator();

            myfile << chunk_size << "\n";
            myfile.close();

//            unsigned int chunk_size = 459037372;
            unsigned int tmp_size = chunk_size;
            first_response_with_size.append(std::to_string(chunk_size));
            first_response_with_size.append(66 - first_response_with_size.size(), 'a');

//            std::cout << "SENT: " << first_response_with_size.size() << std::endl;
            if(f(confd, send_details, first_response_with_size.c_str(), first_response_with_size.size()) < 0) return -1;           

            if(chunk_size <= first_response_with_size.size()) return 0;
            chunk_size -= first_response_with_size.size();

            answer_expected.append(1400, 'y');
            no_answer_expected.append(1400, 'n');

            char testing_buffer[BUFSIZ]; 

            while(chunk_size > 0) {
                double answer_expected_or_not = unif(generator);
                int bytes_to_sent;
                if(chunk_size < 1400) {
                    bytes_to_sent = chunk_size;
                } else {
                    bytes_to_sent = 1400;
                }
                if(answer_expected_or_not < 37) {
//                    std::cout << "SENT: " << bytes_to_sent << std::endl;
                    if(f(confd, send_details, answer_expected.c_str(), bytes_to_sent) < 0) return -1;
                    if(protocol == "tcp") r(confd, send_details, testing_buffer, 66);
                } else {
//                    std::cout << "SENT: " << bytes_to_sent << std::endl;
                    if(f(confd, send_details, no_answer_expected.c_str(), bytes_to_sent) < 0) return -1;
                }
                chunk_size -= bytes_to_sent;
                //poll(0,0,0.5);
            }
    
//            std::cout << "= " << tmp_size << std::endl;

            /*
            while(chunk_size > 0) {
                double result = unif(generator);
                double answer_expected_or_not = unif(generator);
                int bytes_to_sent;
                if(result <= 80) {
                    if(chunk_size < 1460) {
                        bytes_to_sent = chunk_size;
                    } else {
                        bytes_to_sent = 1460;
                    }
i                    if(answer_expected_or_not < 37) {
                        if(f(confd, send_details, answer_expected.c_str(), bytes_to_sent) < 0) return -1;
                    } else {
                        if(f(confd, send_details, no_answer_expected.c_str(), bytes_to_sent) < 0) return -1;
                    }
                    chunk_size -= 1460;
                } else {
                    if(chunk_size < 66) {
                        bytes_to_sent = chunk_size;
                    } else {
                        bytes_to_sent = 66;
                    }
                    if(answer_expected_or_not < 37) {
                        if(f(confd, send_details, answer_expected.c_str(), bytes_to_sent) < 0) return -1;
                    } else {
                        if(f(confd, send_details, no_answer_expected.c_str(), bytes_to_sent) < 0) return -1;
                    }
                    chunk_size -= 66;             
                }
            }
            */

            return 0;
        }

        unsigned int
        chunk_size_calculator()
        {
            double result = unif(generator);
            if(result <= 92.93) {
                unsigned int flowSize = round(1000*mice_flow_dist->operator()(generator));
                std::cout << "MICE FLOW SIZE: " << flowSize << std::endl;
                return flowSize;
            } else if(result <= 99.19) {
                unsigned int chunk_size = 1000000*buffalo_flow_dist->operator()(generator);
                if(chunk_size < 4000) chunk_size = 4000;
                else if(chunk_size > 10000000) chunk_size = 10000000;
                std::cout << "BUFFALO FLOW SIZE: " << chunk_size << std::endl;
                return chunk_size;
            } else {
                unsigned int chunk_size = 1000000*elephant_flow_dist->operator()(generator);
                std::cout << "ELEPHANT FLOW SIZE: " << chunk_size << std::endl;
                return chunk_size;
            }

        }

    private:
        unsigned int id;
        double lower_bound, upper_bound;
        std::default_random_engine generator;
        std::uniform_real_distribution<double> unif;
        std::shared_ptr<Mice_Flow_Dist> mice_flow_dist;
        std::shared_ptr<Buffalo_Flow_Dist> buffalo_flow_dist; 
        std::shared_ptr<Elephant_Flow_Dist> elephant_flow_dist;
};

#endif
