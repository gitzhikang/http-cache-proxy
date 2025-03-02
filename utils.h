//
// Created by 宋志康 on 2025/2/15.
//

#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include "request.h"
#include "response.h"

int build_server_socket(const char *port);
int build_client_socket(const char *host, const char *port);

int block_accept(int socket_fd,std::string* ip);

int get_local_socket_port(int socket_fd);
std::string get_local_socket_ip(int socket_fd);

int get_peer_socket_port(int socket_fd);
std::string get_peer_socket_ip(int socket_fd);

std::string get_current_time();

void get_entire_http_request_by_content_length(int client_socket_fd, std::string &req_msg,size_t content_length);

size_t get_content_length_from_head(std::string &http_head);

void get_entire_http_content(int recv_fd, std::string &req_msg);

std::string get_cache_query_key(int client_socket_fd,Request& req);

std::string add_Via_to_response(const char * content,std::string &host);

std::string add_Age_to_response(const char * content,Response &res);

std::string max_age_to_GMT(std::string& response_time_str,int max_age);

bool is_GMT_time_greater_than_current_time(std::string &time_str);



#endif //UTILS_H
