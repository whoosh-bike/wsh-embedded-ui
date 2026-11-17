#ifndef __LOG_H
#define __LOG_H

/* The firmware's logger API over Sim_Log(): the tests drop the lines, the lab shows them
 * against the frame they were printed on */

typedef enum {
	LOG_LVL_TRACE = 0,
	LOG_LVL_DEBUG,
	LOG_LVL_INFO,
	LOG_LVL_WARNING,
	LOG_LVL_ERROR,
	LOG_LVL_CRITIC,
	LOG_LVL_DISABLE,
} LOG_LVL_t;

void Sim_Log(LOG_LVL_t lvl, const char* pcFmt, ...) __attribute__((format(printf, 2, 3)));

#define LOG(_f_, ...)				Sim_Log(LOG_LVL_INFO, (_f_), ##__VA_ARGS__)
#define LOG_COLOR(c, _f_, ...)		Sim_Log(LOG_LVL_INFO, (_f_), ##__VA_ARGS__)
#define LOG_LVL(l, _f_, ...)		Sim_Log((l), (_f_), ##__VA_ARGS__)
#define LOG_LVL_LOCAL(l, _f_, ...)	Sim_Log((l), (_f_), ##__VA_ARGS__)
#define LOG_RAW(_f_, ...)			Sim_Log(LOG_LVL_INFO, (_f_), ##__VA_ARGS__)
#define LOG_RAW_NL(_f_, ...)		Sim_Log(LOG_LVL_INFO, (_f_), ##__VA_ARGS__)
#define LOG_RAW_DIRECT(_f_, ...)	Sim_Log(LOG_LVL_INFO, (_f_), ##__VA_ARGS__)
#define LOG_RAW_DIRECT_NL(_f_, ...) Sim_Log(LOG_LVL_INFO, (_f_), ##__VA_ARGS__)

#endif /* __LOG_H */
