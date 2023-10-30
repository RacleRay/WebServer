#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <syslog.h>

#include <Config.h>

extern "C" {

    static int log_to_stderr = 1;

    static void log_doit(
        int errnoflag, int error, int priority, const char *fmt, va_list ap);

    void log_msg(const char *fmt, ...) {
        va_list ap;

        va_start(ap, fmt);
        log_doit(0, 0, LOG_ERR, fmt, ap);
        va_end(ap);
    }

    void log_sys(const char *fmt, ...) {
        va_list ap;

        va_start(ap, fmt);
        log_doit(1, errno, LOG_ERR, fmt, ap);
        va_end(ap);
        exit(2);
    }

    static void log_doit(
        int errnoflag, int error, int priority, const char *fmt, va_list ap) {
        char buf[MAXLINE];

        vsnprintf(buf, MAXLINE - 1, fmt, ap);
        if (errnoflag)
            snprintf(
                buf + strlen(buf), MAXLINE - strlen(buf) - 1, ": %s",
                strerror(error));
        strcat(buf, "\n");

        // interactive mode
        if (log_to_stderr) {
            fflush(stdout);
            fputs(buf, stderr);
            fflush(stderr);
        } else { // daemon mode
            syslog(priority, "%s", buf);
        }
    }

    static char *scan_configfile(const char *keyword) {
        int n, match;
        char keybuf[MAX_KEYWORD_LEN], pattern[MAX_FORMAT_LEN];
        char line[MAX_CONFIG_LINE];
        static char valbuf[MAX_CONFIG_LINE];
        FILE *fp;

        if ((fp = fopen(CONFIG_FILE, "r")) == NULL) {
            log_sys("scan_configfile: Can't open config file %s", CONFIG_FILE);
        }

        sprintf(
            pattern, "%%%ds %%%ds", MAX_KEYWORD_LEN - 1, MAX_FORMAT_LEN - 1);

        match = 0;
        while (fgets(line, MAX_CONFIG_LINE, fp) != NULL) {
            n = sscanf(line, pattern, keybuf, valbuf);
            if (n == 2 && strcmp(keyword, keybuf) == 0) {
                match = 1;
                break;
            }
        }

        fclose(fp);
        if (match) { return (valbuf); }
        return NULL;
    }

    int get_nthread() {
        return strtol(scan_configfile("THREADNUMBER"), NULL, 10);
    }

    int get_port() {
        return strtol(scan_configfile("PORT"), NULL, 10);
    }
}