/*
Copyright (C) 2019 Leonard Bradatsch
Contact: leonard.bradatsch@uni-ulm.de
*/

#ifndef CLIENT_TASK_HANDLER_HPP
#define CLIENT_TASK_HANDLER_HPP

#define MODE_REGULAR "regular"
#define MODE_RAW "raw"
#define MODE_ATTACK "attack"
#define MODE_EXTERN "extern"

/* --- General Parameters ---*/
#define NOF_CONNECTIONS "numberOfConnections"
#define DEFAULT_NOF_CONNECTIONS 1
#define SESSION_LENGTH "sessionLength"
#define DEFAULT_SESSION_LENGTH 10

/* --- EXTERN mode Parameters ---*/
#define VOD "VoD"
#define MARKETPLACE "marketplace"
#define STORAGE "storage"

/* --- L4 PROTOCOLS --- */
#define LAYER4_TCP "tcp"
#define LAYER4_UDP "udp"

/* --- L7 PROTOCOLS --- */
#define LAYER7_1460B "f1460b"

/* --- WEB BROWSING --- */
#define LAYER7_WEBBROWSING "webBrowsing"
#define WEB_BROWSING_DEFAULT_SESSION_LENGTH 10
#define WEB_BROWSING_DEFAULT_REQUEST_SIZE 350

/* --- FILE SHARING --- */
#define LAYER7_FILESHARING "filesharing"
#define IDLE_TIME "idleTime"
#define DEFAULT_IDLE_TIME 0

/* --- STORAGE/MARKETPLACE ---*/
#define OPMODE "opMode"
#define UPLOAD "upload"
#define DOWNLOAD "download"
#define ITERATIONS 1 // defines how often the given file is downloaded

/* --- L7 MODES --- */
#define LAYER7_1460B_CONST "const"

#include "L4.hpp"
// #include "L7.hpp" mother class; is included in child classes
#include "L7_web_browsing.hpp"
#include "L7_file_sharing.hpp"
#include "L7_vod.hpp"
#include "L7_storage_marketplace.hpp"
#include "../misc/Distribution_Functions.hpp"
#include "../misc/useful_functions.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

template<class Key, class Value>
class ClientTaskHandler {
    public:
        ClientTaskHandler(unsigned int& _id, std::map<Key, Value> const& _parameters) : id{_id}, parameters{_parameters} {
            print_status(id, "INFO", "Handler object has been created...", "");
        }

