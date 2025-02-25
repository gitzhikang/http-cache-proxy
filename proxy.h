//
// Created by 宋志康 on 2025/2/19.
//

#ifndef PROXY_H
#define PROXY_H

#include <fstream>
#include "thread_safe_cache.h"
#include "request.h"
class Proxy {
    const char * port_num;
    std::ofstream log_file;
    pthread_mutex_t log_mutex;
    ThreadSafeCache cache;

    struct ThreadData {
        Proxy* proxy;
        int client_socket_fd;
        std::string client_ip;
        int client_id;
    };

    void* handle_request(void* info);
    void handle_connect(int client_socket_fd, int origin_socket_fd, int client_id);
    void handle_post(int client_socket_fd, int origin_socket_fd, int client_id,
                    Request& req, std::string& req_msg);
    void handle_get(int client_socket_fd, int origin_socket_fd, int client_id,
                   Request& req, std::string& req_msg);
    void handle_cache_miss(int client_socket_fd, int origin_socket_fd, int client_id,
                          Request& req, std::string& req_msg);
    void send_response_to_client(int client_socket_fd, bool is_cache_hit,
                               const char* content, Response& res);
    void save_response_to_cache(std::string key, Response& res,int client_id);

    static void* thread_handler(void* arg);
    void write_log_with_lock(const std::string &log);
public:
    Proxy(const char* p_num, const char* log_path)
        : port_num(p_num), log_file(log_path), log_mutex(PTHREAD_MUTEX_INITIALIZER) {}
    void run();
    ~Proxy() {
        //close the log file
        log_file.close();
    };

};



#endif //PROXY_H
