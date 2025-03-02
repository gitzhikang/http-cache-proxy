#ifndef THREAD_SAFE_CACHE_H
#define THREAD_SAFE_CACHE_H

#include <unordered_map>
#include <list>
#include <string>
#include "response.h"

class ThreadSafeCache {
private:
    // 缓存最大容量
    size_t capacity;
    
    // LRU列表，存储指向unordered_map中key的指针
    std::list<const std::string*> lru_list;
    
    // 哈希表，存储key和Response以及指向LRU列表中位置的迭代器
    struct CacheValue {
        Response response;
        typename std::list<const std::string*>::iterator lru_iter;
    };
    
    std::unordered_map<std::string, CacheValue> cache;
    
    // 读写锁
    pthread_rwlock_t rwlock;

public:
    // 默认缓存容量
    ThreadSafeCache(size_t cap = 1000) : capacity(cap) {
        pthread_rwlock_init(&rwlock, NULL);
    }

    // 插入或更新缓存项
    void insert(const std::string &key, const Response &value) {
        pthread_rwlock_wrlock(&rwlock);
        
        // 如果key已存在，更新值并调整位置
        auto it = cache.find(key);
        if (it != cache.end()) {
            // 更新值
            it->second.response = value;
            // 将对应的指针移到LRU列表前端
            lru_list.erase(it->second.lru_iter);
            lru_list.push_front(&it->first);
            // 更新迭代器
            it->second.lru_iter = lru_list.begin();
        } else {
            // 如果缓存已满，移除最久未使用的项
            if (cache.size() >= capacity && !cache.empty()) {
                // 获取最久未使用的key
                const std::string* oldest_key = lru_list.back();
                // 从缓存中移除
                cache.erase(*oldest_key);
                // 从LRU列表中移除
                lru_list.pop_back();
            }
            
            // 先将key插入cache，这样可以获取到稳定的key引用
            auto inserted = cache.insert({key, {value, {}}});
            
            // 添加key的指针到LRU列表前端
            lru_list.push_front(&(inserted.first->first));
            
            // 更新LRU迭代器
            inserted.first->second.lru_iter = lru_list.begin();
        }
        
        pthread_rwlock_unlock(&rwlock);
    }

    // 查找缓存项是否存在
    bool find(const std::string &key) {
        pthread_rwlock_rdlock(&rwlock);
        bool exists = (cache.find(key) != cache.end());
        pthread_rwlock_unlock(&rwlock);
        return exists;
    }

    // 获取缓存项并更新其位置
    Response* get(const std::string &key) {
        pthread_rwlock_wrlock(&rwlock); // 需要写锁因为会修改LRU列表
        
        auto it = cache.find(key);
        if (it == cache.end()) {
            pthread_rwlock_unlock(&rwlock);
            return nullptr;
        }
        
        // 将对应的指针移到LRU列表前端
        lru_list.erase(it->second.lru_iter);
        lru_list.push_front(&it->first);
        it->second.lru_iter = lru_list.begin();
        
        Response* result = &(it->second.response);
        pthread_rwlock_unlock(&rwlock);
        return result;
    }

    // 从缓存中删除项
    void erase(const std::string &key) {
        pthread_rwlock_wrlock(&rwlock);
        
        auto it = cache.find(key);
        if (it != cache.end()) {
            // 从LRU列表中移除
            lru_list.erase(it->second.lru_iter);
            // 从缓存中移除
            cache.erase(it);
        }
        
        pthread_rwlock_unlock(&rwlock);
    }

    // 获取当前缓存大小
    size_t size(){
        pthread_rwlock_rdlock(&rwlock);
        size_t sz = cache.size();
        pthread_rwlock_unlock(&rwlock);
        return sz;
    }

    // 清空缓存
    void clear() {
        pthread_rwlock_wrlock(&rwlock);
        lru_list.clear();
        cache.clear();
        pthread_rwlock_unlock(&rwlock);
    }

    // 析构函数
    ~ThreadSafeCache() {
        pthread_rwlock_destroy(&rwlock);
    }
};

#endif //THREAD_SAFE_CACHE_H