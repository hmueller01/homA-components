/**
 * @file
 * @brief Missing ESP8266 SDK function prototypes and defines. GCC will warn on these if in c99 mode.
 * @author Holger Mueller
 * see also https://github.com/popsodav/esp8266-lpd6803/blob/master/include/espmissingincludes.h
 */
#ifndef __ETS_MISSING_H__
#define __ETS_MISSING_H__

#include <c_types.h>
#include <ets_sys.h>
#include <stdint.h>
#include <version.h>


#ifdef __cplusplus
// fix bug in c_types.h include, not defining BOOL, TRUE, FALSE for c++
#ifndef BOOL
#	define BOOL bool
#endif // BOOL
#ifndef TRUE
#	define TRUE true
#endif // TRUE
#ifndef FALSE
#	define FALSE false
#endif // FALSE
#endif // __cplusplus


// Missing ESP8266 SDK function prototypes. GCC will warn on these if in c99 mode.
#if ESP_SDK_VERSION_NUMBER < 0x030004
void ets_isr_mask(unsigned intr);
void ets_isr_unmask(unsigned intr);
#endif // ESP_SDK_VERSION_NUMBER

#endif // __ETS_MISSING_H__
