/*
Copyright (C) 2019 Leonard Bradatsch
Contact: leonard.bradatsch@uni-ulm.de
*/

#ifndef L7_VOD_HPP
#define L7_VOD_HPP

#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

class VoD_External
{
    public:
        VoD_External(unsigned int _id, std::string _server) : id{_id}, server{_server}
        {    
            #ifdef DEBUG
            std::cout << "L7 VoD External Constructor" << std::endl;
            #endif
        }

        ~VoD_External()
        {
            #ifdef DEBUG
            std::cout << "L7 VoD External Deconstructor" << std::endl;
            #endif           
        }

        int
        start_session()
        {
            std::string path = "/usr/bin/cvlc";
            std::string file = "cvlc";
            std::string cache = "--network-caching=0";
            std::string exit_after_play = "--play-and-exit";
            std::string output = "--sout";
            std::string dup = "#duplicate{dst=display{novideo,noaudio}}";

            int wstatus;
            pid_t child = fork();

            if(child == -1) {
                print_status(id, "ERROR", "Forking failed: ", strerror(errno));
                return -1;
            }
            if(!child) {
                int dev_null = open("/dev/null", O_WRONLY);
                dup2(dev_null, 1); dup2(dev_null, 2);
                execl(path.c_str(), file.c_str(), cache.c_str(), exit_after_play.c_str(), server.c_str(), output.c_str(), dup.c_str(), NULL);
                print_status(id, "ERROR", "Could not set up process image: ", strerror(errno));
                return -1;
            } else {
                waitpid(child, &wstatus, 0);
                return 0;
            }
        }

    private:

        /* --- class parameters --- */
        unsigned int id;
        std::string server;
}; 

#endif
