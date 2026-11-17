#ifndef __WEUI_COMMON_TYPES_H
#define __WEUI_COMMON_TYPES_H

#include "mathlib_common.h"

/* M_PI is a double, and M_PIf is a glibc extension newlib does not have */
#define WEUI_PI_F 3.14159265f

// =================== HSB color bounds ===================

#define WEUI_COLOR_CHANNEL_MIN 0x00
#define WEUI_COLOR_CHANNEL_MAX 0xFF

#define WEUI_COLOR_HSB_MAX_HUE 359.9f
#define WEUI_COLOR_HSB_MIN_HUE 0.0f
#define WEUI_COLOR_HSB_MAX_SAT 100.0f
#define WEUI_COLOR_HSB_MIN_SAT 0.0f
#define WEUI_COLOR_HSB_MAX_BRI 100.0f
#define WEUI_COLOR_HSB_MIN_BRI 0.0f

#define WEUI_COLOR_HSB_FULL_TURN 360.0f

#define WEUI_COLOR_GET_HUE(h)	 ((h) * (30.0f))
#define WEUI_COLOR_CUT_HUE(h, m) (fmodf(h, m))

// =================== Named colors ===================

#define WEUI_COLOR_TABLE()         \
	X_ENTRY(WEUI_COLOR_RED)        \
	X_ENTRY(WEUI_COLOR_ORANGE)     \
	X_ENTRY(WEUI_COLOR_YELLOW)     \
	X_ENTRY(WEUI_COLOR_CHARTREUSE) \
	X_ENTRY(WEUI_COLOR_GREEN)      \
	X_ENTRY(WEUI_COLOR_EMERALD)    \
	X_ENTRY(WEUI_COLOR_CYAN)       \
	X_ENTRY(WEUI_COLOR_AZURE)      \
	X_ENTRY(WEUI_COLOR_BLUE)       \
	X_ENTRY(WEUI_COLOR_VIOLET)     \
	X_ENTRY(WEUI_COLOR_PURPLE)     \
	X_ENTRY(WEUI_COLOR_CRIMSON)

#define X_ENTRY(enum) enum,
typedef enum { WEUI_COLOR_TABLE() WEUI_COLOR_ENUM_SIZE } WEUI_COLOR_t;
#undef X_ENTRY

// =================== Color scheme types ===================

/**
 * @brief HSB color
 * @param Hue ∈ [0.0; 360.0)
 * @param Sat ∈ [0.0; 100.0]
 * @param Bri ∈ [0.0; 100.0]
 */
typedef struct {
	float Hue;
	float Sat;
	float Bri;
} WeUi_ColorSchemeHsb_t;

/**
 * @brief RGB color
 * @param Red   ∈ [0.0; 1.0]
 * @param Green ∈ [0.0; 1.0]
 * @param Blue  ∈ [0.0; 1.0]
 */
typedef struct {
	float Red;
	float Green;
	float Blue;
} WeUi_ColorSchemeRgb_t;

#endif /* __WEUI_COMMON_TYPES_H */
