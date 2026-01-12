#include	"compiler.h"
// #include	<sys/time.h>
// #include	<signal.h>
// #include	<unistd.h>
#include	"scrnmng.h"
#include	"scrndraw.h"
#include	"vramhdl.h"
#include	"menubase.h"
#include <stdlib.h>

static SDL_Window *s_sdlWindow;
static SDL_Renderer *s_renderer;
static SDL_Texture *s_texture;

typedef struct {
	BOOL		enable;
	int			width;
	int			height;
	int			bpp;
	SDL_Surface	*surface;
	VRAMHDL		vram;
} SCRNMNG;

typedef struct {
	int		width;
	int		height;
} SCRNSTAT;

static const char app_name[] = "Neko Project II";

static	SCRNMNG		scrnmng;
static	SCRNSTAT	scrnstat;
static	SCRNSURF	scrnsurf;

typedef struct {
	int		xalign;
	int		yalign;
	int		width;
	int		height;
	int		srcpos;
	int		dstpos;
} DRAWRECT;

static BRESULT calcdrawrect(DRAWRECT *dr, VRAMHDL s, const RECT_T *rt) {
    int pos;

    dr->xalign = 2;  // RGB565: 2 bytes per pixel
    dr->yalign = scrnmng.width * 2;  // fixed pitch
    dr->srcpos = 0;
    dr->dstpos = 0;
    dr->width = min(scrnmng.width, s->width);
    dr->height = min(scrnmng.height, s->height);

    if (rt) {
        pos = max(rt->left, 0);
        dr->srcpos += pos;
        dr->dstpos += pos * dr->xalign;
        dr->width = min(rt->right, dr->width) - pos;

        pos = max(rt->top, 0);
        dr->srcpos += pos * s->width;
        dr->dstpos += pos * dr->yalign;
        dr->height = min(rt->bottom, dr->height) - pos;
    }
    if ((dr->width <= 0) || (dr->height <= 0)) {
        return(FAILURE);
    }
    return(SUCCESS);
}


void scrnmng_initialize(void) {

	scrnstat.width = 640;
	scrnstat.height = 400;
}

SDL_Window* scrnmng_get_window(void) {
    return s_sdlWindow;
}

BRESULT scrnmng_create(int width, int height) {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Error: SDL_Init: %s\n", SDL_GetError());
        return(FAILURE);
    }
    s_sdlWindow = SDL_CreateWindow(app_name, width, height, SDL_WINDOW_RESIZABLE);
    if (!s_sdlWindow) return FAILURE;

    s_renderer = SDL_CreateRenderer(s_sdlWindow, NULL);
    if (!s_renderer) return FAILURE;

    //SDL_SetRenderLogicalPresentation(s_renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    
    SDL_SetRenderLogicalPresentation(s_renderer, 640, 400, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!s_texture) return FAILURE;

    scrnmng.width = width;
    scrnmng.height = height;
    scrnmng.bpp = 16;
    scrnmng.enable = TRUE;

    return SUCCESS;
}

void scrnmng_destroy(void) {

	scrnmng.enable = FALSE;
}

RGB16 scrnmng_makepal16(RGB32 pal32) {

	RGB16	ret;

	ret = (pal32.p.r & 0xf8) << 8;
#if defined(SIZE_QVGA)
	ret += (pal32.p.g & 0xfc) << (3 + 16);
#else
	ret += (pal32.p.g & 0xfc) << 3;
#endif
	ret += pal32.p.b >> 3;
	return(ret);
}

void scrnmng_setwidth(int posx, int width) {

	scrnstat.width = width;
}

void scrnmng_setheight(int posy, int height) {

	scrnstat.height = height;
}

const SCRNSURF *scrnmng_surflock(void) {
    void *pixels;
    int pitch;

    if (!scrnmng.enable) return NULL;

    if (SDL_LockTexture(s_texture, NULL, &pixels, &pitch) < 0) return NULL;

    scrnsurf.ptr = (UINT8*)pixels;
    scrnsurf.xalign = 2;
    scrnsurf.yalign = pitch;
    scrnsurf.width = scrnmng.width;
    scrnsurf.height = scrnmng.height;
    scrnsurf.bpp = 16;

    return &scrnsurf;
}

