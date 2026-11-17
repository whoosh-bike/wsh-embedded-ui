#include "weui_gfx_image.h"
#include "log.h"

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT LOG
#else /* DEBUG_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* DEBUG_ENABLE */

#if WEUI_USE_COMPONENT__IMAGE

#include "def_macro.h"
#include "weui_fs.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_base.h"
#include "weui_gfx_transform.h"

#if WEUI_USE_COMPONENT__IMAGE_DITHER
#include "weui_gfx_dithering.h"
#endif /* WEUI_USE_COMPONENT__IMAGE_DITHER */

#if WEUI_USE_COMPONENT__IMAGE_COMPRESS
#include "lz4_wrapper.h"
#endif /* WEUI_USE_COMPONENT__IMAGE_COMPRESS */

static WeUi_Gfx_Color_t WEUI_IMAGES_OPS_DATA_PLACEMENT
	Image_RamBuffOne[WEUI_DISPLAY_WIDTH * WEUI_DISPLAY_HEIGHT];
static WeUi_Gfx_Color_t WEUI_IMAGES_OPS_DATA_PLACEMENT
	Image_RamBuffTwo[WEUI_DISPLAY_WIDTH * WEUI_DISPLAY_HEIGHT];

static u32 ImageBuffMaxLen = sizeof(Image_RamBuffOne);

/**
 * @brief Init WeUi_Gfx_ImageTransform_t structure with default values
 * 
 * @param pTransform image transform params pointer
 */
void WeUi_Gfx_Image_InitTransformStruct(WeUi_Gfx_ImageTransform_t* pTransform) {
	if (!pTransform)
		return;

	pTransform->Pos		  = IMG_POS_CORNER;
	pTransform->ColorMode = IMG_COLOR_MODE_NORMAL;
	pTransform->Flip	  = IMG_FLIP_NO;
	pTransform->Intersect = IMG_INTERSECT_FRONT_OPACITY_DISABLED;
	pTransform->Dithering = IMG_DITHERING_NO;
	WeUi_Gfx_Transform_InitImageColorAdjustStruct(pTransform);
}

/**
 * @brief Init WeUi_Gfx_Image_t structure with default values
 * 
 * @param pImgObj image object pointer
 */
void WeUi_Gfx_Image_InitStruct(WeUi_Gfx_Image_t* pImgObj) {
	if (!pImgObj)
		return;

	memset((void*)&(pImgObj->Storage), 0, sizeof(WeUi_Gfx_Storage_t));
	pImgObj->Storage.StorageType  = STORAGE_TYPE_UNKNOWN;
	pImgObj->Storage.GraphicsType = GRAPHICS_TYPE_IMAGE;
	pImgObj->ImageOrig			  = NULL;
	pImgObj->ImageAux			  = NULL;
	pImgObj->Width				  = 0;
	pImgObj->Height				  = 0;

	WeUi_Gfx_Image_InitTransformStruct(&pImgObj->Transform);
}

static void WeUi_Gfx_Image_SwapExtraBuffers(WeUi_Gfx_Image_t* pImgObj) {
	WeUi_Gfx_Color_t* pTmp = pImgObj->ImageOrig;
	pImgObj->ImageOrig	   = pImgObj->ImageAux;
	pImgObj->ImageAux	   = pTmp;
}

/**
 * @brief Bring the pixels into ImageOrig and set Width/Height to what it holds
 * @return false when the asset cannot be shown from the work buffers
 */
static bool WeUi_Gfx_Image_Load(WeUi_Gfx_Image_t* pImgObj) {
	const WeUi_Gfx_Storage_t* pcStorage = &pImgObj->Storage;
	u32 rawSize = (u32)pcStorage->Width * pcStorage->Height * sizeof(WeUi_Gfx_Color_t);

	pImgObj->Width	= pcStorage->Width;
	pImgObj->Height = pcStorage->Height;

	if (pcStorage->IsCompressed) {
#if WEUI_USE_COMPONENT__IMAGE_COMPRESS
		/* Compressed data only decodes whole, there is no crop to fall back on */
		if (pcStorage->Size > ImageBuffMaxLen || rawSize > ImageBuffMaxLen)
			return false;

		WeUi_Fs_Storage_Read((u8*)pImgObj->ImageOrig, pcStorage->StorageType, pcStorage->Addr,
							 pcStorage->Size);
		if (LZ4_Wrapper_BlockDecompress((u8*)pImgObj->ImageOrig, pcStorage->Size,
										(u8*)pImgObj->ImageAux, rawSize) != RET_STATE_SUCCESS)
			return false;

		WeUi_Gfx_Image_SwapExtraBuffers(pImgObj);
		return true;
#else  /* WEUI_USE_COMPONENT__IMAGE_COMPRESS */
		PANIC();
		return false;
#endif /* WEUI_USE_COMPONENT__IMAGE_COMPRESS */
	}

	if (rawSize <= ImageBuffMaxLen) {
		if (pcStorage->Size < rawSize)
			return false;

		WeUi_Fs_Storage_Read((u8*)pImgObj->ImageOrig, pcStorage->StorageType, pcStorage->Addr,
							 rawSize);
		return true;
	}

	/* Too big for the buffers: the top-left part a panel can show, read a row at a time
	 * since a stored row is longer than a cropped one */
	pImgObj->Width	= GET_MIN(pcStorage->Width, WEUI_DISPLAY_WIDTH);
	pImgObj->Height = GET_MIN(pcStorage->Height, WEUI_DISPLAY_HEIGHT);
	u32 rowSize		= pImgObj->Width * sizeof(WeUi_Gfx_Color_t);

	for (u32 y = 0; y < pImgObj->Height; y++) {
		WeUi_Fs_Storage_Read((u8*)pImgObj->ImageOrig + y * rowSize, pcStorage->StorageType,
							 pcStorage->Addr + y * pcStorage->Width * sizeof(WeUi_Gfx_Color_t),
							 rowSize);
	}

	return true;
}

