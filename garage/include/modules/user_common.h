/**
 * @file
 * @brief Common user functions and definitions
 * $Id: user_common.h 16 2015-10-29 21:49:45Z hmueller $
 */
#ifndef __USER_COMMON_H__
#define __USER_COMMON_H__

#include <c_types.h>
#include <osapi.h>
#include "user_config.h"

#ifndef INFO
#define INFO(format, ...) os_printf("I " format, ## __VA_ARGS__)
#endif
#ifndef ERROR
#define ERROR(format, ...) os_printf("E " format, ## __VA_ARGS__)
#endif
#ifdef DEBUG_ON
#  ifndef DEBUG
#define DEBUG(format, ...) os_printf("D " format, ## __VA_ARGS__)
#  endif
#else
#  ifndef DEBUG
#define DEBUG(format, ...)
#  endif
#endif

#define CRLF "\r\n"


char *ftoa(char *s, float f);
char *itoa(char *s, uint16_t i);

#endif /* __USER_COMMON_H__ */
