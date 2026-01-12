#ifdef __cplusplus
extern "C" {
#endif

extern SINT16 mouse_dx;
extern SINT16 mouse_dy;
extern UINT8 mouse_btn;

UINT8 mousemng_getstat(SINT16 *x, SINT16 *y, int clear);

#ifdef __cplusplus
}
#endif
