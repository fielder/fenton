#ifndef __APPIO_H__
#define __APPIO_H__

#include <SDL_keysym.h>

enum
{
	MBUTTON_LEFT,
	MBUTTON_MIDDLE,
	MBUTTON_RIGHT,
	MBUTTON_WHEEL_UP,
	MBUTTON_WHEEL_DOWN,
	MBUTTON_LAST,
};

struct input_s
{
	struct
	{
		int delta[2];
		struct
		{
			int state[MBUTTON_LAST];
			int press[MBUTTON_LAST];
			int release[MBUTTON_LAST];
		} button;
	} mouse;

	struct
	{
		int state[SDLK_LAST];
		int press[SDLK_LAST];
		int release[SDLK_LAST];
	} key;
};

struct video_s
{
	int w, h, bpp;

	/*
	pixel_t *bouncebuf;
	pixel_t **rows;
	pixel_t red, green, blue;
	*/

	int realw, realh, realbpp;
	int scale;
};

extern struct video_s video;
extern struct input_s input;

extern void
IO_Print (const char *s);

extern void
IO_Init (void);

extern void
IO_SetMode (int w, int h, int bpp, int scale);

extern void
IO_Shutdown (void);

extern void
IO_Terminate (void);

extern unsigned int
IO_MSecs (void);

extern void
IO_FetchInput (void);

extern void
IO_SetGrab (int grab);

extern void
IO_ToggleGrab (void);

extern void
IO_Swap (void);

#endif /* __APPIO_H__ */
