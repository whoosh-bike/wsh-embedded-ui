#include "display_m.h"

/* Separate heap blocks sized to the panel: ASan flags an access past one, and Idx()
 * flags one that would still land inside, in a neighbouring row */
static u32* pDrawing;
static u32* pDisplay;
static u32* pAux;
static DisplayM_Params_t Params;
static u8 BrightLut[UINT8_MAX + 1];
static DisplaySim_FrameHook_t FrameHook;
static u32 FramesPushed;
static u32 VirtualMs;
static u32 RandState = 1;

/* weui draws its randomness from rand(): a generator of our own keeps a run, and its
 * frame hashes, the same whatever the host libc */
void srand(unsigned seed) {
	RandState = seed ? seed : 1;
}

int rand(void) {
	RandState ^= RandState << 13;
	RandState ^= RandState >> 17;
	RandState ^= RandState << 5;
	return (int)(RandState & INT32_MAX);
}

u32 Sim_TickMs(void) {
	return VirtualMs;
}

void Sim_DelayMs(u32 ms) {
	VirtualMs += ms;
}

void Sim_DelayUntilMs(u32* pPrev, u32 incr) {
	u32 wake = *pPrev + incr;
	if ((s32)(wake - VirtualMs) > 0)
		VirtualMs = wake;
	*pPrev = wake;
}

/* Outside the panel it lands on pixel 0 once reported, so a host that carries on writes
 * nothing past the frame */
static u32 Idx(u16 x, u16 y) {
	if (x >= Params.Width || y >= Params.Height) {
		Sim_Panic(__FILE__, __LINE__);
		return 0;
	}
	return (u32)y * Params.Width + x;
}

u32 DisplaySim_ApplyBright(u32 color) {
	return BrightLut[color & 0xFF] | (BrightLut[(color >> 8) & 0xFF] << 8) |
		   (BrightLut[(color >> 16) & 0xFF] << 16);
}

void DisplaySim_SetBrightScale(float scale) {
	for (u32 i = 0; i <= UINT8_MAX; i++) {
		float v		 = (float)i * scale;
		BrightLut[i] = (v > (float)UINT8_MAX) ? UINT8_MAX : (u8)v;
	}
}

void DisplaySim_Init(u16 w, u16 h) {
	DisplaySim_Deinit();

	Params = (DisplayM_Params_t){
		.Width	  = w,
		.Height	  = h,
		.MidX	  = w / 2,
		.Y_Mid	  = h / 2,
		.X_Max	  = w - 1,
		.Y_Max	  = h - 1,
		.PixelNum = (u32)w * h,
		.ColorMax = UINT8_MAX,
		.ColorMid = UINT8_MAX / 2,
	};

	pDrawing = calloc(Params.PixelNum, sizeof(u32));
	pDisplay = calloc(Params.PixelNum, sizeof(u32));
	pAux	 = calloc(Params.PixelNum, sizeof(u32));
	if (!pDrawing || !pDisplay || !pAux)
		Sim_Panic(__FILE__, __LINE__);

	DisplaySim_SetBrightScale(1.0f);
	FramesPushed = 0;
	VirtualMs	 = 0;
}

void DisplaySim_Deinit(void) {
	free(pDrawing);
	free(pDisplay);
	free(pAux);
	pDrawing = pDisplay = pAux = NULL;
	FrameHook				   = NULL;
}

void DisplaySim_SetFrameHook(DisplaySim_FrameHook_t hook) {
	FrameHook = hook;
}

u32 DisplaySim_GetFramesPushed(void) {
	return FramesPushed;
}

const u32* DisplaySim_GetDisplayFrame(void) {
	return pDisplay;
}

u64 DisplaySim_Hash(const u32* pcFrame) {
	u64 hash = 1469598103934665603ULL; /* FNV-1a */
	for (u32 i = 0; i < Params.PixelNum; i++) {
		hash ^= pcFrame[i];
		hash *= 1099511628211ULL;
	}
	return hash;
}

DisplayM_Params_t DisplayM_GetParams(void) {
	return Params;
}

void DisplayM_DrawFrame(void) {
	u32* pTmp = pDisplay;
	pDisplay  = pDrawing;
	pDrawing  = pTmp;

	if (FrameHook)
		FrameHook(pDisplay, FramesPushed);
	FramesPushed++;
}

u32 DisplayM_GetPixel(u16 x, u16 y) {
	return pDrawing[Idx(x, y)];
}

u32 DisplayM_GetPixelAux(u16 x, u16 y) {
	return pAux[Idx(x, y)];
}

void DisplayM_SetPixel(u16 x, u16 y, u32 color) {
	pDrawing[Idx(x, y)] = DisplaySim_ApplyBright(color);
}

void DisplayM_SetPixelRaw(u16 x, u16 y, u32 color) {
	pDrawing[Idx(x, y)] = color;
}

void DisplayM_CopyFrame_DisplayToDrawing(void) {
	memcpy(pDrawing, pDisplay, Params.PixelNum * sizeof(u32));
}

void DisplayM_SaveFrame_DisplayToAux(void) {
	memcpy(pAux, pDisplay, Params.PixelNum * sizeof(u32));
}

void* DisplayM_GetFrameMem(u8 frame, u32* pStride, u8* pPx) {
	u32* const frames[] = {pDrawing, pDisplay, pAux};
	*pStride			= Params.Width;
	*pPx				= 0;
	return frame < 3 ? frames[frame] : NULL;
}

void DisplayM_FillDrawingFrame(u32 color) {
	u32 scaled = DisplaySim_ApplyBright(color);
	for (u32 i = 0; i < Params.PixelNum; i++)
		pDrawing[i] = scaled;
}

void DisplayM_FillDisplayAndDrawingFrames(u32 color) {
	DisplayM_FillDrawingFrame(color);
	DisplayM_DrawFrame();
	DisplayM_FillDrawingFrame(color);
}

/* Bands, as a driver with no frame takes them: written into both frames, so that a frame push
 * after them shows them still. One that shows its frames as memory keeps them apart from the
 * bands, as a panel driver holding frames only for a transition does: a transition composing in
 * bands reads the incoming picture from the drawing frame all the while */
#define SIM_BAND_ROWS 8

static u16 SimBand[WEUI_DISPLAY_WIDTH * SIM_BAND_ROWS];

u16* DisplayM_BandGet(u32* pPixels) {
	*pPixels = NUM_ELEMENTS(SimBand);
	return SimBand;
}

void DisplayM_BandSend(u16 x0, u16 y0, u16 x1, u16 y1, u16* pBuf) {
	u32 w = (u32)(x1 - x0);
	for (u16 y = y0; y < y1; y++) {
		for (u16 x = x0; x < x1; x++) {
			u16 px = pBuf[(u32)(y - y0) * w + (u32)(x - x0)]; /* the default, RGB565 */
			u32 r = (px >> 8) & 0xF8, g = (px >> 3) & 0xFC, b = (px << 3) & 0xF8;
			u32 rgb				= (r | r >> 5) | (g | g >> 6) << 8 | (b | b >> 5) << 16;
			pDisplay[Idx(x, y)] = rgb;
#if !WEUI_DISPLAY_FRAME_MEM
			pDrawing[Idx(x, y)] = rgb;
#endif
		}
	}
}
