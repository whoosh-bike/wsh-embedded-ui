#ifndef __WEUI_GFX_PACKET_H
#define __WEUI_GFX_PACKET_H

#include "weui_gfx_types.h"

/*
 * PACKET DESCRIPTION
 *
 * +-----------+-----------+------------+---------+     +---------+----------+     +----------+
 * | MagicWord | BlocksNum | OffsetsNum | Block#1 | ... | Block#N | Offset#1 | ... | Offset#N |
 * +-----------+-----------+------------+---------+     +---------+----------+     +----------+
 *
 * 1) MagicWord: 4-byte identifier "WHSH"
 * 2) BlocksNum: number of compressed data blocks (4 bytes)
 * 3) OffsetsNum: number of offset entries, must equal BlocksNum (4 bytes)
 * 4) Block#1..N: compressed data blocks (variable size each)
 * 5) Offset#1..N: byte offset of each block from packet start (4 bytes each)
 */

/**
 * @brief Initialize WeUi_Gfx_Packet_t from a compressed data packet in storage.
 *        Verifies magic word and reads block/offset metadata.
 * @param[out] pPacket      Packet structure to initialize
 * @param[in]  packetAddr   Address where packet is stored
 * @param[in]  packetSize   Total size of the packet in bytes
 * @param[in]  storageType  Storage type (RAM, INT_FLASH, EXT_FLASH)
 * @retval RET_STATE_SUCCESS  Packet initialized successfully
 * @retval RET_STATE_ERROR    Invalid magic word or mismatched block/offset counts
 */
RET_STATE_t WeUi_Gfx_Packet_Init(WeUi_Gfx_Packet_t* pPacket, WeUi_Gfx_StorageAddr_t packetAddr,
								 u32 packetSize, WEUI_STORAGE_TYPE_t storageType);

/**
 * @brief Get address and size of a compressed block by index.
 * @param[in]  pPacket   Initialized packet structure
 * @param[out] pBlock    Block descriptor (addr + size) to fill
 * @param[in]  blockIdx  Zero-based block index (clamped to last block if out of range)
 * @retval RET_STATE_SUCCESS    Block descriptor filled
 * @retval RET_STATE_ERR_PARAM  pPacket is NULL
 */
RET_STATE_t WeUi_Gfx_Packet_BlockDescrByIdx_Get(WeUi_Gfx_Packet_t* pPacket,
												WeUi_Gfx_Packet_Block_t* pBlock, u32 blockIdx);

#endif /* __WEUI_GFX_PACKET_H */
