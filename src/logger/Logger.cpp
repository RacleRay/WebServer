#include <cassert>
#include <ctime>
#include <iostream>
#include <sys/time.h>

#include "AsyncLogging.h"
#include "Logger.h"
#include "ReadConfig.h"


// default log file path.
std::string Logger::m_log_filename = "./webserver.log"; 

namespace {
    AsyncLogging* asyncLogger;
    pthread_once_t once_control = PTHREAD_ONCE_INIT;

    void asynclog_once_init() {
        asyncLogger = new AsyncLogging(Logger::get_log_file_name());
        asyncLogger->start();
    }

    void write_to_buf(const char* message, size_t len) {
        // will initialize once, even if called multiple times
        pthread_once(&once_control, asynclog_once_init);

        asyncLogger->append(message, len);
    }
}  // namespace


/** 
 * @param code_filename will be Marco __FILE__
 * @param line will be Marco __LINE__
 */
Logger::Logger(const char* code_filename, int line): m_impl(code_filename, line) {}


Logger::~Logger() {
    m_impl.m_stream << " -- " << m_impl.m_code_filename << ":" << m_impl.m_line << "\n";
    const LogStream::Buffer& stream_buf{stream().get_buffer()};  // 4096 sized buffer
    
    // async log
    write_to_buf(stream_buf.data(), stream_buf.length());
}


Logger::Impl::Impl(const char* code_filename, int line)
    : m_code_filename(code_filename), m_line(line) {
    print_format_time();
}


void Logger::Impl::print_format_time() {
    struct timeval tv;
    time_t time;
    
    gettimeofday(&tv, nullptr);
    time = tv.tv_sec;
    struct tm* p_tm = localtime(&time);
    
    char time_str[32] = {0};
    (void)strftime(time_str, 32, "%Y-%m-%d %H:%M:%S \n", p_tm);

    m_stream << time_str;
}