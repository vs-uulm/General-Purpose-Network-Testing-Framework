//
// Created by philipp on 11.02.19.
//

#ifndef GPNTF_L4_SYN_FLOODING_H
#define GPNTF_L4_SYN_FLOODING_H

#include "L4_Flooding.h"
#include "flooding_helpers.hpp"

class SYN_Flooding_External : public L4_Flooding {
public:

    SYN_Flooding_External(unsigned int id, const std::string &ip_dst, const std::string &ip_ssrc, unsigned int port,
                          unsigned int intervalTime, unsigned int startDelaySec, unsigned int endDelaySec,
                          const std::string &logDir) : L4_Flooding(id, ip_dst, ip_ssrc, port, intervalTime,
                                                                   startDelaySec, endDelaySec, logDir) {}

private:

    void get_hping_arguments() override {
        inputArguments = {HPING_FILE,
                          SYN_FLAG,
                          PORT_FLAG,
                          std::to_string(port)};

        GPNTFFloodingHelpers::addSenderToVector(ip_ssrc, inputArguments);
        GPNTFFloodingHelpers::addAttackVolumeToVector(intervalTime, inputArguments);
        inputArguments.push_back(ip_dst);
    };
};

#endif //GPNTF_L4_SYN_FLOODING_H
