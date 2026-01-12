#include "compiler.h"
#include	"strres.h"
#include	"np2.h"
#include	"dosio.h"
#include	"commng.h"
#include	"fontmng.h"
#include	"inputmng.h"
#include	"scrnmng.h"
#include	"soundmng.h"
#include	"sysmng.h"
#include	"taskmng.h"
#include	"sdlkbd.h"
#include	"ini.h"
#include	"pccore.h"
#include	"statsave.h"
#include	"iocore.h"
#include	"scrndraw.h"
#include	"s98.h"
#include	"fdd/diskdrv.h"
#include	"timing.h"
#include	"keystat.h"
#include	"vramhdl.h"
#include	"menubase.h"
#include	"sysmenu.h"
#include <SDL3/SDL_mouse.h>
#include "mousemng.h"
#include <stdbool.h>
#include "stdlib.h"
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
extern SDL_Window *scrnmng_get_window(void);
int mouse_x = 320;
int mouse_y = 200;
static bool rel_mode_enabled = false;
static int last_x = -1, last_y = -1;
static bool grabbed = false;
bool mouse_captured = false;

NP2OSCFG	np2oscfg = {0, 0, 0, 0, 0, 0, {{0}}, {{{0}}}};
static	UINT		framecnt;
static	UINT		waitcnt;
static	UINT		framemax = 1;

static void usage(const char *progname) {

	printf("Usage: %s [options]\n", progname);
	printf("\t--help   [-h]       : print this message\n");
}


// ---- resume

static void getstatfilename(char *path, const char *ext, int size)
{
	char filename[32];
	sprintf(filename, "np2sdl2.%s", ext);

	file_cpyname(path, file_getcd(filename), size);
}

static int flagsave(const char *ext) {

	int		ret;
	char	path[MAX_PATH];

	getstatfilename(path, ext, sizeof(path));
	ret = statsave_save(path);
	if (ret) {
		file_delete(path);
	}
	return(ret);
}

static void flagdelete(const char *ext) {

	char	path[MAX_PATH];

	getstatfilename(path, ext, sizeof(path));
	file_delete(path);
}

static int flagload(const char *ext, const char *title, BOOL force) {

	int		ret;
	int		id;
	char	path[MAX_PATH];
	char	buf[1024];
	char	buf2[1024 + 256];

	getstatfilename(path, ext, sizeof(path));
	id = DID_YES;
	ret = statsave_check(path, buf, sizeof(buf));
	if (ret & (~STATFLAG_DISKCHG)) {
		menumbox("Couldn't restart", title, MBOX_OK | MBOX_ICONSTOP);
		id = DID_NO;
	}
	else if ((!force) && (ret & STATFLAG_DISKCHG)) {
		SPRINTF(buf2, "Conflict!\n\n%s\nContinue?", buf);
		id = menumbox(buf2, title, MBOX_YESNOCAN | MBOX_ICONQUESTION);
	}
	if (id == DID_YES) {
		statsave_load(path);
	}
	return(id);
}


// ---- proc

#define	framereset(cnt)		framecnt = 0

static void processwait(UINT cnt) {

	if (timing_getcount() >= cnt) {
		timing_setcount(0);
		framereset(cnt);
	}
	else {
		taskmng_sleep(1);
	}
}

