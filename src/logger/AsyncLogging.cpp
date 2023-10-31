#include <cassert>
#include <cstdio>
#include <functional>
#include <unistd.h>

#include "AsyncLogging.h"
#include "LogFile.h"


constexpr int INIT_BUF_VEC_SIZE = 16;

AsyncLogging::AsyncLogging(const std::string& filename, int flush_buf_timeout)
    : m_filename(filename),
      m_flush_buf_timeout(flush_buf_timeout) {
    assert(m_filename.size() > 1);
    assert(m_flush_buf_timeout > 0);

    m_buffers.reserve(INIT_BUF_VEC_SIZE);
}


void AsyncLogging::append(const char* logline, size_t len) {
    MutexGuard lock(m_mutex);

    if (m_cur_buf->available_length() > len) {
        m_cur_buf->append(logline, len);
        return;
    }

    // switch to a new buffer, when current buffer is full
    m_buffers.push_back(m_cur_buf);
    m_cur_buf.reset();  // countdwon the reference count. vector<BufferPtr>> kept a reference count.

    if (m_next_buf) {
        m_cur_buf = m_next_buf;
    } else {
        m_cur_buf.reset(new Buffer);
    }

    // write to new buffer
    m_cur_buf->append(logline, len);
    m_cond.notify();  // when buffer is m_buffers is not empty(), notify the wait_for_seconds() in thread_func().
}


void AsyncLogging::start() {
    m_is_running = true;
    m_thread.start();
    m_barrier.wait();
}


void AsyncLogging::stop() {
    m_cond.notify();
    m_thread.join();
    m_is_running = false;
}


void AsyncLogging::thread_func() {
    assert(m_is_running == true);

    m_barrier.countdown();  // notify the started wait() in AsyncLogging::start().

    LogFile output_file(m_filename);

    BufferVector buffers_to_write;
    buffers_to_write.reserve(INIT_BUF_VEC_SIZE);

    // 这个缓存池的实现有点搓，每个缓冲区直接设计一个标记，而不是判断缓存池是否为空
    // 这样申请内存、释放内存的开销会少不少
    while (m_is_running) {
        assert(buffers_to_write.empty());

        {
            MutexGuard lock(m_mutex);
            while (m_buffers.empty()) {
                // wait until m_buffers is not empty(), or time out
                m_cond.wait_for_seconds(m_flush_buf_timeout);
            }
            // 此时 m_buffers 缓存池中有一个满的 buffer，再将当前未满的 buffer 中的内容
            // 追加到 m_buffers 缓存池中
            // 或者，m_buffers 缓存池中是 time out 退出，push_back 的是第一个 buffer
            // 准备写入 LogFile 文件中
            m_buffers.push_back(m_cur_buf);
            m_cur_buf.reset(new Buffer);

            if (!m_next_buf) {
                m_next_buf.reset(new Buffer);
            }

            buffers_to_write.swap(m_buffers);  // m_buffers become empty
        }

        assert(!buffers_to_write.empty());
        
        for (const auto& buffer: buffers_to_write) {
            output_file.append(buffer->data(), buffer->length());
        }

        buffers_to_write.clear();
        output_file.flush();
    }

    output_file.flush();
}