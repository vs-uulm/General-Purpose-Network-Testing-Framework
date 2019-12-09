#ifndef GPNTF_L4_DISTRIBUTED_FLOODING_H
#define GPNTF_L4_DISTRIBUTED_FLOODING_H


class L4_Distributed_Flooding {

private:
    unsigned int id;
    std::vector<std::string> senders;
    std::string type;
    std::string ip_dst; // ip address of the victim to attack
    unsigned int port; // port to flood
    unsigned int intervalTime; // time to wait between packets in microseconds, set to zero for flooding
    unsigned int startDelaySec; // time to wait before launching the attack
    unsigned int durationSec;
    std::string logPath;

public:

    L4_Distributed_Flooding(unsigned int id, const std::vector<std::string> &senders, const std::string &type,
                            const std::string &ip_dst, unsigned int port, unsigned int intervalTime,
                            unsigned int startDelaySec, unsigned int durationSec, const std::string logPath);



    int start_sessions();

};


#endif //GPNTF_L4_DISTRIBUTED_FLOODING_H
