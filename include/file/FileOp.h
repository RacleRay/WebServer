#pragma once

#include <string>

#include "Config.h"
#include "noncopyable.h"


class FileOpBase: private Noncopyable {
public:
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