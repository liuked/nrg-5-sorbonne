#ifndef __LOG_H__
#define __LOG_H__

#include "config.h"
#include <stdio.h>

#if defined(LOG_DEBUG_ON)
    #define LOG_DEBUG(fmt, ...)
#else
    #define LOG_DEBUG(fmt, ...)
#endif


#if defined(LOG_INFO_ON)
    #define LOG_INFO(fmt, ...) fprintf(stdout, "[%s:%s:%d]: " fmt "\n", __FILE__, __func__, __LINE__, __VA_ARGS__)
#else
    #define LOG_INFO(fmt, ...)
#endif


#endif
