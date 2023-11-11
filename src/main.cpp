#include <getopt.h>

#include <cstring>
#include <iostream>
#include <string>

#include "EventLoop.h"
#include "Logger.h"
#include "ReadConfig.h"
#include "Server.h"
#include "Debug.h"


int main(int argc, char* argv[]) {
    int nthread = get_nthread();
    int port = get_port();
    char logfile[32];
    get_logfile(logfile);

    int opt;
    const char* prompts = "n:l:p:";
    while ((opt = getopt(argc, argv, prompts)) != -1) {
        switch (opt) {
            case 'n': {
                nthread = strtol(optarg, nullptr, 10);
                break;
            }
            case 'l': {
                memcpy(logfile, optarg, 32);
                if (strlen(logfile) < 2 || optarg[0] != '/') {
                    std::cerr << "logfile path should start with \"/\"." << std::endl;
                    abort();
                }
            }
            case 'p': {
                port = strtol(optarg, nullptr, 10);
                break;
            }
            default: {
                break;
            }
        }
    }

    Logger::set_log_file_name(std::string(logfile));
    
    // init main loop
    EventLoop main_loop;
    // init server
    Server server(&main_loop, nthread, port);
    // start server
    PRINT("start server...");
    server.start();
    // run event loop
    main_loop.loop();

    return 0;
}