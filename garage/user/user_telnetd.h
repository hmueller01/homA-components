/**
 * @file
 * @brief Telnet server (telnetd)
 * @author Holger Mueller
 */
#ifndef __USER_TELNETD_H__
#define __USER_TELNETD_H__

// C++ wrapper
#ifdef __cplusplus
extern "C" {
#endif
// put C includes inside here to avoid undefined references by linker.
#include <c_types.h>
#ifdef __cplusplus
}
#endif

#define TELNETD_TIMEOUT 3600
#define REC_BUF_LEN 1024
#define PROMPT "> "

void Log__(const char *data);
bool TelnetdActive(void);
void Telnetd_Init(int port);

#endif // __USER_TELNETD_H__
