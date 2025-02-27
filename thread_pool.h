//
// Created by 宋志康 on 2025/2/26.
//

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <functional>
#include <pthread.h>

class ThreadPool {
private:
    std::vector<pthread_t> threads;
    std::queue<std::function<void()>> tasks;
    pthread_mutex_t queue_mutex;
    pthread_cond_t condition;
    bool stop;
    static void* worker_thread(void* arg) {
        ThreadPool* pool = static_cast<ThreadPool*>(arg);

        while (true) {
            std::function<void()> task;

            {
                pthread_mutex_lock(&pool->queue_mutex);
                while (!pool->stop && pool->tasks.empty()) {
                    pthread_cond_wait(&pool->condition, &pool->queue_mutex);
                }

                if (pool->stop && pool->tasks.empty()) {
                    pthread_mutex_unlock(&pool->queue_mutex);
                    return NULL;
                }

                task = pool->tasks.front();
                pool->tasks.pop();
                pthread_mutex_unlock(&pool->queue_mutex);
            }

            task();
        }

        return NULL;
    }
 public:
    ThreadPool(size_t num_threads) : stop(false) {
        pthread_mutex_init(&queue_mutex, NULL);
        pthread_cond_init(&condition, NULL);
        for (size_t i = 0; i < num_threads; i++) {
            pthread_t thread;
            pthread_create(&thread, NULL, &ThreadPool::worker_thread, this);
            threads.push_back(thread);
        }
    }

    void enqueue(std::function<void()> task) {

      pthread_mutex_lock(&queue_mutex);
      if(!stop){
          tasks.push(task);
      }
      pthread_mutex_unlock(&queue_mutex);

      pthread_cond_signal(&condition);
    }

    ~ThreadPool() {

      pthread_mutex_lock(&queue_mutex);
      stop = true;
      pthread_mutex_unlock(&queue_mutex);


      pthread_cond_broadcast(&condition);
    for (size_t i = 0; i < threads.size(); i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&condition);
    }


};



#endif //THREAD_POOL_H
