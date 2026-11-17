#ifndef __WEUI_IND_GRADIENT_H
#define __WEUI_IND_GRADIENT_H

#include "weui_ind_types.h"

#define WEUI_IND_GRADIENT_MAP_LIST  \
	X_ENTRY(GRAD_VIRIDIS)           \
	X_ENTRY(GRAD_PLASMA)            \
	X_ENTRY(GRAD_COOL)              \
	X_ENTRY(GRAD_HSV)               \
	X_ENTRY(GRAD_BRG)               \
	X_ENTRY(GRAD_TURBO)             \
	X_ENTRY(GRAD_SBER)              \
	X_ENTRY(GRAD_PINKY)             \
	X_ENTRY(GRAD_BATT)              \
	X_ENTRY(GRAD_BOOK)              \
	X_ENTRY(GRAD_IN_USE)            \
	X_ENTRY(GRAD_IN_USE_THROTT_OFF) \
	X_ENTRY(GRAD_IN_USE_BRAKE_ON)

typedef struct {
	const WeUi_ColorSchemeHsb_t* Ptr;
	u16 Len;
} WeUi_Ind_Grad_t;

typedef enum {
#define X_ENTRY(name) WEUI_IND_##name,
	WEUI_IND_GRADIENT_MAP_LIST
#undef X_ENTRY
		WEUI_IND_GRAD_ENUM_SIZE
} WEUI_IND_GRAD_t;

WeUi_ColorSchemeHsb_t WeUi_Ind_Gradient_GetColor(WEUI_IND_GRAD_t grad, u32 idx);
u32 WeUi_Ind_Gradient_GetLen(WEUI_IND_GRAD_t grad);
u16 WeUi_Ind_Gradient_GetIdxByNorm(WEUI_IND_GRAD_t grad, float normIdx);
WeUi_ColorSchemeHsb_t WeUi_Ind_Gradient_GetColorByNorm(WEUI_IND_GRAD_t grad, float normIdx);
u16 WeUi_Ind_Gradient_GetWrapIdxByCircle(WEUI_IND_GRAD_t grad, float circleIdx);
WeUi_ColorSchemeHsb_t WeUi_Ind_Gradient_GetWrapColorByCircle(WEUI_IND_GRAD_t grad, float circleIdx);

#endif /* __WEUI_IND_GRADIENT_H */