static void draw_onmenu(void) {
    RECT_T rt;
    DRAWRECT dr;
    const UINT8 *p, *q, *a;
    int salign, dalign, x;

    rt.left = 0;
    rt.top = 0;
    rt.right = min(scrnstat.width, 640);
    rt.bottom = min(scrnstat.height, 400);
#if defined(SIZE_QVGA)
    rt.right >>= 1;
    rt.bottom >>= 1;
#endif

    void *pixels;
    int pitch;
    if (SDL_LockTexture(s_texture, NULL, &pixels, &pitch) < 0) return;
    UINT8 *dst = (UINT8 *)pixels;

    if (calcdrawrect(&dr, menuvram, &rt) == SUCCESS) {
        p = scrnmng.vram->ptr + (dr.srcpos * 2);
        q = dst + dr.dstpos;
        a = menuvram->alpha + dr.srcpos;
        salign = menuvram->width;
        dalign = pitch - (dr.width * 2);  // xalign=2 for RGB565

        do {
            x = 0;
            do {
                if (a[x] == 0) {
                    *(UINT16 *)q = *(UINT16 *)(p + (x * 2));
                }
                q += 2;
            } while (++x < dr.width);
            p += salign * 2;
            q += dalign;
            a += salign;
        } while (--dr.height);
    }

    SDL_UnlockTexture(s_texture);
    SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 255);
    SDL_RenderClear(s_renderer);
    SDL_RenderTexture(s_renderer, s_texture, NULL, NULL);
    SDL_RenderPresent(s_renderer);
}

static void draw_onmenu_composite(void) {
    // Copy entire body of draw_onmenu() without the final part:
    RECT_T rt = {0, 0, scrnmng.width, scrnmng.height};
    DRAWRECT dr;
    void *pixels;
    int pitch;

    if (calcdrawrect(&dr, menuvram, &rt) != SUCCESS) return;

    if (SDL_LockTexture(s_texture, NULL, &pixels, &pitch) < 0) return;

    UINT8 *p = menuvram->ptr + (dr.srcpos * 2);       // menu pixels
    UINT8 *q = (UINT8*)pixels + dr.dstpos;           // destination (emulation)
    UINT8 *a = menuvram->alpha + dr.srcpos;
    int salign = menuvram->width;
    int dalign = pitch - (dr.width * 2);
    int x;

    do {
        x = 0;
        do {
            if (a[x] != 0) {
                *(UINT16 *)q = *(UINT16 *)(p + (x * 2));      // draw menu pixel where alpha > 0
            }
            q += 2;
        } while (++x < dr.width);
        p += salign * 2;
        q += dalign;
        a += salign;
    } while (--dr.height);

    SDL_UnlockTexture(s_texture);
}

void scrnmng_surfunlock(const SCRNSURF *surf) {
    if (surf) {
        SDL_UnlockTexture(s_texture);

        if (scrnmng.vram != NULL && menuvram) {
            draw_onmenu_composite();
        }

        SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 255);
        SDL_RenderClear(s_renderer);
        SDL_RenderTexture(s_renderer, s_texture, NULL, NULL);
        SDL_RenderPresent(s_renderer);
    }
}


// ----

BRESULT scrnmng_entermenu(SCRNMENU *smenu) {

	if (smenu == NULL) {
		goto smem_err;
	}
	vram_destroy(scrnmng.vram);
	scrnmng.vram = vram_create(scrnmng.width, scrnmng.height, FALSE,
																scrnmng.bpp);
	if (scrnmng.vram == NULL) {
		goto smem_err;
	}
	scrndraw_redraw();
	smenu->width = scrnmng.width;
	smenu->height = scrnmng.height;
	smenu->bpp = (scrnmng.bpp == 32)?24:scrnmng.bpp;
	return(SUCCESS);

smem_err:
	return(FAILURE);
}

void scrnmng_leavemenu(void) {

	VRAM_RELEASE(scrnmng.vram);
    scrndraw_redraw();
}

void scrnmng_menudraw(const RECT_T *rct) {
    if (!scrnmng.enable || menuvram == NULL) return;

    DRAWRECT dr;
    UINT8 *p, *q, *alpha;
    UINT8 *dst;
    int salign, dalign, x;

    void *pixels;
    int pitch;
    if (SDL_LockTexture(s_texture, NULL, &pixels, &pitch) < 0) return;
    dst = (UINT8 *)pixels;

    if (calcdrawrect(&dr, menuvram, rct) == SUCCESS) {
        int base_offset = menuvram->posy * pitch + menuvram->posx * 2;

        p = scrnmng.vram->ptr + (dr.srcpos * 2);
        q = menuvram->ptr + (dr.srcpos * 2);
        alpha = menuvram->alpha + dr.srcpos;
        salign = menuvram->width;
        dalign = pitch - (dr.width * 2);

        int y = dr.height;
        dst += base_offset + dr.dstpos;

        do {
            x = 0;
            UINT8 *line_dst = dst;
            do {
                if (alpha[x]) {
                    if (alpha[x] & 2) {
                        *(UINT16 *)line_dst = *(UINT16 *)(q + (x * 2));
                    } else {
                        alpha[x] = 0;
                        *(UINT16 *)line_dst = *(UINT16 *)(p + (x * 2));
                    }
                }
                line_dst += 2;
            } while (++x < dr.width);
            p += salign * 2;
            q += salign * 2;
            alpha += salign;
            dst += pitch;
        } while (--y);
    }

    SDL_UnlockTexture(s_texture);
    SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 255);
    SDL_RenderClear(s_renderer);
    SDL_RenderTexture(s_renderer, s_texture, NULL, NULL);
    SDL_RenderPresent(s_renderer);
}

