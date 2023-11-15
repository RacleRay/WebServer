#pragma once 

#include <memory>
#include <string>

#include "FileOp.h"
#include "Mutex.h"
#include "noncopyable.h"


class LogFile : Noncopyable {
public:
    explicit LogFile(const std::string& filename, int flush_interval = 512);
    ~LogFile() = default;

    void append(const char* logline, size_t len);
    void flush();

private:
    void append_guarded(const char* logline, size_t len);

    const std::string m_filename;
    const int m_flush_interval;
    int m_count { 0 };

    // TODO: unique_ptr ? no need 
    mutable Mutex m_mutex{};
    std::unique_ptr<FileOpBase> m_file;

    // 这只是该类的私有资源，直接在类实例中作为成员也可以
    // Mutex m_mutex{};
    // FileOpBase m_file;
};
