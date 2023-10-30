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


// current thread local variables.
namespace CurrentThread {
    __thread pid_t t_cached_tid = 0;
    __thread char t_tid_string[32];
    __thread int t_tid_string_length = 6;
    __thread const char* t_thread_name = "default";
}  // namespace CurrentThread


namespace {
    pid_t get_cur_tid() {
        return static_cast<pid_t>(syscall(SYS_gettid));
    }
} // namespace


void CurrentThread::cache_tid() {
    if (t_cached_tid == 0) {
        t_cached_tid = get_cur_tid();
        t_tid_string_length = \
            snprintf(t_tid_string, sizeof(t_tid_string), "%5d", t_cached_tid);
    }
}





