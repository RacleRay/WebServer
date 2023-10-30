#pragma once

#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <string>

#include "LogStream.h"

class AsyncLogging;


class Logger {
public:
    Logger(const char* filename, int line);
    ~Logger();

    LogStream& stream() { return m_impl.m_stream; }
    static std::string get_log_file_name() { return m_log_filename; }

private:
    class Impl {
    public:
        Impl(const char* filename, int line);
        void format_time();

        int m_line;
        LogStream m_stream;
        std::string m_filename;
    };

    Impl m_impl;

    static std::string m_log_filename;
};


#define LOG Logger(__FILE__, __LINE__).stream()