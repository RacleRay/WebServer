#pragma once

#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <string>

#include "LogStream.h"

class AsyncLogging;


class Logger {
public:
    Logger(const char* code_filename, int line);
    ~Logger();

    LogStream& stream() { return m_impl.m_stream; }

    static void set_log_file_name(const std::string& log_filename) { 
        m_log_filename = log_filename; 
    }
    static std::string get_log_file_name() { return m_log_filename; }

private:
    class Impl {
    public:
        Impl(const char* code_filename, int line);
        void print_format_time();

        int m_line;
        std::string m_code_filename;

        LogStream m_stream{};
    };

    Impl m_impl;

    static std::string m_log_filename;
};


#define LOG Logger(__FILE__, __LINE__).stream()