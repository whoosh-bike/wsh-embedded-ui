#include "lab_api.h"
#include "weui_demo.h"

/* The library's own showpieces: the round a demo firmware plays from boot, and each of its
 * demos on its own */
void Lab_Target_Init(void) {
	Lab_AddAction("round", WeUi_Demo_PlayRound);

	for (u32 i = 0; i < WeUi_Demo_Count(); i++)
		Lab_AddAction(WeUi_Demo_Get(i)->Name, WeUi_Demo_Get(i)->Play);
}
