#include <getopt.h>

#include <iostream>
#include <string>

#include <Config.h>


int main(int argc, char* argv[]) {
    int nthread = get_nthread();
    int port = get_port();
    std::string logfile = LOG_FILE;

    int opt;
    const char* prompts = "n:l:p:";
    while ((opt = getopt(argc, argv, prompts)) != -1) {
        switch (opt) {
            case 'n': {
                nthread = strtol(optarg, nullptr, 10);
                break;
            }
            case 'l': {
                logfile = optarg;
                if (logfile.size() < 2 || optarg[0] != '/') {
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

    // set logger

    // init main loop

    // init server

    // start server

    // run event loop

    return 0;
}