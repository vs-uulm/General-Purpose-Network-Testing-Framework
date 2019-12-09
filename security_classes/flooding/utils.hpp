//
// Created by philipp on 14.01.19.
//
#include <algorithm>
#include <iostream>
#include "stdio.h"
#include "vector"

#ifndef GPNTF_UTILS_HPP
#define GPNTF_UTILS_HPP

namespace GPNTFUtil {
    /**
     * Turn vector of strings to vector of char*'s.
     */
    std::vector<char *> toExecArgumentList(std::vector<std::string> &input);

    /**
     * Pretty-print vector of strings.
     */
    void printVector(std::vector<std::string> &vector);

    int ipRangeToVector(std::string input, std::vector<std::string> &output);

    int splitString(std::string &delim, std::string &str, std::vector<std::string> &out);
}

#endif //GPNTF_UTILS_HPP


