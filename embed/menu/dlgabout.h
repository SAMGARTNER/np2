
#if defined(SIZE_QVGA)
enum {
	DLGABOUT_WIDTH	= 240,
	DLGABOUT_HEIGHT	= 40
};
#else
enum {
	DLGABOUT_WIDTH	= 350,
	DLGABOUT_HEIGHT	= 58
};
#endif


#ifdef __cplusplus
extern "C" {
#endif

int dlgabout_cmd(int msg, MENUID id, long param);

#ifdef __cplusplus
}
#endif

