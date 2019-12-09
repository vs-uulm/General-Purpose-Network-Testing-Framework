//
// Created by philipp on 01.02.19.
//

#include <string>
#include <vector>
#include <cstring>
#include <zconf.h>
#include <wait.h>
#include <fcntl.h>
#include <thread>
#include "stdio.h"
#include "utils.hpp"
#include "../../misc/useful_functions.hpp"
#include "../reporting/Reporter.hpp"

#ifndef GPNTF_FLOODING_H
#define GPNTF_FLOODING_H

class L4_Flooding {

protected:
    std::string HPING_PATH = "/usr/sbin/hping3";
    std::string HPING_FILE = "hping3";
    std::string SYN_FLAG = "-S";
    std::string UDP_FLAG = "-2";
    std::string FLOOD_FLAG = "--flood";
    std::string INTERVAL_FLAG = "-i";
    std::string MICROSECONDS_PREFIX = "u";
    std::string PORT_FLAG = "-p";
    std::string SPOOF_SENDER_FLAG = "-a";

    std::vector<std::string> inputArguments;

    unsigned int id;
    std::string ip_dst, ip_ssrc;
    unsigned int port;
    unsigned int intervalTime;
    unsigned int startDelaySec;
    unsigned int durationSec;
    std::string flowType;
    std::string logDir;

    /**
     * In the implementation, add all arguments for hping3 to the vector.
     */
    virtual void get_hping_arguments() {
        std::cerr << "entered default arguments!";
    };

public:

    L4_Flooding(unsigned int id,
                const std::string &ip_dst,
                const std::string &ip_ssrc,
                unsigned int port,
                unsigned int intervalTime,
                unsigned int startDelaySec = 0,
                unsigned int endDelaySec = 0,
                const std::string &logDir = "./") : id(id), ip_dst(ip_dst), ip_ssrc(ip_ssrc),
                                                    port(port), intervalTime(intervalTime),
                                                    startDelaySec(startDelaySec),
                                                    durationSec(endDelaySec),
                                                    logDir(logDir) {}

    int start_session() {
        {
            std::cout << "entered session start" << std::endl;
            get_hping_arguments();
            GPNTFUtil::printVector(inputArguments);
            std::cout << "hping args " << inputArguments.size() << std::endl;
            std::vector<char *> argVector = GPNTFUtil::toExecArgumentList(inputArguments);
            char **hpingArgs = argVector.data();
            Reporter rep{id, path{logDir}, flowType};

            int wstatus;
            pid_t child = fork();

            if (child == -1) {
                print_status(id, "ERROR", "Forking failed: ", strerror(errno));
                return -1;
            }
            if (!child) {
                if (startDelaySec > 0) {
                    std::cout << "waiting for " << std::to_string(startDelaySec) << "sec" << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(startDelaySec));
                }
                std::cout << "executing...";
                rep.logBeginFlow();
                int dev_null = open("/dev/null", O_WRONLY);
                dup2(dev_null, 1);
                dup2(dev_null, 2);
                execv(HPING_PATH.c_str(), hpingArgs);
                print_status(id, "ERROR", "Could not set up process image: ", strerror(errno));
                return -1;
            } else {
                if (durationSec > 0) {
                    std::cout << "waiting for " << std::to_string(startDelaySec + durationSec) << "sec" << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(startDelaySec + durationSec));

                    int childStatus;
                    if (waitpid(child, &childStatus, WNOHANG) == 0) {
                        kill(child, SIGTERM);
                    }
                }
                waitpid(child, &wstatus, 0);
                rep.logEndFlow();
                return 0;
            }
        }
    }
};


#endif //GPNTF_FLOODING_H
