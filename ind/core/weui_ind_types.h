#ifndef __WEUI_IND_TYPES_H
#define __WEUI_IND_TYPES_H

#include "ind_cfg.h"
#include "weui_common_types.h"

typedef enum {
	WEUI_IND_BLINK_SPEED_DISABLE   = 0,
	WEUI_IND_BLINK_SPEED_VERY_FAST = 100,
	WEUI_IND_BLINK_SPEED_FAST	   = 300,
	WEUI_IND_BLINK_SPEED_MIDDLE	   = 600,
	WEUI_IND_BLINK_SPEED_SLOW	   = 2000,
	WEUI_IND_BLINK_SPEED_VERY_SLOW = 6000,
	WEUI_IND_BLINK_SPEED_ENABLE	   = SYS_MAX_TIMEOUT
} WEUI_IND_BLINK_SPEED_t;

typedef struct {
	const u16* Ptr;
	u16 Len;
} WeUi_Ind_VirtMap_t;

#endif /* __WEUI_IND_TYPES_H */
