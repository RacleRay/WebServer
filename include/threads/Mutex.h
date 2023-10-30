#pragma once

#include <cstdio>
#include <pthread.h>

#include "noncopyable.h"


class Mutex: private Noncopyable {
public:
    Mutex() { pthread_mutex_init(&m_mutex, nullptr); }
    ~Mutex() {
        pthread_mutex_lock(&m_mutex);
        pthread_mutex_unlock(&m_mutex);
        pthread_mutex_destroy(&m_mutex);
    }

    void lock() { pthread_mutex_lock(&m_mutex); }   
    void unlock() { pthread_mutex_unlock(&m_mutex); }
    pthread_mutex_t* get() { return &m_mutex; }

    friend class Condition;

private:
    pthread_mutex_t m_mutex;
};


class MutexGuard: private Noncopyable {
public:
    explicit MutexGuard(Mutex& mutex): m_mutex_ref(mutex) { m_mutex_ref.lock(); }
    ~MutexGuard() { m_mutex_ref.unlock(); }

private:
    Mutex& m_mutex_ref;
};