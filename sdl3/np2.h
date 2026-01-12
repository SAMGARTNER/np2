
typedef struct {
	UINT8	NOWAIT;
	UINT8	DRAW_SKIP;
	UINT8	F12KEY;
	UINT8	resume;
	UINT8	jastsnd;
    UINT8 vf_profile;  // 0-2
    UINT8 vf_type[3][3];  // type[profile][filter]
    UINT8 vf_param[3][3][8];  // param[profile][filter][param]
} NP2OSCFG;


#if defined(SIZE_QVGA)
enum {
	FULLSCREEN_WIDTH	= 320,
	FULLSCREEN_HEIGHT	= 240
};
#else
enum {
	FULLSCREEN_WIDTH	= 640,
	FULLSCREEN_HEIGHT	= 400
};
#endif

extern	NP2OSCFG	np2oscfg;

extern int np2_main(int argc, char *argv[]);
