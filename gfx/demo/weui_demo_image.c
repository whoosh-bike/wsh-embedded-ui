#include "weui_demo.h"

#if WEUI_USE_COMPONENT__DEMO

#include "log.h"
#include "weui_fs.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_fps_control.h"
#include "weui_gfx_image.h"
#include "weui_gfx_transform.h"

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT LOG
#else /* LOCAL_DEBUG_PRINT_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* LOCAL_DEBUG_PRINT_ENABLE */

#define DEMO_IMAGE_HOLD_FPS 2 /* seven stills, plus two exposure sweeps */

void WeUi_DrawDemo_Image(void) {
	WeUi_Gfx_Point_t pt = {
		.X = 0,
		.Y = 0,
	};

	WeUi_Gfx_Image_t img;
	WeUi_Gfx_Image_InitStruct(&img);
	img.Transform.Dithering = IMG_DITHERING_NO;

	img.Transform.Pos = IMG_POS_CORNER;
	WeUi_Gfx_Adapter_ClearDrawingFrame();
	if (WeUi_Fs_Storage_Fetch("smile", &(img.Storage)) == RET_STATE_SUCCESS)
		WeUi_Gfx_Image_Draw(&pt, &img);
	WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_IMAGE_HOLD_FPS);

	img.Transform.Pos = IMG_POS_CORNER;
	WeUi_Gfx_Adapter_ClearDrawingFrame();
	if (WeUi_Fs_Storage_Fetch("rgb", &(img.Storage)) == RET_STATE_SUCCESS)
		WeUi_Gfx_Image_Draw(&pt, &img);
	WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_IMAGE_HOLD_FPS);

	pt.X			  = WeUi_Gfx_Adapter_GetX_Mid();
	pt.Y			  = WeUi_Gfx_Adapter_GetY_Mid();
	img.Transform.Pos = IMG_POS_CENTER_VER_HOR;

	WeUi_Gfx_Adapter_ClearDrawingFrame();
	if (WeUi_Fs_Storage_Fetch("qr", &(img.Storage)) == RET_STATE_SUCCESS)
		WeUi_Gfx_Image_Draw(&pt, &img);
	WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_IMAGE_HOLD_FPS);

	WeUi_Gfx_Adapter_ClearDrawingFrame();
	if (WeUi_Fs_Storage_Fetch("rainbow", &(img.Storage)) == RET_STATE_SUCCESS)
		WeUi_Gfx_Image_Draw(&pt, &img);
	WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_IMAGE_HOLD_FPS);

	pt.X			  = 0;
	pt.Y			  = 0;
	img.Transform.Pos = IMG_POS_CORNER;

	WeUi_Gfx_Adapter_ClearDrawingFrame();
	if (WeUi_Fs_Storage_Fetch("cats", &(img.Storage)) == RET_STATE_SUCCESS)
		WeUi_Gfx_Image_Draw(&pt, &img);
	WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_IMAGE_HOLD_FPS);

	img.Transform.Dithering = IMG_DITHERING_FLOYD_STEINBERG;
	img.Transform.Exposure	= 1.5f;
	while (img.Transform.Exposure > 0.3f) {

		WeUi_Gfx_Adapter_ClearDrawingFrame();
		if (WeUi_Fs_Storage_Fetch("cat_0", &(img.Storage)) == RET_STATE_SUCCESS)
			WeUi_Gfx_Image_Draw(&pt, &img);
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(200);

		LOCAL_DEBUG_PRINT("Exp: %f", img.Transform.Exposure);
		img.Transform.Exposure -= 0.01f;
	}

	img.Transform.Dithering = IMG_DITHERING_NO;

	WeUi_Gfx_Adapter_ClearDrawingFrame();
	if (WeUi_Fs_Storage_Fetch("mount_0", &(img.Storage)) == RET_STATE_SUCCESS)
		WeUi_Gfx_Image_Draw(&pt, &img);
	WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_IMAGE_HOLD_FPS);

	img.Transform.Dithering = IMG_DITHERING_NO;

	WeUi_Gfx_Adapter_ClearDrawingFrame();
	if (WeUi_Fs_Storage_Fetch("synth_dith_0", &(img.Storage)) == RET_STATE_SUCCESS)
		WeUi_Gfx_Image_Draw(&pt, &img);
	WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_IMAGE_HOLD_FPS);

	img.Transform.Dithering = IMG_DITHERING_FLOYD_STEINBERG;

	WeUi_Gfx_Transform_InitImageColorAdjustStruct(&img.Transform);
	img.Transform.Exposure = 0.5f;
	while (img.Transform.Exposure < 1.2f) {

		WeUi_Gfx_Adapter_ClearDrawingFrame();
		if (WeUi_Fs_Storage_Fetch("synth_do_scaled_0", &(img.Storage)) == RET_STATE_SUCCESS)
			WeUi_Gfx_Image_Draw(&pt, &img);
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(200);

		img.Transform.Exposure += 0.005f;
		LOCAL_DEBUG_PRINT("Exp: %f", img.Transform.Exposure);
	}
}

#endif /* WEUI_USE_COMPONENT__DEMO */
