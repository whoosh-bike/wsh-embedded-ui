#include "weui_fs.h"
#include "log.h"
#include "str.h"

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT LOG_RAW_DIRECT_NL
#else /* DEBUG_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* DEBUG_ENABLE */

#if WEUI_FS_USE_STORAGE
#include "json.h"
#include "linked_list.h"

/* Caught here instead of at link time, as a wall of undefined references */
#if defined(WSH_USE_JSMN) && !WSH_USE_JSMN
#error "weui_fs storage parses JSON headers with jsmn: set WSH_USE_JSMN 1!"
#endif

#if defined(WSH_USE_COLLECTIONS_LINKED_LIST) && !WSH_USE_COLLECTIONS_LINKED_LIST
#error "weui_fs storage caches assets: set WSH_USE_COLLECTIONS_LINKED_LIST 1!"
#endif
#endif /* WEUI_FS_USE_STORAGE */

#if WEUI_FS_USE_INT_STORAGE
// NOLINTBEGIN(clang-diagnostic-reserved-identifier)
extern u32 _flashGraphicsStorage_Start[];
extern u32 _flashGraphicsStorage_End[];
extern u32 _flashGraphicsStorage_FullSize[];
extern u32 _flashGraphicsStorage_RealSize[];
// NOLINTEND(clang-diagnostic-reserved-identifier)
#endif /* WEUI_FS_USE_INT_STORAGE */

#if WEUI_FS_USE_STORAGE
#define WEUI_FS_FETCH_LOCK_TMO_MS 100

/**
 * @brief it's the main handler of data storage. There
 * are no access to it directly from an app
 */
static LinkedList_Handle_t WeUi_Gfx_DataStorage_Handle;
static SemaphoreHandle_t WeUi_Gfx_DataStorage_Sem;

