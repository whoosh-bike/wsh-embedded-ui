#ifndef __TEST_PNG_H
#define __TEST_PNG_H

#include "mathlib_common.h"

/** @brief Write a frame as a PNG, each panel pixel a `scale`-sized square */
bool Test_SavePng(const char* pcPath, const u32* pcFrame, u32 scale);

#endif /* __TEST_PNG_H */
