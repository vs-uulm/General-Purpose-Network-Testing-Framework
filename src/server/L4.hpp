/*
Copyright (C) 2019 Leonard Bradatsch
Contact: leonard.bradatsch@uni-ulm.de
*/

#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include <string>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <iostream>
#include <arpa/inet.h>
#include <signal.h>
#include <poll.h>
#include <cstddef>
#include <memory>
#include <limits.h>
#include <vector>

template<typename L7_object>
class L4
{
    public:
        L4(unsigned int _id, std::shared_ptr<L7_object> object, std::string _L4_protocol)
            : id{_id}, handler{object}, send_details{_L4_protocol}
        {
            #ifdef DEBUF
            std::cout << "> [" << id << "] DEBUG: L4 Constructor" << std::endl;
            #endif
        }

        struct Sending_Details {
            Sending_Details(std::string _L4_protocol) : L4_protocol{_L4_protocol}
            {
                ;
            }

            std::string L4_protocol;
            struct sockaddr_storage receiver_addr;
            socklen_t addr_len;
        };

    protected:
        unsigned int id;
        std::shared_ptr<L7_object> handler;

    public:
        Sending_Details send_details;
};

template<typename L7_object>
class TCP : public L4<L7_object>
{
    public:
        /* Constructor */
        TCP(unsigned int _id, std::shared_ptr<L7_object> object, std::string  _L4_protocol)
            : L4<L7_object>{_id, object, _L4_protocol}, socketfd{0}
        {
            #ifdef DEBUG
            std::cout << "> [" << L4<L7_object>::id << "] DEBUG: TCP Constructor" << std::endl;
            #endif
        }

        ~TCP() 
        {
            #ifdef DEBUG
            std::cout << "> [" << L4<L7_object>::id << "] DEBUG: TCP Deconstructor" << std::endl;
            #endif

            freeaddrinfo(info);
            close(socketfd);
        }

        TCP(const TCP& orig) = delete;
        TCP(TCP&& orig) = delete;
        TCP& operator= (TCP other) = delete;

        /* Basic Methods */
        int 
        setup_socket(std::string _server_port, std::string _server_address)
        {
            if(socketfd > 0) {
                close(socketfd);
                freeaddrinfo(info);
            }

            server_address = _server_address;
            server_port = _server_port;

            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;

            int _reuseaddr = 1;

            if(getaddrinfo(server_address.c_str(), server_port.c_str(), &hints, &info) != 0) return -1;

            for(p = info; p != NULL; p = p->ai_next) {
                if((socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                    continue;
                }
                break;
            }
            if(p == NULL) return -1;

            /* TODO: setter method? resonable? */
            if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &_reuseaddr, sizeof _reuseaddr) < 0) return -1;
            if(bind(socketfd, info->ai_addr, info->ai_addrlen) < 0) return -1;
                
