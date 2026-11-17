/* Differential oracle: keep the scalar span path while sharing paint and raster setup. */
#define WEUI_GFX_FILL_TEST_REFERENCE
#define WeUi_Gfx_Fill_PanelClip	   WeUi_Ref_Fill_PanelClip
#define WeUi_Gfx_Paint_Solid	   WeUi_Ref_Paint_Solid
#define WeUi_Gfx_Paint_Check	   WeUi_Ref_Paint_Check
#define WeUi_Gfx_Fill_PaintShapeTo WeUi_Ref_Fill_PaintShapeTo
#define WeUi_Gfx_Fill_PaintShape   WeUi_Ref_Fill_PaintShape
#define WeUi_Gfx_Fill_Paint		   WeUi_Ref_Fill_Paint
#define WeUi_Gfx_Fill_Solid		   WeUi_Ref_Fill_Solid
#define WeUi_Gfx_Fill_SoftBounds   WeUi_Ref_Fill_SoftBounds
#define WeUi_Gfx_Fill_SoftTo	   WeUi_Ref_Fill_SoftTo
#define WeUi_Gfx_Fill_Soft		   WeUi_Ref_Fill_Soft

#include "../gfx/vector/weui_gfx_fill.c"
