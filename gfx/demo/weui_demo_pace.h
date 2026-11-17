#ifndef __WEUI_DEMO_PACE_H
#define __WEUI_DEMO_PACE_H

/* The pace the demos share. Private to demo/, not API */

/* Every demo lasts about this long, so a round of them keeps one pace */
#define WEUI_DEMO_SEC 5

/* The demos that animate frame by frame run at this rate */
#define WEUI_DEMO_LIVE_FPS	  30
#define WEUI_DEMO_LIVE_FRAMES (WEUI_DEMO_SEC * WEUI_DEMO_LIVE_FPS)

#endif /* __WEUI_DEMO_PACE_H */
