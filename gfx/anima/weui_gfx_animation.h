#ifndef __WEUI_GFX_ANIMATION_H
#define __WEUI_GFX_ANIMATION_H

#include "weui_gfx_types.h"

/**
 * @brief Init WeUi_Gfx_Animation_t structure with default values
 * 
 * @param animaObj animation object pointer
 */
void WeUi_Gfx_Animation_InitStruct(WeUi_Gfx_Animation_t* animaObj);

/**
 * @brief Init animation object from storage
 * 
 * @param animaObj animation object pointer
 * @param storageType storage type (ram, int_flash, ext_flash)
 * @param errCnt optional error counter 
 * @param fName filename if storage is external flash
 * @retval RET_STATE_ERR_PARAM wrong input parameter
 * @retval RET_STATE_SUCCESS initialization was successfull
 * @retval RET_STATE_ERR_EMPTY there are no valid storage for data extraction
 */
RET_STATE_t WeUi_Gfx_Animation_InitObject(WeUi_Gfx_Animation_t* animaObj,
										  WEUI_STORAGE_TYPE_t storageType, ErrorCounter_t* errCnt,
										  ...);

/**
 * @brief Draw animation object in reference XY point
 * 
 * @param refPt XY point coord structure pointer
 * @param animaObj animation object pointer to draw
 * @param frame frame to draw from animation frames flow
 */
void WeUi_Gfx_Animation_DrawFrame(u32 frame, WeUi_Gfx_Point_t* refPt,
								  WeUi_Gfx_Animation_t* animaObj,
								  WeUi_Gfx_ImageTransform_t* imageTransfrom, bool reverse);

#endif /* __WEUI_GFX_ANIMATION_H */
