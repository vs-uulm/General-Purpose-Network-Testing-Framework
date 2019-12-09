/*
Copyright (C) 2019 Leonard Bradatsch
Contact: leonard.bradatsch@uni-ulm.de
*/

#ifndef TCP_CLIENT_HPP
#define TCP_CLIENT_HPP

#include <arpa/inet.h>

#include <errno.h>
#include <string>
#include <netdb.h>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <iostream>
#include <thread>
#include <cstddef>
#include <limits.h>
#include "../misc/useful_functions.hpp"

class L4
{
    public:
        virtual ~L4() {};
        virtual int setup_socket() = 0;
        virtual int _send(unsigned int nmbr_of_packets, const std::string buf, bool buffered = false, unsigned int bufsiz = BUFSIZ) = 0;
        virtual int _recv(char* buf, size_t bufSize) = 0;
        virtual int _close() = 0;
};

class TCP : public L4
{
    public:
        /* Constructors */
        TCP(unsigned int _id, std::string _target_address, std::string _target_port) 
            : hints{0}, info{0}, id{_id}, address_assigned{0}, src_port{0}, socketfd{0}, 
            target_address{_target_address}, target_port{_target_port}
        {
            ;
        }

        ~TCP()
        {
            //freeaddrinfo(info);
            //close(socketfd);
        }

        TCP(const TCP& orig) : socketfd{0}, target_address{orig.target_address}, target_port{orig.target_port}
        {
            ;
        }

        TCP(TCP&& orig) = delete;
        TCP& operator= (TCP) = delete;
        
