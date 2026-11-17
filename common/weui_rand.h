#ifndef __WEUI_RAND_H
#define __WEUI_RAND_H

#include "weui_common_types.h"

/**
 * @brief Return a random integer in [min, max] (inclusive).
 */
s32 WeUi_Rand_Between(s32 min, s32 max);

/**
 * @brief Return a random hue value in [0, WEUI_COLOR_HSB_MAX_HUE).
 */
s32 WeUi_Rand_Hue(void);

/**
 * @brief Return a random boolean value.
 */
bool WeUi_Rand_Bool(void);

/**
 * @brief The same pseudo-random value for the same (x, y, salt) on every call - for a
 *        pattern that has to hold still from frame to frame. Another salt, another pattern.
 *        Inline: a dissolve takes one a pixel.
 */
static inline u32 WeUi_Rand_Hash(u32 x, u32 y, u32 salt) {
	u32 hash = x * 73856093u ^ y * 19349663u ^ salt * 83492791u;
	hash ^= hash >> 13;
	hash *= 0x5bd1e995u;
	hash ^= hash >> 15;

	return hash;
}

#endif /* __WEUI_RAND_H */
