#include "weui_ind_layout.h"

static const Point2D_t* LayoutMask = NULL;
static u16 LayoutLen			   = 0;

void WeUi_Ind_LedsLayout_SetMask(const Point2D_t* pMask, u16 len) {
	LayoutMask = pMask;
	LayoutLen  = len;
}

Point2D_t WeUi_Ind_LedsLayout_GetPt(u16 idx) {
	Point2D_t pt = {
		.X = 0.0f,
		.Y = 0.0f,
	};

	if (LayoutMask && idx < LayoutLen)
		pt = LayoutMask[idx];

	return pt;
}