        /* Basic Methods */
        int 
        setup_socket()
        {   
            if(socketfd > 0) {
                close(socketfd);
             // freeaddrinfo(info); <-- Do i need this? i dont think so.
            }

            if(!address_assigned) {
                memset(&hints, 0, sizeof hints);
                hints.ai_family = AF_UNSPEC;
                hints.ai_socktype = SOCK_STREAM;

                if(getaddrinfo(target_address.c_str(), target_port.c_str(), &hints, &info) != 0) {
                    print_status(id, "ERROR", "Connection failure occured", strerror(errno)); 
                    freeaddrinfo(info);
                    return -1;
                }
                address_assigned = 1;
            }

            struct addrinfo *p;
            for(p = info; p != NULL; p = p->ai_next) {

                //sockaddr_in* iface_addr = (sockaddr_in*) p->ai_addr;
                //char tmp_addr [10];
                //unsigned long my_addr = iface_addr->sin_addr.s_addr;
                //std::cout << "IP: " << inet_ntoa(iface_addr->sin_addr) << std::endl;

                if((socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                    continue;
                }
                break;
            }
            if(p == NULL) {
                print_status(id, "ERROR", "Could not find suitable socket/filedescriptor"); 
                freeaddrinfo(info); address_assigned = 0;
                return -1;
            }

            int _reuseaddr = 1;
            struct timeval tv;
            tv.tv_sec = 2400;
            tv.tv_usec = 0;

            if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &_reuseaddr, sizeof _reuseaddr) < 0) {
                print_status(id, "ERROR", "Connection failure occured", strerror(errno)); 
                freeaddrinfo(info); address_assigned = 0;
                return -1;
            }
            if(setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof tv) < 0) {
                print_status(id, "ERROR", "Connection failure occured", strerror(errno)); 
                freeaddrinfo(info); address_assigned = 0;
                return -1;
            }

            // EXPERIMENTAL
            /*
            struct sockaddr_in serv_addr, sin;
            socklen_t len = sizeof(sin);
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(src_port);
            serv_addr.sin_addr.s_addr = INADDR_ANY;
            if(bind(socketfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
                freeaddrinfo(info); address_assigned = 0;
                return -1;
            }
            if (getsockname(socketfd, (struct sockaddr *)&sin, &len) < 0) {
                freeaddrinfo(info); address_assigned = 0;
                return -1;
            }
            src_port = ntohs(sin.sin_port);
            std::cout << "SRC PORT: " << src_port << std::endl;
            */
            // END EXPERIMENTAL

            if(connect(socketfd, info->ai_addr, info->ai_addrlen) < 0) {
                freeaddrinfo(info); address_assigned = 0;
                if(errno == EADDRNOTAVAIL) {
                    return -2;
                } else { 
                    print_status(id, "ERROR", "Connection failure occured", strerror(errno));
                    return -1;
                }
            }

            freeaddrinfo(info);
            return 0;
        }

        int 
        _send(unsigned int nmbr_of_packets, const std::string buf, bool buffered = false, unsigned int bufsiz = BUFSIZ)
        {
            if(!buffered) {
                nmbr_of_packets = flood_unbuffered(nmbr_of_packets, buf);
            } else {
                nmbr_of_packets = flood_buffered(bufsiz, nmbr_of_packets, buf);
            }

            return 0;
        }

        int
        _recv(char* buf, size_t bufSize)
        {
            int read = recv(socketfd, buf, bufSize, 0);
            // Remote side closed connection
            if(read == 0) return 0;
            // Receiving error occured
            if(read == -1) {
                print_status(id, "ERROR", "Failure on receiving server response", strerror(errno));
                return -1;
            }
            return read;
        }

        int
        _close()
        {   
            // EXPERIMENTAL
            shutdown(socketfd, SHUT_RDWR);
            struct linger lin;
            lin.l_onoff = 1;
            lin.l_linger = 0;
            setsockopt(socketfd, SOL_SOCKET, SO_LINGER, (char *) &lin, sizeof(lin));
            // END EXPERIMENTAL

            if(close(socketfd) < 0) {
                address_assigned = 0;
                return -1;
            }
    
            address_assigned = 0;
            socketfd = 0;
            return 0;
        }

        int 
        is_connected()
        {   
            unsigned char buf;
            int err = recv(socketfd,&buf,1,MSG_PEEK);
            return err == -1 ? -1 : 0;
        }

        /*
        void
        reset_socketfds()
        {
            size_t size = socketfds.size();
            socketfds.clear();
            socketfds.assign(size, 0);
            freeaddrinfo(info);
        }
        */
        /*
        void 
        print_socketfds() const
        {
            for(auto socketfd: socketfds)
                std::cout << socketfd << std::endl;
        }
        */

        /* Socket Settings Methods */
        /*
        int 
        set_rcvbuf_all(int rcvbuf)
        {
            for(auto socketfd: socketfds) {
                if(socketfd < 1) return -1;
                if(setsockopt(socketfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof rcvbuf) < 0) return -1;
            }

            return 0;
        }
        */

        /*
        int 
        set_sndbuf_all(int sndbuf)
        {
            for(auto socketfd: socketfds) {
                if(socketfd < 1) return -1;
                if(setsockopt(socketfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof sndbuf) < 0) return -1;
            }

            return 0;
        }
        */

        /* TCP Settings Methods */
        /*
        int 
        tcp_nodelay_all(int nodelay)
        {
            for(auto socketfd: socketfds) {
                if(socketfd < 1) return -1;
                if(setsockopt(socketfd, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof nodelay) < 0) return -1;
            }

            return 0;
        }
        */

        /*
        int 
        tcp_congestion_all(std::string congestion)
        {
            for(auto socketfd: socketfds) {
                if(socketfd < 1) return -1;
                if(setsockopt(socketfd, IPPROTO_TCP, TCP_CONGESTION, congestion.c_str(), congestion.size()) < 0) {
                    std::cout << "Could not set TCP CONGESTION ALGORITHM" << std::endl;
                    return -1;
                }
            }
             
            return 0;
        }
        void 
        get_tcp_congestion() const
        {
            char ca[128];
            socklen_t size = 128;
            for(auto socketfd: socketfds) {
                getsockopt(socketfd, IPPROTO_TCP, TCP_CONGESTION, ca, &size);
                std::cout << "[Socket:" << socketfd << "] " << ca << std::endl;
            }
        }
        */

    private:
        /* Private Functions */
        int 
        flood_unbuffered(unsigned int nmbr_of_packets, std::string buf)
        {
            // if(connect(socketfd, info->ai_addr, info->ai_addrlen) < 0) return -1; <-- For later use mb

            for(unsigned int i = 0; i < nmbr_of_packets; ++i) {
                if(send(socketfd, buf.c_str(), buf.size(), 0) < 0) {
                    std::cerr << strerror(errno) << std::endl;
                    return i;    
                }
            }

            return nmbr_of_packets;
        }

        int 
        flood_buffered(unsigned int bufsiz, unsigned int nmbr_of_packets, std::string content)
        {
            std::string buffer;
            buffer.reserve(bufsiz);
            if(buffer.capacity() < bufsiz) return -1;

            // if(connect(socketfd, info->ai_addr, info->ai_addrlen) < 0) return -1; <<- For alter use mb

            for(unsigned int i = 0; i < nmbr_of_packets; ++i) {
                write_to_buffer(buffer, content);
                if(buffer.size() == buffer.capacity()) flush_buffer(buffer);
            }
            if(buffer.size() != 0) flush_buffer(buffer);

            return 0;
        }

        int 
        write_to_buffer(std::string& buffer, std::string  content)
        {
            if(buffer.size() + content.size() > buffer.capacity()) {
                int rest;
                rest = buffer.capacity() - buffer.size();
                buffer.append(content, 0, rest);
                return rest;
            } else {
                buffer.append(content);
                return content.size();
            }
        }

        int 
        flush_buffer(std::string& buffer)
        {
            const char* obuf = buffer.c_str();
            ssize_t left = buffer.size();
            ssize_t written = 0;    
            while(left > 0) {
                ssize_t nbytes;
                do {
                    errno = 0;
                    nbytes = send(socketfd, obuf + written, left, 0);
                } while (nbytes < 0 && errno == EINTR);
                if (nbytes <= 0) return -1;
                left -= nbytes;
                written += nbytes;
            }
            buffer.clear();
            return written;
        }
            
        struct addrinfo hints;
        struct addrinfo *info;
        unsigned int id, address_assigned, src_port;
        int socketfd;
        std::string target_address, target_port;
};