            return 0;           
        }

        int 
        _listen(unsigned int queue_length = SOMAXCONN)
        {
            if(listen(socketfd, queue_length) < 0) return -1;

            std::cout << "> [" << L4<L7_object>::id << "] INFO: Server is listening on " << server_address << ""
                ":" << server_port << std::endl;

            return 0;
        }

        int 
        accept_one_connection()
        {
            struct sockaddr_storage inc_data;
            socklen_t addr_size = sizeof inc_data;
            int confd;

            if((confd = accept(socketfd, (struct sockaddr*) &inc_data, &addr_size)) < 0) return -1;

            char hostaddr[NI_MAXHOST];
            char hostport[NI_MAXSERV];
            getnameinfo((struct sockaddr*) &inc_data, addr_size, hostaddr, sizeof hostaddr, hostport, sizeof hostport, 
                NI_NUMERICHOST | NI_NUMERICSERV);
    
            std::cout << "> [" << L4<L7_object>::id << "] INFO: Connection accepted from " << hostaddr << ":" << hostport << std::endl;

            close(socketfd); // <<<<---------------------- ATTENTION !!!

            char buf[4096];
            while(1) {
                int result = recv(confd, buf, sizeof buf, 0);
                if(result == 0) {
                    std::cout << "> [" << L4<L7_object>::id << "] INFO: Connection " << hostaddr << ":" << hostport << " ended." 
                        " Remote host has closed connection." << std::endl;
                    return -1;
                }
                if(result == -1) {
                    std::cerr << "> [" << L4<L7_object>::id << "] INFO: Connection " << hostaddr << ":" << hostport << " ended."
                        " Receiving Error Occured." << std::endl;
                    return -1;
                }
                if(handle_request(confd, buf) < 0) {
                    std::cerr << "> [" << L4<L7_object>::id << "] ERROR: Connection " << hostaddr << ":" << hostport << " is going to close."
                        " Sending error occured." << std::endl;
                    break;
                }
            }
            close(confd);
            freeaddrinfo(info);
            return 0;
        }

        int 
        accept_fork(bool buffered = false, unsigned int bufsiz = BUFSIZ)
        {
            struct sockaddr_storage inc_data;
            socklen_t addr_size = sizeof inc_data;
            
            struct sigaction action = {0};
            action.sa_handler = SIG_IGN;
            action.sa_flags = SA_NOCLDWAIT;  
            if(sigaction(SIGCHLD, &action, 0) < 0) return -1;

            int confd;
            int connection_counter{};
            while((confd = accept(socketfd, (struct sockaddr*) &inc_data, &addr_size)) >= 0) {
                ++connection_counter;
                std::cout << "CONNECTION NMBR: " << connection_counter << std::endl;

                pid_t pid = fork();
                if(pid == 0) {
                    close(socketfd);
                    connection_handler(confd, inc_data, buffered, bufsiz);
                    exit(0);
                }
                close(confd);
            }
            return 0;
        }

        int 
        accept_fork_limited(unsigned int max_connections = 90, bool buffered = false, unsigned bufsiz = BUFSIZ)
        {
            unsigned int nof_connections = 0;
            struct sockaddr_storage inc_data;
            socklen_t addr_size = sizeof inc_data;
            
            struct sigaction action = {0};
            action.sa_handler = SIG_IGN;
            action.sa_flags = SA_NOCLDWAIT;  
            if(sigaction(SIGCHLD, &action, 0) < 0) return -1;

            int confd;
            while(((confd = accept(socketfd, (struct sockaddr*) &inc_data, &addr_size)) >= 0) && nof_connections < max_connections) {

                pid_t pid = fork();
                if(pid == 0) {
                    close(socketfd);
                    connection_handler(confd, inc_data, buffered, bufsiz);
                    exit(0);
                }
                nof_connections++;
                close(confd);
            }
            return 0;    
        }

        int 
        accept_prefork(int number_of_processes, bool buffered = false, unsigned int bufsiz = BUFSIZ)
        {
            
            if(number_of_processes < 1) return -1;

            struct sigaction action = {0};
            action.sa_handler = SIG_IGN;
            action.sa_flags = SA_NOCLDWAIT;
            if(sigaction(SIGCHLD, &action, 0) < 0) return -1;

            pid_t child_pid[number_of_processes];
            struct pollfd pollfds[number_of_processes];
            for(int i = 0; i < number_of_processes; ++i) {
                int pipefds[2];
                pid_t pid = spawn_preforked_process(pipefds, buffered, bufsiz);
                pollfds[i] = (struct pollfd) {0};
                pollfds[i].fd = pipefds[0];
                pollfds[i].events = POLLIN;
                if(pid < 0) return -1;
                child_pid[i] = pid;
            }

            bool terminate = 0;
            while(!terminate) {
                if(poll(pollfds, number_of_processes, -1) <= 0) break;
                for(int i = 0; i < number_of_processes; ++i) {
                    if(pollfds[i].revents == 0) continue;
                    close(pollfds[i].fd);
                    int pipefds[2];
                    pid_t pid = spawn_preforked_process(pipefds, buffered, bufsiz);
                    if(pid < 0) return -1;
                    pollfds[i] = (struct pollfd) {0};
                    pollfds[i].fd = pipefds[0];
                    pollfds[i].events = POLLIN;
                    child_pid[i] = pid;
                }
            }

            for(int i = 0; i < number_of_processes; ++i) {
                if(child_pid[i] > 0)
                    kill(child_pid[i], SIGTERM);
            }

            return 0;    
        }

    private:
        /* Private Methods */
        typedef struct inbuf {
            int confd;
            std::string buf;
        } inbuf;

        pid_t 
        spawn_preforked_process(int pipefds[2], bool buffered, unsigned int bufsiz)
        {
            struct sockaddr_storage inc_data;
            socklen_t addr_size = sizeof inc_data;

            if(pipe(pipefds) < 0) return -1;
            pid_t child = fork();
            if(child) {
                close(pipefds[1]);
                return child;
            }
            close(pipefds[0]);
            
            int confd = accept(socketfd, (struct sockaddr*) &inc_data, &addr_size);
            close(socketfd);
            if(confd < 0) exit(1);
            /*  Signal the parent process that the child process has accpeted a new connection
                and is now busy processing the packets */
            close(pipefds[1]);

            connection_handler(confd, inc_data, buffered, bufsiz);
            
            exit(0);
        }

        void 
        connection_handler(int confd, struct sockaddr_storage inc_data, bool buffered, unsigned int bufsiz)
        {
            if(!buffered) {
                handle_unbuffered(confd, inc_data);       
            } else {
                std::shared_ptr<inbuf> inbuf_ptr = std::make_shared<inbuf>();
                inbuf_ptr->confd = confd;
                inbuf_ptr->buf.reserve(bufsiz);
                handle_buffered(inc_data, inbuf_ptr);
            }
        }

        void 
        handle_unbuffered(int confd, struct sockaddr_storage inc_data)
        {
            char buf[BUFSIZ];

            char hostaddr[NI_MAXHOST];
            char hostport[NI_MAXSERV];
            getnameinfo((struct sockaddr*) &inc_data, sizeof inc_data, hostaddr, sizeof hostaddr, hostport, sizeof hostport,
                NI_NUMERICHOST | NI_NUMERICSERV);
            std::cout << "> [" << L4<L7_object>::id << "] INFO: Connection accepted from " << hostaddr << ":" << hostport << std::endl;

            for(;;) {
                int result = recv(confd, buf, sizeof buf, 0);
                if(result == 0) {
                    std::cout << "> [" << L4<L7_object>::id << "] INFO: Connection " << hostaddr << ":" << hostport << " ended." 
                        " Remote host has closed connection." << std::endl;
                    break;
                }
                if(result == -1) {
                    std::cerr << "> [" << L4<L7_object>::id << "] INFO: Connection " << hostaddr << ":" << hostport << " is going to close."
                        " Receiving error occured: " << errno << ":" << strerror(errno) << std::endl;
                    break;
                }
                if(handle_request(confd, buf) < 0) {
                    std::cerr << "> [" << L4<L7_object>::id << "] ERROR: Connection " << hostaddr << ":" << hostport << " is going to close."
                        " Sending error occured." << std::endl;
                    break;
                }
            }
            close(confd);
        }

        void 
        handle_buffered(struct sockaddr_storage inc_data, std::shared_ptr<inbuf> inbuf_ptr)
        {
            std::string buf;
            buf.reserve(BUFSIZ);

            char hostaddr[NI_MAXHOST];
            char hostport[NI_MAXSERV];
            getnameinfo((struct sockaddr*) &inc_data, sizeof inc_data, hostaddr, sizeof hostaddr, hostport, sizeof hostport,
                NI_NUMERICHOST | NI_NUMERICSERV);
            std::cout << "> [" << L4<L7_object>::id << "] INFO: Connection accepted from " << hostaddr << ":" << hostport << std::endl;

            for(;;) {
                int result = inbuf_read(inbuf_ptr, buf, buf.capacity(), 0);
                if(result == 0) {
                    std::cout << "> [" << L4<L7_object>::id << "] INFO: Connection " << hostaddr << ":" << hostport << " ended." 
                        " Remote host has closed connection." << std::endl;
                    break;
                }
                if(result == -1) {
                    std::cerr << "> [" << L4<L7_object>::id << "] ERROR: Connection " << hostaddr << ":" << hostport << " is going to close."
                        " Receiving error occured." << std::endl;
                    break;
                }
                if(handle_request(inbuf_ptr->confd, buf.c_str()) < 0) {
                    std::cerr << "> [" << L4<L7_object>::id << "] ERROR: Connection " << hostaddr << ":" << hostport << " is going to close."
                        " Sending error occured." << std::endl;
                    break;
                }
            }
            close(inbuf_ptr->confd);
        }
        
        ssize_t 
        inbuf_read(std::shared_ptr<inbuf> inbuf_ptr, std::string& buf, ssize_t size, int flags = 0)
        {
            if(size == 0) return 0;
            if(inbuf_ptr->buf.size() <= 0) {
                char tmpbuf[inbuf_ptr->buf.capacity()];
                int nbytes;
                do {
                    errno = 0;
                    nbytes = recv(inbuf_ptr->confd, tmpbuf, sizeof tmpbuf, flags);
                } while(nbytes < 0 && errno == EINTR);
                if(nbytes == 0) return 0;
                if(nbytes < 0) return -1;
                inbuf_ptr->buf.insert(0, tmpbuf, nbytes);
            }
            ssize_t nbytes = inbuf_ptr->buf.size();
            if(size < nbytes) nbytes = size;
            buf.replace(0, nbytes, inbuf_ptr->buf);
            inbuf_ptr->buf.erase(0, nbytes);
            return nbytes;
        }

        int
        handle_request(int confd, const char* request)
        {
            /*
            std::vector<std::string> answers;
            answers = L4<L7_object>::handler->handle(confd, _send, request);
            for(unsigned int i = 0; i < answers.size(); ++i) {
                if(_send(confd, answers[i].c_str(), answers[i].size()) < 0) return -1;
            }        
            return 0;
            */
            if(L4<L7_object>::handler->handle(confd, _send, _recv, (void*) &(L4<L7_object>::send_details), 
                L4<L7_object>::send_details.L4_protocol, request) < 0) return -1;
            return 0;
        }

        static int
        _send(int confd, void* send_details, const char* answer, size_t size)
        {
            /*
            typename L4<L7_object>::Sending_Details* test;
            test = (typename L4<L7_object>::Sending_Details*) send_details;
            std::cout << "PROTOCOL: " << test->L4_protocol << std::endl;
            */

            if(!size) return 0;
            size_t written = 0;
            while(written < size) {
                ssize_t outbytes = write(confd, answer+written, size-written);
                if(outbytes < 0) {
                    std::cerr << "COULDNT WRITE TO SOCKET" << std::endl;
                    return -1;
                }
                written += outbytes;
            }
            return 0;
        }

        static int
        _recv(int confd, void* send_details, char* buf, size_t bufSize)
        {
            int read = recv(confd, buf, bufSize, 0);
            // Remote side closed connection
            if(read == 0) {
                return 0;
            }
            // Receiving error occured
            if(read == -1) {
                //print_status(id, "ERROR", "Failure on receiving client response", strerror(errno));
                return -1;
            }
            return read;
        }

    private:

        int socketfd;
        struct addrinfo hints;
        struct addrinfo* info, *p;
        std::string server_address, server_port;
};

