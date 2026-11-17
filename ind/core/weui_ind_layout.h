#ifndef __WEUI_IND_LAYOUT_H
#define __WEUI_IND_LAYOUT_H

#include "weui_ind_types.h"

/** @brief Register the LED grid layout mask. pMask must point to an array of 2D coordinates
 *         describing the physical position of each LED as routed on the PCB. */
void WeUi_Ind_LedsLayout_SetMask(const Point2D_t* pMask, u16 len);

/** @brief Get the 2D grid coordinate of LED at idx. Returns {0, 0} if idx is out of range. */
Point2D_t WeUi_Ind_LedsLayout_GetPt(u16 idx);

#endif /* __WEUI_IND_LAYOUT_H */
