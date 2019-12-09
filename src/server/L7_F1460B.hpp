/*
Copyright (C) 2019 Leonard Bradatsch
Contact: leonard.bradatsch@uni-ulm.de
*/

#ifndef L7_F1460B_HPP
#define L7_F1460B_HPP

#include "L7.hpp"

class F1460B_ignore : public L7 
{
    public:
        int
        handle(int confd, int (*f) (int , void*, const char* , size_t), int (*r)(int, void*, char*, size_t),
            void* send_details, std::string protocol, const char* request) 
        {
            return 0;
        }
};

#endif
