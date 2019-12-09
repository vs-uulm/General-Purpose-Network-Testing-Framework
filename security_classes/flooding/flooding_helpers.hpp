//
// Created by philipp on 29.01.19.
//

#include <string>
#include <vector>
#include <sstream>
#include "hping3_data.h"

#ifndef FLOODING_HELPERS_HPP
#define FLOODING_HELPERS_HPP

namespace GPNTFFloodingHelpers {
    static void addSenderToVector(std::string &srcIP, std::vector<std::string> &commandVector);

    static void addAttackVolumeToVector(unsigned int &interval, std::vector<std::string> &commandVector);

/**
 * If srcIP is empty, do nothing. Else, add hping3 spoof sender flag and srcIP to the commandVector.
 */
    static void addSenderToVector(std::string &srcIP, std::vector<std::string> &commandVector) {
        if (srcIP.empty()) {
            return;
        }
        commandVector.push_back(HPING3_SPOOF_SENDER_FLAG);
        commandVector.push_back(srcIP);
    }

/**
 * If interval==0 add '--flood' to commandVector. Else add '-i', 'u$interval' to commandVector.
 */
    static void addAttackVolumeToVector(unsigned int &interval, std::vector<std::string> &commandVector) {
        if (interval == 0) {
            commandVector.push_back(HPING3_FLOOD_FLAG);
            return;
        }
        commandVector.push_back(HPING3_INTERVAL_FLAG);
        std::stringstream intervalWithPrefix;
        intervalWithPrefix << HPING3_MICROSECONDS_PREFIX << std::to_string(interval);
        commandVector.push_back(intervalWithPrefix.str());
    }
}
#endif