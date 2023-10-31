#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>

#include "noncopyable.h"


// TODO: 双缓冲区设计
constexpr int STREAM_SMALL_BUF_SIZE = 4096;
constexpr int STREAM_LARGE_BUF_SIZE = 4096 * 1000;

// For convert integer(maybe negetive) to string.
constexpr char digits[] = "9876543210123456789";
constexpr char* zero = const_cast<char*>(digits + 9);

class AsyncLogging;


template <int SIZE>
class FixedBuffer : private Noncopyable {
public:
    FixedBuffer() : m_cur_pos(m_data) {}
    ~FixedBuffer() = default;

    // Only append data when there is enough space in the buffer.
    void append(const char* data, size_t len) {
        if (available_length() > len) {
            memcpy(m_cur_pos, data, len);
            m_cur_pos += len;
        }
    }

    [[nodiscard]] const char* data() const { return m_data; }

    // the actual content storage length.
    [[nodiscard]] size_t length() const { return static_cast<size_t>(m_cur_pos - begin()); }
    [[nodiscard]] size_t available_length() const { return static_cast<size_t>(end() - m_cur_pos); }

    char* current_ptr() { return m_cur_pos; }
    void shift_from_current(size_t len) { m_cur_pos += len; }

    void reset() { m_cur_pos = m_data; }
    // void bzero() { memset(static_cast<void*>(begin()), 0, SIZE); }

private:
    // [[nodiscard]], which is a C++11 feature. 
    // It indicates that the return value of this function should not be ignored by the caller. 
    [[nodiscard]] const char* begin() const { return m_data; }
    [[nodiscard]] const char* end() const { return m_data + SIZE; }

    char m_data[SIZE]{0};
    char* m_cur_pos;
};


// Formatter
class LogStream : private Noncopyable {
public:
    using Buffer = FixedBuffer<STREAM_SMALL_BUF_SIZE>;
    
    LogStream& operator<<(bool);

    LogStream& operator<<(char);
    LogStream& operator<<(unsigned char);

    LogStream& operator<<(const char*);
    LogStream& operator<<(const unsigned char*);

    LogStream& operator<<(const std::string&);

    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(float);
    LogStream& operator<<(double);
    LogStream& operator<<(long double);

    void append(const char* data, size_t len) {
        m_buffer.append(data, len);
    }

    [[nodiscard]] const Buffer& get_buffer() const {
        return m_buffer;
    }
    
    void reset_buffer() { m_buffer.reset(); }

private:
    Buffer m_buffer;
    static constexpr int min_append_size = 32;

    // write integer to buffer string.
    template <typename T>
    void format_integer(T value) {
        // 空间不足直接不写入
        if (m_buffer.available_length() < min_append_size) {
            return;
        }

        size_t len = convert_int_to_string(m_buffer.current_ptr(), value);
        m_buffer.shift_from_current(len);
    }

    // return the length of converted string
    template <typename T>
    static size_t convert_int_to_string(char buf[], T value) {
        T temp = value;
        char* p = buf;

        do {
            int shift = static_cast<int>(temp % 10);
            temp /= 10;
            *p++ = zero[shift];  // * is prior to ++
        } while (temp != 0);

        if (value < 0) {
            *p++ = '-';
        }
        *p = '\0';

        std::reverse(buf, p);

        return p - buf;
    }
};
