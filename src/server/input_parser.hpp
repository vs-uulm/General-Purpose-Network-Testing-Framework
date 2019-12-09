/*
Copyright (C) 2019 Leonard Bradatsch
Contact: leonard.bradatsch@uni-ulm.de
*/

#ifndef INPUT_PARSER_HPP
#define INPUT_PARSER_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <map>
#include <list>
#include <regex>

std::regex flow_reg{"\\[(F|f)low[0123456789]+\\]"};

class IParser {
    public:
        IParser() = delete;

        static void usage(char const* const _cmdname) {
            std::string usgmsgPrefix{"Usage: "};
            std::string cmdname{_cmdname};
            std::string usgmsgPostfix{" <path/parameter_file.ini>"};
            std::cout << usgmsgPrefix << cmdname << usgmsgPostfix  << std::endl;
        }

        template<class Key, class Value>
        static void identify_flows(std::string const& filename, std::list<std::map<Key, Value>>& parameter_list) {
            std::string line;
            std::ifstream input(filename);

            if(input.is_open()) {
                while(getline(input, line)) {
                    if(std::regex_match(line, flow_reg)) {
                        std::cout << "> Flow found: " << line <<  " ...Start processing the flow...";
                        std::map<Key, Value> flow{};
                        parse_flow(input, flow);
                        parameter_list.push_back(flow);
                        std::cout << "Done." << std::endl;
                    } else {
                        std::cout << "> Line does not match criteria..." << std::endl;
                    }
                }
                std::cout << "> Flows have been renamed..." << std::endl;
                std::cout << "> All flows processed." << std::endl << std::endl;
            } else {
                std::cerr << "> Error opening file..." << std::endl;
                exit(1);
            }
        }

        template< class Key, class Value>
        static void print_list(std::list<std::map<Key, Value>> const& list) {
            int flow_index = 0;
            for(auto const& ele : list) {
                std::cout << "> Flow " << flow_index << ":" << std::endl;
                flow_index++;
                print_flow(ele);
            }
        }

    private:
        template<class Key, class Value>
        static void parse_flow(std::ifstream& input, std::map<Key, Value>& parameters) {
            std::string line;
            std::string keyword, value;
            int position;

            while(((position = input.tellg()) || 1 ) && getline(input, line)) {
                if(std::regex_match(line, flow_reg)) {
                    input.seekg(position);
                    break;
                }
                std::stringstream iss(line);
                if(line.find(":") == std::string::npos) {
                    std::cerr << "> Syntactically incorret line. Skipping line..." << std::endl;
                    continue;
                }
                if(std::getline(iss, keyword, ':') && std::getline(iss, value)) {
                    keyword.erase(std::remove(keyword.begin(), keyword.end(), ' '), keyword.end());
                    value.erase(std::remove(value.begin(), value.end(), ' '), value.end());
                    parameters.insert(std::pair<Key, Value>(keyword, value));
                } else {
                    std::cerr << "> Error parsing line. Skipping line..." << std::endl;
                }
            }
        }

        template<class Key, class Value>
        static void print_flow(std::map<Key, Value> const& parameters) {
            for(auto const& ele : parameters)
                std::cout << ele.first << " : " << ele.second << std::endl;
        }
};

#endif
