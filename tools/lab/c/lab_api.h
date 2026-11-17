#ifndef __LAB_API_H
#define __LAB_API_H

#include "mathlib_common.h"

/**
 * What a target gives the lab. A target is the C a project wants to look at - the
 * library demos, or an app's scenes - built with the library into one wasm module.
 * It implements Lab_Target_Init() and registers there what the UI may run.
 */

#define LAB_ACTIONS_MAX 32
#define LAB_INPUTS_MAX	32
#define LAB_GROUPS_MAX	8

/**
 * @brief A one-shot call the UI lists and runs from a fresh start, e.g. a demo. In a
 *        live target it is a place to start a session from instead, run before the
 *        first poll - "in this scene", say - and the session goes on live from there.
 */
void Lab_AddAction(const char* pcName, VoidFunc_t fn);

/**
 * @brief Makes the target live, as an app's UI task is: the lab calls poll and sleeps
 *        what it returns, for as long as the UI plays - e.g. WeUi_Gfx_SceneMgr_Poll
 */
void Lab_SetPoll(u32 (*poll)(void));

/** @brief A button of a live target, called as fn(arg) at the virtual time it is pressed */
void Lab_AddInput(const char* pcName, void (*fn)(u32 arg), u32 arg);

/**
 * @brief A heading for the inputs added after it. Keys 1-9 go to the first group's
 *        inputs, or to the first nine with no groups at all.
 */
void Lab_AddGroup(const char* pcName);

/**
 * @brief Names for what the scene manager traces: the scenes, and an input as it was
 *        posted. Without them the lab shows indices and nothing for the input.
 */
void Lab_SetScenes(u32 count, const char* (*name)(u32 scene));
void Lab_SetInputName(const char* (*name)(const void* pcInput));

/** @brief Called once per run, after the panel and the library are up */
void Lab_Target_Init(void);

#endif /* __LAB_API_H */
