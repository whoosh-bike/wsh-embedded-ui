#ifndef __WEUI_FS_H
#define __WEUI_FS_H

#include "weui_gfx_types.h"

/**
 * Asset storage backends, both off by default - the module then compiles down
 * to stubs (WeUi_Fs_Storage_ParseJSON included) and needs neither jsmn nor the
 * linked list.
 *
 * INT: scans a linker-defined flash region, so the linker script must export
 * _flashGraphicsStorage_Start/_End/_FullSize/_RealSize.
 * EXT: scans a linear region of an external flash, see the bindings below.
 */
#ifndef WEUI_FS_USE_INT_STORAGE
#define WEUI_FS_USE_INT_STORAGE 0
#endif /* WEUI_FS_USE_INT_STORAGE */

#ifndef WEUI_FS_USE_EXT_STORAGE
#define WEUI_FS_USE_EXT_STORAGE 0
#endif /* WEUI_FS_USE_EXT_STORAGE */

#define WEUI_FS_USE_STORAGE (WEUI_FS_USE_INT_STORAGE || WEUI_FS_USE_EXT_STORAGE)

/* WeUi_Fs_ForceSync: FatFS -> ext flash copy. Off for a pre-flashed ext storage */
#ifndef WEUI_FS_USE_FAT_SYNC
#define WEUI_FS_USE_FAT_SYNC WEUI_FS_USE_EXT_STORAGE
#endif /* WEUI_FS_USE_FAT_SYNC */

#if WEUI_FS_USE_FAT_SYNC && !WEUI_FS_USE_EXT_STORAGE
#error "WEUI_FS_USE_FAT_SYNC requires WEUI_FS_USE_EXT_STORAGE - there is nothing to sync into!"
#endif

#if WEUI_FS_USE_EXT_STORAGE
#if !defined(WEUI_FS_EXT_STORAGE_BASE_ADDR) || !defined(WEUI_FS_EXT_STORAGE_SIZE) || \
	!defined(WEUI_FS_EXT_SECTOR_SIZE) || !defined(WEUI_FS_EXT_READ)
#error "WEUI_FS_USE_EXT_STORAGE needs the WEUI_FS_EXT_* bindings in weui_cfg.h!"
#endif
#endif /* WEUI_FS_USE_EXT_STORAGE */

#if WEUI_FS_USE_FAT_SYNC
#if !defined(WEUI_FS_FAT_ROOT_PATH) || !defined(WEUI_FS_FAT_OPENDIR) || \
	!defined(WEUI_FS_EXT_WRITE) || !defined(WEUI_FS_EXT_ERASE_SECTOR) || !defined(WEUI_FS_ALLOC)
#error "WEUI_FS_USE_FAT_SYNC needs the WEUI_FS_FAT_*/ALLOC bindings in weui_cfg.h!"
#endif
#endif /* WEUI_FS_USE_FAT_SYNC */

/* Sized for the header the asset converter emits - 8 fields, 17 tokens */
#ifndef WEUI_FS_JSON_MAX_LEN
#define WEUI_FS_JSON_MAX_LEN 256
#endif /* WEUI_FS_JSON_MAX_LEN */

#ifndef WEUI_FS_JSON_TOKENS_MAX_NUM
#define WEUI_FS_JSON_TOKENS_MAX_NUM 24
#endif /* WEUI_FS_JSON_TOKENS_MAX_NUM */

/* Object + one key/value pair - anything shorter carries no asset data */
#ifndef WEUI_FS_JSON_TOKENS_MIN_NUM
#define WEUI_FS_JSON_TOKENS_MIN_NUM 2
#endif /* WEUI_FS_JSON_TOKENS_MIN_NUM */

typedef struct {
	ErrorCounter_t DirOpen;
	ErrorCounter_t FileOpen;
	ErrorCounter_t FileRead;
} WeUi_Fs_ForceSync_Stats_t;

/**
 * @brief Force-sync the filesystem. Error statistics returned in pStats.
 *        Stub returning RET_STATE_ERR_EMPTY without WEUI_FS_USE_FAT_SYNC.
 */
RET_STATE_t WeUi_Fs_ForceSync(WeUi_Fs_ForceSync_Stats_t* pStats);

/**
 * @brief Scan image/animation directories and parse JSON metadata for all assets.
 *        Call once during initialization before any WeUi_Fs_Storage_Fetch calls.
 *        Stub without WEUI_FS_USE_STORAGE.
 */
RET_STATE_t WeUi_Fs_Storage_Scan(ErrorCounter_t* pErrCnt);

/** @brief Retrieve storage metadata by asset name into pFileInfo */
RET_STATE_t WeUi_Fs_Storage_Fetch(char* pFileName, WeUi_Gfx_Storage_t* pFileInfo);

/** @brief Read raw pixel data from storage (RAM/INT_FLASH/EXT_FLASH) into pDstBuff */
void WeUi_Fs_Storage_Read(u8* pDstBuff, WEUI_STORAGE_TYPE_t storageType,
						  WeUi_Gfx_StorageAddr_t addr, u32 size);

/** @brief Scan storage for a JSON metadata block. Returns JSON length, 0 if not found. */
u32 WeUi_Fs_Storage_FindJSON(char* pJsonBuff, u32 jsonMaxLen);

/** @brief Parse JSON metadata string into WeUi_Gfx_Storage_t structure */
bool WeUi_Fs_Storage_ParseJSON(char* pJsonBuff, u32 jsonLen, WeUi_Gfx_Storage_t* pData);

#endif /* __WEUI_FS_H */