// TODO: struct addrinfo* --> smart pointer!
class UDP : public L4
{
    public:
        /* --- Constructors --- */
        UDP(unsigned int _id, std::string _target_address, std::string _target_port) 
            : hints{0}, info{0}, id{_id}, socketfd{0}, target_address{_target_address}, target_port{_target_port}
        {
            #ifdef DEBUG
            std::cout << "> [" << id << "] DEBUG: UDP Constructor." << std::endl;
            #endif
        }

        ~UDP()
        {
            #ifdef DEBUG
            std::cout << "> [" << id << "] DEBUG: UDP Deconstructor." << std::endl;
            #endif

            freeaddrinfo(info);
        }

        UDP(const UDP& orig) = delete;
        UDP(UDP&& orig) = delete;
        UDP& operator= (UDP) = delete;

        int
        setup_socket()
        {
            if(socketfd > 0) {
                close(socketfd);
                freeaddrinfo(info);
            }

            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_DGRAM;

            if(getaddrinfo(target_address.c_str(), target_port.c_str(), &hints, &info) != 0) return -1;

            struct addrinfo *p;
            for(p = info; p != NULL; p = p->ai_next) {
                if((socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                    continue;
                }
                break;
            }
            if(p == NULL) return -1;

            int _reuseaddr = 1;
            if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &_reuseaddr, sizeof _reuseaddr) < 0) return -1; 

            if(connect(socketfd, p->ai_addr, p->ai_addrlen) < 0) return -1;
            
            return 0;
        }

        int
        _send(unsigned int nmbr_of_packets, const std::string buf, bool buffered = false, unsigned int bufsiz = BUFSIZ)
        {
            for(unsigned int i = 0; i < nmbr_of_packets; ++i) {
                if(write(socketfd, buf.c_str(), buf.size()) < 0) {
                    return i;
                }
                /*
                if(sendto(socketfd, buf.c_str(), buf.size(), 0, p->ai_addr, p->ai_addrlen) < 0) {
                    return i;
                }
                */
            }

            return nmbr_of_packets;
        }

        int
        _recv(char* buf, size_t bufSize)
        {
            struct sockaddr_storage their_addr;
            socklen_t addr_len = sizeof their_addr;
            
            int read = recvfrom(socketfd, buf, bufSize, 0, (struct sockaddr*) &their_addr, &addr_len);
            // Remote side closed connection
            if(read == 0) return 0;
            // Receiving error occured
            if(read == -1) {
                print_status(id, "ERROR", "Failure on receiving server response", strerror(errno));
                return -1;
            }
            return read;
        }

        int
        _close()
        {
            if(close(socketfd) < 0) return -1;
            return 0;
        }
    private:
        struct addrinfo hints;
        struct addrinfo *info;
        unsigned int id;
        int socketfd;
        std::string target_address, target_port;
};

#endif
