#pragma once

#include <string>

#include "noncopyable.h"


// #define FILEOP_BUFFER_SIZE (64 * 1024)
constexpr int FILEOP_BUFFER_SIZE = 64 * 1024;


class FileOpBase: private Noncopyable {
public:
    // FileOpBase() = delete;
    explicit FileOpBase(const std::string& filename);
    ~FileOpBase();

    // write to file buffer
    void append(const char* logline, size_t len);
    // write to file storage
    void flush();

private:
    size_t write(const char* logline, size_t len);
    
    FILE* m_fp;
    char m_buffer[FILEOP_BUFFER_SIZE];
};