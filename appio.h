#ifndef __APPIO_H__
#define __APPIO_H__

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
IO_Swap (void);

#endif /* __APPIO_H__ */
