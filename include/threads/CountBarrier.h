#pragma once

#include "Condition.h"
#include "Mutex.h"
#include "noncopyable.h"

class CountBarrier: private Noncopyable {
public:
    explicit CountBarrier(int count) 
        : m_count_mutex(), m_count_cond(m_count_mutex), m_count(count) {}

    // wait until count reaches 0
    void wait() {
        MutexGuard guard(m_count_mutex);
        while (m_count > 0) {
            m_count_cond.wait();
        }
    }

    // atomically count down by 1
    void countdown() {
        MutexGuard guard(m_count_mutex);
        --m_count;
        if (m_count == 0) {
            // notify all waiting CountBarrier instances.
            m_count_cond.notify_all();
        }
    }

private:
    Mutex m_count_mutex;
    Condition m_count_cond;

    int m_count;
};