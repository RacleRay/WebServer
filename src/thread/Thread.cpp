#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <linux/unistd.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <memory>

#include "CurrentThread.h"
#include "Thread.h"


// current thread local variables init.
namespace CurrentThread {
    __thread pid_t t_cached_tid = 0;
    __thread char t_tid_string[32];
    __thread int t_tid_string_length = 6;
    __thread const char* t_thread_name = "default";
}  // namespace CurrentThread


// Local functions
namespace {
    pid_t get_cur_tid() {
        return static_cast<pid_t>(syscall(SYS_gettid));
    }

    void* start_thread(void* arg) {
        auto* data = static_cast<ThreadData*>(arg);

        data->run_in_thread();  // will notify the waiting created threads.

        // data->m_tid = nullptr;
        // data->m_barrier = nullptr;
        delete data;
        return nullptr;
    }
} // namespace


void CurrentThread::cache_tid() {
    if (t_cached_tid == 0) {
        t_cached_tid = get_cur_tid();
        t_tid_string_length = \
            snprintf(t_tid_string, sizeof(t_tid_string), "%5d", t_cached_tid);
    }
}


// ===========================================================================
// Thread

Thread::Thread(ThreadFunc func, std::string name)
    : m_func(std::move(func)), m_name(std::move(name)) {
    _set_default_name();
}


Thread::~Thread() {
    if (m_started && !m_joined) {
        pthread_detach(m_pthread_id);
    }
}


// set default thread name to be Thread-<thread_id>
void Thread::_set_default_name() {
    if (!m_name.empty()) {
        return;
    }

    char buf[32];
    (void)snprintf(buf, sizeof(buf), "Thread-%ld", m_pthread_id);

    m_name = buf;
}


void Thread::start() {
    assert(!m_started);

    m_started = true;
    auto* data = new ThreadData(m_func, m_name, &m_tid, &m_barrier);

    int result = pthread_create(&m_pthread_id, nullptr, start_thread, data);
    if (result < 0) {
        m_started = false;
        delete data;
        return;
    }

    // m_barrier init with count value to be 1.
    // wait for thread to be allocated.
    // When the thread is created, it will notify the waiting threads by calling
    //      ThreadData::run_in_thread() --> m_barrier->countdown().
    m_barrier.wait();
    assert(m_tid > 0);
}


int Thread::join() {
    assert(m_started);
    assert(!m_joined);
    m_joined = true;
    return pthread_join(m_pthread_id, nullptr);
}


// ============================================================================
// ThreadData
void ThreadData::run_in_thread() const {
    *m_tid = CurrentThread::get_tid();
    // TODO: why set m_tid and m_barrier to be nullptr ?
    m_barrier->countdown();  // will notify barrier`s wait()

    // m_thread_name is from Thread::m_name
    CurrentThread::t_thread_name = m_thread_name.empty() ? "Thread" : m_thread_name.c_str();
    prctl(PR_SET_NAME, CurrentThread::t_thread_name);  // set thread name

    // run
    m_func();

    CurrentThread::t_thread_name = "finished";
}