        void 
        start() 
        {
            // Determine mode            
            std::string mode;
            find_and_return_parameter("mode", mode);
            if(mode != "false") {
                if(mode == MODE_REGULAR) {
                    print_status(id, "INFO", "Client starts in NORMAL mode", ""); 
                    if(start_regular() < 0) {
                        print_status(id, "ERROR", "Exiting...", "");
                        exit(EXIT_FAILURE);
                    }
                } else if(mode == MODE_RAW) {
                    print_status(id, "INFO", "Client starts RAW mode", "");
                } else if(mode == MODE_ATTACK) {
                    print_status(id, "INFO", "Client starts in ATTACK mode", "");
                } else if(mode == MODE_EXTERN) {
                    print_status(id, "INFO", "Client starts in EXTERN mode", "");
                    if(start_extern() < 0) {
                        print_status(id, "ERROR", "Exiting...");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    print_status(id, "ERROR", "No mode could be recognized. Exiting...", "");
                    exit(EXIT_FAILURE);
                }
            } else {
                print_status(id, "ERROR", "Could not find mode entry in parameter list. Exiting...", "");
                exit(EXIT_FAILURE);
            }
            print_status(id, "INFO", "Flow sucessfully processed. Exiting...", "");
        }

    private:

        /* --- EXTERNAL MODULES START HERE --- */

        int
        start_extern()
        {
            std::string layer7protocol;
            find_and_return_parameter("layer7Protocol", layer7protocol);
            if(layer7protocol != "false") {
                if(layer7protocol == VOD) {
                    if(setup_extern_vod() < 0) return -1;
                } else if(layer7protocol == MARKETPLACE || layer7protocol == STORAGE) {
                    if(setup_extern_marketplace_storage() < 0) return -1;
                } else {
                    print_status(id, "ERROR", "layer7Protocol Prameter is not supported", layer7protocol);
                    return -1;
                }
            } else {
                print_status(id, "ERROR", "Could not find 'layer4Protocol' entry in parameter list");
                return -1;
            }
            return 0;
        }

        int
        setup_extern_vod()
        {
            /*
            PARAMETERS THE USER MUST DEFINE
                mode: supports extern or regular, depends on the implementation
                layer7Protocol: Vod here
                protocol: defines the video transmission protocol e.g. HTTP
                ip_dst: defines the server's ip address
                port_dst: defines the server's port address
                manifest: defines the video manifest

            EXEC PROCESS EXAMPLE
                cvlc -vvv --color http://192.168.88.8/BigBuckBunny_15s_onDemand_2014_05_09.mpd 
                    --sout '#duplicate{dst=display{novideo,noaudio}}'

            EXAMPLE
                [Flow0]
                mode: extern
                layer7Protocol: VoD
                protocol: http
                ip_dst: 192.168.1.1
                port_dst: 80
                manifest: TearsOfSteel_15s_simple_2014_05_09.mpd
            */

            print_status(id, "INFO", "Layer 7 Protocol 'Video On Demand' detected", "");

            std::string server;
            if(setup_vod_server(server) < 0) return -1;
            VoD_External vod_object{id, server};
            if(vod_object.start_session() < 0) return -1;

            return 0;
        }

        int
        setup_extern_marketplace_storage()
        {
            /* 
            PARAMETERS THE USER MUST DEFINE:
                mode: supports extern or regular, depends on the implementation 
                sendInterface: defines the physical NIC that is used for sending/receiving
                layer7Protocol: storage or marketplace here
                ip_dst: defines the server's ip address, 
                amount: defines the amount of data to send (-n), see man iperf3 for format info;
                flows: defines the amount of concurrent connections (-P), see man iperf3 for more info;

            EXEC PROCESS EXAMPLE
                iperf3 -c 192.168.1.1 -n 10G -P 10

            EXAMPLE
               [Flow0]
                mode: extern
                sendInterface: p4p1
                layer7Protocol: storage
                ip_dst: 192.168.1.1
                flows: 50
                amount: 250G 
            */
        
            print_status(id, "INFO", "Layer 7 Protocol 'Storage/Marketplace Upload' detected", "");
            
            std::string ip_dst;
            std::string amount;
            std::string flows;

            if(extern_marketplace_storage_collect_parameters(ip_dst, amount, flows) < 0) return -1;

            StorageMarketplace_External sm_object{id, ip_dst, amount, flows};
            if(sm_object.start_session() < 0) return -1;

            return 0;
        }

        int
        extern_marketplace_storage_collect_parameters(std::string& ip_dst, std::string& amount, std::string& flows)
        {
            find_and_return_parameter("ip_dst", ip_dst);
            if(ip_dst == "false") {
                print_status(id, "ERROR", "Could not fine 'ip_dst' entry in parameter list", "");   
                return -1;
            }

            find_and_return_parameter("amount", amount);
            if(amount == "false") {
                print_status(id, "ERROR", "Could not fine 'amount' entry in parameter list", "");   
                return -1;
            }

            find_and_return_parameter("flows", flows);
            if(flows == "false") {
                print_status(id, "ERROR", "Could not fine 'flows' entry in parameter list", "");   
                return -1;
            }

            return 0;
        }

        int
        setup_vod_server(std::string& server)
        {
            std::string protocol;
            find_and_return_parameter("protocol", protocol);
            std::string ip_dst;
            find_and_return_parameter("ip_dst", ip_dst);
            std::string port_dst;
            find_and_return_parameter("port_dst", port_dst);
            std::string manifest;
            find_and_return_parameter("manifest", manifest);

            std::string f = "false";
            int error = 0;
            if(protocol == f) {
                print_status(id, "ERROR", "Could not find 'protocol' entry in parameter list", "");
                error = 1;
            }
            if(ip_dst == f) {
                print_status(id, "ERROR", "Could not find 'ip_dst' entry in parameter list", "");
                error = 1;
            }
            if(port_dst == f) {
                print_status(id, "ERROR", "Could not find 'port_dst' entry in parameter list", "");
                error = 1;
            }
            if(manifest == f) {
                print_status(id, "ERROR", "Could not find 'manifest' entry in parameter list", "");
                error = 1;
            }

            if(error) return -1;

            server = protocol + "://" + ip_dst + ":" + port_dst + "/" + manifest;

            return 0;
        }

        /* --- EXTERNAL MODULES END HERE --- */

        /* --- REGULAR MODULES START HERE --- */

        int 
        start_regular()
        {
            std::vector<std::shared_ptr<L4>> sockets;
            if(setup_regular_layer4(sockets) < 0) return -1;
            if(setup_regular_layer7(sockets) < 0) return -1;
            return 0;
        }

            /* --- L4 METHODS --- */

        int
        setup_regular_layer4(std::vector<std::shared_ptr<L4>>& sockets)
        {   
            unsigned int nof_connections = extract_nmbr_of_sth_with_default(NOF_CONNECTIONS, DEFAULT_NOF_CONNECTIONS);
            if(!nof_connections) return -1;

            std::string layer4protocol;
            find_and_return_parameter("layer4Protocol", layer4protocol);
            if(layer4protocol != "false") {
                /* --- TCP --- */
                if(layer4protocol == LAYER4_TCP) {
                    for(unsigned int i = 0; i < nof_connections; ++i) {
                        if(setup_regular_tcp(sockets) < 0) return -1;
                    }
                }   
                /* --- UDP --- */
                else if(layer4protocol == LAYER4_UDP) {
                    for(unsigned int i = 0; i < nof_connections; ++i) {
                        if(setup_regular_udp(sockets) < 0) return -1;
                    }
                }
                /* --- DEFAULT --- */
                else {
                    print_status(id, "WARNING", "Given Layer 4 Protocol not yet supported", layer4protocol);
                    return -1;
                }
            } else {
                print_status(id, "ERROR", "Could not find 'layer4Protocol' entry in parameter list.");
                return -1;
            }
            return 0;
        }

        int
        setup_regular_tcp(std::vector<std::shared_ptr<L4>>& sockets)
        {
            print_status(id, "INFO", "Layer 4 protocol 'TCP' detected."); 
            print_status(id, "INFO", "Trying to set up regular TCP socket..."); 
                     
            std::string ip_dst;
            find_and_return_parameter("ip_dst", ip_dst);
            if(ip_dst == "false") {
                print_status(id, "ERROR", "Could not find 'ip_dst' entry in parameter list.");
                return -1;
            }

            std::string port_dst;
            find_and_return_parameter("port_dst", port_dst);
            if(port_dst == "false") {
                print_status(id, "ERROR", "Could not find 'port_dst' entry in parameter list.");
                return -1;
            }

            sockets.push_back(std::make_shared<TCP>(id, ip_dst, port_dst));

            return 0;
        }

        int
        setup_regular_udp(std::vector<std::shared_ptr<L4>>& sockets)
        {
            print_status(id, "INFO", "Layer 4 protocol 'UDP' detected.");
            print_status(id, "INFO", "Trying to set up regular UDP socket...");
            
            std::string ip_dst;
            find_and_return_parameter("ip_dst", ip_dst);
            if(ip_dst == "false") {
                print_status(id, "ERROR", "Could not find 'ip_dst' entry in parameter list.");
                return -1;
            }

            std::string port_dst;
            find_and_return_parameter("port_dst", port_dst);
            if(port_dst == "false") {
                print_status(id, "ERROR", "Could not find 'port_dst' entry in parameter list.");
                return -1;
            }           

            sockets.push_back(std::make_shared<UDP>(id, ip_dst, port_dst));
            /*
            if(sockets.back()->setup_socket() < 0) {
                std::cout << "> [" << id << "] ERROR: Could not set up regular UDP socket." << std::endl;
                sockets.pop_back();
                return -1;
            }
            */          
            return 0;
        }

            /* --- L4 METHODS END --- */

            /* --- L7 METHODS --- */

        int
        setup_regular_layer7(std::vector<std::shared_ptr<L4>>& sockets)
        {
            typename std::map<Key, Value>::const_iterator layer7protocol_it = parameters.find("layer7Protocol");
            if(layer7protocol_it != parameters.end()) {
                /* --- F1460B ---*/
                if(layer7protocol_it->second == LAYER7_1460B) {
                   return setup_regular_f1460b(sockets);
                } 
                /* --- Web Browsing ---*/
                else if (layer7protocol_it->second == LAYER7_WEBBROWSING) {
                    return setup_regular_webbrowsing(sockets);
                }
                /* --- File Sharing ---*/
                else if (layer7protocol_it->second == LAYER7_FILESHARING) {
                    return setup_regular_filesharing(sockets);
                }
                /* --- DEFAULT --- */
                else {
                    std::cout << "> [" << id << "] WARNING: Layer 7 protocol " << layer7protocol_it->second  << ""
                        " not yet supported." << std::endl;
                    return -1;
                }
            } else {
                std::cout << "> [" << id << "] ERROR: Could not find 'layer7Protocol' entry in parameter list" << std::endl;
                return -1;
            } 
            return 0;
        }

        int
        setup_regular_filesharing(std::vector<std::shared_ptr<L4>>& sockets)
        {

            /* 
            PARAMETERS THE USER MUST DEFINE:
                mode: supports extern or regular, depends on the implementation 
                sendInterface: defines the physical NIC that is used for sending/receiving
                layer7Protocol: filesharing here
                layer4Protocol: defines the l4 protocol to be used; either TCP or UDP
                ip_dst: defines the server's ip address, 
                port_dst: destination port
            PARAMETERS THE USER CAN DEFINE:
                idleTime: defines the idle time between two chunk downloads
                sessionLength: defines the number of chunks to be downloaded
                numberOfConnections: defines the number of concurrent connections per process 

            EXAMPLE
                [Flow0]
                mode: regular
                sendInterface: p4p1
                layer7Protocol: filesharing
                layer4Protocol: tcp
                numberOfConnections: 1
                sessionLength: 10
                idleTime: 5
                ip_dst: 192.168.1.1
                port_dst: 6881
            */

            std::cout << "> [" << id << "] INFO: Layer 7 protocol 'File Sharing' detected." << std::endl;

            std::string ip_dst, port_dst;
            find_and_return_parameter("ip_dst", ip_dst);
            find_and_return_parameter("tcp_dst", port_dst);

            std::vector<std::string> ips, ports;
            ips.push_back(ip_dst);
            ports.push_back(port_dst);

            bool logging = false;
            std::string path = "./";
            unsigned int session_length = extract_nmbr_of_sth_with_default("sessionLength", DEFAULT_SESSION_LENGTH);
            unsigned int idle_time_ms = extract_nmbr_of_sth_with_default("idleTime", DEFAULT_IDLE_TIME);
            if(!session_length) return -1;

            File_Sharing<L4> test{sockets, logging, path, id, session_length, idle_time_ms, ips, ports};
            test.start_session();
            return 0; 
        }

        int
        setup_regular_webbrowsing(std::vector<std::shared_ptr<L4>>& sockets)
        {
            std::cout << "> [" << id << "] INFO: Layer 7 protocol 'Web Browsing' detected." << std::endl;

            bool logging = false;
            std::string path = "./";  
            std::shared_ptr<Distribution_Function> view_time_dist{new Lognormal_Distribution{-0.495204, 2.7731}};
            std::shared_ptr<Distribution_Function> nof_inline_objects_dist{new Exponential_Distribution{1/31.9291}};
            unsigned int request_size = WEB_BROWSING_DEFAULT_REQUEST_SIZE;
            unsigned int session_length = extract_nmbr_of_sth_with_default("sessionLength", WEB_BROWSING_DEFAULT_SESSION_LENGTH);

            Web_Browsing<L4> test{sockets, logging, path, id, request_size, session_length, view_time_dist, nof_inline_objects_dist};
            test.start_session();
            return 0;
        }

        int
        setup_regular_f1460b(std::vector<std::shared_ptr<L4>>& sockets)
        {
            std::cout << "> [" << id << "] INFO: Layer 7 protocol 'F1460B' detected." << std::endl;

            typename std::map<Key, Value>::const_iterator layer7mode_it = parameters.find("layer7Mode");
            if(layer7mode_it != parameters.end()) {
                /* --- F1460B CONSTANT MODE --- */
                if(layer7mode_it->second == LAYER7_1460B_CONST) {
                    return setup_regular_f1460b_const(sockets);
                } 
                /* --- DEFAULT --- */
                else {
                    std::cout << "> [" << id << "] WARNING: Layer 7 mode " << layer7mode_it->second  << ""
                        " not yet supported." << std::endl;
                    return -1;
                }
            } else {
                std::cout << "> [" << id << "] ERROR: Could not find 'layer7Mode' entry in parameter list" << std::endl;
                return -1;
            }
        }

        int 
        setup_regular_f1460b_const(std::vector<std::shared_ptr<L4>> sockets)
        {
            std::cout << "> [" << id << "] INFO: Layer 7 mode 'F1460B_CONST' detected." << std::endl;

            std::string path = "./";
            unsigned int nmbr_of_packets = extract_nmbr_of_sth("packetsToSend");
            if(!nmbr_of_packets) return -1;

            F1460B_const<L4> test{sockets, true, path, nmbr_of_packets};
            test.start();
            return 0;
        }

            /* --- L7 METHODS END --- */

        /* --- REGULAR MODULES END HERE --- */

        /* --- GENERAL METHODS --- */

        unsigned int
        extract_nmbr_of_sth(std::string keyword)
        {
            typename std::map<Key, Value>::const_iterator nmbrOfSth_it = parameters.find(keyword);
            if(nmbrOfSth_it != parameters.end()) {
                if(strtod(nmbrOfSth_it->second.c_str(), NULL) <= 0) {
                    std::cerr << "> [" << id << "] ERROR: Value of " << keyword << " is zero or negative" << std::endl;
                }
                try {
                    unsigned int nmbr_of_sth = std::stoul(nmbrOfSth_it->second, nullptr, 0);
                    return nmbr_of_sth;
                }
                catch(const std::invalid_argument& ia) {
                    std::cerr << "> [" << id << "] ERROR: Value of " << keyword << " is not an Unsigned Integer" << std::endl;
                    return 0;
                }
                catch(const std::out_of_range& oor) {
                    std::cerr << "> [" << id << "] ERROR: Value of " << keyword << " is out of range" << std::endl;
                    return 0;
                }
            } else {
                std::cout << "> [" << id << "] ERROR: Could not find " << keyword << " entry in parameter list" << std::endl;
                return 0;
            }
        }

        /* ---  Returns 0 On Error; Else defaultValue Or The Actual Value:
                TODO: RV -1 maybe better? --- */
        unsigned int
        extract_nmbr_of_sth_with_default(std::string keyword, unsigned int defaultValue)
        {
            typename std::map<Key, Value>::const_iterator nmbrOfSth_it = parameters.find(keyword);
            if(nmbrOfSth_it != parameters.end()) {
                if(strtod(nmbrOfSth_it->second.c_str(), NULL) < 0) {
                    std::cerr << "> [" << id << "] ERROR: Value of " << keyword << " is zero or negative" << std::endl;
                    return 0;
                }
                try {
                    unsigned int nmbr_of_sth = std::stoul(nmbrOfSth_it->second, nullptr, 0);
                    return nmbr_of_sth;
                }
                catch(const std::invalid_argument& ia) {
                    std::cerr << "> [" << id << "] ERROR: Value of " << keyword << " is not an Unsigned Integer" << std::endl;
                    return 0;
                }
                catch(const std::out_of_range& oor) {
                    std::cerr << "> [" << id << "] ERROR: Value of " << keyword << " is out of range" << std::endl;
                    return 0;
                }
            } else {
                std::cout << "> [" << id << "] INFO: Could not find " << keyword << " entry in parameter list."
                    << " Continuing with default value..." << defaultValue << std::endl;
                return defaultValue;
            }
        }

        void
        find_and_return_parameter(std::string keyword, std::string& value)
        {
            typename std::map<Key, Value>::const_iterator it = parameters.find(keyword);

            if(it == parameters.end()) {
                value = "false";
            } else {
                value = it->second;
            }
        }

        /* --- CLASS PARAMETERS --- */

        unsigned int id;
        std::map<Key, Value> parameters;
};

#endif
