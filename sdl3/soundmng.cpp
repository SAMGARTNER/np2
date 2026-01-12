#include "compiler.h"
#include "soundmng.h"
#include <algorithm>
#include "parts.h"
#include "sound.h"
#if defined(VERMOUTH_LIB)
#include "commng.h"
#include "cmver.h"
#endif
#if defined(SUPPORT_EXTERNALCHIP)
#include "ext/externalchipmanager.h"
#endif

#define	NSNDBUF				2

typedef struct {
	BOOL	opened;
	int		nsndbuf;
	int		samples;
	SINT16	*buf[NSNDBUF];
} SOUNDMNG;

static	SOUNDMNG	soundmng;

static SDL_AudioStream *audio_stream = NULL;

static void sound_play_cb(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
    (void)total_amount;
    const SINT32 *src = sound_pcmlock();
    if (src) {
        SDL_PutAudioStreamData(stream, src, additional_amount * 2);  // SINT32 → convert to SINT16
        sound_pcmunlock(src);
    } else {
        Uint8 silence[additional_amount];
        SDL_memset(silence, 0, additional_amount);
        SDL_PutAudioStreamData(stream, silence, additional_amount);
    }
}

UINT soundmng_create(UINT rate, UINT ms) {

	SDL_AudioSpec	fmt;
	UINT			s;
	UINT			samples;
	SINT16			*tmp;

	if (soundmng.opened) {
		goto smcre_err1;
	}
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Error: SDL_Init: %s\n", SDL_GetError());
		goto smcre_err1;
	}

	s = rate * ms / (NSNDBUF * 1000);
	samples = 1;
	while(s > samples) {
		samples <<= 1;
	}
	soundmng.nsndbuf = 0;
	soundmng.samples = samples;
	for (s=0; s<NSNDBUF; s++) {
		tmp = (SINT16 *)_MALLOC(samples * 2 * sizeof(SINT16), "buf");
		if (tmp == NULL) {
			goto smcre_err2;
		}
		soundmng.buf[s] = tmp;
		ZeroMemory(tmp, samples * 2 * sizeof(SINT16));
	}

	ZeroMemory(&fmt, sizeof(fmt));
	fmt.freq = rate;
	fmt.format = SDL_AUDIO_S16;
	fmt.channels = 2;
    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &fmt, sound_play_cb, NULL);
    if (!audio_stream) {
        fprintf(stderr, "Error: SDL_OpenAudioDeviceStream: %s\n", SDL_GetError());
        return FAILURE;
    }
    SDL_ResumeAudioStreamDevice(audio_stream);  // Add this line
#if defined(VERMOUTH_LIB)
	cmvermouth_load(rate);
#endif
	soundmng.opened = TRUE;
	return(samples);

smcre_err2:
	for (s=0; s<NSNDBUF; s++) {
		tmp = soundmng.buf[s];
		soundmng.buf[s] = NULL;
		if (tmp) {
			_MFREE(tmp);
		}
	}

smcre_err1:
	return(0);
}

void soundmng_destroy(void) {

	int		i;
	SINT16	*tmp;

	if (soundmng.opened) {
		soundmng.opened = FALSE;
        SDL_PauseAudioStreamDevice(audio_stream);
        SDL_DestroyAudioStream(audio_stream); audio_stream = NULL;
		for (i=0; i<NSNDBUF; i++) {
			tmp = soundmng.buf[i];
			soundmng.buf[i] = NULL;
			_MFREE(tmp);
		}
#if defined(VERMOUTH_LIB)
//		cmvermouth_unload();
#endif
	}
}

void soundmng_play(void)
{
	if (soundmng.opened)
	{
        SDL_ResumeAudioStreamDevice(audio_stream);
#if defined(SUPPORT_EXTERNALCHIP)
		CExternalChipManager::GetInstance()->Mute(false);
#endif
	}
}

void soundmng_stop(void)
{
	if (soundmng.opened)
	{
        SDL_PauseAudioStreamDevice(audio_stream);
#if defined(SUPPORT_EXTERNALCHIP)
		CExternalChipManager::GetInstance()->Mute(true);
#endif
	}
}


// ----

void soundmng_initialize()
{
#if defined(SUPPORT_EXTERNALCHIP)
	CExternalChipManager::GetInstance()->Initialize();
#endif
}

void soundmng_deinitialize()
{
#if defined(SUPPORT_EXTERNALCHIP)
	CExternalChipManager::GetInstance()->Deinitialize();
#endif

#if defined(VERMOUTH_LIB)
	cmvermouth_unload();
#endif
}
