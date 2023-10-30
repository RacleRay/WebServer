#pragma once

#include <functional>
#include <memory>
#include <pthread.h>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>

#include "noncopyable.h"
#include "CurrentThread.h"


class Thread: private Noncopyable {
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(const ThreadFunc& func, const std::string& name = std::string());
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
    
    pthread_t m_pthread_id;
    pid_t m_tid;

    ThreadFunc m_func;
    std::string m_name;

    void _set_default_name();

};


struct ThreadData {
    using ThreadFunc = Thread::ThreadFunc;

    ThreadFunc m_func;
    std::string m_name;
    pid_t* m_tid;

    
};
