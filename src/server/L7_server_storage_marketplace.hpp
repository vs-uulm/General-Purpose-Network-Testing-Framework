/*
Copyright (C) 2019 Leonard Bradatsch
Contact: leonard.bradatsch@uni-ulm.de
*/

#ifndef L7_SERVER_STORAGE_MARKETPLACE_HPP
#define L7_SERVER_STORAGE_MARKETPLACE_HPP

#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

class Server_StorageMarketplace_External
{
    public:
        Server_StorageMarketplace_External(unsigned int _id)
            : id{_id}
        {    
            #ifdef DEBUG
            std::cout << "L7 Server Storage/Marketplace External Constructor" << std::endl;
            #endif
        }

        ~Server_StorageMarketplace_External()
        {
            #ifdef DEBUG
            std::cout << "L7 Server Storage/Marketplace External Deconstructor" << std::endl;
            #endif           
        }

        int
        start_session()
        {
            std::string path = "/usr/bin/iperf3";
            std::string file = "iperf3";
            std::string mode = "-s";
//            std::string end_after_one_connection = "-1";

            int wstatus;
            pid_t child = fork();

            if(child == -1) {
                print_status(id, "ERROR", "Forking failed: ", strerror(errno));
                return -1;
            }
            if(!child) {
                int dev_null = open("/dev/null", O_WRONLY);
                dup2(dev_null, 1); dup2(dev_null, 2);
                execl(path.c_str(), file.c_str(), mode.c_str(), NULL);
                print_status(id, "ERROR", "Could not set up process image: ", strerror(errno));
                return -1;
            } else {
                print_status(id, "INFO", "Server started and is listening...");
                waitpid(child, &wstatus, 0);
                return 0;
            }
        }

    private:

        /* --- class parameters --- */
        unsigned int id;
}; 

#endif
