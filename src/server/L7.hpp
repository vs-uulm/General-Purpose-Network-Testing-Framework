/*
Copyright (C) 2019 Leonard Bradatsch
Contact: leonard.bradatsch@uni-ulm.de
*/

#ifndef L7_HPP
#define L7_HPP

#include <vector>
#include "../misc/Distribution_Functions.hpp"
#include "../misc/useful_functions.hpp"

class L7
{
    public:
        virtual ~L7() {};
        virtual int handle(int confd, int (*f)(int, void*, const char*, size_t), int (*r)(int, void*, char*, size_t),
            void* send_details, std::string protocol, const char* request) = 0;
};

#endif
