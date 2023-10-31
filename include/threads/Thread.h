#pragma once

#include <functional>
#include <memory>
#include <pthread.h>
#include <string>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "CurrentThread.h"
#include "CountBarrier.h"
#include "noncopyable.h"


class Thread: private Noncopyable {
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func, std::string name = std::string());
    ~Thread();

    void start();
    int join();
    [[nodiscard]] bool started() const { return m_started; }
    [[nodiscard]] pid_t tid() const { return m_tid; }
    [[nodiscard]] const std::string& thread_name() const { return m_name; }

    // bool started() const { return m_started; }
    // pid_t tid() const { return m_tid; }
    // const std::string& thread_name() const { return m_name; }

private:
    bool m_started {false};
    bool m_joined {false};
    
    pthread_t m_pthread_id {0};
    pid_t m_tid {0};

    ThreadFunc m_func;
    std::string m_name;

    CountBarrier m_barrier {1};

    void _set_default_name();
};


struct ThreadData {
public:
    using ThreadFunc = Thread::ThreadFunc;

    ThreadFunc m_func;   // Thread::m_func is passed to ThreadData::m_func.
    std::string m_thread_name;
    
    pid_t* m_tid;
    CountBarrier* m_barrier;

public:
    ThreadData(ThreadFunc func, std::string name, pid_t* tid, CountBarrier* barrier)
        : m_func(std::move(func)), m_thread_name(std::move(name)), m_tid(tid), m_barrier(barrier) {}  
    
    // This will notify the waiting procedure (Thread::start), and then run the
    // ThreadData::m_func (same as Thread::m_func)
    void run_in_thread() const;    
};
