#pragma once

#include <cerrno>
#include <cstdint>
#include <ctime>
#include <pthread.h>

#include "Mutex.h"
#include "noncopyable.h"


class Condition: private Noncopyable {
public:
    explicit Condition(Mutex& mutex) : m_mutex_ref(mutex) {
        pthread_cond_init(&cond, nullptr);
    }
    ~Condition() {
        pthread_cond_destroy(&cond);
    }

    void wait() { pthread_cond_wait(&cond, m_mutex_ref.get()); }

    // If wait timeout , return true. Else return false.
    bool wait_for_seconds(int seconds) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += static_cast<time_t>(seconds);
        return (ETIMEDOUT == pthread_cond_timedwait(&cond, m_mutex_ref.get(), &ts)); 
    }

    void notify() { pthread_cond_signal(&cond); }
    void notify_all() { pthread_cond_broadcast(&cond); }

private:
    Mutex& m_mutex_ref;
    pthread_cond_t cond;
};
