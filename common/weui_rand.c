#include "weui_rand.h"

s32 WeUi_Rand_Between(s32 min, s32 max) {
	if (min >= max)
		return min;
	return min + rand() % (max - min + 1);
}

s32 WeUi_Rand_Hue(void) {
	return WeUi_Rand_Between(0, (s32)WEUI_COLOR_HSB_MAX_HUE);
}

bool WeUi_Rand_Bool(void) {
	return (rand() & 1) != 0;
}
