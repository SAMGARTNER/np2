#include	"compiler.h"
#include	"inputmng.h"
#include	"taskmng.h"
#include	"sdlkbd.h"
#include	"vramhdl.h"
#include	"menubase.h"
#include	"sysmenu.h"
#include    "scrnmng.h"
#include "stdlib.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

	BOOL	task_avail;


void sighandler(int signo) {

	(void)signo;
	task_avail = FALSE;
}


void taskmng_initialize(void) {

	task_avail = TRUE;
}

void taskmng_exit(void) {

	task_avail = FALSE;
}

//void taskmng_rol(void) {
//
//	SDL_Event	e;
//
//	if ((!task_avail) || (!SDL_PollEvent(&e))) {
//		return;
//	}
//	switch(e.type) {
//		case SDL_EVENT_MOUSE_MOTION:
//			if (menuvram == NULL) {
//			}
//			else {
//				menubase_moving(e.motion.x, e.motion.y, 0);
//			}
//			break;
//
//		case SDL_EVENT_MOUSE_BUTTON_UP:
//			switch(e.button.button) {
//				case SDL_BUTTON_LEFT:
//					if (menuvram != NULL)
//					{
//						menubase_moving(e.button.x, e.button.y, 2);
//					}
//#if defined(__IPHONEOS__)
//					else if (SDL_IsTextInputActive())
//					{
//						SDL_StopTextInput();
//					}
//					else if (e.button.y >= 320)
//					{
//						SDL_StartTextInput();
//					}
//#endif
//					else
//					{
//                        if (!mouse_captured) {
//                                sysmenu_menuopen(0, e.button.x, e.button.y);
//                            }
//					}
//					break;
//
//				case SDL_BUTTON_RIGHT:
//					break;
//			}
//			break;
//
//		case SDL_EVENT_MOUSE_BUTTON_DOWN:
//			switch(e.button.button) {
//				case SDL_BUTTON_LEFT:
//					if (menuvram != NULL)
//					{
//						menubase_moving(e.button.x, e.button.y, 1);
//					}
//					break;
//
//				case SDL_BUTTON_RIGHT:
//					break;
//			}
//			break;
//
//		case SDL_EVENT_KEY_DOWN:
//			if (e.key.key == SDLK_F11) {
//				if (menuvram == NULL) {
//					sysmenu_menuopen(0, 0, 0);
//				}
//				else {
//					menubase_close();
//				}
//			}
//			else {
//				sdlkbd_keydown(e.key.key);
//			}
//			break;
//
//		case SDL_EVENT_KEY_UP:
//			sdlkbd_keyup(e.key.key);
//			break;
//
//		case SDL_EVENT_QUIT:
//			task_avail = FALSE;
//			break;
//	}
//}

void taskmng_rol(void) {
    SDL_Event e;

    if ((!task_avail) || (!SDL_PollEvent(&e))) {
        return;
    }

    // Get current window size
    int win_w, win_h;
    SDL_GetWindowSize(scrnmng_get_window(), &win_w, &win_h);

    // Calculate scale and render area (zoomed letterbox)
    float scale_x = (float)win_w / 640.0f;
    float scale_y = (float)win_h / 400.0f;
    float scale = MIN(scale_x, scale_y);

    int render_w = (int)(640 * scale);
    int render_h = (int)(400 * scale);

    int offset_x = (win_w - render_w) / 2;
    int offset_y = (win_h - render_h) / 2;

    // Remap mouse coordinates to logical 640x400
    if (e.type == SDL_EVENT_MOUSE_MOTION ||
        e.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
        e.type == SDL_EVENT_MOUSE_BUTTON_UP) {

        // Mouse in window pixels
        int window_x = e.motion.x;
        int window_y = e.motion.y;

        // Convert to logical (640x400) coordinates
        int logical_x = (window_x - offset_x) * 640 / render_w;
        int logical_y = (window_y - offset_y) * 400 / render_h;

        // Clamp to logical 640x400
        logical_x = MAX(0, MIN(logical_x, 639));
        logical_y = MAX(0, MIN(logical_y, 399));

        // Ignore if outside render area (black bars)
        if (window_x < offset_x || window_x >= offset_x + render_w ||
            window_y < offset_y || window_y >= offset_y + render_h) {
            return; // Skip event
        }

        // Update event with logical coordinates
        if (e.type == SDL_EVENT_MOUSE_MOTION) {
            e.motion.x = logical_x;
            e.motion.y = logical_y;
        } else {
            e.button.x = logical_x;
            e.button.y = logical_y;
        }
    }

    // Process the remapped event
    switch(e.type) {
        case SDL_EVENT_MOUSE_MOTION:
            if (menuvram == NULL) {
                // Emulator mouse handled in np2_main
            } else {
                menubase_moving(e.motion.x, e.motion.y, 0);
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            switch(e.button.button) {
                case SDL_BUTTON_LEFT:
                    if (menuvram != NULL) {
                        menubase_moving(e.button.x, e.button.y, 2);
                    } else if (!mouse_captured) {
                        sysmenu_menuopen(0, e.button.x, e.button.y);
                    }
                    break;
                case SDL_BUTTON_RIGHT:
                    break;
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            switch(e.button.button) {
                case SDL_BUTTON_LEFT:
                    if (menuvram != NULL) {
                        menubase_moving(e.button.x, e.button.y, 1);
                    }
                    break;
                case SDL_BUTTON_RIGHT:
                    break;
            }
            break;

        case SDL_EVENT_KEY_DOWN:
            if (e.key.key == SDLK_F11) {
                if (menuvram == NULL) {
                    sysmenu_menuopen(0, 0, 0);
                } else {
                    menubase_close();
                }
            } else {
                sdlkbd_keydown(e.key.key);
            }
            break;

        case SDL_EVENT_KEY_UP:
            sdlkbd_keyup(e.key.key);
            break;

        case SDL_EVENT_QUIT:
            task_avail = FALSE;
            break;
    }
}

BOOL taskmng_sleep(UINT32 tick) {

	UINT32	base;

	base = GETTICK();
	while((task_avail) && ((GETTICK() - base) < tick)) {
		taskmng_rol();
		SDL_Delay(1);
	}
	return(task_avail);
}

