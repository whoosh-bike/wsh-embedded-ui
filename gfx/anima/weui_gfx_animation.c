#include "weui_gfx_animation.h"
#include "log.h"
#include "weui_fs.h"
#include "weui_gfx_base.h"
#include "weui_gfx_image.h"

#if WEUI_USE_COMPONENT__IMAGE_COMPRESS
#include "weui_gfx_packet.h"
#endif

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE  0	 //default 0
#define LOCAL_DEBUG_BBOXES_ENABLE 0	 //default 0
#endif

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT LOG
#else /* DEBUG_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* DEBUG_ENABLE */

void WeUi_Gfx_Animation_InitStruct(WeUi_Gfx_Animation_t* animaObj) {
	if (!animaObj)
		return;

	animaObj->StorageType = STORAGE_TYPE_INT_FLASH;
	memset((void*)&(animaObj->Storage), 0, sizeof(WeUi_Gfx_Storage_t));
	animaObj->Storage.GraphicsType = GRAPHICS_TYPE_ANIMA;

	animaObj->FrameSize = 0;
	animaObj->FramesNum = 0;

#if WEUI_USE_COMPONENT__IMAGE_COMPRESS
	memset(&animaObj->Packet, 0, sizeof(WeUi_Gfx_Packet_t));
#endif
}

RET_STATE_t WeUi_Gfx_Animation_InitObject(WeUi_Gfx_Animation_t* animaObj,
										  WEUI_STORAGE_TYPE_t storageType, ErrorCounter_t* errCnt,
										  ...) {
	if (!animaObj || storageType == STORAGE_TYPE_UNKNOWN)
		return RET_STATE_ERR_PARAM;

	va_list args;
	WeUi_Gfx_Animation_InitStruct(animaObj);
	animaObj->StorageType = storageType;

	switch (storageType) {
		case STORAGE_TYPE_EXT_FLASH:
			va_start(args, errCnt);
			char* fName = va_arg(args, char*);
			va_end(args);

			if (!fName)
				return RET_STATE_ERR_PARAM;

			RET_STATE_t fetchState = WeUi_Fs_Storage_Fetch(fName, &(animaObj->Storage));
			if (fetchState != RET_STATE_SUCCESS) {
				if (errCnt)
					errCnt->Err++;

				LOCAL_DEBUG_PRINT("Anima object with name %s wasn't init", fName);
				return fetchState;
			}
			break;
		case STORAGE_TYPE_RAM:
		case STORAGE_TYPE_INT_FLASH:
		default:
			va_start(args, errCnt);
			u8* animaAddr = va_arg(args, u8*);
			va_end(args);

			if (!animaAddr)
				return RET_STATE_ERR_PARAM;

			char jsonBuff[WEUI_FS_JSON_MAX_LEN];
			memcpy((u8*)jsonBuff, animaAddr, sizeof(jsonBuff));

			u32 jsonLen = WeUi_Fs_Storage_FindJSON(jsonBuff, sizeof(jsonBuff));
			if (!jsonLen) {
				if (errCnt)
					errCnt->Err++;

				LOCAL_DEBUG_PRINT("No new data in mem 0x%x", animaAddr);
				return RET_STATE_ERR_EMPTY;
			}

			animaObj->Storage.Addr = (WeUi_Gfx_StorageAddr_t)animaAddr + jsonLen;
			if (!WeUi_Fs_Storage_ParseJSON(jsonBuff, jsonLen, &animaObj->Storage)) {
				if (errCnt)
					errCnt->Err++;

				LOCAL_DEBUG_PRINT("No JSON in mem 0x%x", animaAddr);
				return RET_STATE_ERR_EMPTY;
			}

			break;
	}

	LOCAL_DEBUG_PRINT("Animation object \"%s\" inited: %d, %d, %d, %d, 0x%x",
					  animaObj->Storage.Name, animaObj->Storage.GraphicsType,
					  animaObj->Storage.Width, animaObj->Storage.Height, animaObj->Storage.Size,
					  animaObj->Storage.Addr);

	animaObj->FrameSize =
		animaObj->Storage.Width * animaObj->Storage.Height * WEUI_IMG_STORAGE_COLOR_SIZE;
	if (!animaObj->FrameSize) {
		if (errCnt)
			errCnt->Err++;

		LOCAL_DEBUG_PRINT("Anima object \"%s\" has no frame size", animaObj->Storage.Name);
		return RET_STATE_ERR_PARAM;
	}

#if WEUI_USE_COMPONENT__IMAGE_COMPRESS
	if (animaObj->Storage.IsCompressed) {
		if (WeUi_Gfx_Packet_Init(&animaObj->Packet, animaObj->Storage.Addr, animaObj->Storage.Size,
								 animaObj->StorageType) != RET_STATE_SUCCESS) {
			LOCAL_DEBUG_PRINT("WeUi_Gfx_Packet_Init: FAILED");
			return RET_STATE_ERROR;
		}

		animaObj->FramesNum = animaObj->Packet.BlocksNum;
	} else
#endif
		animaObj->FramesNum = animaObj->Storage.Size / animaObj->FrameSize;

	if (errCnt)
		errCnt->Luck++;

	LOCAL_DEBUG_PRINT("Storage type: %d, frame size: %d, frames num: %d", animaObj->StorageType,
					  animaObj->FrameSize, animaObj->FramesNum);

	return RET_STATE_SUCCESS;
}

