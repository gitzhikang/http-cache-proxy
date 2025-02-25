//
// Created by 宋志康 on 2025/2/19.
//

#ifndef REQUEST_H
#define REQUEST_H

#include <string>

class Request {
    std::string method;
    std::string host;
    std::string port;
    std::string line;
    size_t content_length;
    public:
    Request() :
        method(""),
        host(""),
        port(""),
        line("") {}
    void pharse_request(std::string request);
    std::string get_host();
    std::string get_port();
    std::string get_line();
    std::string get_method();
    size_t get_content_length();
};



#endif //REQUEST_H
