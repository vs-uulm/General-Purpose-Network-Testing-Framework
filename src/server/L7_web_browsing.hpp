/*
Copyright (C) 2019 Leonard Bradatsch
Contact: leonard.bradatsch@uni-ulm.de
*/

#ifndef L7_WEB_BROWSING_HPP
#define L7_WEB_BROWSING_HPP

#include "L7.hpp"

template <typename Main_Object_Size_Dist, typename Inline_Object_Size_Dist>
class Web_Browsing : public L7 
{
    public:
        Web_Browsing(std::shared_ptr<Main_Object_Size_Dist> _main_object_size_dist, std::shared_ptr<Inline_Object_Size_Dist> _inline_object_size_dist)
            : generator{RandomEngine()}, main_object_size_dist{_main_object_size_dist}, inline_object_size_dist{_inline_object_size_dist}
        {
            ;
        }

        int
        handle(int confd, int (*f)(int, void*, const char*, size_t), int (*r)(int, void*, char*, size_t),
            void* send_details, std::string protocol, const char* request)
        {
            static int initialized = 0;
            if(!initialized) {
                generator = RandomEngine();
                initialized = 1; 
            }

            std::string answer;

            if(request[0] == 'g') {
                int main_object_size = round(main_object_size_dist->operator()(generator));
                if(main_object_size < 2) main_object_size = 2;
                answer.append(std::to_string(main_object_size));
                answer.append(main_object_size - answer.size(), 'a');
            } else {
                int inline_object_size = round(inline_object_size_dist->operator()(generator));
                if(inline_object_size < 2) inline_object_size = 2;
                answer.append(std::to_string(inline_object_size));
                answer.append(inline_object_size - answer.size(), 'a');
            }
                        
            if(f(confd, send_details, answer.c_str(), answer.size()) < 0) return -1;
            return 0; 
        }
    private:
        std::default_random_engine generator;
        std::shared_ptr<Main_Object_Size_Dist> main_object_size_dist;
        std::shared_ptr<Inline_Object_Size_Dist> inline_object_size_dist;
};

#endif
