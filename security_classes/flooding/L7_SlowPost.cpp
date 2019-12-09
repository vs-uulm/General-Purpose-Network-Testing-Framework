//
// Created by philipp on 05.02.19.
//

#include <utility>

#include "L7_Flooding.h"

class SlowPost_External : public L7_flooding {
public:
    SlowPost_External(unsigned int id, const std::string &url, unsigned int numConnections, unsigned int dataDelay,
                      unsigned int testDuration, unsigned int startDelaySec, bool generateCSVReport,
                      const std::string &logPath) : L7_flooding(id, url, numConnections, dataDelay, testDuration,
                                                                startDelaySec, generateCSVReport, logPath) {}

private:
    void get_sht_arguments() override {
        inputArguments = {SLOWHTTPTEST_FILE,
                          SLOWPOST_FLAG,
                          NUMBER_CONNECTIONS_FLAG, std::to_string(numConnections),
                          DATA_DELAY_FLAG, std::to_string(dataDelay),
                          TEST_DURATION_FLAG, std::to_string(testDuration)};
        if (generateCSVReport) {
            inputArguments.push_back(GENERATE_CSV_FLAG);
        }
        inputArguments.push_back(URL_FLAG);
        inputArguments.push_back(url);
    }
};
