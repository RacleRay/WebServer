#pragma once

#include <iostream>

#ifdef __MY_DEBUG__
#define PRINT(msg) \
    do { \
        std::cout << "[" << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << "]: " << msg << std::endl; \
    } while(0)
#else
#define PRINT(msg) do {} while(0)
#endif