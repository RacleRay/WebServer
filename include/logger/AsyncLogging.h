#pragma once

#include <functional>
#include <string>
#include <vector>

#include "CountBarrier.h"
#include "LogStream.h"
#include "Mutex.h"
#include "Thread.h"
#include "noncopyable.h"

/**
 * @brief 负责启动 log 线程。使用“双缓冲区”，定时或者缓冲区满时写入 log file 中
 * 
 */
class AsyncLogging: private Noncopyable {
public:
    explicit AsyncLogging(const std::string& filename, int flush_buf_timeout = 2);
    ~AsyncLogging() {
        if (m_is_running) {
            stop();
        }
    };

    void append(const char* logline, size_t len);

    void start();

    void stop();

private:
    // main logic for async logging. Will be passed to m_thread initializaion.
    // Will be invoked at ThreadData::run_in_thread().
    void thread_func();

    using Buffer = FixedBuffer<STREAM_LARGE_BUF_SIZE>;
    using BufferPtr = std::shared_ptr<Buffer>;
    using BufferVector = std::vector<BufferPtr>;

    bool m_is_running { false };
    const int m_flush_buf_timeout;
    std::string m_filename;

    Thread m_thread{[this]()->void {this->thread_func();}, "Logging"};
    // 注意这里的 mutable 是合理的，试想一下，一个成员读操作应该是const的，但是需要加锁修改。
    mutable Mutex m_mutex{};
    Condition m_cond{m_mutex};

    BufferPtr m_cur_buf{new Buffer};
    BufferPtr m_next_buf{new Buffer};

    // Buffer pool
    BufferVector m_buffers;

    CountBarrier m_barrier{1};  // different from the Thread`s barrier.
};
