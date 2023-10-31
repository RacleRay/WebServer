#pragma once

#define CONFIG_FILE "./config.conf"
// #define LOG_FILE "./server.log"

extern "C" {
int get_nthread();
int get_port();
void get_logfile(char *log_filename);
}