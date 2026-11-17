#include <zlib.h>

#include "display_m.h"
#include "test_png.h"

#define TEST_PNG_GAP_COLOR 0x101010

static void PngChunk(FILE* pFile, const char* pcType, const u8* pcData, u32 len) {
	u8 hdr[] = {len >> 24, len >> 16, len >> 8, len, pcType[0], pcType[1], pcType[2], pcType[3]};
	fwrite(hdr, 1, sizeof(hdr), pFile);
	if (len)
		fwrite(pcData, 1, len, pFile);

	uLong crc  = crc32(crc32(0, (const Bytef*)pcType, 4), pcData, len);
	u8 crcBe[] = {crc >> 24, crc >> 16, crc >> 8, crc};
	fwrite(crcBe, 1, sizeof(crcBe), pFile);
}

/* A dark gap between the squares keeps single LEDs readable, as on the real matrix */
bool Test_SavePng(const char* pcPath, const u32* pcFrame, u32 scale) {
	DisplayM_Params_t params = DisplayM_GetParams();
	u32 w = params.Width * scale, h = params.Height * scale;
	u32 rowLen	= 1 + w * 3; /* filter byte + RGB */
	u8* pRaw	= calloc(rowLen, h);
	uLongf zLen = compressBound(rowLen * h);
	u8* pZ		= malloc(zLen);
	bool isOk	= false;

	if (pRaw && pZ) {
		for (u32 y = 0; y < h; y++) {
			for (u32 x = 0; x < w; x++) {
				bool isGap = scale > 2 && ((x % scale) == scale - 1 || (y % scale) == scale - 1);
				u32 c =
					isGap ? TEST_PNG_GAP_COLOR : pcFrame[(y / scale) * params.Width + x / scale];
				u8* pPx = &pRaw[y * rowLen + 1 + x * 3];
				pPx[0] = c & 0xFF, pPx[1] = (c >> 8) & 0xFF, pPx[2] = (c >> 16) & 0xFF;
			}
		}
		isOk = compress2(pZ, &zLen, pRaw, rowLen * h, Z_BEST_COMPRESSION) == Z_OK;
	}

	FILE* pFile = isOk ? fopen(pcPath, "wb") : NULL;
	if (pFile) {
		static const u8 sig[] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
		u8 ihdr[] = {w >> 24, w >> 16, w >> 8, w, h >> 24, h >> 16, h >> 8, h, 8, 2, 0, 0, 0};
		fwrite(sig, 1, sizeof(sig), pFile);
		PngChunk(pFile, "IHDR", ihdr, sizeof(ihdr));
		PngChunk(pFile, "IDAT", pZ, (u32)zLen);
		PngChunk(pFile, "IEND", NULL, 0);
		fclose(pFile);
	}

	free(pRaw);
	free(pZ);
	return pFile != NULL;
}
