#ifndef DEFINES_HPP
#define DEFINES_HPP

#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <string>

#define LOG(fmt, ...)                    \
    fprintf(stdout, fmt, ##__VA_ARGS__); \
    fflush(stdout);

#define EXIT(msg)                                          \
    do                                                     \
    {                                                      \
        std::cerr << __FUNCTION__ << " : " << msg << '\n'; \
        exit(EXIT_FAILURE);                                \
    } while (0)

#define VK_CHECK(val)                  \
    do                                 \
    {                                  \
        if (val != VK_SUCCESS)         \
        {                              \
            assert(val == VK_SUCCESS); \
        }                              \
    } while (false)

#endif // DEFINES_HPP