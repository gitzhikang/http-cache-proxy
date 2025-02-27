//
// Created by 宋志康 on 2025/2/19.
//

#ifndef PROXY_H
#define PROXY_H

#include <fstream>
#include <vector>
#include <atomic>
#include <pthread.h>
#include "worker_reactor.h"
#include "thread_safe_cache.h"
#include "request.h"
#include "connection_info.h"

class WorkerReactor;

class Proxy {
    const char * port_num;
    std::ofstream log_file;
    pthread_mutex_t log_mutex;
    ThreadSafeCache cache;

    // field about reactor
    int main_epoll_fd;
    std::vector<WorkerReactor*> worker_reactors;
    std::vector<pthread_t> worker_threads;
    std::vector<std::pair<int, int>> pipes; // 管道数组，用于主从通信
    int next_worker; // 轮询分配worker
    std::atomic<int> client_id_counter;

    struct ThreadData {
        Proxy* proxy;
        int client_socket_fd;
        std::string client_ip;
        int client_id;
    };


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
    int accept_connection(int server_fd, std::string &client_ip);
    void write_log_with_lock(const std::string &log);
public:
    Proxy(const char* p_num, const char* log_path)
        : port_num(p_num), next_worker(0),client_id_counter(0) {
        log_file.open(log_path);
        pthread_mutex_init(&log_mutex, NULL);
    }
    void run(int worker_count = 4, int thread_pool_size = 4);
    void * handle_request(ConnectionInfo* conn_info);
    ~Proxy();

};



#endif //PROXY_H
