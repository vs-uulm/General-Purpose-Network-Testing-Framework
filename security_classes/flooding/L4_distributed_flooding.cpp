//
// Created by philipp on 08.02.19.
//

#include <string>
#include <vector>
#include <zconf.h>
#include <cstring>
#include <iostream>
#include "L4_distributed_flooding.h"
#include "L4_Flooding.h"
#include "./L4_syn_flooding.h"
#include "./L4_udp_flooding.h"

L4_Distributed_Flooding::L4_Distributed_Flooding(unsigned int id,
                                                 const std::vector<std::string> &senders,
                                                 const std::string &type,
                                                 const std::string &ip_dst,
                                                 unsigned int port,
                                                 unsigned int intervalTime,
                                                 unsigned int startDelaySec,
                                                 unsigned int durationSec,
                                                 const std::string logPath) : id(id),
                                                                              senders(senders),
                                                                              type(type),
                                                                              ip_dst(ip_dst),
                                                                              port(port),
                                                                              intervalTime(intervalTime),
                                                                              startDelaySec(startDelaySec),
                                                                              durationSec(durationSec),
                                                                              logPath(logPath) {}

int L4_Distributed_Flooding::start_sessions() {
    for (const std::string &sender: senders) {
        int wstatus;
        pid_t child = fork();

        if (child == -1) {
            print_status(id, "ERROR", "Forking failed: ", strerror(errno));
            return -1;
        }
        if (child) {
            if (type == "synflood") {
                SYN_Flooding_External flooding = SYN_Flooding_External{id, ip_dst, sender, port, intervalTime,
                                                                       startDelaySec, durationSec, logPath};
                if (flooding.start_session() < 0) {
                    print_status(id, "ERROR", "Could not set up process image: ", strerror(errno));
                    return -1;
                }
                return 0;
            } else if (type == "udpflood") {
                UDP_Flooding_External flooding = UDP_Flooding_External{id, ip_dst, sender, port, intervalTime,
                                                                       startDelaySec, durationSec, logPath};
                if (flooding.start_session() < 0) {
                    print_status(id, "ERROR", "Could not set up process image: ", strerror(errno));
                    return -1;
                }
            }
        }
    }
    return 0;
}

