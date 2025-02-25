//
// Created by 宋志康 on 2025/2/19.
//

#ifndef INFO_H
#define INFO_H
#include <string>
#include "proxy.h"
class Info{
  public:
    int client_socket_fd;
    int client_id;
    std::string client_ip;
    Proxy * proxy;
};

#endif //INFO_H
