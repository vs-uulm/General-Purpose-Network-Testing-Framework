//
// Created by philipp on 01.02.19.
//

#include <algorithm>
#include <iostream>
#include <vector>
#include <sstream>

#include "utils.hpp"

namespace GPNTFUtil {
    std::vector<char *> toExecArgumentList(std::vector<std::string> &input) {
        std::vector<char *> result;

        // remember the nullptr terminator
        result.reserve(input.size() + 1);

        std::transform(
                begin(input),
                end(input),
                std::back_inserter(result),
                [](std::string &s) {
                    return s.data();
                });
        result.push_back(nullptr);
        return result;
    }

    void printVector(std::vector<std::string> &vector) {
        for (const auto &i: vector) {
            std::cout << i << " ";
        }
        std::cout << "\n";
    }

    int ipRangeToVector(std::string input, std::vector<std::string> &result) {
        std::string delimiter = ".";
        std::string rangeIndicator = "-";

        std::vector<std::string> parts;
        splitString(delimiter, input, parts);

        if (parts.size() < 4) {
            return -1;
        }

        std::vector<std::string> range;
        splitString(rangeIndicator, parts[3], range);
        if (range.size() != 2) {
            result.push_back(input);
            return 0;
        }

        int from = std::stoi(range[0]);
        int to = std::stoi(range[1]);
        if (from > to) {
            return -1;
        }

        while (from <= to) {
            std::stringstream finalIp;
            finalIp << parts[0] << "." << parts[1] << "." << parts[2] << "." << std::to_string(from);
            result.push_back(finalIp.str());
            from++;
        }
        return 0;
    }

    int splitString(std::string &delim, std::string &str, std::vector<std::string> &output) {
        std::size_t current, previous = 0;
        current = str.find(delim);
        while (current != std::string::npos) {
            output.push_back(str.substr(previous, current - previous));
            previous = current + 1;
            current = str.find(delim, previous);
        }
        output.push_back(str.substr(previous, current - previous));
        return 0;
    }
}
