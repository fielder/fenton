#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "fenton.h"
#include "appio.h"

static SDL_Surface *sdl_surf = NULL;

struct video_s video;
struct input_s input;


void
IO_Print (const char *s)
{
	fputs (s, stdout);
	fflush (stdout);
}


void
IO_Init (void)
{
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
	{
		F_Error ("video init failed");
		F_Quit ();
	}
}


static void
DestroyWindow (void)
{
	if (sdl_surf != NULL)
	{
		SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		SDL_WM_GrabInput (SDL_GRAB_OFF);
		SDL_ShowCursor (SDL_ENABLE);

		SDL_FreeSurface (sdl_surf);
		sdl_surf = NULL;
	}

#if 0
	if (video.bouncebuf != NULL)
	{
		free (video.bouncebuf);
		video.bouncebuf = NULL;
	}

	if (video.rows != NULL)
	{
		free (video.rows);
		video.rows = NULL;
	}
#endif
}


void
IO_SetMode (int w, int h, int bpp, int scale)
{
	DestroyWindow ();
	Uint32 flags;
#if 0
	int y;
#endif

	IO_Print ("Setting mode %dx%dx%d\n", w, h, bpp);
	printf ("Scaling %dx\n", scale);

	flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_HWPALETTE;
#if 0
	if ((sdl_surf = SDL_SetVideoMode(video.w * video.scale, video.h * video.scale, sizeof(pixel_t) * 8, flags)) == NULL)
	{
		fprintf (stderr, "ERROR: failed setting video mode\n");
		Quit ();
	}
	printf ("Set %dx%dx%d real video mode\n", sdl_surf->w, sdl_surf->h, sdl_surf->format->BytesPerPixel * 8);
	printf ("RGB mask: 0x%x 0x%x 0x%x\n", sdl_surf->format->Rmask, sdl_surf->format->Gmask, sdl_surf->format->Bmask);

	video.rows = malloc (video.h * sizeof(*video.rows));

	if (video.scale == 1)
	{
		for (y = 0; y < video.h; y++)
			video.rows[y] = (pixel_t *)((uintptr_t)sdl_surf->pixels + y * sdl_surf->pitch);
	}
	else
	{
		video.bouncebuf = malloc (video.w * video.h * sizeof(pixel_t));
		for (y = 0; y < video.h; y++)
			video.rows[y] = video.bouncebuf + y * video.w;
	}
#endif
}


void
IO_Shutdown (void)
{
	DestroyWindow ();
}


void
IO_Terminate (void)
{
	SDL_Quit ();
	exit (EXIT_SUCCESS);
}


unsigned int
IO_MSecs (void)
{
	return SDL_GetTicks ();
}


static int _mouse_grabbed = 0;
static int _mouse_ignore_move = 1;

void
IO_FetchInput (void)
{
	SDL_Event sdlev;

	memset (input.key.press, 0, sizeof(input.key.press));
	memset (input.key.release, 0, sizeof(input.key.release));

	input.mouse.delta[0] = 0;
	input.mouse.delta[1] = 0;
	memset (input.mouse.button.press, 0, sizeof(input.mouse.button.press));
	memset (input.mouse.button.release, 0, sizeof(input.mouse.button.release));

	while (SDL_PollEvent(&sdlev))
	{
		switch (sdlev.type)
		{
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			{
				int k = sdlev.key.keysym.sym;
				if (k >= 0 && k < sizeof(input.key.state) / sizeof(input.key.state[0]))
				{
					if (sdlev.type == SDL_KEYDOWN)
					{
						input.key.state[k] = 1;
						input.key.press[k] = 1;
					}
					else
					{
						input.key.state[k] = 0;
						input.key.release[k] = 1;
					}
				}
				break;
			}

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				int b = -1;
				switch (sdlev.button.button)
				{
					case 1: b = MBUTTON_LEFT; break;
					case 2: b = MBUTTON_MIDDLE; break;
					case 3: b = MBUTTON_RIGHT; break;
					case 4: b = MBUTTON_WHEEL_UP; break;
					case 5: b = MBUTTON_WHEEL_DOWN; break;
					default: break;
				}

				if (b != -1)
				{
					if (sdlev.type == SDL_MOUSEBUTTONDOWN)
					{
						input.mouse.button.state[b] = 1;
						input.mouse.button.press[b] = 1;
					}
					else
					{
						input.mouse.button.state[b] = 0;
						input.mouse.button.release[b] = 1;
					}
				}

				break;
			}

			case SDL_MOUSEMOTION:
			{
				if (_mouse_grabbed)
				{
					/* when grabbing the mouse, the initial
					 * delta we get can be HUGE, so ignore
					 * the first mouse movement after grabbing
					 * input */
					if (_mouse_ignore_move == 0)
					{
						//FIXME: a grabbed mouse under vmware gives jacked deltas
						input.mouse.delta[0] = sdlev.motion.xrel;
						input.mouse.delta[1] = sdlev.motion.yrel;
					}
					else
					{
						_mouse_ignore_move = 0;
					}
				}
				break;
			}

			case SDL_QUIT:
				F_Quit ();
				break;

			default:
				break;
		}
	}
}


void
IO_Swap (void)
{
	//TODO
}


extern void
F_Init (void);

extern void
F_RunTime (int msecs);


int
main (int argc, const char **argv)
{
	unsigned int last, duration, now;

	if (SDL_InitSubSystem(SDL_INIT_TIMER) != 0)
	{
		fprintf (stderr, "ERROR: SDL init failed\n");
		exit (EXIT_FAILURE);
	}

	F_Init ();

	last = IO_MSecs ();
	for (;;)
	{
		do
		{
			now = IO_MSecs ();
			duration = now - last;
		} while (duration < 1);
		last = now;

		F_RunTime (duration);
	}

	return 0;
}