template<typename L7_object>
class UDP : public L4<L7_object>
{
    public:
        /* --- Constructor --- */
        UDP(unsigned int _id, std::shared_ptr<L7_object> object, std::string _L4_protocol)
            : L4<L7_object>{_id, object, _L4_protocol}, socketfd{0}
        {
            #ifdef DEBUG
            std::cout << "> [" << L4<L7_object>::id << "] DEBUG: UDP Constructor" << std::endl;
            #endif
        }

        ~UDP()
        {
            #ifdef DEBUG
            std::cout << "> [" << L4<L7_object>::id << "] DEBUG: UDP Deconstructor" << std::endl;
            #endif

            freeaddrinfo(info);
            close(socketfd);
        }

        UDP(const UDP& orig) = delete;
        UDP(UDP&& orig) = delete;
        UDP& operator= (UDP) = delete;

        /* --- Basic Methods --- */
        int
        setup_socket(std::string _server_port, std::string _server_address)
        {
            if(socketfd > 0) {
                close(socketfd);
                freeaddrinfo(info);
            }

            server_address = _server_address;
            server_port = _server_port;

            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_DGRAM;

            if(getaddrinfo(server_address.c_str(), server_port.c_str(), &hints, &info) != 0) return -1;

            for(p = info; p != NULL; p = p->ai_next) {
                if((socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                    continue;
                }
                break;
            }
            if(p == NULL) return -1;

            int _reuseaddr = 1;
            if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &_reuseaddr, sizeof _reuseaddr) < 0) return -1;
            if(bind(socketfd, p->ai_addr, p->ai_addrlen) < 0) return -1;

