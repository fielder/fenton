#include <stdarg.h>
#include <stdio.h>

//#include "bswap.h"
//#include "pak.h"
//#include "render.h"
#include "appio.h"
#include "fenton.h"

double frametime;
unsigned int elapsedtime_ms = 0;

//static const char *pakpath = "doom.pak";

static struct
{
	int framecount;
	double rate;
	unsigned int calc_start;
} fps;


void
F_Quit (void)
{
	//R_Shutdown ();
	//Pak_CloseAll ();
	IO_Shutdown ();
	IO_Terminate ();
}


void
F_Error (const char *fmt, ...)
{
	char tmp[1024];
	va_list args;
	static int recursive = 0;

	if (recursive)
	{
		IO_Print ("\nERROR: recursive errors\n");
		IO_Terminate ();
	}
	recursive = 1;

	va_start (args, fmt);
	vsnprintf (tmp, sizeof(tmp), fmt, args);
	va_end (args);

	IO_Print ("\nERROR: ");
	IO_Print (tmp);
	IO_Print ("\n");

	F_Quit ();
}


void
F_Log (const char *fmt, ...)
{
	char tmp[1024];
	va_list args;

	va_start (args, fmt);
	vsnprintf (tmp, sizeof(tmp), fmt, args);
	va_end (args);

	IO_Print (tmp);
}


void
F_Init (void)
{
	//SwapInit ();

//	if (!Pak_AddFile(pakpath))
//		F_Error ("unable to load %s", pakpath);

	IO_Init ();

	IO_SetMode (640, 480, 24, 2, 0);

//	R_Init ();
}


static void
RunInput (void);

void
F_RunTime (int msecs)
{
	frametime = msecs / 1000.0;
	elapsedtime_ms += msecs;

	IO_FetchInput ();

	RunInput ();

	{
		int i;
		for (i = 0; i < video.h; i++)
		{
			if (video.bpp == 16)
			{
				((unsigned short *)video.rows[i])[i] = 0xffff;
			}
			else
			{
				((unsigned int *)video.rows[i])[i] = 0x000000ff;
			}
		}
	}
//	R_Refresh ();

	IO_Swap ();

	fps.framecount++;

	/* calculate the framerate */
	{
		unsigned int now = IO_MSecs ();
		if ((now - fps.calc_start) > 250)
		{
			fps.rate = fps.framecount / ((now - fps.calc_start) / 1000.0);
			fps.framecount = 0;
			fps.calc_start = now;
		}
	}
}


static void
RunInput (void)
{
	if (input.key.release[FK_ESCAPE])
		F_Quit ();

	if (input.key.release['g'])
		IO_ToggleGrab ();

	if (input.key.press['f'])
		F_Log ("%g\n", fps.rate);

#if 0
	r_showtex = input.key.state['\''];

	if (input.key.release['c'])
	{
		if (input.key.state[SDLK_LSHIFT] || input.key.state[SDLK_RSHIFT])
		{
			Vec_Clear (camera.pos);
			Vec_Clear (camera.angles);
		}
		else
			PrintCamera ();
	}

	if (input.key.release['d'])
		r_debugframe = 1;

	if (input.key.release['x'])
	{
		Vec_Clear (camera.pos);
		Vec_Clear (camera.angles);
		if (input.key.state[SDLK_LSHIFT] || input.key.state[SDLK_RSHIFT])
		{
			r_debugframe = 1;

			/* causes just a tiny chip of the distant poly
			 * to be clipped into the view, but does not
			 * cross any pixel centers */
			camera.pos[0] = -115.205;
			camera.pos[1] = 86.2427;
			camera.pos[2] = 18.0858;
			camera.angles[0] = 0.836552;
			camera.angles[1] = 0.83939;
			camera.angles[2] = 0;

			/* camera is so close to the poly that the clip
			 * epsilon is many pixels wide, rejecting edges
			 * on the back-face of a vplane */
			/*
			camera.pos[0] = 94.8478;
			camera.pos[1] = 0.0;
			camera.pos[2] = 510.809;
			*/

			/*
			camera.pos[0] = 94.8478;
			camera.pos[1] = 0.0;
			camera.pos[2] = 507.16;
			*/

			/*
			camera.pos[0] = 96.3198;
			camera.pos[1] = 0.0;
			camera.pos[2] = 511.767;
			*/
		}
		else
		{
			camera.pos[0] = 94.8478012;
			camera.pos[1] = 0.0;
			camera.pos[2] = 507.256012 - 20;
		}
	}

	CameraMovement ();
#endif
}
