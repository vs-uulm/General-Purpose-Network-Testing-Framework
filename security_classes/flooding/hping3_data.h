//
// Created by philipp on 29.01.19.
//

#ifndef GPNTF_HPING3_DATA_H
#define GPNTF_HPING3_DATA_H


#include <string>

static const std::string HPING3_PATH = "/usr/sbin/hping3";
static const std::string HPING3_FILE = "hping3";
static const std::string HPING3_SYN_FLAG = "-S";

static const std::string HPING3_UDP_FLAG = "-2";

static const std::string HPING3_FLOOD_FLAG = "--flood";
static const std::string HPING3_INTERVAL_FLAG = "-i";
static const std::string HPING3_MICROSECONDS_PREFIX = "u";

static const std::string HPING3_PORT_FLAG = "-p";

static const std::string HPING3_SPOOF_SENDER_FLAG = "-a";
#endif //GPNTF_HPING3_DATA_H