static bool WeUi_Fs_WaitEvent(void* pEvent, u32 timeoutMs) {
	return xSemaphoreTake((SemaphoreHandle_t)pEvent, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

static void WeUi_Fs_SignalEvent(void* pEvent) {
	xSemaphoreGive((SemaphoreHandle_t)pEvent);
}
#endif /* WEUI_FS_USE_STORAGE */

#if WEUI_FS_USE_FAT_SYNC
/**
 * FOR SERIAL FLASH
 * We need to use serial flash as a fast non-volatile storage,
 * but there are some problems with data access on it
 *
 * a, b, c, d - separate binary files with different sizes.
 * Each one placed on the new sector for easy update flow
 * in future (if we need to modify just particulal file).
 * Zero sector is empty for future system using
 *
 * [sect0][sect1][sect2][sect3][sect4][sect5][sect6]...[sectN]
 * [_____][aa___][b____][cccc_][ddddd][ddddd][d____]...[_____]
 */

/**
 * @brief Copy all assets from FatFS (ui/images, ui/animas) to ext flash linear storage.
 *        Each file is written sector-aligned; sector 0 of the region is reserved.
 *        File format on FAT must be: {JSON header}[raw pixel data].
 *
 * @retval RET_STATE_SUCCESS all files synced
 * @retval RET_STATE_WARNING one or more files failed, sync continued
 * @retval RET_STATE_ERR_MEMORY allocation failed
 */
RET_STATE_t WeUi_Fs_ForceSync(WeUi_Fs_ForceSync_Stats_t* pStats) {
	const char* paths[] = {
		WEUI_FS_FAT_ROOT_PATH WEUI_FS_IMAGE_PATH,
		WEUI_FS_FAT_ROOT_PATH WEUI_FS_ANIMA_PATH,
	};

	u8* pChunk = WEUI_FS_ALLOC(WEUI_FS_FILE_CHUNK_SIZE);
	if (!pChunk)
		return RET_STATE_ERR_MEMORY;

	u32 sectorSize	 = WEUI_FS_EXT_SECTOR_SIZE();
	u32 writeAddr	 = WEUI_FS_EXT_STORAGE_BASE_ADDR + sectorSize; /* sector 0 reserved */
	u32 partitionEnd = WEUI_FS_EXT_STORAGE_BASE_ADDR + WEUI_FS_EXT_STORAGE_SIZE;

	u32 dirOpenOk = 0, dirOpenErr = 0;
	u32 fileOpenOk = 0, fileOpenErr = 0;
	u32 fileReadOk = 0, fileReadErr = 0;
	RET_STATE_t rs = RET_STATE_SUCCESS;

	for (u32 pathIdx = 0; pathIdx < NUM_ELEMENTS(paths); pathIdx++) {
		WEUI_FS_FAT_DIR_T dir = {0};
		if (WEUI_FS_FAT_OPENDIR(&dir, paths[pathIdx]) != RET_STATE_SUCCESS) {
			LOCAL_DEBUG_PRINT("Dir \"%s\" open err", paths[pathIdx]);
			dirOpenErr++;
			rs = RET_STATE_WARNING;
			continue;
		}
		dirOpenOk++;

		WEUI_FS_FAT_DIRENT_T entry = {0};
		for (;;) {
			memset(&entry, 0, sizeof(entry));
			if (WEUI_FS_FAT_READDIR(&dir, &entry) != RET_STATE_SUCCESS || entry.Name[0] == 0)
				break;
			if (WEUI_FS_FAT_DIRENT_IS_DIR(&entry))
				continue;

			char filePath[WEUI_FS_DEF_PATH_LEN];
			snprintf(filePath, sizeof(filePath), "%s/%s", paths[pathIdx], entry.Name);

			WEUI_FS_FAT_FILE_T file = {0};
			if (WEUI_FS_FAT_OPEN(&file, filePath, WEUI_FS_FAT_MODE_READ) != RET_STATE_SUCCESS) {
				LOCAL_DEBUG_PRINT("File \"%s\" open err", filePath);
				fileOpenErr++;
				rs = RET_STATE_WARNING;
				continue;
			}
			fileOpenOk++;

			u32 fileSize	  = entry.Size;
			u32 sectorsNeeded = (fileSize + sectorSize - 1) / sectorSize;
			bool writeErr	  = false;

			/* Past the partition the erase would take out whatever shares the flash */
			if (writeAddr + sectorsNeeded * sectorSize > partitionEnd) {
				LOCAL_DEBUG_PRINT("No room for \"%s\", %u bytes", filePath, fileSize);
				WEUI_FS_FAT_CLOSE(&file);
				rs = RET_STATE_WARNING;
				continue;
			}

			for (u32 i = 0; i < sectorsNeeded; i++) {
				if (WEUI_FS_EXT_ERASE_SECTOR(writeAddr + i * sectorSize) != RET_STATE_SUCCESS) {
					LOCAL_DEBUG_PRINT("Erase err at 0x%x", writeAddr + i * sectorSize);
					writeErr = true;
					break;
				}
			}

			u32 dstAddr	  = writeAddr;
			u32 bytesLeft = fileSize;
			while (!writeErr && bytesLeft > 0) {
				u32 chunkLen =
					(bytesLeft < WEUI_FS_FILE_CHUNK_SIZE) ? bytesLeft : WEUI_FS_FILE_CHUNK_SIZE;
				u32 rd = 0;
				if (WEUI_FS_FAT_READ(&file, pChunk, chunkLen, &rd) != RET_STATE_SUCCESS ||
					rd == 0) {
					fileReadErr++;
					rs		 = RET_STATE_WARNING;
					writeErr = true;
					break;
				}
				fileReadOk++;
				if (WEUI_FS_EXT_WRITE(dstAddr, pChunk, rd) != RET_STATE_SUCCESS) {
					writeErr = true;
					break;
				}
				dstAddr += rd;
				bytesLeft -= rd;
			}

			WEUI_FS_FAT_CLOSE(&file);

			if (!writeErr) {
				LOCAL_DEBUG_PRINT("Synced \"%s\" → 0x%x, %u bytes", filePath, writeAddr, fileSize);
				writeAddr += sectorsNeeded * sectorSize;
			} else {
				rs = RET_STATE_WARNING;
			}
		}

		WEUI_FS_FAT_CLOSEDIR(&dir);
	}

	WEUI_FS_FREE(pChunk);

	if (pStats) {
		pStats->DirOpen.Err	  = dirOpenErr;
		pStats->DirOpen.Luck  = dirOpenOk;
		pStats->FileOpen.Err  = fileOpenErr;
		pStats->FileOpen.Luck = fileOpenOk;
		pStats->FileRead.Err  = fileReadErr;
		pStats->FileRead.Luck = fileReadOk;
	}

	return rs;
}

#else /* WEUI_FS_USE_FAT_SYNC */

RET_STATE_t WeUi_Fs_ForceSync(WeUi_Fs_ForceSync_Stats_t* pStats) {
	if (pStats)
		memset(pStats, 0, sizeof(*pStats));

	return RET_STATE_ERR_EMPTY;
}

#endif /* WEUI_FS_USE_FAT_SYNC */

#if WEUI_FS_USE_STORAGE

/**
 * @brief Scan storage for valid data
 * @param pErrCnt error counter for data reading and parsing (optional)
 *
 * @retval RET_STATE_SUCCESS: data scanned successfuly
 */
RET_STATE_t WeUi_Fs_Storage_Scan(ErrorCounter_t* pErrCnt) {
	LOCAL_DEBUG_PRINT("Storage scan started");
	/**
	 * flush linked list if it wasn't empty, or create the new one
	 */
	if (!WeUi_Gfx_DataStorage_Sem)
		WeUi_Gfx_DataStorage_Sem = xSemaphoreCreateBinary();
	RET_STATE_t rs = LinkedList_Create(&WeUi_Gfx_DataStorage_Handle, WeUi_Gfx_DataStorage_Sem,
									   WeUi_Fs_WaitEvent, WeUi_Fs_SignalEvent);
	if (rs != RET_STATE_SUCCESS) {
		if (rs == RET_STATE_ERR_BUSY) {
			rs = LinkedList_Flush(&WeUi_Gfx_DataStorage_Handle);
			if (rs != RET_STATE_SUCCESS) {
				LOCAL_DEBUG_PRINT("LinkedList not flushed with err %s", RetState_GetStr(rs));
				return rs;
			}
		} else {
			LOCAL_DEBUG_PRINT("LinkedList not created with err %s", RetState_GetStr(rs));
			return rs;
		}
	}

#if WEUI_FS_USE_INT_STORAGE
	LOCAL_DEBUG_PRINT("Scan internal storage");

	//_flashGraphicsStorage_FullSize for a full region scan
	WeUi_Gfx_Storage_t storage;
	storage.StorageType					   = STORAGE_TYPE_INT_FLASH;
	WeUi_Gfx_StorageAddr_t storageBaseAddr = (WeUi_Gfx_StorageAddr_t)_flashGraphicsStorage_Start;
	u32 storageLen						   = (u32)_flashGraphicsStorage_RealSize;

	for (u32 byteIdx = 0, missCnt = 0; byteIdx < storageLen; byteIdx++) {
		char* pText = (char*)(storageBaseAddr + byteIdx);

		if (pText[0] == '{') {
			u32 jsLen = WeUi_Fs_Storage_FindJSON(pText, WEUI_FS_JSON_MAX_LEN);

			if (!jsLen) {
				if (pErrCnt)
					pErrCnt->Err++;
				LOCAL_DEBUG_PRINT("No full JSON on addr 0x%x", &pText);
				continue;
			}

			if (!WeUi_Fs_Storage_ParseJSON(pText, jsLen, &storage)) {
				if (pErrCnt)
					pErrCnt->Err++;
				continue;
			}

			rs = LinkedList_Insert(&WeUi_Gfx_DataStorage_Handle, (void*)&storage, sizeof(storage),
								   LINKED_LIST_POS_FRONT);
			ASSERT_CHECK(rs == RET_STATE_SUCCESS);
			if (rs != RET_STATE_SUCCESS) {
				if (pErrCnt)
					pErrCnt->Err++;
			}

			LOG_LVL(LOG_LVL_DEBUG,
					"File \"%s\" cached in linkedList with rs %s, miss bytes cnt: %d", storage.Name,
					RetState_GetStr(rs), missCnt);

			missCnt = 0;
			byteIdx += jsLen + storage.Size - 1;  //-1 because `byteIdx++` in new cycle
		} else {
			missCnt++;
		}
	}
#endif /* WEUI_FS_USE_INT_STORAGE */

#if WEUI_FS_USE_EXT_STORAGE
	LOCAL_DEBUG_PRINT("Scan external storage");

	u32 sectorSize	= WEUI_FS_EXT_SECTOR_SIZE();
	u32 sectorCount = WEUI_FS_EXT_STORAGE_SIZE / sectorSize;
	u8 jsonBuf[WEUI_FS_JSON_MAX_LEN];

	/* Files are sector-aligned (sector 0 is reserved), scan sector-by-sector. */
	for (u32 sect = 1; sect < sectorCount;) {
		u32 sectAddr = WEUI_FS_EXT_STORAGE_BASE_ADDR + sect * sectorSize;

		if (WEUI_FS_EXT_READ(sectAddr, jsonBuf, sizeof(jsonBuf)) != RET_STATE_SUCCESS) {
			if (pErrCnt)
				pErrCnt->Err++;
			sect++;
			continue;
		}

		if (jsonBuf[0] != '{') {
			sect++;
			continue;
		}

		u32 jsLen = WeUi_Fs_Storage_FindJSON((char*)jsonBuf, sizeof(jsonBuf));
		if (!jsLen) {
			if (pErrCnt)
				pErrCnt->Err++;
			sect++;
			continue;
		}

		WeUi_Gfx_Storage_t extEntry = {0};
		extEntry.StorageType		= STORAGE_TYPE_EXT_FLASH;
		if (!WeUi_Fs_Storage_ParseJSON((char*)jsonBuf, jsLen, &extEntry)) {
			if (pErrCnt)
				pErrCnt->Err++;
			sect++;
			continue;
		}
		/* ParseJSON sets Addr to a RAM pointer — override with the real flash address */
		extEntry.Addr = sectAddr + jsLen;

		rs = LinkedList_Insert(&WeUi_Gfx_DataStorage_Handle, &extEntry, sizeof(extEntry),
							   LINKED_LIST_POS_FRONT);
		if (rs != RET_STATE_SUCCESS && pErrCnt)
			pErrCnt->Err++;

		LOG_LVL(LOG_LVL_DEBUG, "Ext \"%s\" at 0x%x, size %u", extEntry.Name, extEntry.Addr,
				extEntry.Size);

		u32 occupiedSects = (jsLen + extEntry.Size + sectorSize - 1) / sectorSize;
		sect += occupiedSects ? occupiedSects : 1;
	}
#endif /* WEUI_FS_USE_EXT_STORAGE */

	LOG_LVL(LOG_LVL_DEBUG, "Found %d entries in storage",
			LinkedList_GetNodesNum(&WeUi_Gfx_DataStorage_Handle));

	return RET_STATE_SUCCESS;
}

/**
 * @brief Fetch data from linkedList cache to file object
 *
 * @param pFileName file name in linkedList
 * @param pStorageData file storage parameters
 * @retval RET_STATE_ERR_PARAM: bad input parameter
 * @retval RET_STATE_ERR_EMPTY: no data in linkedList cache
 * @retval RET_STATE_SUCCESS: data fetched successfuly
 */
RET_STATE_t WeUi_Fs_Storage_Fetch(char* pFileName, WeUi_Gfx_Storage_t* pStorageData) {
	if (!pStorageData || !pFileName) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	/* Any failure, not just busy: unlocking a lock not taken would release another reader's */
	RET_STATE_t lockState =
		LinkedList_SharedLock(&WeUi_Gfx_DataStorage_Handle, WEUI_FS_FETCH_LOCK_TMO_MS);
	if (lockState != RET_STATE_SUCCESS) {
		LOCAL_DEBUG_PRINT("linkedList sharedLock isn't ready!");
		return lockState;
	}

	RET_STATE_t fetchState = RET_STATE_ERR_EMPTY;

	u32 nodesNum = LinkedList_GetNodesNum(&WeUi_Gfx_DataStorage_Handle);
	for (u32 llIdx = 0; llIdx < nodesNum; llIdx++) {
		const WeUi_Gfx_Storage_t* pLocFileInfo;
		LinkedList_GetDataPtr(&WeUi_Gfx_DataStorage_Handle, (void**)&pLocFileInfo, NULL, llIdx);

		/**
		 * cut pFileName if it's len > sizeof(pLocFileInfo->Name)
		 */
		if (!strncmp(pLocFileInfo->Name, pFileName, sizeof(pLocFileInfo->Name))) {
			*pStorageData = *pLocFileInfo;
			LOCAL_DEBUG_PRINT(
				"File fetched from linkedList cache. Storage "
				"type: %d, image object \"%s\" inited: t:%d, "
				"w:%d, h:%d, s:%d, addr:0x%x, scheme:%d, alpha:%d",
				pStorageData->StorageType, pStorageData->Name, pStorageData->GraphicsType,
				pStorageData->Width, pStorageData->Height, pStorageData->Size, pStorageData->Addr,
				pStorageData->ColorScheme, pStorageData->AlphaChannel);
			fetchState = RET_STATE_SUCCESS;
			break;
		}
	}

	LinkedList_SharedUnlock(&WeUi_Gfx_DataStorage_Handle);

	return fetchState;
}

#else /* WEUI_FS_USE_STORAGE */

RET_STATE_t WeUi_Fs_Storage_Scan(ErrorCounter_t* pErrCnt) {
	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Fs_Storage_Fetch(char* pFileName, WeUi_Gfx_Storage_t* pStorageData) {
	if (!pStorageData || !pFileName) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	return RET_STATE_ERR_EMPTY;
}

#endif /* WEUI_FS_USE_STORAGE */

/**
 * @brief Read data from storage
 */
void WeUi_Fs_Storage_Read(u8* pDstBuff, WEUI_STORAGE_TYPE_t storageType,
						  WeUi_Gfx_StorageAddr_t addr, u32 size) {
	switch (storageType) {
		case STORAGE_TYPE_EXT_FLASH:
#if WEUI_FS_USE_EXT_STORAGE
			WEUI_FS_EXT_READ(addr, pDstBuff, size);
#else  /* WEUI_FS_USE_EXT_STORAGE */
			/* addr belongs to an unbound flash - blank the buffer, don't deref it */
			LOCAL_DEBUG_PRINT("Ext storage read on addr 0x%x without ext storage support", addr);
			memset((void*)pDstBuff, 0, size);
#endif /* WEUI_FS_USE_EXT_STORAGE */
			break;
		case STORAGE_TYPE_RAM:
		case STORAGE_TYPE_INT_FLASH:
		default:
			memcpy((void*)pDstBuff, (void*)addr, size);
			break;
	}
}

/**
 * @brief Check if there are two json figure brackets
 *
 * @param pJsonBuff JSON buff ptr
 * @param jsonMaxLen json buff max len
 * @retval u32 JSON real length (0 in case of no brackets in buff)
 */
u32 WeUi_Fs_Storage_FindJSON(char* pJsonBuff, u32 jsonMaxLen) {
	if (pJsonBuff[0] != '{') {
		LOCAL_DEBUG_PRINT("No '{' symbol in memory");
		return 0;
	}

	u32 jsonLen = 0;
	while (pJsonBuff[jsonLen] != '}' && jsonLen < jsonMaxLen)
		jsonLen++;

	if (jsonLen < jsonMaxLen) {
		jsonLen++;
#if LOCAL_DEBUG_PRINT_ENABLE
		char tmpBuff[jsonLen + 1];
		memcpy(tmpBuff, pJsonBuff, jsonLen);
		tmpBuff[jsonLen] = '\0';
		for (u32 i = 0; i < jsonLen; i++) {
			if (tmpBuff[i] < ' ' || tmpBuff[i] > '~') {
				tmpBuff[i] = '.';
			}
		}

		LOCAL_DEBUG_PRINT("JSON file info: %s", tmpBuff);
#endif /* LOCAL_DEBUG_PRINT_ENABLE */
		return jsonLen;
	}

	LOCAL_DEBUG_PRINT("JSON data too long");
	return 0;
}

#if WEUI_FS_USE_STORAGE

/* Matched whole: strncmp over the token length alone takes "t" for "type" and an empty
 * token for any key */
static bool WeUi_Fs_JsonTokenIs(const char* pcTok, u16 tokLen, const char* pcKey) {
	return tokLen == strlen(pcKey) && !strncmp(pcTok, pcKey, tokLen);
}

/**
 * @brief parse JSON with graphics data
 *
 * @param pJsonBuff buff with JSON data
 * @param jsonLen len on the JSON from '{' till '}'
 * @param pStorageData pointer for the storage structure
 * @retval true in case of valid data in JSON buff
 */
bool WeUi_Fs_Storage_ParseJSON(char* pJsonBuff, u32 jsonLen, WeUi_Gfx_Storage_t* pStorageData) {
	jsmntok_t tokensDescr[WEUI_FS_JSON_TOKENS_MAX_NUM];
	jsmn_parser parser;

	jsmn_init(&parser);
	s32 tokenNum = jsmn_parse(&parser, pJsonBuff, jsonLen, tokensDescr, NUM_ELEMENTS(tokensDescr));
	LOCAL_DEBUG_PRINT("Found %d tokens in JSON on addr 0x%x", tokenNum, pJsonBuff);

	/* Default value */
	pStorageData->IsCompressed = false;
	pStorageData->Addr		   = (WeUi_Gfx_StorageAddr_t)(pJsonBuff + jsonLen);
	pStorageData->ColorScheme  = IMG_COLOR_SCHEME_GRAY;
	pStorageData->AlphaChannel = false;

	if (tokenNum <= WEUI_FS_JSON_TOKENS_MIN_NUM) {
		// Too much errors while JSON scan
		// LOG_LVL(LOG_LVL_ERROR, "JSON parsing error: %d", tokenNum);
		return false;
	}

	for (s32 token = 2; token <= tokenNum - 1; token++) {
		char* keyStr	 = &pJsonBuff[tokensDescr[token].start];
		char* prevKeyStr = &pJsonBuff[tokensDescr[token - 1].start];
		u16 keyLen		 = tokensDescr[token].end - tokensDescr[token].start;
		u16 prevKeyLen	 = tokensDescr[token - 1].end - tokensDescr[token - 1].start;

		if (WeUi_Fs_JsonTokenIs(prevKeyStr, prevKeyLen, "type")) {
			if (WeUi_Fs_JsonTokenIs(keyStr, keyLen, "img"))
				pStorageData->GraphicsType = GRAPHICS_TYPE_IMAGE;
			else if (WeUi_Fs_JsonTokenIs(keyStr, keyLen, "anm"))
				pStorageData->GraphicsType = GRAPHICS_TYPE_ANIMA;
			else
				pStorageData->GraphicsType = GRAPHICS_TYPE_UNKNOWN;
		} else if (WeUi_Fs_JsonTokenIs(prevKeyStr, prevKeyLen, "name")) {
			if (keyLen >= sizeof(pStorageData->Name))
				return false;

			memcpy(pStorageData->Name, keyStr, keyLen);
			pStorageData->Name[keyLen] = '\0';
		} else if (WeUi_Fs_JsonTokenIs(prevKeyStr, prevKeyLen, "widt"))
			pStorageData->Width = strtoul(keyStr, NULL, 10);
		else if (WeUi_Fs_JsonTokenIs(prevKeyStr, prevKeyLen, "heig"))
			pStorageData->Height = strtoul(keyStr, NULL, 10);
		else if (WeUi_Fs_JsonTokenIs(prevKeyStr, prevKeyLen, "size"))
			pStorageData->Size = strtoul(keyStr, NULL, 10);
		else if (WeUi_Fs_JsonTokenIs(prevKeyStr, prevKeyLen, "comp")) {
			if (WeUi_Fs_JsonTokenIs(keyStr, keyLen, "true"))
				pStorageData->IsCompressed = true;
		} else if (WeUi_Fs_JsonTokenIs(prevKeyStr, prevKeyLen, "scheme"))
			pStorageData->ColorScheme = strtoul(keyStr, NULL, 10);
		else if (WeUi_Fs_JsonTokenIs(prevKeyStr, prevKeyLen, "alpha"))
			pStorageData->AlphaChannel = strtoul(keyStr, NULL, 10);
		else
			continue;

		token++;
	}

	LOG_LVL(LOG_LVL_DEBUG, "Parsed data: %d, %s, %d, %d, 0x%x, %d, %d, %d",
			pStorageData->GraphicsType, pStorageData->Name, pStorageData->Width,
			pStorageData->Height, pStorageData->Addr, pStorageData->Size, pStorageData->ColorScheme,
			pStorageData->AlphaChannel);

	return true;
}

#else /* WEUI_FS_USE_STORAGE */

bool WeUi_Fs_Storage_ParseJSON(char* pJsonBuff, u32 jsonLen, WeUi_Gfx_Storage_t* pStorageData) {
	return false;
}

#endif /* WEUI_FS_USE_STORAGE */
