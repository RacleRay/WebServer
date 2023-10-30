#include <cassert>
#include <ctime>
#include <iostream>
#include <sys/time.h>

#include "Logger.h"


/** 
 * @param filename will be Marco __FILE__
 * @param line will be Marco __LINE__
 */
Logger::Logger(const char* filename, int line): m_impl(filename, line) {}

Logger::~Logger() {
    m_impl.m_stream << " -- " << m_impl.m_filename << ":" << m_impl.m_line << std::endl;
    const LogStream::Buffer& buf(m_stream().buffer());
    // async log
}