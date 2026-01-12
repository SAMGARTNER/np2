#include "compiler.h"

SINT16 mouse_dx = 0;
SINT16 mouse_dy = 0;
UINT8 mouse_btn = 0xff;

UINT8 mousemng_getstat(SINT16 *x, SINT16 *y, int clear) {
    //printf("Sending to emulator: dx=%d dy=%d\n", mouse_dx, mouse_dy);
    if (x) *x = mouse_dx;
    if (y) *y = mouse_dy;
    UINT8 ret = mouse_btn;
    if (clear) {
        mouse_dx = 0;
        mouse_dy = 0;
    }
    return ret;
}
