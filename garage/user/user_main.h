/**
 * @file
 * @brief Header of main application
 */
#ifndef __USER_MAIN_H__
#define __USER_MAIN_H__

// C++ wrapper
#ifdef __cplusplus
extern "C" {
#endif
// put C includes inside here to avoid undefined references by linker.
#include <c_types.h>
#ifdef __cplusplus
}
#endif

extern uint16_t m_cistern_timeout_time;
extern uint16_t m_cistern_timeout_cnt;
extern uint8_t m_cistern_level;
extern bool m_cistern_status;
extern bool m_door_status;


#ifdef __cplusplus
extern "C" {
#endif
	uint32 user_rf_cal_sector_set(void);
	void user_init(void);
#ifdef __cplusplus
}
#endif


#endif /* __USER_MAIN_H__ */
