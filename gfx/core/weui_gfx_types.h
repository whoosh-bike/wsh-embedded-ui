#ifndef __WEUI_GFX_TYPES_H
#define __WEUI_GFX_TYPES_H

#include "weui_cfg.h"
#include "weui_cfg_check.h"
#include "weui_common_types.h"

typedef u8 WeUi_Gfx_ColorChannel_t;
typedef s16 WeUi_Gfx_PtPos_t;
typedef u16 WeUi_Gfx_ObjSize_t;

/** @brief A pixel as the driver stores it. Not a WeUi_Gfx_Color_t: never transform it. */
typedef u32 WeUi_Gfx_ColorRaw_t;

/** @brief An external flash offset, or a pointer to a RAM / internal flash asset */
typedef uintptr_t WeUi_Gfx_StorageAddr_t;

typedef WeUi_ColorSchemeRgb_t WeUi_Gfx_ColorSchemeRgb_t;
typedef WeUi_ColorSchemeHsb_t WeUi_Gfx_ColorSchemeHsb_t;

/**
 * @brief Simple 2D coord point
 */
typedef struct {
	WeUi_Gfx_PtPos_t X;
	WeUi_Gfx_PtPos_t Y;
} WeUi_Gfx_Point_t;

typedef struct {
	WeUi_Gfx_ColorChannel_t R;

#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
	WeUi_Gfx_ColorChannel_t G;
	WeUi_Gfx_ColorChannel_t B;
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */

#if WEUI_LIB_COLOR_ALPHA_CHANNEL
	WeUi_Gfx_ColorChannel_t A;
#endif /* WEUI_LIB_COLOR_ALPHA_CHANNEL */
} WeUi_Gfx_Color_t;

static_assert(sizeof(WeUi_Gfx_Color_t) == WEUI_LIB_COLOR_TOTAL_SIZE,
			  "WeUi_Gfx_Color_t does not match WEUI_LIB_COLOR_TOTAL_SIZE");

/**
 * @brief Simple 2D draw point
 */
typedef struct {
	WeUi_Gfx_Point_t Pt;
	WeUi_Gfx_Color_t Color;
} WeUi_Gfx_ColorPoint_t;

/**
 * @brief One usefull and polymorphic structure for 
 * different vector graphic objects
 */
typedef struct {
	WeUi_Gfx_Point_t PtOne;
	WeUi_Gfx_Point_t PtTwo;
	WeUi_Gfx_ObjSize_t Width;
	WeUi_Gfx_ObjSize_t Height;
	WeUi_Gfx_Color_t Color;
} WeUi_Gfx_Base_t;

typedef WeUi_Gfx_Base_t WeUi_Gfx_Rect_t;

typedef enum {
	WEUI_LINE_DIR_HORIZONTAL = 0,
	WEUI_LINE_DIR_VERTICAL	 = 1,
} WEUI_LINE_DIR_t;

typedef struct {
	WeUi_Gfx_Base_t Base;
	float Hatching; /**< Drawn share of each dash period ∈ [0.0, 1.0]: 1 = solid, 0 = invisible */
} WeUi_Gfx_Line_t;

typedef struct {
	WeUi_Gfx_Base_t Base;
	WeUi_Gfx_ObjSize_t Radius;
} WeUi_Gfx_Circle_t;

/**
 * @brief Simple 3D point for transform operations
 */
typedef struct {
	float X;
	float Y;
	float Z;
} WeUi_Gfx_ThreeCoord_t;

typedef enum {
	AXE_X,
	AXE_Y,
	AXE_Z,

	AXES_NUM
} WEUI_THREE_AXES_t;

typedef enum {
	WEUI_TEXT_BG_MODE_SOLID		  = 0,
	WEUI_TEXT_BG_MODE_TRANSPARENT = 1,
} WEUI_TEXT_BG_MODE_t;

// Always clockwise
typedef enum {
	WEUI_TEXT_ROTATION_NONE = 0,
	WEUI_TEXT_ROTATION_90	= 1,
	WEUI_TEXT_ROTATION_180	= 2,
	WEUI_TEXT_ROTATION_270	= 3,
} WEUI_TEXT_ROTATION_MODE_t;

typedef enum {
	WEUI_TEXT_ALIGN_LEFT = 0,
	WEUI_TEXT_ALIGN_RIGHT,
	WEUI_TEXT_ALIGN_CENTER,
} WEUI_TEXT_ALIGN_t;

typedef enum {
	WEUI_TEXT_VALIGN_TOP = 0,
	WEUI_TEXT_VALIGN_BOTTOM,
	WEUI_TEXT_VALIGN_MIDDLE,
} WEUI_TEXT_VALIGN_t;

/**
 * @brief Text description structure
 */
typedef struct {
	char* Str;
	const u8* Font;
	WEUI_TEXT_BG_MODE_t BgMode;
	WEUI_TEXT_ROTATION_MODE_t Rotation;
	WeUi_Gfx_Color_t TextColor;
	WeUi_Gfx_Color_t BackColor;
} WeUi_Gfx_Text_t;

