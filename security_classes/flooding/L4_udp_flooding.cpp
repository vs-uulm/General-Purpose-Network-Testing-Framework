#include <utility>
#include <fcntl.h>

#include "utils.hpp"

#include "flooding_helpers.hpp"


#include "../../misc/useful_functions.hpp"
#include "L4_Flooding.h"

/**
 * @author Philipp Spiegelt
 *
 * Handles flooding of a host using UDP.
 * Uses hping3, see: man hping3 [https://linux.die.net/man/8/hping3].
 *
 */
class UDP_Flooding_External : public L4_Flooding {
public:
    UDP_Flooding_External(unsigned int id, const std::string &ip_dst, const std::string &ip_ssrc, unsigned int port,
                          unsigned int intervalTime, unsigned int startDelaySec, unsigned int endDelaySec,
                          const std::string &logDir) : L4_Flooding(id, ip_dst, ip_ssrc, port, intervalTime,
                                                                   startDelaySec, endDelaySec, logDir) {
        flowType = "udpflood";
    }

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
