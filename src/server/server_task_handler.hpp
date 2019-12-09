/*
Copyright (C) 2019 Leonard Bradatsch
Contact: leonard.bradatsch@uni-ulm.de
*/

#ifndef CLIENT_TASK_HANDLER_HPP
#define CLIENT_TASK_HANDLER_HPP

#define MODE_REGULAR "regular"
#define MODE_RAW "raw" // On server side as well?
#define MODE_ATTACK "attack" // On server side as well?
#define MODE_EXTERN "extern"

/* --- EXTERN mode parameters --- */


/* --- L4 PROTOCOLS --- */
#define LAYER4_TCP "tcp"
#define TCP_MODE "tcpMode"
#define LAYER4_UDP "udp"

/* --- L7 PROTOCOLS --- */
#define F1460B "f1460b"
#define WEB_BROWSING "webbrowsing"
#define FILE_SHARING "filesharing"
#define MARKETPLACE "marketplace"
#define STORAGE "storage"

/* --- L7 MODES --- */
#define LAYER7_F1460B_IGNORE "ignore"

#include "L4.hpp"
#include "L7_F1460B.hpp"
#include "L7_web_browsing.hpp"
#include "L7_file_sharing.hpp"
#include "L7_server_storage_marketplace.hpp"
#include "../misc/Distribution_Functions.hpp"
#include "../misc/useful_functions.hpp"

#include <unistd.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <stdlib.h>

template<class Key, class Value>
class ServerTaskHandler {
    public:
        ServerTaskHandler(unsigned int& _id, std::map<Key, Value> const& _parameters) : id{_id}, parameters{_parameters} {
            print_status(id, "INFO", "Server handler object has been created...");
        }

