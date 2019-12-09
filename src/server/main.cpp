/*
Copyright (C) 2019 Leonard Bradatsch
Contact: leonard.bradatsch@uni-ulm.de
*/


#include "input_parser.hpp"
#include "server_task_handler.hpp"

#include <limits.h>

int main(int argc, char** argv) {

    if(argc != 2) {
        IParser::usage(argv[0]);
        exit(1);
    }
    
    std::string filename{argv[1]};
    std::list<std::map<std::string, std::string>> parameter_list{};
    IParser::identify_flows(filename, parameter_list);

    pid_t child;
    unsigned int id = 0;
    for(auto const& ele : parameter_list) {
        id++;
        child = fork();
        if(child == -1) {
            std::cout << strerror(errno) << std::endl;
            exit(1);
        }
        if(!child) {
            ServerTaskHandler<std::string, std::string> flow{id, ele};
            flow.start();
            exit(0);
        }
    }

    while(waitpid(-1, 0, 0) > 0);
    std::cout << "\n\r> All child processes terminated. We are done here!" << std::endl;
}
