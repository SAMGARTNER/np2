/**
 * @file	sdlkey.c
 * @brief	Implementation of the keyboard
 */

#include "compiler.h"
#include "np2.h"
#include "sdlkbd.h"
#include "keystat.h"

#include <SDL3/SDL_mouse.h>
#include "scrnmng.h"
#include <stdbool.h>
#include "sdlkbd.h"

extern int mouse_x;
extern int mouse_y;
extern UINT8 mouse_buttons;

typedef struct {
	SDL_Keycode sdlkey;
	UINT8 keycode;
} SDLKCNV;

#define		NC		0xff

/*! 101 keyboard key table */
static const SDLKCNV sdlcnv101[] =
{
			{SDLK_ESCAPE,		0x00},	{SDLK_1,			0x01},
			{SDLK_2,			0x02},	{SDLK_3,			0x03},
			{SDLK_4,			0x04},	{SDLK_5,			0x05},
			{SDLK_6,			0x06},	{SDLK_7,			0x07},

			{SDLK_8,			0x08},	{SDLK_9,			0x09},
			{SDLK_0,			0x0a},	{SDLK_MINUS,		0x0b},
			{SDLK_CARET,		0x0c},	{SDLK_BACKSLASH,	0x0d},
			{SDLK_BACKSPACE,	0x0e},	{SDLK_TAB,			0x0f},

			{SDLK_Q,			0x10},	{SDLK_W,			0x11},
			{SDLK_E,			0x12},	{SDLK_R,			0x13},
			{SDLK_T,			0x14},	{SDLK_Y,			0x15},
			{SDLK_U,			0x16},	{SDLK_I,			0x17},

			{SDLK_O,			0x18},	{SDLK_P,			0x19},
			{SDLK_AT,			0x1a},	{SDLK_LEFTBRACKET,	0x1b},
			{SDLK_RETURN,		0x1c},	{SDLK_A,			0x1d},
			{SDLK_S,			0x1e},	{SDLK_D,			0x1f},

			{SDLK_F,			0x20},	{SDLK_G,			0x21},
			{SDLK_H,			0x22},	{SDLK_J,			0x23},
			{SDLK_K,			0x24},	{SDLK_L,			0x25},
			{SDLK_SEMICOLON,	0x26},	{SDLK_COLON,		0x27},

			{SDLK_RIGHTBRACKET,	0x28},	{SDLK_Z,			0x29},
			{SDLK_X,			0x2a},	{SDLK_C,			0x2b},
			{SDLK_V,			0x2c},	{SDLK_B,			0x2d},
			{SDLK_N,			0x2e},	{SDLK_M,			0x2f},

			{SDLK_COMMA,		0x30},	{SDLK_PERIOD,		0x31},
			{SDLK_SLASH,		0x32},	{SDLK_UNDERSCORE,	0x33},
			{SDLK_SPACE,		0x34},
			{SDLK_PAGEUP,		0x36},	{SDLK_PAGEDOWN,		0x37},

			{SDLK_INSERT,		0x38},	{SDLK_DELETE,		0x39},
			{SDLK_UP,			0x3a},	{SDLK_LEFT,			0x3b},
			{SDLK_RIGHT,		0x3c},	{SDLK_DOWN,			0x3d},
			{SDLK_HOME,			0x3e},	{SDLK_END,			0x3f},

			{SDLK_KP_MINUS,		0x40},	{SDLK_KP_DIVIDE,	0x41},
			{SDLK_KP_7,			0x42},	{SDLK_KP_8,			0x43},
			{SDLK_KP_9,			0x44},	{SDLK_KP_MULTIPLY,	0x45},
			{SDLK_KP_4,			0x46},	{SDLK_KP_5,			0x47},

			{SDLK_KP_6,			0x48},	{SDLK_KP_PLUS,		0x49},
			{SDLK_KP_1,			0x4a},	{SDLK_KP_2,			0x4b},
			{SDLK_KP_3,			0x4c},	{SDLK_KP_EQUALS,	0x4d},
			{SDLK_KP_0,			0x4e},

			{SDLK_KP_PERIOD,	0x50},

			{SDLK_PAUSE,		0x60},	{SDLK_PRINTSCREEN,	0x61},
			{SDLK_F1,			0x62},	{SDLK_F2,			0x63},
			{SDLK_F3,			0x64},	{SDLK_F4,			0x65},
			{SDLK_F5,			0x66},	{SDLK_F6,			0x67},

			{SDLK_F7,			0x68},	{SDLK_F8,			0x69},
			{SDLK_F9,			0x6a},	{SDLK_F10,			0x6b},

			{SDLK_RSHIFT,		0x70},	{SDLK_LSHIFT,		0x70},
			{SDLK_CAPSLOCK,		0x71},
			{SDLK_RALT,			0x73},	{SDLK_LALT,			0x73},
			{SDLK_RCTRL,		0x74},	{SDLK_LCTRL,		0x74},

			/* = */
			{SDLK_EQUALS,		0x0c},

			/* MacOS Yen */
			{0xa5,				0x0d},
};

/*! extend key */
static const UINT8 f12keys[] = {0x61, 0x60, 0x4d, 0x4f};

/**
 * Initialize
 */
void sdlkbd_initialize(void)
{
}

/**
 * Serializes
 * @param[in] key Key code
 * @return PC-98 data
 */
static UINT8 getKey(SDL_Keycode key)
{
	size_t i;

	for (i = 0; i < SDL_arraysize(sdlcnv101); i++)
	{
		if (sdlcnv101[i].sdlkey == key)
		{
			return sdlcnv101[i].keycode;
		}
	}
	return NC;
}

/**
 * Get F12 settings
 * @return PC-98 data
 */
static UINT8 getf12key(void)
{
	UINT	key;

	key = np2oscfg.F12KEY - 1;
	if (key < SDL_arraysize(f12keys))
	{
		return f12keys[key];
	}
	else
	{
		return NC;
	}
}

void sdlkbd_keydown(UINT key)
{
    UINT8 data;

    printf("KEY DOWN: SDLK_%s (0x%x)", SDL_GetKeyName(key), key);

    if (key == SDLK_F12)
    {
        data = getf12key();
        printf(" -> F12 remapped to 0x%02x", data);
        if (data == NC) {
            mouse_captured = !mouse_captured;
            if (mouse_captured) {
                //SDL_HideCursor();
                mouse_x = 320;
                mouse_y = 200;
            } else {
                //SDL_ShowCursor();
            }
            printf(" (Mouse capture: %s)\n", mouse_captured ? "ON" : "OFF");
            return;
        }
        printf("\n");
    }
    else
    {
        data = getKey(key);
        printf(" -> PC98 0x%02x\n", data);
    }
    if (data != NC) {
        keystat_senddata(data);
    } else if (key != SDLK_F12) {
        printf(" (ignored)\n");
    }
}

void sdlkbd_keyup(UINT key)
{
    UINT8 data;

    printf("KEY UP:   SDLK_%s (0x%x)", SDL_GetKeyName(key), key);

    if (key == SDLK_F12)
    {
        data = getf12key();
        printf(" -> F12 remapped to 0x%02x\n", data);
    }
    else
    {
        data = getKey(key);
        printf(" -> PC98 0x%02x\n", data | 0x80);
    }
    if (data != NC)
    {
        keystat_senddata((UINT8)(data | 0x80));
    }
    else
    {
        printf(" (ignored)\n");
    }
}

/**
 * Reset F12
 */
void sdlkbd_resetf12(void)
{
	size_t i;

	for (i = 0; i < SDL_arraysize(f12keys); i++)
	{
		keystat_forcerelease(f12keys[i]);
	}
}
