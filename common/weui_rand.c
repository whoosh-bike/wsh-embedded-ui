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

u32 WeUi_Rand_Hash(u32 x, u32 y, u32 salt) {
	u32 hash = x * 73856093u ^ y * 19349663u ^ salt * 83492791u;
	hash ^= hash >> 13;
	hash *= 0x5bd1e995u;
	hash ^= hash >> 15;

	return hash;
}