/**
 * @brief Draw image object in reference XY point
 * 
 * @param pRefPt XY point coord structure pointer
 * @param pImgObj image object pointer to draw
 */
void WeUi_Gfx_Image_Draw(WeUi_Gfx_Point_t* pRefPt, WeUi_Gfx_Image_t* pImgObj) {
	if (!pRefPt || !pImgObj)
		return;

	pImgObj->ImageOrig = Image_RamBuffOne;
	pImgObj->ImageAux  = Image_RamBuffTwo;

	if (!WeUi_Gfx_Image_Load(pImgObj))
		return;

#if WEUI_USE_COMPONENT__IMAGE_DITHER
	if (pImgObj->Transform.Dithering == IMG_DITHERING_FLOYD_STEINBERG) {
		if (WeUi_Gfx_Dither_FloydSteinberg(pImgObj))
			WeUi_Gfx_Image_SwapExtraBuffers(pImgObj);
	}
#endif /* WEUI_USE_COMPONENT__IMAGE_DITHER */

	bool colorAdjustNeeded = (pImgObj->Transform.Exposure != WEUI_TRANSFORM_EXPOSURE_DEF ||
							  pImgObj->Transform.Brightness != WEUI_TRANSFORM_BRIGHTNESS_DEF ||
							  pImgObj->Transform.Contrast != WEUI_TRANSFORM_CONTRAST_DEF);

	WeUi_Gfx_ColorPoint_t cpt;
	for (u16 yImgIdx = 0; yImgIdx < pImgObj->Height; yImgIdx++) {
		for (u16 xImgIdx = 0; xImgIdx < pImgObj->Width; xImgIdx++) {
			u32 idx = 0;

			//TODO check for RGBA img (1 or WEUI_COLOR_SCHEME_MULTIPL?)
			switch (pImgObj->Transform.Flip) {
				case IMG_FLIP_VER:
					idx = yImgIdx * pImgObj->Width + (pImgObj->Width - xImgIdx - 1);
					break;
				case IMG_FLIP_HOR:
					idx = (pImgObj->Height - yImgIdx - 1) * pImgObj->Width + xImgIdx;
					break;
				case IMG_FLIP_VER_HOR:
					idx = (pImgObj->Height - yImgIdx - 1) * pImgObj->Width +
						  (pImgObj->Width - xImgIdx - 1);
					break;
				case IMG_FLIP_NO:
				default:
					idx = yImgIdx * pImgObj->Width + xImgIdx;
					break;
			}

			cpt.Color = pImgObj->ImageOrig[idx];

			if (colorAdjustNeeded) {
				cpt.Color.R = (WeUi_Gfx_ColorChannel_t)WeUi_Gfx_Transform_ColorAdjust(
					(float)cpt.Color.R, &pImgObj->Transform);
#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
				cpt.Color.G = (WeUi_Gfx_ColorChannel_t)WeUi_Gfx_Transform_ColorAdjust(
					(float)cpt.Color.G, &pImgObj->Transform);
				cpt.Color.B = (WeUi_Gfx_ColorChannel_t)WeUi_Gfx_Transform_ColorAdjust(
					(float)cpt.Color.B, &pImgObj->Transform);
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */
			}

			if (pImgObj->Transform.ColorMode == IMG_COLOR_MODE_REVERSE)
				cpt.Color = WeUi_Gfx_Transform_InvertColor(cpt.Color);

			cpt.Pt.X = pRefPt->X + xImgIdx;
			cpt.Pt.Y = pRefPt->Y + yImgIdx;

			switch (pImgObj->Transform.Pos) {
				case IMG_POS_CENTER_VER:
					cpt.Pt.X -= pImgObj->Width / 2;
					break;
				case IMG_POS_CENTER_HOR:
					cpt.Pt.Y -= pImgObj->Height / 2;
					break;
				case IMG_POS_CENTER_VER_HOR:
					cpt.Pt.X -= pImgObj->Width / 2;
					cpt.Pt.Y -= pImgObj->Height / 2;
					break;
				case IMG_POS_CORNER:
				default:
					break;
			}

#if defined(WEUI_DISPLAY_GRAY)
			WeUi_Gfx_ColorChannel_t frontColor = cpt.Color.R;
			switch (pImgObj->Transform.Intersect) {
				case IMG_INTERSECT_FRONT_OPACITY_ENABLED:
					/* Left as it is: fetched and redrawn, the stored pixel would go
					 * through the driver brightness LUT a second time */
					if (frontColor == WeUi_Gfx_Adapter_GetColor_Min())
						continue;
					break;
				case IMG_INTERSECT_STRONG_OVERRIDES_WEAK:
					WeUi_Gfx_Point_Fetch(&cpt);
					if (cpt.Color.R < frontColor)
						cpt.Color.R = frontColor;
					break;
				case IMG_INTERSECT_SUMM: {
					WeUi_Gfx_Point_Fetch(&cpt);
					u16 colorSumm = cpt.Color.R + frontColor;
					cpt.Color.R	  = (colorSumm > WeUi_Gfx_Adapter_GetColor_Max())
										? WeUi_Gfx_Adapter_GetColor_Max()
										: (WeUi_Gfx_ColorChannel_t)colorSumm;
					break;
				}
				case IMG_INTERSECT_DIFF: {
					WeUi_Gfx_Point_Fetch(&cpt);
					cpt.Color.R = ABS((s16)cpt.Color.R - (s16)frontColor);
					break;
				}
				case IMG_INTERSECT_FRONT_OPACITY_DISABLED:
				default:
					break;
			}

#elif defined(WEUI_DISPLAY_COLOR)
			WeUi_Gfx_Color_t frontColor = cpt.Color;
			switch (pImgObj->Transform.Intersect) {
				case IMG_INTERSECT_FRONT_OPACITY_ENABLED:
					if (frontColor.R == 0 && frontColor.G == 0 && frontColor.B == 0)
						continue;
					break;
				case IMG_INTERSECT_STRONG_OVERRIDES_WEAK: {
					WeUi_Gfx_Point_Fetch(&cpt);
					if (cpt.Color.R < frontColor.R)
						cpt.Color.R = frontColor.R;
					if (cpt.Color.G < frontColor.G)
						cpt.Color.G = frontColor.G;
					if (cpt.Color.B < frontColor.B)
						cpt.Color.B = frontColor.B;
					break;
				}
				case IMG_INTERSECT_SUMM: {
					WeUi_Gfx_Point_Fetch(&cpt);
					u16 r		= (u16)cpt.Color.R + frontColor.R;
					u16 g		= (u16)cpt.Color.G + frontColor.G;
					u16 b		= (u16)cpt.Color.B + frontColor.B;
					cpt.Color.R = (r > WEUI_COLOR_CHANNEL_MAX) ? WEUI_COLOR_CHANNEL_MAX
															   : (WeUi_Gfx_ColorChannel_t)r;
					cpt.Color.G = (g > WEUI_COLOR_CHANNEL_MAX) ? WEUI_COLOR_CHANNEL_MAX
															   : (WeUi_Gfx_ColorChannel_t)g;
					cpt.Color.B = (b > WEUI_COLOR_CHANNEL_MAX) ? WEUI_COLOR_CHANNEL_MAX
															   : (WeUi_Gfx_ColorChannel_t)b;
					break;
				}
				case IMG_INTERSECT_DIFF: {
					WeUi_Gfx_Point_Fetch(&cpt);
					cpt.Color.R = ABS((s16)cpt.Color.R - (s16)frontColor.R);
					cpt.Color.G = ABS((s16)cpt.Color.G - (s16)frontColor.G);
					cpt.Color.B = ABS((s16)cpt.Color.B - (s16)frontColor.B);
					break;
				}
				case IMG_INTERSECT_FRONT_OPACITY_DISABLED:
				default:
					break;
			}
#endif /* WEUI_DISPLAY_COLOR */

			WeUi_Gfx_Point_Draw(&cpt);
		}
	}
}

#else  /* WEUI_USE_COMPONENT__IMAGE */
void WeUi_Gfx_Image_InitTransformStruct(WeUi_Gfx_ImageTransform_t* pTransform) {
}

void WeUi_Gfx_Image_InitStruct(WeUi_Gfx_Image_t* pImgObj) {
}

void WeUi_Gfx_Image_Draw(WeUi_Gfx_Point_t* pRefPt, WeUi_Gfx_Image_t* pImgObj) {
}
#endif /* WEUI_USE_COMPONENT__IMAGE */