void WeUi_Gfx_Animation_DrawFrame(u32 frame, WeUi_Gfx_Point_t* refPt,
								  WeUi_Gfx_Animation_t* animaObj,
								  WeUi_Gfx_ImageTransform_t* imageTransfrom, bool reverse) {
	if (!refPt || !animaObj || !animaObj->FramesNum)
		return;

	frame %= animaObj->FramesNum;
	if (reverse)
		frame = animaObj->FramesNum - frame - 1;

	WeUi_Gfx_Packet_Block_t animationFrame;

#if WEUI_USE_COMPONENT__IMAGE_COMPRESS
	if (animaObj->Storage.IsCompressed)
		WeUi_Gfx_Packet_BlockDescrByIdx_Get(&animaObj->Packet, &animationFrame, frame);
	else
#endif
	{
		animationFrame.Addr = animaObj->Storage.Addr + frame * animaObj->FrameSize;
		animationFrame.Size = animaObj->FrameSize;
	}

	WeUi_Gfx_Image_t imgObj;
	WeUi_Gfx_Image_InitStruct(&imgObj);
	imgObj.Storage.StorageType	= animaObj->StorageType;
	imgObj.Storage.IsCompressed = animaObj->Storage.IsCompressed;
	imgObj.Storage.Addr			= animationFrame.Addr;
	imgObj.Storage.Size			= animationFrame.Size;
	imgObj.Storage.Width		= animaObj->Storage.Width;
	imgObj.Storage.Height		= animaObj->Storage.Height;
	if (imageTransfrom)
		imgObj.Transform = *imageTransfrom;
	WeUi_Gfx_Image_Draw(refPt, &imgObj);

	LOCAL_DEBUG_PRINT("Frame %d, of %d; addr: 0x%x", frame, animaObj->FramesNum,
					  imgObj.Storage.Addr);

#if LOCAL_DEBUG_BBOXES_ENABLE
	WeUi_Gfx_Rect_t bboxRect;
	WeUi_Gfx_Rect_InitStruct(&bboxRect);
	bboxRect.PtOne.X = refPt->X;
	bboxRect.PtOne.Y = refPt->Y;
	bboxRect.Width	 = imgObj.Width;
	bboxRect.Height	 = imgObj.Height;
	bboxRect.Color.R = WEUI_COLOR_CHANNEL_MAX;
	WeUi_Gfx_RectEdges_Draw(&bboxRect);
#endif /* LOCAL_DEBUG_BBOXES_ENABLE */
}
