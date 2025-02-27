//
// Created by 宋志康 on 2025/2/26.
//

#ifndef WORKER_REACTOR_H
#define WORKER_REACTOR_H


#include "thread_pool.h"
#include "proxy.h"
#include <atomic>
#include <unistd.h>


class Proxy;

class WorkerReactor {
    int epoll_fd;
    int pipe_read_fd;
    Proxy* proxy;
    ThreadPool thread_pool;
    std::atomic<bool> running;

public:
    WorkerReactor(Proxy* p, int pipe_fd, int thread_count);
    ~WorkerReactor(){close(epoll_fd);}
    // set non_blocking mode for a file descriptor IO
    static int set_nonblocking(int fd);
    void run();
    void stop();

};



#endif //WORKER_REACTOR_H
