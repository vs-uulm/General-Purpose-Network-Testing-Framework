/*
Copyright (C) 2019 Leonard Bradatsch
Contact: leonard.bradatsch@uni-ulm.de
*/

#ifndef L7_HPP
#define L7_HPP

#include <poll.h>
#include <iostream>
#include <string>
#include <memory>
#include <chrono>
#include <fstream>
#include <array>
#include "../misc/Distribution_Functions.hpp"
#include "../misc/useful_functions.hpp"

/*
class TCP {
    public:
        TCP() 
        {
            std::cout << "TCP Constructor" << std::endl;
        }

        TCP(const TCP& orig)
        {
            std::cout << "TCP Copy Constructor" << std::endl;
        }

        TCP(TCP&& orig)
        {
            std::cout << "TCP Move Constructor" << std::endl;
        }

        ~TCP()
        {
            std::cout << "TCP Destructor" << std::endl;
        }

        TCP& operator=(TCP orig)
        {
            std::cout << "Move Assignment" << std::endl;
            return *this;
        }

        void
        send(std::string& content)
        {
            std::cout << content << std::endl;
        }

        void
        recv()
        {
            std::cout << "Recv Method" << std::endl;
        }
};
*/

template<typename L4_object>
class L7_object {
    public:
        L7_object(std::vector<std::shared_ptr<L4_object>>& _sockets, bool _logging, std::string _path)
            : sockets{_sockets}, logging{_logging}, path{_path}
        {
            #ifdef DEBUG
            std::cout << "L7 Constructor" << std::endl;
            #endif
        }
    protected:
        // std::shared_ptr<L4_object> socket;
        std::vector<std::shared_ptr<L4_object>>& sockets;
        bool logging;
        std::string path;
};

template<typename L4_object>
class F1460B_const : public L7_object<L4_object> {
    public:
        F1460B_const(std::vector<std::shared_ptr<L4_object>>& object, bool _logging, std::string _path, unsigned int _nmbr_of_packets)
            : L7_object<L4_object>{object, _logging, _path}, content{}, nmbr_of_packets{_nmbr_of_packets}
        {
            #ifdef DEBUG
            std::cout << "F1460B_const Constructor" << std::endl;
            #endif

            content.append(1460, 'a');
        }

        void
        start()
        {
            for(unsigned int i = 0; i < L7_object<L4_object>::sockets.size(); ++i) {
                L7_object<L4_object>::sockets[i]->setup_socket();

                L7_object<L4_object>::sockets[i]->_send(nmbr_of_packets, content);

                L7_object<L4_object>::sockets[i]->_close();;
            }
        }

    private:
        std::string content;
        unsigned int nmbr_of_packets;
};

#endif
