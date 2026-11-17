#include "weui_gfx_packet.h"
#include "weui_fs.h"

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0	//default 0
#endif

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT LOG
#else /* DEBUG_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* DEBUG_ENABLE */

#define MAGIC_WORD		"WHSH"
#define MAGIC_WORD_SIZE 4
#define INFO_FIELD_SIZE 4

#define MAGIC_WORD_SHIFT  0
#define BLOCKS_NUM_SHIFT  (MAGIC_WORD_SHIFT + MAGIC_WORD_SIZE)
#define OFFSETS_NUM_SHIFT (BLOCKS_NUM_SHIFT + INFO_FIELD_SIZE)
#define DATA_START_SHIFT  (OFFSETS_NUM_SHIFT + INFO_FIELD_SIZE)

#define FOUR_BYTE_TO_VAL(bt) (bt[0] << 24 | bt[1] << 16 | bt[2] << 8 | bt[3])

static void WeUi_Gfx_Packet_ReadExternal(WeUi_Gfx_StorageAddr_t addr, u8* pBuff, u32 len) {
	WeUi_Fs_Storage_Read(pBuff, STORAGE_TYPE_EXT_FLASH, addr, len);
}

static void WeUi_Gfx_Packet_ReadInternal(WeUi_Gfx_StorageAddr_t addr, u8* pBuff, u32 len) {
	memcpy(pBuff, (void*)addr, len);
}

static bool WeUi_Gfx_Packet_Verify(WeUi_Gfx_Packet_t* pPacket, WeUi_Gfx_StorageAddr_t packetAddr) {
	u8 magicWord[MAGIC_WORD_SIZE];
	pPacket->ReadVal(packetAddr, magicWord, MAGIC_WORD_SIZE);
	return memcmp(magicWord, MAGIC_WORD, MAGIC_WORD_SIZE) == 0;
}

static u32 WeUi_Gfx_Packet_Value_Get(WeUi_Gfx_Packet_t* pPacket, WeUi_Gfx_StorageAddr_t addr) {
	u8 buff[INFO_FIELD_SIZE];
	pPacket->ReadVal(addr, buff, INFO_FIELD_SIZE);
	return FOUR_BYTE_TO_VAL(buff);
}

RET_STATE_t WeUi_Gfx_Packet_Init(WeUi_Gfx_Packet_t* pPacket, WeUi_Gfx_StorageAddr_t packetAddr,
								 u32 packetSize, WEUI_STORAGE_TYPE_t storageType) {
	switch (storageType) {
		case STORAGE_TYPE_EXT_FLASH:
			pPacket->ReadVal = WeUi_Gfx_Packet_ReadExternal;
			break;

		case STORAGE_TYPE_RAM:
		case STORAGE_TYPE_INT_FLASH:
		default:
			pPacket->ReadVal = WeUi_Gfx_Packet_ReadInternal;
	}

	if (!WeUi_Gfx_Packet_Verify(pPacket, packetAddr))
		return RET_STATE_ERROR;

	u32 blocksNum  = WeUi_Gfx_Packet_Value_Get(pPacket, packetAddr + BLOCKS_NUM_SHIFT);
	u32 offsetsNum = WeUi_Gfx_Packet_Value_Get(pPacket, packetAddr + OFFSETS_NUM_SHIFT);

	if (offsetsNum != blocksNum || offsetsNum == 0 || blocksNum == 0)
		return RET_STATE_ERROR;

	pPacket->Addr		 = packetAddr;
	pPacket->Size		 = packetSize;
	pPacket->OffsetsAddr = (packetAddr + packetSize) - (offsetsNum * INFO_FIELD_SIZE);
	pPacket->BlocksNum	 = blocksNum;

	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Gfx_Packet_BlockDescrByIdx_Get(WeUi_Gfx_Packet_t* pPacket,
												WeUi_Gfx_Packet_Block_t* pBlock, u32 blockIdx) {
	if (pPacket == NULL)
		return RET_STATE_ERR_PARAM;

	u32 nextBlockOffset;

	if (blockIdx >= (pPacket->BlocksNum - 1)) {
		blockIdx		= pPacket->BlocksNum - 1;
		nextBlockOffset = pPacket->OffsetsAddr - pPacket->Addr;
	} else {
		nextBlockOffset = WeUi_Gfx_Packet_Value_Get(
			pPacket, pPacket->OffsetsAddr + ((blockIdx + 1) * INFO_FIELD_SIZE));
	}

	u32 currBlockOffset =
		WeUi_Gfx_Packet_Value_Get(pPacket, pPacket->OffsetsAddr + (blockIdx * INFO_FIELD_SIZE));

	pBlock->Addr = pPacket->Addr + currBlockOffset;
	pBlock->Size = nextBlockOffset - currBlockOffset;

	return RET_STATE_SUCCESS;
}
