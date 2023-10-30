#include <cassert>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "FileOp.h"

FileOpBase::FileOpBase(const std::string& filename)
    : m_fp(fopen(filename.c_str(), "ae")) {
    // e : create if not exist
    setbuffer(m_fp, m_buffer, sizeof(m_buffer));
}


FileOpBase::~FileOpBase() {
    (void)fclose(m_fp);
}


void FileOpBase::append(const char* logline, size_t len) {
    size_t n_total = this->write(logline, len);
    size_t remain = len - n_total;

    while (remain > 0) {
        size_t n_rewrite = this->write(logline + n_total, remain);
        if (n_rewrite == 0) {
            if (ferror(m_fp)) {
                (void)fprintf(stderr, "FileOpBase::append() : failed() \n");
                break;
            }
        }
        n_total += n_rewrite;
        remain -= n_total;
    }
}


void FileOpBase::flush() {
    (void)fflush(m_fp);
}


size_t FileOpBase::write(const char* logline, size_t len) {
    // faster version when locking. Canncelation point without __THROW wrapper.
    return fwrite_unlocked(logline, 1, len, m_fp);
}