            std::cout << "> [" << L4<L7_object>::id << "] INFO: Server is listening on " << server_address << ""
                ":" << server_port << std::endl;

            return 0;
        }
        
        /*
        int
        accept_fork()
        {
            #ifdef DEBUG
            std::cout << "> [" << L4<L7_object>::id << "] DEBUG: Accept Fork. Socketfd = " << socketfd << std::endl;
            #endif

            struct sigaction action = {0};
            action.sa_handler = SIG_IGN;
            action.sa_flags = SA_NOCLDWAIT;
            if(sigaction(SIGCHLD, &action, 0) < 0) return -1;

            char hostaddr[NI_MAXHOST];
            char hostport[NI_MAXSERV];
            struct sockaddr_storage inc_addr;
            socklen_t addr_len = sizeof inc_addr;

            unsigned int ephemeral_port = 10001;

            while(recvfrom(socketfd, 0, 0, 0, (struct sockaddr*) &inc_addr, &addr_len) >= 0) {
                pid_t child;
                child = fork();

                if(child == -1) continue;
                if(!child) {
                    int esfd;
                    if((esfd = _connect(&inc_addr, addr_len, ephemeral_port)) < 0) {
                        std::cout << "> [" << L4<L7_object>::id << "] Could not create connection socket." << std::endl;
                        exit(1);
                    }

                    char hostaddr[NI_MAXHOST];
                    char hostport[NI_MAXSERV];
                    getnameinfo((struct sockaddr*) &inc_data, sizeof inc_data, hostaddr, sizeof hostaddr, hostport, sizeof hostport,
                        NI_NUMERICHOST | NI_NUMERICSERV);
                    std::cout << "> [" << L4<L7_object>::id << "] INFO: Connection accepted from " 
                        << hostaddr << ":" << hostport << std::endl;

                    connection_handler(esfd);
                }
                ++ephemeral_port;
                                
            }

            while(waitpid(-1, 0, 0) > 0);
            return 0;
        }
        */

        int
        accept_fork(unsigned int nmbr_of_procs)
        {
            #ifdef DEBUG
            std::cout << "> [" << L4<L7_object>::id << "] DEBUG: Accept Fork. Socketfd = " << socketfd << std::endl;
            #endif
            
            struct sigaction action = {0};
            action.sa_handler = SIG_IGN;
            action.sa_flags = SA_NOCLDWAIT;
            if(sigaction(SIGCHLD, &action, 0) < 0) return -1;

            if(nmbr_of_procs == UINT_MAX) {
                nmbr_of_procs = sysconf(_SC_NPROCESSORS_ONLN);
                if(nmbr_of_procs < 1) return -1;
            }

            #ifdef DEBUG
            std::cout << "> [" << L4<L7_object>::id << "] DEBUG: Number of Processes to create = " << nmbr_of_procs << std::endl;
            #endif

            pid_t child;
            for(long i = 0; i < nmbr_of_procs; ++i) {
                child = fork();
                if(child == -1) continue;
                if(!child) {
                    connection_handler();
                    exit(0);
                }
            }

            while(waitpid(-1, 0, 0) > 0);
            return 0;
        }

    private:
        int
        _connect(sockaddr_storage* inc_data, socklen_t addr_len, unsigned int ephemeral_port)
        {
            int esfd;
            if((esfd = ephemeral_socket(ephemeral_port)) < 0) return -1;
            if(connect(esfd, (struct sockaddr*) inc_data, addr_len) < 0) {
                close(esfd);
                return -1;
            }

            struct pollfd pollfds[1] = {0};
            pollfds[0].fd = esfd;
            pollfds[0].events = POLLIN;
            unsigned int attempts = 0;

            while(attempts < 10 && poll(pollfds, 1, 100) == 0) {
                ++attempts;
                if(write(esfd, &ephemeral_port, sizeof ephemeral_port) < 0) {
                    close(esfd);
                    return -1;
                }
            }

            if(attempts < 10) {
                unsigned int answer;
                if(recv(esfd, &answer, sizeof answer, 0) > 0 && answer == ephemeral_port)
                    return esfd;
            }

            return -1;
        }

        int
        ephemeral_socket(unsigned int ephemeral_port)
        {
            int esfd;
            struct addrinfo hints, *info, *p;

            std::string ephemeral_server_address = server_address;
            std::string ephemeral_server_port = std::to_string(ephemeral_port);

            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_DGRAM;

            
            if(getaddrinfo(ephemeral_server_address.c_str(), ephemeral_server_port.c_str(), &hints, &info) != 0) return -1;

            for(p = info; p != NULL; p = p->ai_next) {
                if((socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                    continue;
                }
                break;
            }
            if(p == NULL) return -1;

            int _reuseaddr = 1;
            if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &_reuseaddr, sizeof _reuseaddr) < 0) return -1;
            if(bind(socketfd, p->ai_addr, p->ai_addrlen) < 0) return -1;

            return esfd;
        }

        void
        connection_handler()
        {
            #ifdef DEBUG
            std::cout << "> [" << L4<L7_object>::id << "] DEBUG: Handler " << getpid() << " here. Socketfd = " 
                << socketfd << socketfd << std::endl;
            #endif

            struct sockaddr_storage their_addr;
            socklen_t addr_len = sizeof their_addr;
            char buf[BUFSIZ];

            while(recvfrom(socketfd, buf, sizeof buf, 0, (struct sockaddr*) &their_addr, &addr_len) >= 0) {
                #ifdef DEBUG
                std::cout << "> [" << L4<L7_object>::id << "] DEBUG: Request = " << buf << std::endl;
                #endif 
                
                L4<L7_object>::send_details.receiver_addr = their_addr;
                L4<L7_object>::send_details.addr_len = addr_len;

                handle_request(buf);
            }
        }

        int
        handle_request(const char* request)
        {   
            /*
            std::vector<std::string> answers;

            answers = L4<L7_object>::handler->handle(request);
            for(unsigned int i = 0; i < answers.size(); ++i) {
                if(_send(target_addr, addr_len, answers[i].c_str(), answers[i].size()) < 0) return -1;
            }
            return 0;
            */
            if(L4<L7_object>::handler->handle(socketfd, _send, _recv, (void*) &(L4<L7_object>::send_details), 
                L4<L7_object>::send_details.L4_protocol, request) < 0) return -1;
            return 0;
        }
        
        static int
        _send(int confd, void* send_details, const char* answer, size_t size)
        {
            typename L4<L7_object>::Sending_Details* temp_details;
            temp_details = (typename L4<L7_object>::Sending_Details*) send_details;

            if(!size) return 0;
    
            size_t packet_size = 1420;
            const char* temp_answer = answer;

            while(size > 0) {

                if(packet_size > size) {
                    sendto(confd, temp_answer, size, 0, (struct sockaddr*) &(temp_details->receiver_addr), temp_details->addr_len);
                    break;
                }

                ssize_t errornmbr = sendto(confd, temp_answer, packet_size, 0, 
                    (struct sockaddr*) &(temp_details->receiver_addr), temp_details->addr_len);
                if(errornmbr < 0) return -1;

                temp_answer += packet_size;
                size -= packet_size;


            }
            
            return 0;
        }

        static int
        _recv(int confd, void* send_details, char* buf, size_t bufSize)
        {
            struct sockaddr_storage their_addr;
            socklen_t addr_len = sizeof their_addr;

            int read = recvfrom(confd, buf, bufSize, 0, (struct sockaddr*) &their_addr, &addr_len);
            // Remote side closed connection
            if(read == 0) return 0; // not rly when using UDP
            // Receiving error occured
            if(read == -1) return -1;

            return read;
        }

        int socketfd;
        struct addrinfo hints;
        struct addrinfo *info, *p;
        std::string server_address, server_port;        
};

#endif