int np2_main(int argc, char *argv[]) {

	int		pos;
	char	*p;
	int		id;
	pos = 1;
	while(pos < argc) {
		p = argv[pos++];
		if ((!milstr_cmp(p, "-h")) || (!milstr_cmp(p, "--help"))) {
			usage(argv[0]);
			goto np2main_err1;
		}
		else {
			printf("error command: %s\n", p);
			goto np2main_err1;
		}
	}
    SDL_SetHint(SDL_HINT_TRACKPAD_IS_TOUCH_ONLY, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE, "0");
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER, "1");
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SPEED_SCALE, "1.0");
	initload();

	TRACEINIT();

	if (fontmng_init() != SUCCESS) {
		goto np2main_err2;
	}
	sdlkbd_initialize();
	inputmng_init();
	keystat_initialize();

	if (sysmenu_create() != SUCCESS) {
		goto np2main_err3;
	}

	scrnmng_initialize();
	if (scrnmng_create(FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT) != SUCCESS) {
		goto np2main_err4;
	}

	soundmng_initialize();
	commng_initialize();
	sysmng_initialize();
	taskmng_initialize();
	pccore_init();
	S98_init();

	scrndraw_redraw();
	pccore_reset();

	if (np2oscfg.resume) {
		id = flagload(str_sav, str_resume, FALSE);
		if (id == DID_CANCEL) {
			goto np2main_err5;
		}
	}

	while(taskmng_isavail()) {
		taskmng_rol();
        if (mouse_captured) {
            float abs_x_f, abs_y_f;
            Uint32 btn = SDL_GetMouseState(&abs_x_f, &abs_y_f);
            int abs_x = (int)abs_x_f;
            int abs_y = (int)abs_y_f;
            
            // Get current window size
//                int win_w, win_h;
//                SDL_GetWindowSize(scrnmng_get_window(), &win_w, &win_h);

            int target_x = (abs_x * 640 + FULLSCREEN_WIDTH / 2) / FULLSCREEN_WIDTH;
            int target_y = (abs_y * 400 + FULLSCREEN_HEIGHT / 2) / FULLSCREEN_HEIGHT;
            
            // Scale to PC-98 640x400
//                int target_x = (abs_x * 640) / win_w;
//                int target_y = (abs_y * 400) / win_h;

            SINT16 new_dx = (target_x - mouse_x) / 4;
            SINT16 new_dy = (target_y - mouse_y) / 4;

            if (new_dx || new_dy || btn) {
                mouse_dx += new_dx;
                mouse_dy += new_dy;
                mouse_x += new_dx;
                mouse_y += new_dy;

                mouse_btn = 0xff;
                if (btn & SDL_BUTTON_LMASK) mouse_btn &= ~0x80;
                if (btn & SDL_BUTTON_RMASK) mouse_btn &= ~0x20;
            }

            // Log window-relative position
            printf("Window mouse pos: x=%d y=%d (virtual PC-98: x=%d y=%d)\n", abs_x, abs_y, mouse_x, mouse_y);
        }
        
//        if (mouse_captured) {
//            float abs_x_f, abs_y_f;
//            Uint32 btn = SDL_GetMouseState(&abs_x_f, &abs_y_f);
//            int abs_x = (int)abs_x_f;
//            int abs_y = (int)abs_y_f;
//
//            // Get current window size
//            int win_w, win_h;
//            SDL_GetWindowSize(scrnmng_get_window(), &win_w, &win_h);
//
//            // Calculate scale and render area (letterbox)
//            float scale_x = (float)win_w / 640.0f;
//            float scale_y = (float)win_h / 400.0f;
//            float scale = MIN(scale_x, scale_y);
//
//            int render_w = (int)(640 * scale);
//            int render_h = (int)(400 * scale);
//
//            int offset_x = (win_w - render_w) / 2;
//            int offset_y = (win_h - render_h) / 2;
//
//            // Check if mouse is inside the actual render area (not black bars)
//            if (abs_x >= offset_x && abs_x < offset_x + render_w &&
//                abs_y >= offset_y && abs_y < offset_y + render_h) {
//                // Mouse is over emulator image - process movement
//                int logical_x = abs_x - offset_x;
//                int logical_y = abs_y - offset_y;
//
//                // Scale to PC-98 640x400
//                int target_x = logical_x * 640 / render_w;
//                int target_y = logical_y * 400 / render_h;
//
//                SINT16 new_dx = (target_x - mouse_x) / 4;
//                SINT16 new_dy = (target_y - mouse_y) / 4;
//
//                if (new_dx || new_dy || btn) {
//                    mouse_dx += new_dx;
//                    mouse_dy += new_dy;
//                    mouse_x += new_dx * 4;
//                    mouse_y += new_dy * 4;
//
//                    mouse_btn = 0xff;
//                    if (btn & SDL_BUTTON_LMASK) mouse_btn &= ~0x80;
//                    if (btn & SDL_BUTTON_RMASK) mouse_btn &= ~0x20;
//                }
//            } else {
//                // Mouse over black bars - **do not send any movement**
//                // This prevents menus from detecting clicks outside the render area
//                // Optional: show cursor over bars if desired
//                // SDL_ShowCursor();
//            }
//        }

       
        if (np2oscfg.NOWAIT) {
			pccore_exec(framecnt == 0);
			if (np2oscfg.DRAW_SKIP) {			// nowait frame skip
				framecnt++;
				if (framecnt >= np2oscfg.DRAW_SKIP) {
					processwait(0);
				}
			}
			else {							// nowait auto skip
				framecnt = 1;
				if (timing_getcount()) {
					processwait(0);
				}
			}
		}
		else if (np2oscfg.DRAW_SKIP) {		// frame skip
			if (framecnt < np2oscfg.DRAW_SKIP) {
				pccore_exec(framecnt == 0);
				framecnt++;
			}
			else {
				processwait(np2oscfg.DRAW_SKIP);
			}
		}
		else {								// auto skip
			if (!waitcnt) {
				UINT cnt;
				pccore_exec(framecnt == 0);
				framecnt++;
				cnt = timing_getcount();
				if (framecnt > cnt) {
					waitcnt = framecnt;
					if (framemax > 1) {
						framemax--;
					}
				}
				else if (framecnt >= framemax) {
					if (framemax < 12) {
						framemax++;
					}
					if (cnt >= 12) {
						timing_reset();
					}
					else {
						timing_setcount(cnt - framecnt);
					}
					framereset(0);
				}
			}
			else {
				processwait(waitcnt);
				waitcnt = framecnt;
			}
		}
	}

	pccore_cfgupdate();
	if (np2oscfg.resume) {
		flagsave(str_sav);
	}
	else {
		flagdelete(str_sav);
	}
	pccore_term();
	S98_trash();
	soundmng_deinitialize();

	sysmng_deinitialize();

	scrnmng_destroy();
	sysmenu_destroy();
	TRACETERM();
	SDL_Quit();
	return(SUCCESS);

np2main_err5:
	pccore_term();
	S98_trash();
	soundmng_deinitialize();

np2main_err4:
	scrnmng_destroy();

np2main_err3:
	sysmenu_destroy();

np2main_err2:
	TRACETERM();
	SDL_Quit();

np2main_err1:
	return(FAILURE);
}