        void 
        start() 
        {
            /* --- Determines mode in which the frameworks starts
            PARAMETERS THE USER MUST GENERALLY DEFINE
                mode: defines the mode;
                sendInterface: defines the physical interface the framework uses for sending packets;
                layer7Protocol: deinfes the layer-7 protocol the framework starts in;
            --- */
            typename std::map<Key, Value>::const_iterator mode_it = parameters.find("mode");
            if(mode_it != parameters.end()) {
                if(mode_it->second == MODE_REGULAR) {
                    print_status(id, "INFO", "Server starts in NORMAL mode");
                    
                    if(start_regular() < 0) {
                        std::cerr << "> [" << id << "] ERROR: Exiting..." << std::endl;
                        exit(1);
                    }
                } else if(mode_it->second == MODE_RAW) {
                    print_status(id, "INFO", "Server starts in RAW mode");
                } else if(mode_it->second == MODE_ATTACK) {
                    print_status(id, "INFO", "Server starts in ATTACK mode");
                } else if(mode_it->second == MODE_EXTERN) {
                    print_status(id, "INFO", "Server starts in EXTERN mode", "");
                    if(start_extern() < 0) {
                        print_status(id, "ERROR", "Exiting...");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    print_status(id, "ERROR", "No mode could be recognized. Exiting...");
                    exit(1);
                }
            } else {
                print_status(id, "ERROR", "Could not find mode entry in parameter list. Exiting...");
                exit(1);
            }
            print_status(id, "INFO", "Flow successfully processed. Exiting...");
        }

    private:
        int 
        start_regular()
        {
            std::shared_ptr<L7> L7_object;
            if((L7_object = setup_regular_layer7()) == NULL) return -1;
            if(setup_regular_layer4(L7_object) < 0) return -1;
            return 0;
        }

        int
        start_extern()
        {
            std::string layer7protocol;
            find_and_return_parameter("layer7Protocol", layer7protocol);
            if(layer7protocol != "false") {
                if(layer7protocol == MARKETPLACE || layer7protocol == STORAGE) {
                    if(setup_extern_marketplace_storage() < 0) return -1;
                } else {
                    print_status(id, "ERROR", "layer7Protocol Prameter is not supported yet", layer7protocol);
                    return -1;
                }
            } else {
                print_status(id, "ERROR", "Could not find 'layer7Protocol' entry in parameter list", "");
                return -1;
            }
            return 0;        
        }

        int
        setup_extern_marketplace_storage()
        {
            /* EXAMPLE
            iperf3 -s

            PARAMETERS THE USER MUST DEFINE:
                -
            */
        
            print_status(id, "INFO", "Layer 7 Protocol 'Storage/Marketplace Server' detected");

            Server_StorageMarketplace_External sm_object{id};
            if(sm_object.start_session() < 0) return -1;

            return 0;
        }      

        int
        setup_regular_layer4(std::shared_ptr<L7> object)
        {
            typename std::map<Key, Value>::const_iterator layer4protocol_it = parameters.find("layer4Protocol");
            if(layer4protocol_it != parameters.end()) {
                /* --- TCP --- */
                if(layer4protocol_it->second == LAYER4_TCP) {
                    return setup_regular_tcp(object, layer4protocol_it->second);  
                }   
                /* --- UDP --- */
                else if(layer4protocol_it->second == LAYER4_UDP) {
                    return setup_regular_udp(object, layer4protocol_it->second);
                }
                /* --- DEFAULT --- */
                else {
                    std::cout << "> [" << id << "] WARNING: Layer 4 protocol " << layer4protocol_it->second  << ""
                        " not yet supported." << std::endl;
                    return -1;
                }
            } else {
                std::cerr << "> [" << id << "] ERROR: Could not find TEST 'layer4Protocol' entry in parameter list" << std::endl;
                return -1;
            }
        }

        int
        setup_regular_tcp(std::shared_ptr<L7> object, std::string L4_protocol)
        {
            print_status(id, "INFO", "Layer 4 Protocol 'TCP' detected.");
            print_status(id, "INFO", "Trying to set up regular TCP socket...");
                     
            typename std::map<Key, Value>::const_iterator ip_src_it = parameters.find("ip_src");
            if(ip_src_it == parameters.end()) {
                print_status(id, "ERROR", "Could not find 'ip_src' entry in parameter list");
                return -1;
            }
            typename std::map<Key, Value>::const_iterator tcp_src_it = parameters.find("tcp_src");
            if(tcp_src_it == parameters.end()) {
                print_status(id, "ERROR", "Could not find 'tcp_src' entry in parameter list.");
                return -1;
            }

            TCP<L7> socket{id, object, L4_protocol};
            if(socket.setup_socket(tcp_src_it->second, ip_src_it->second) < 0) {
                print_status(id, "ERROR", "Could not set up regular TCP socket.");
                return -1;
            }
            if(socket._listen() < 0) {
                print_status(id, "ERROR", "Listen system call failed.");
                return -1;
            }

            std::string tcp_mode;
            find_and_return_parameter("tcpMode", tcp_mode);
            if(tcp_mode == "fork") {
                if(socket.accept_fork() < 0) {
                    print_status(id, "ERROR", "Forking attempt failed.");
                    return -1;
                }
            } else if (tcp_mode == "forkLimited") {
                if(socket.accept_fork_limited(100) < 0) {
                    print_status(id, "ERROR", "Forking attempt failed.");
                    return -1;
                }
            } else if (tcp_mode == "prefork") {
                if(socket.accept_prefork(100) < 0) {
                    print_status(id, "ERROR", "Forking attempt failed.");
                    return -1;
                }
            } else {
                print_status(id, "INFO", "Could not find 'tcpMode' entry in parameter list. Continuing with default fork model...");
                    if(socket.accept_fork() < 0) {
                        print_status(id, "ERROR", "Forking attempt failed.");
                        return -1;
                    }
            }

            return 0;
        }

        int
        setup_regular_udp(std::shared_ptr<L7> object, std::string L4_protocol)
        {
            std::cout << "> [" << id << "] INFO: Layer 4 protocol 'UDP' detected." << std::endl;
            std::cout << "> [" << id << "] INFO: Trying to set up regular UDP socket..." << std::endl;
                     
            typename std::map<Key, Value>::const_iterator ip_src_it = parameters.find("ip_src");
            if(ip_src_it == parameters.end()) {
                std::cerr << "> [" << id << "] ERROR: Could not find 'ip_src' entry in parameter list." << std::endl;
                return -1;
            }
            typename std::map<Key, Value>::const_iterator udp_src_it = parameters.find("udp_src");
            if(udp_src_it == parameters.end()) {
                std::cerr << "> [" << id << "] ERROR: Could not find 'udp_src' entry in parameter list." << std::endl;
                return -1;
            }

            UDP<L7> socket{id, object, L4_protocol};
            if(socket.setup_socket(udp_src_it->second, ip_src_it->second) < 0) {
                std::cerr << "> [" << id << "] ERROR: Could not set up regular UDP socket." << std::endl;
                return -1;
            }

            unsigned int nmbr_of_processes = extract_nmbr_of_sth_with_default("udp_processes", sysconf(_SC_NPROCESSORS_ONLN));
            if(!nmbr_of_processes) return -1;

            if(socket.accept_fork(nmbr_of_processes) < 0) {
                std::cerr << "> [" << id << "] ERROR: Forking attempt failed." << std::endl;
                return -1;
            }            
            return 0;
        }

        std::shared_ptr<L7>
        setup_regular_layer7()
        {
            std::shared_ptr<L7> handler;
            typename std::map<Key, Value>::const_iterator layer7protocol_it = parameters.find("layer7Protocol");
            if(layer7protocol_it != parameters.end()) {
                /* --- F1460B --- */
                if(layer7protocol_it->second == F1460B) {
                    handler = setup_regular_f1460b();
                    return handler;   
                /* --- WEB BROWSING ---*/
                } else if (layer7protocol_it->second == WEB_BROWSING) {
                    handler = setup_regular_web_browsing();
                    return handler;
                /* --- FILE SHARING ---*/
                } else if (layer7protocol_it->second == FILE_SHARING) {
                    handler = setup_regular_file_sharing();
                    return handler;
                /* --- DEFAULT --- */
                } else {
                std::cout << "> [" << id << "] WARNING: Layer 7 protocol " << layer7protocol_it->second  << ""
                    " not yet supported." << std::endl;
                return 0;
                }
            } else {
                std::cerr << "> [" << id << "] ERROR: Could not find 'layer7Protocol' entry in parameter list" << std::endl;
                return 0;
            }
        }

        std::shared_ptr<L7>
        setup_regular_file_sharing()
        {
            std::cout << "> [" << id << "] INFO: Layer 7 mode 'FILE SHARING' detected." << std::endl;

            //mice_flow_dist{1.36, 0.81}, buffalo_flow_dist{0.005, 0.35}, elephant_flow_dist{400, 1.42}
            std::shared_ptr<Distribution_Function> mice_size_dist, buffalo_size_dist, elephant_size_dist;
            mice_size_dist = std::make_shared<Weibull_Distribution> (0.81, 1.36);
            buffalo_size_dist = std::make_shared<Pareto_Distribution> (0.005, 0.35);
            elephant_size_dist = std::make_shared<Pareto_Distribution> (400, 1.42);


            std::shared_ptr<L7> handler{new File_Sharing<Distribution_Function, Distribution_Function, Distribution_Function>{id, 
            mice_size_dist, buffalo_size_dist, elephant_size_dist}}; 
            return handler;
        }

        std::shared_ptr<L7>
        setup_regular_web_browsing()
        {   
            std::cout << "> [" << id << "] INFO: Layer 7 mode 'WEB BROWSING' detected." << std::endl;

            std::shared_ptr<Distribution_Function> main_object_size_dist;
            std::string main_object_size_parameter;
            find_and_return_parameter("MainObjectSizeDistribution", main_object_size_parameter);
            if(main_object_size_parameter != "false") {
                if(main_object_size_parameter == "Weibull") {
                    int shape, scale;
                    if(!(shape = extract_nmbr_of_sth("MainObjectSizeDistributionWeibullShape"))) {
                        return 0;
                    }
                    if(!(scale = extract_nmbr_of_sth("MainObjectSizeDistributionWeibullScale"))) {
                        return 0;
                    }
                    main_object_size_dist = std::make_shared<Weibull_Distribution> (shape, scale);
                    std::cout << "> [" << id << "] INFO: Weibull distribution with shape = " << shape 
                        << " and scale = " << scale << " as main object size distribution detected." << std::endl; 
                } else if(main_object_size_parameter == "Lognormal") {
                    int mean, std_deviation;
                    if(!(mean = extract_nmbr_of_sth("MainObjectSizeDistributionLognormalMean"))) {
                        return 0;    
                    }
                    if(!(std_deviation = extract_nmbr_of_sth("MainObjectSizeDistributionLognormalStdDeviation"))) {
                        return 0;
                    } 
                    main_object_size_dist = std::make_shared<Lognormal_Distribution> (mean, std_deviation);
                    std::cout << "> [" << id << "] INFO: Lognormal distribution with mean = " << mean 
                        << " and std deviation = " << std_deviation << " as main object size distribution detected." << std::endl;
                } else if(main_object_size_parameter == "Exponential") {
                    int lambda;
                    if(!(lambda = extract_nmbr_of_sth("MainObjectSizeDistributionExponentialLambda"))) {
                        return 0;    
                    }
                    main_object_size_dist = std::make_shared<Exponential_Distribution> (lambda);
                    std::cout << "> [" << id << "] INFO: Exponential distribution with lambda = " << lambda 
                        << " as main object size distribution detected." << std::endl;
                } else {
                    main_object_size_dist = std::make_shared<Weibull_Distribution> (0.814944, 28242.8);

                    std::cout << "> [" << id << "] WARNING: Main Object Size Distribution: " << main_object_size_parameter
                        << " not yet supported.  Using Default... Weibull Distribution(0.814944, 28242.8)" << std::endl;
                    return 0;
                }
            } else {
                main_object_size_dist = std::make_shared<Weibull_Distribution> (0.814944, 28242.8);
            
                std::cerr << "> [" << id << "] INFO: Could not find 'MainObjectSizeDistribution' entry in parameter list." << ""
                    " Using Default... Weibull Distribution(0.814944, 28242.8)" << std::endl;
            }

            std::shared_ptr<Distribution_Function> inline_object_size_dist;
            std::string inline_object_size_parameter;
            find_and_return_parameter("InlineObjectSizeDistribution", inline_object_size_parameter);
            if(inline_object_size_parameter != "false") {
                if(inline_object_size_parameter == "Weibull") {
                    int shape, scale;
                    if(!(shape = extract_nmbr_of_sth("InlineObjectSizeDistributionWeibullShape"))) {
                        return 0;    
                    }
                    if(!(scale = extract_nmbr_of_sth("InlineObjectSizeDistributionWeibullScale"))) {
                        return 0;
                    }
                    inline_object_size_dist = std::make_shared<Weibull_Distribution> (shape, scale);
                    std::cout << "> [" << id << "] INFO: Weibull distribution with shape = " << shape 
                        << " and scale = " << scale << " as inline object size distribution detected." << std::endl;
                } else if(inline_object_size_parameter == "Lognormal") {
                    int mean, std_deviation;
                    if(!(mean = extract_nmbr_of_sth("InlineObjectSizeDistributionLognormalMean"))) {
                        return 0;    
                    }
                    if(!(std_deviation = extract_nmbr_of_sth("InlineObjectSizeDistributionLognormalStdDeviation"))) {
                        return 0;
                    } 
                    inline_object_size_dist = std::make_shared<Lognormal_Distribution> (mean, std_deviation);
                    std::cout << "> [" << id << "] INFO: Lognormal distribution with mean = " << mean 
                        << " and std deviation = " << std_deviation << " as inline object size distribution detected." << std::endl;
                } else if(inline_object_size_parameter == "Exponential") {
                    int lambda;
                    if(!(lambda = extract_nmbr_of_sth("InlineObjectSizeDistributionExponentialLambda"))) {
                        return 0;    
                    }
                    main_object_size_dist = std::make_shared<Exponential_Distribution> (lambda);
                    std::cout << "> [" << id << "] INFO: Exponential distribution with lambda = " << lambda 
                        << " as inline object size distribution detected." << std::endl;
                } else {
                    inline_object_size_dist = std::make_shared<Lognormal_Distribution> (9.17979,1.24646);

                    std::cout << "> [" << id << "] WARNING: Inline Object Size Distribution: " << inline_object_size_parameter
                        << " not yet supported.  Using Default... Lognormal Distribution(9.17979,1.24646)" << std::endl;
                    return 0;
                }
            } else {
                inline_object_size_dist = std::make_shared<Lognormal_Distribution> (9.17979,1.24646);

                std::cerr << "> [" << id << "] INFO: Could not find 'InlineObjectSizeDistribution' entry in parameter list." << ""
                    " Using Default... Lognormal Distribution(9.17979,1.24646)" << std::endl;
            }

            std::shared_ptr<L7> handler{new Web_Browsing{main_object_size_dist, inline_object_size_dist}};
            return handler;
        }

        std::shared_ptr<L7>
        setup_regular_f1460b()
        {
            std::shared_ptr<L7> handler;
            typename std::map<Key, Value>::const_iterator f1460bmode_it = parameters.find("layer7Mode");
            if(f1460bmode_it != parameters.end()) {
                /* --- F1460B_IGNORE --- */
                if(f1460bmode_it->second == LAYER7_F1460B_IGNORE) {
                    handler = setup_regular_f1460b_ignore();
                    return handler;
                }
                /* --- DEFAULT --- */
                else {
                    std::cout << "> [" << id << "] WARNING: Layer 7 mode " << f1460bmode_it->second  << ""
                        " not yet supported." << std::endl;
                    return 0;
                }
            } else {
                std::cerr << "> [" << id << "] ERROR: Could not find 'layer7Mode' entry in parameter list" << std::endl;
                return 0;
            }
        }

        std::shared_ptr<F1460B_ignore>
        setup_regular_f1460b_ignore()
        {
            std::cout << "> [" << id << "] INFO: Layer 7 mode 'F1460B_IGNORE' detected." << std::endl;

            std::shared_ptr<F1460B_ignore> handler{new F1460B_ignore{}};
            return handler;
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

        template <typename defaultValue>
        unsigned int
        extract_nmbr_of_sth_with_default(std::string keyword, defaultValue value)
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
                std::cout << "> [" << id << "] INFO: Could not find " << keyword << " entry in parameter list."
                    << " Continuing with default value " << value << std::endl;
                return UINT_MAX;
            }
        }

        template<typename T>
        void synced_print(T t) const {
            write(1, t.c_str(), t.length());
        }

        template<typename T, typename... Args>
        void synced_print(T t, Args... args) const {
            write(1, t.c_str(), t.length());
            synced_print(args...);
        }

        unsigned int id;
        std::map<Key, Value> parameters;
};

#endif
