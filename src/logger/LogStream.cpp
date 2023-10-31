#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

#include "LogStream.h"


LogStream& LogStream::operator<<(bool val) {
    m_buffer.append(val ? "1" : "0", 1);
    return *this;
}

LogStream& LogStream::operator<<(char val) {
    m_buffer.append(&val, 1);
    return *this;
}

LogStream& LogStream::operator<<(const unsigned char val) {
    return operator<<(static_cast<char>(val));
}

LogStream& LogStream::operator<<(const char* str) {
    if (str) {
        m_buffer.append(str, strlen(str));
    } else {
        m_buffer.append("(null)", 6);
    }
    return *this;
}

LogStream& LogStream::operator<<(const unsigned char* str) {
    return operator<<(reinterpret_cast<const char*>(str));
}

LogStream& LogStream::operator<<(const std::string& str) {
    m_buffer.append(str.c_str(), str.size());
    return *this;
}

LogStream& LogStream::operator<<(short val) {
    return operator<<(static_cast<int>(val));
}

LogStream& LogStream::operator<<(unsigned short val) {
    return operator<<(static_cast<unsigned int>(val));
}

LogStream& LogStream::operator<<(int val) {
    format_integer(val);
    return *this;
}

LogStream& LogStream::operator<<(unsigned int val) {
    format_integer(val);
    return *this;
}

LogStream& LogStream::operator<<(long val) {
    format_integer(val);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long val) {
    format_integer(val);
    return *this;
}

LogStream& LogStream::operator<<(long long val) {
    format_integer(val);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long long val) {
    format_integer(val);
    return *this;
}

LogStream& LogStream::operator<<(float val) {
    return operator<<(static_cast<double>(val));
}

LogStream& LogStream::operator<<(double val) {
    if (m_buffer.available_length() < min_append_size) {
        return *this;
    }

    size_t len = snprintf(m_buffer.current_ptr(), min_append_size, "%.12g", val);
    m_buffer.shift_from_current(len);
    return *this;
}

LogStream& LogStream::operator<<(long double val) {
    if (m_buffer.available_length() < min_append_size) {
        return *this;
    }

    size_t len = snprintf(m_buffer.current_ptr(), min_append_size, "%.12Lg", val);
    m_buffer.shift_from_current(len);
    return *this;
}
