#include <cassert>
#include <cstdio>
#include <ctime>
#include <memory>
#include <utility>

#include "LogFile.h"
#include "FileOp.h"


LogFile::LogFile(const std::string& filename, int flush_interval)
    : m_filename(filename),
      m_flush_interval(flush_interval) { 
    m_file = std::make_unique<FileOpBase>(filename.c_str());
}


void LogFile::append(const char* logline, size_t len) {
    MutexGuard gurad(m_mutex);
    append_guarded(logline, len);
}


void LogFile::flush() {
    MutexGuard gurad(m_mutex);
    m_file->flush();
}


void LogFile::append_guarded(const char* logline, size_t len) {
    m_file->append(logline, len);
    ++m_count;
    if (m_count >= m_flush_interval) {
        m_count = 0;
        m_file->flush();
    }
}