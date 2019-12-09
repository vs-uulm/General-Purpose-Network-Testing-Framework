/*
Copyright (C) 2019 Leonard Bradatsch
Contact: leonard.bradatsch@uni-ulm.de
*/

#ifndef L7_STORAGE_MARKETPLACE_HPP
#define L7_STORAGE_MARKETPLACE_HPP

#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

class StorageMarketplace_External
{
    public:
        StorageMarketplace_External(unsigned int _id, std::string _ip_dst, std::string _amount, std::string _flows) 
            : id{_id}, ip_dst{_ip_dst}, amount{_amount}, flows{_flows}
        {    
            #ifdef DEBUG
            std::cout << "L7 Storage/Marketplace External Constructor" << std::endl;
            #endif
        }

        ~StorageMarketplace_External()
        {
            #ifdef DEBUG
            std::cout << "L7 Storage/Marketplace External Deconstructor" << std::endl;
            #endif           
        }

        int
        start_session()
        {
            std::string path = "/usr/bin/iperf3";
            std::string file = "iperf3";
            std::string mode = "-c";
            std::string bytes = "-n";
            std::string parallel = "-P";

            int wstatus;
            pid_t child = fork();

            if(child == -1) {
                print_status(id, "ERROR", "Forking failed: ", strerror(errno));
                return -1;
            }
            if(!child) {
                int dev_null = open("/dev/null", O_WRONLY);
                dup2(dev_null, 1); dup2(dev_null, 2);
                execl(path.c_str(), file.c_str(), mode.c_str(), ip_dst.c_str(), bytes.c_str(), amount.c_str(), 
                    parallel.c_str(), flows.c_str(), NULL);
                print_status(id, "ERROR", "Could not set up process image: ", strerror(errno));
                return -1;
            } else {
                print_status(id, "INFO", "Storage/Marketplace starts processing task...");
                waitpid(child, &wstatus, 0);
                return 0;
            }
        }

    private:

        /* --- class parameters --- */
        unsigned int id;
        std::string ip_dst, amount, flows;
}; 

#endif
