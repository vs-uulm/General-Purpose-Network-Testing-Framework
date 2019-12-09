//
// Created by philipp on 04.02.19.
//

#ifndef GPNTF_L7_FLOODING_H
#define GPNTF_L7_FLOODING_H


#include <string>
#include <vector>
#include <zconf.h>
#include <cstring>
#include <useful_functions.hpp>
#include <fcntl.h>
#include <wait.h>
#include <thread>
#include "utils.hpp"
#include "../reporting/Reporter.hpp"

#define SHT_PATH  "/usr/bin/slowhttptest";
#define SHT_FILE  "slowhttptest";
#define SHT_SLOWLORIS_FLAG "-H";
#define SHT_SLOWPOST_FLAG "-B";
#define SHT_URL_FLAG "-u";
#define SHT_NUMBER_CONNECTIONS_FLAG "-c";
#define SHT_DATA_DELAY_FLAG "-i";
#define SHT_TEST_DURATION_FLAG "-l";
#define SHT_GENERATE_CSV_FLAG "-g";


class L7_flooding {
protected:
    std::string SLOWHTTPTEST_PATH = SHT_PATH;
    std::string SLOWHTTPTEST_FILE = SHT_FILE;
    std::string SLOWLORIS_FLAG = SHT_SLOWLORIS_FLAG;
    std::string SLOWPOST_FLAG = SHT_SLOWPOST_FLAG;
    std::string URL_FLAG = SHT_URL_FLAG;
    std::string NUMBER_CONNECTIONS_FLAG = SHT_NUMBER_CONNECTIONS_FLAG;
    std::string DATA_DELAY_FLAG = SHT_DATA_DELAY_FLAG;
    std::string TEST_DURATION_FLAG = SHT_TEST_DURATION_FLAG;
    std::string GENERATE_CSV_FLAG = SHT_GENERATE_CSV_FLAG;

    std::vector<std::string> inputArguments;

    unsigned int id;
    unsigned int startDelaySec;
    std::string url;
    unsigned int numConnections;
    unsigned int dataDelay;
    unsigned int testDuration;
    bool generateCSVReport;
    std::string logPath;

    virtual void get_sht_arguments() {};


public:
    L7_flooding(unsigned int id,
                const std::string &url,
                unsigned int numConnections,
                unsigned int dataDelay,
                unsigned int testDuration,
                unsigned int startDelaySec,
                bool generateCSVReport,
                const std::string &logPath) : id(id), startDelaySec(
            startDelaySec), url(url), numConnections(numConnections), dataDelay(dataDelay), testDuration(testDuration),
                                             generateCSVReport(generateCSVReport),
                                             logPath(logPath) {}

    int start_session() {
        {
            get_sht_arguments();
            std::vector<char *> argVector = GPNTFUtil::toExecArgumentList(inputArguments);
            char **shtArgs = argVector.data();

            int wstatus;
            pid_t child = fork();

            if (child == -1) {
                print_status(id, "ERROR", "Forking failed: ", strerror(errno));
                return -1;
            }
            if (!child) {
                if (startDelaySec > 0) {
                    std::cout << "waiting for " << std::to_string(startDelaySec) << "sec" << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(startDelaySec));
                }
                std::cout << "executing...";
                int dev_null = open("/dev/null", O_WRONLY);
                dup2(dev_null, 1);
                dup2(dev_null, 2);
                chdir(logPath.c_str());
                execv(SLOWHTTPTEST_PATH.c_str(), shtArgs);
                print_status(id, "ERROR", "Could not set up process image: ", strerror(errno));
                return -1;
            } else {
                waitpid(child, &wstatus, 0);
                return 0;
            }
        }
    }

};


#endif //GPNTF_L7_FLOODING_H