typedef enum {
	IMG_POS_CORNER = 0,	   /**< Reference point is the top-left corner of the image */
	IMG_POS_CENTER_VER,	   /**< X centred on the reference point, Y kept */
	IMG_POS_CENTER_HOR,	   /**< Y centred on the reference point, X kept */
	IMG_POS_CENTER_VER_HOR /**< Center both axes — reference point maps to image center */
} WEUI_IMG_POS_t;

typedef enum {
	IMG_COLOR_MODE_NORMAL = 0,
	IMG_COLOR_MODE_REVERSE,
} WEUI_IMG_COLOR_MODE_t;

typedef enum {
	IMG_FLIP_NO = 0,
	IMG_FLIP_VER,
	IMG_FLIP_HOR,
	IMG_FLIP_VER_HOR,
} WEUI_IMG_FLIP_t;

typedef enum {
	IMG_INTERSECT_FRONT_OPACITY_DISABLED = 0, /**< Draw all pixels (default) */
	IMG_INTERSECT_FRONT_OPACITY_ENABLED, /**< Black image pixels are transparent: the drawing frame shows through */
	IMG_INTERSECT_STRONG_OVERRIDES_WEAK, /**< Pixel written only if image pixel is brighter than display pixel */
	IMG_INTERSECT_SUMM,					 /**< Additive blend: clamp(display + image) */
	IMG_INTERSECT_DIFF,					 /**< Absolute difference: |display - image| */
} WEUI_IMG_INTERSECT_t;

typedef enum {
	IMG_COLOR_SCHEME_GRAY = 0,
	IMG_COLOR_SCHEME_RGB,
} WEUI_IMG_COLOR_SCHEME_t;

typedef enum {
	IMG_DITHERING_NO = 0,
	IMG_DITHERING_FLOYD_STEINBERG,
} WEUI_IMG_DITHERING_t;

typedef struct {
	WEUI_IMG_POS_t Pos;
	WEUI_IMG_COLOR_MODE_t ColorMode;
	WEUI_IMG_FLIP_t Flip;
	WEUI_IMG_INTERSECT_t Intersect;
	WEUI_IMG_DITHERING_t Dithering;
	float
		Exposure; /**< Multiplicative channel scale. Default = WEUI_TRANSFORM_EXPOSURE_DEF (1.0f) */
	float
		Brightness; /**< Additive channel offset. Default = WEUI_TRANSFORM_BRIGHTNESS_DEF (0.0f) */
	float Contrast; /**< Contrast factor. Default = WEUI_TRANSFORM_CONTRAST_DEF (1.0f) */
} WeUi_Gfx_ImageTransform_t;

typedef enum {
	GRAPHICS_TYPE_UNKNOWN = 0,
	GRAPHICS_TYPE_IMAGE,
	GRAPHICS_TYPE_ANIMA
} WEUI_GRAPHICS_TYPE_t;

typedef enum {
	STORAGE_TYPE_UNKNOWN,
	STORAGE_TYPE_RAM,
	STORAGE_TYPE_INT_FLASH,
	STORAGE_TYPE_EXT_FLASH
} WEUI_STORAGE_TYPE_t;

typedef struct {
	WEUI_STORAGE_TYPE_t StorageType;
	char Name[32];
	WEUI_GRAPHICS_TYPE_t GraphicsType;
	WeUi_Gfx_StorageAddr_t Addr;
	WeUi_Gfx_ObjSize_t Width;
	WeUi_Gfx_ObjSize_t Height;
	u32 Size;
	bool IsCompressed;
	WEUI_IMG_COLOR_SCHEME_t ColorScheme;
	bool AlphaChannel;
} WeUi_Gfx_Storage_t;

typedef struct {
	WeUi_Gfx_StorageAddr_t Addr;
	u32 Size;
} WeUi_Gfx_Packet_Block_t;

typedef void (*WeUi_Gfx_Packet_ReadVal_t)(WeUi_Gfx_StorageAddr_t addr, u8* pBuff, u32 len);

typedef struct {
	WeUi_Gfx_StorageAddr_t Addr;
	u32 Size;
	WeUi_Gfx_StorageAddr_t OffsetsAddr;
	u32 BlocksNum;
	WeUi_Gfx_Packet_ReadVal_t ReadVal;
} WeUi_Gfx_Packet_t;

/**
 * @brief Image description structure
 */
typedef struct {
	WeUi_Gfx_Storage_t Storage;
	WeUi_Gfx_Color_t* ImageOrig;
	WeUi_Gfx_Color_t* ImageAux;
	WeUi_Gfx_ObjSize_t Width;
	WeUi_Gfx_ObjSize_t Height;
	WeUi_Gfx_ImageTransform_t Transform;
} WeUi_Gfx_Image_t;

/**
 * @brief Animation description structure
 * 
 * @param StorageType type of data storage
 * @param Storage object info was read from storage
 * @param FrameSize Frame size of animation
 * @param FramesNum Frames num in animation
 */
typedef struct {
	WEUI_STORAGE_TYPE_t StorageType;
	WeUi_Gfx_Storage_t Storage;
#if WEUI_USE_COMPONENT__IMAGE_COMPRESS
	WeUi_Gfx_Packet_t Packet;
#endif /* WEUI_USE_COMPONENT__IMAGE_COMPRESS */
	u32 FrameSize;
	u32 FramesNum;
} WeUi_Gfx_Animation_t;

#endif /* __WEUI_GFX_TYPES_H */
