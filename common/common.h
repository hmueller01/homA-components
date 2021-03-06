/**
 * @file
 * @brief Common ESP8266 user functions and definitions
 *
 * MIT License
 * Copyright (c) 2017 Holger Mueller
 */
#ifndef __COMMON_H__
#define __COMMON_H__

#include <c_types.h>
#include <osapi.h>
#include <ctype.h>
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

#define CR "\r"
#define LF "\n"
#define CRLF "\r\n"

#ifndef NAN
#define NAN __builtin_nanf("0x7fc00000")
#endif
#ifndef INFINITY
#define INFINITY __builtin_huge_valf()
#endif

#define strtolower(sptr) for (char *p = sptr; *p; ++p) *p = tolower(*p)

// calculate size (number of elements) of an array
#define ARRAYSIZE(array) ((int) (sizeof(array) / sizeof(*array)))

// helper macro to cycle through a list in a for loop
// see https://stackoverflow.com/questions/400951/does-c-have-a-foreach-loop-construct
#define EACH_LIST(item, list) \
	(__typeof(*(list)) *(item) = (list); (item); (item) = (item)->next)

// helper macro to cycle through an array in a for loop
#define EACH_ARRAY(item, array) \
	(__typeof(*(array)) *p = (array), (item) = *p; p < &((array)[ARRAYSIZE(array)]); p++, (item) = *p)


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

char *ftoa(char *s, float f);
char *itoa(char *s, uint16_t i);
double atof(const char *s);
float expf(float x);
float logf(float x);
float powf(float x, float y);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* __COMMON_H__ */
