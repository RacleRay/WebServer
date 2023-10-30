#pragma once

#define MAX_KEYWORD_LEN 16
#define MAX_FORMAT_LEN 16
#define MAX_CONFIG_LINE 512

#define MAXLINE 4096

#define FILEOP_BUFFER_SIZE (64 * 1024)

#define CONFIG_FILE "./config.conf"
#define LOG_FILE "./server.log"

extern "C" {
int get_nthread();
int get_port();
}