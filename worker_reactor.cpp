//
// Created by 宋志康 on 2025/2/26.
//

#include "worker_reactor.h"
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <iostream>
#include "connection_info.h"

WorkerReactor::WorkerReactor(Proxy* p, int pipe_fd, int thread_count): proxy(p), pipe_read_fd(pipe_fd), thread_pool(thread_count), running(true) {
    // 创建 epoll 实例
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Worker epoll_create1");
        exit(EXIT_FAILURE);
    }

    // 添加管道读端到 epoll
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; // ET
    ev.data.ptr = new ConnectionInfo{pipe_read_fd, 0};
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe_read_fd, &ev) == -1) {
        perror("Worker epoll_ctl: pipe_read_fd");
        exit(EXIT_FAILURE);
    }

}

int WorkerReactor::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK");
        return -1;
    }
    return 0;
}

void WorkerReactor::run(){
  struct epoll_event events[64];

        while (running) {
            int n = epoll_wait(epoll_fd, events, 64, -1);
            if (n == -1) {
                if (errno == EINTR) continue;
                perror("Worker epoll_wait");
                break;
            }

            //read the events
            for (int i = 0; i < n; i++) {
                int fd = events[i].data.ptr ? static_cast<ConnectionInfo*>(events[i].data.ptr)->client_socket_fd : pipe_read_fd;

                if (fd == pipe_read_fd) {
                    // 从管道读取新连接信息 - ET模式下需要循环读取所有数据

                while (true) {  // 循环读取直到没有更多数据
                    ConnectionInfo new_conn;
                    ssize_t size = read(pipe_read_fd, &new_conn, sizeof(new_conn));


                    if (size == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // 没有更多数据可读
                            break;
                        }
                        perror("Worker read from pipe");
                        break;
                    } else if (size == 0) {
                        // 管道已关闭
                        break;
                    } else if (size == sizeof(new_conn)) {
                        // 将客户端socket设为非阻塞
                        set_nonblocking(new_conn.client_socket_fd);

                        // 添加到epoll
                        struct epoll_event ev;
                        ev.events = EPOLLIN | EPOLLET; // ET
                        ev.data.ptr = new ConnectionInfo{new_conn.client_socket_fd, new_conn.client_id};

                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_conn.client_socket_fd, &ev) == -1) {
                            perror("Worker epoll_ctl: client_socket_fd");
                            close(new_conn.client_socket_fd);
                            delete static_cast<ConnectionInfo*>(ev.data.ptr);
                            continue;
                        }
                    } else {
                        // 读取了部分数据，这是一个错误情况
                        std::cerr << "Partial read from pipe: " << size << " bytes" << std::endl;
                        break;
                    }
                }
                } else {
                    // 处理客户端请求
                    ConnectionInfo* conn_info = static_cast<ConnectionInfo*>(events[i].data.ptr);

                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL); // 从epoll中删除，避免其他线程处理

                    // 将请求处理放入线程池
                    thread_pool.enqueue([this, conn_info]() {
                        this->proxy->handle_request(conn_info);
                        delete conn_info; // 处理完毕后释放连接信息
                    });
                }
            }
        }
}

void WorkerReactor::stop() {
     running = false;
}