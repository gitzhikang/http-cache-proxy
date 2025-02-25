//
// Created by 宋志康 on 2025/2/19.
//

#ifndef THREAD_SAFE_CACHE_H
#define THREAD_SAFE_CACHE_H

#include <unordered_map>
#include "response.h"

class ThreadSafeCache {
    std::unordered_map<std::string, Response> cache;
    pthread_rwlock_t rwlock;
public:
    ThreadSafeCache() {
        pthread_rwlock_init(&rwlock, NULL);
    }

    void insert(const std::string &key, const Response &value) {
        pthread_rwlock_wrlock(&rwlock);
        cache[key] = value;
        pthread_rwlock_unlock(&rwlock);
    }

    bool find(const std::string &key) {
        pthread_rwlock_rdlock(&rwlock);
        auto it = cache.find(key);
        if (it == cache.end()) {
            pthread_rwlock_unlock(&rwlock);
            return false;
        }
        pthread_rwlock_unlock(&rwlock);
        return true;
    }

    Response* get(const std::string &key) {
        pthread_rwlock_rdlock(&rwlock);
        auto it = cache.find(key);
        if (it == cache.end()) {
            pthread_rwlock_unlock(&rwlock);
            return nullptr;
        }
        Response value = it->second;
        pthread_rwlock_unlock(&rwlock);
        return &value;
    }

    void erase(const std::string &key) {
        pthread_rwlock_wrlock(&rwlock);
        cache.erase(key);
        pthread_rwlock_unlock(&rwlock);
    }

    ~ThreadSafeCache() {
        pthread_rwlock_destroy(&rwlock);
    }
};



#endif //THREAD_SAFE_CACHE_H
