#pragma once 

#include <cstdint>
#include <pthread.h>

namespace CurrentThread {

    // __thread: each thread will have its own copy of the variable "t_cached_tid"
    extern __thread pid_t t_cached_tid;
    extern __thread char t_tid_string[32];
    extern __thread int t_tid_string_length;
    extern __thread const char* t_thread_name;

    void cache_tid();

    inline pid_t get_tid() {
        if (__builtin_expect(t_cached_tid == 0, false)) {
            cache_tid();
        }
        return t_cached_tid;
    }

    inline const char* get_tid_string() {
        return t_tid_string;
    }

    inline int get_tid_string_length() {
        return t_tid_string_length;
    }

    inline const char* get_thread_name() {
        return t_thread_name;
    }
}  // namespace CurrentThread

