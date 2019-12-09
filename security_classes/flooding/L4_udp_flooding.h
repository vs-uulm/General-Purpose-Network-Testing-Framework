//
// Created by philipp on 11.02.19.
//

#ifndef GPNTF_L4_UDP_FLOODING_H
#define GPNTF_L4_UDP_FLOODING_H


class UDP_Flooding_External : public L4_Flooding {
public:
    UDP_Flooding_External(unsigned int id, const std::string &ip_dst, const std::string &ip_ssrc, unsigned int port,
                          unsigned int intervalTime, unsigned int startDelaySec, unsigned int endDelaySec,
                          const std::string &logDir) : L4_Flooding(id, ip_dst, ip_ssrc, port, intervalTime,
                                                                   startDelaySec, endDelaySec, logDir) {}

private:
    void get_hping_arguments() override {
        std::cout << "getting hping arguments";
        inputArguments = {HPING_FILE,
                          UDP_FLAG, PORT_FLAG,
                          std::to_string(port)};

        GPNTFFloodingHelpers::addSenderToVector(ip_ssrc, inputArguments);
        GPNTFFloodingHelpers::addAttackVolumeToVector(intervalTime, inputArguments);
        inputArguments.push_back(ip_dst);
    }


};

#endif //GPNTF_L4_UDP_FLOODING_H
