#include <stdio.h>
#include <stdarg.h>

#include "map.h"
#include "fdata.h"
#include "vec.h"
#include "render.h"
#include "appio.h"
#include "fenton.h"

#define FLYSPEED 64.0
#define LOADPAK NULL

#if 0
/* wasd-style on a kinesis advantage w/ dvorak */
static const int bind_forward = '.';
static const int bind_back = 'e';
static const int bind_left = 'o';
static const int bind_right = 'u';
#else
/* qwerty-style using arrows */
static const int bind_forward = 'w';//FK_UP;
static const int bind_back = 's';//FK_DOWN;
static const int bind_left = 'a';//FK_LEFT;
static const int bind_right = 'd';//FK_RIGHT;
#endif

double frametime;
unsigned int elapsedtime_ms = 0;

static struct
{
	int framecount;
	double rate;
	unsigned int calc_start;
} fps;


void
F_Quit (void)
{
	Map_Unload ();
	R_Shutdown ();
	Data_CloseAll ();
	IO_Shutdown ();
	IO_Terminate ();
}


void
F_Error (const char *fmt, ...)
{
	char tmp[1024];
	va_list args;
	static int recursive = 0;

	va_start (args, fmt);
	vsnprintf (tmp, sizeof(tmp), fmt, args);
	va_end (args);

	IO_Print ("\nERROR: ");
	IO_Print (tmp);
	IO_Print ("\n");

	if (recursive)
	{
		IO_Print ("\nERROR: recursive errors\n");
		IO_Terminate ();
	}
	else
	{
		recursive = 1;
		F_Quit ();
	}
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
F_AddPak (const char *path)
{
	if (Data_AddPath(path))
		F_Log ("Loaded \"%s\"\n", path);
	else
		F_Error ("unable to load \"%s\"\n", path);
}


void
F_LoadMap (const char *name)
{
	if (!Map_Load(name))
	{
		F_Log ("Failed loading \"%s\": %s\n", name, map_error);
	}
	else
	{
		F_Log ("Loaded \"%s\"\n", name);
		F_Log (" %d planes\n", map.num_planes);
		F_Log (" %d vertices\n", map.num_vertices);
		F_Log (" %d edges\n", map.num_edges);
		F_Log (" %d portals\n", map.num_portals);
		F_Log (" %d leafs\n", map.num_leafs);
		F_Log (" %d nodes\n", map.num_nodes);
		F_Log (" %d surfaces\n", map.num_surfaces);
		F_Log (" %d bytes allocated\n", map.allocsz);
	}
}


void
F_Init (void)
{
	int w, h, bpp, scale, full;

	if (LOADPAK)
		F_AddPak (LOADPAK);

	IO_Init ();

	w = 320;
	h = 240;
	bpp = 24;
	scale = 1;
	full = 0;
	IO_SetMode (w, h, bpp, scale, full);

	R_Init ();

	R_CameraSizeChanged (w, h);
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

	R_Refresh ();

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
CameraMovement (void)
{
	/* camera angles */

	camera.angles[YAW] += -input.mouse.delta[0] * (camera.fov_x / video.w);
	while (camera.angles[YAW] >= 2.0 * M_PI)
		camera.angles[YAW] -= 2.0 * M_PI;
	while (camera.angles[YAW] < 0.0)
		camera.angles[YAW] += 2.0 * M_PI;
	camera.angles[PITCH] += input.mouse.delta[1] * (camera.fov_y / video.h);
	if (camera.angles[PITCH] > M_PI / 2.0)
		camera.angles[PITCH] = M_PI / 2.0;
	if (camera.angles[PITCH] < -M_PI / 2.0)
		camera.angles[PITCH] = -M_PI / 2.0;

	/* view vectors */

	R_CalcViewXForm ();

	/* movement */

	int left, forward, up;
	double speed = FLYSPEED;
	double v[3];

	if (input.key.state[FK_LSHIFT] || input.key.state[FK_RSHIFT])
		speed *= 1.5;

	left = 0;
	left += input.key.state[bind_left] ? 1 : 0;
	left -= input.key.state[bind_right] ? 1 : 0;
	Vec_Copy (camera.left, v);
	Vec_Scale (v, left * speed * frametime);
	Vec_Add (camera.pos, v, camera.pos);

	forward = 0;
	forward += input.key.state[bind_forward] ? 1 : 0;
	forward -= input.key.state[bind_back] ? 1 : 0;
	Vec_Copy (camera.forward, v);
	Vec_Scale (v, forward * speed * frametime);
	Vec_Add (camera.pos, v, camera.pos);

	up = 0;
	up += input.mouse.button.state[MBUTTON_RIGHT] ? 1 : 0;
	up -= input.mouse.button.state[MBUTTON_MIDDLE] ? 1 : 0;
	Vec_Copy (camera.up, v);
	Vec_Scale (v, up * speed * frametime);
	Vec_Add (camera.pos, v, camera.pos);
}


static void
RunInput (void)
{
	if (input.key.release[FK_ESCAPE])
		F_Quit ();

	if (input.key.release['g'])
		IO_ToggleGrab ();

	if (input.key.release['f'])
		F_Log ("%g\n", fps.rate);

	if (input.key.release['1'])
		F_LoadMap ("TEST1");
	if (input.key.release['2'])
		F_LoadMap ("TEST2");
	if (input.key.release['3'])
		F_LoadMap ("TEST3");
	if (input.key.release['4'])
		F_LoadMap ("TEST4");

	if (input.key.release['d'])
	{
	}

	if (input.key.release['p'])
	{
		if (input.key.state[FK_LSHIFT] || input.key.state[FK_RSHIFT])
		{
			Vec_Clear (camera.pos);
			Vec_Clear (camera.angles);
		}
		else
		{
			F_Log ("(%g %g %g)\n", camera.pos[0], camera.pos[1], camera.pos[2]);
			F_Log ("dist: %g\n", camera.dist);
			F_Log ("angles: %g %g %g\n", camera.angles[0], camera.angles[1], camera.angles[2]);
			F_Log ("left: (%g %g %g)\n", camera.left[0], camera.left[1], camera.left[2]);
			F_Log ("up: (%g %g %g)\n", camera.up[0], camera.up[1], camera.up[2]);
			F_Log ("forward: (%g %g %g)\n", camera.forward[0], camera.forward[1], camera.forward[2]);
			F_Log ("left: acc %d %d %d\n",camera.vplanes[0].accept[0],camera.vplanes[0].accept[1],camera.vplanes[0].accept[2]);
			F_Log ("      rej %d %d %d\n",camera.vplanes[0].reject[0],camera.vplanes[0].reject[1],camera.vplanes[0].reject[2]);
			F_Log ("righ: acc %d %d %d\n",camera.vplanes[1].accept[0],camera.vplanes[1].accept[1],camera.vplanes[1].accept[2]);
			F_Log ("      rej %d %d %d\n",camera.vplanes[1].reject[0],camera.vplanes[1].reject[1],camera.vplanes[1].reject[2]);
			F_Log ("top:  acc %d %d %d\n",camera.vplanes[2].accept[0],camera.vplanes[2].accept[1],camera.vplanes[2].accept[2]);
			F_Log ("      rej %d %d %d\n",camera.vplanes[2].reject[0],camera.vplanes[2].reject[1],camera.vplanes[2].reject[2]);
			F_Log ("bott: acc %d %d %d\n",camera.vplanes[3].accept[0],camera.vplanes[3].accept[1],camera.vplanes[3].accept[2]);
			F_Log ("      rej %d %d %d\n",camera.vplanes[3].reject[0],camera.vplanes[3].reject[1],camera.vplanes[3].reject[2]);
			F_Log ("\n");
		}
	}

	if (input.key.release['x'])
	{
		Vec_Clear (camera.pos);
		Vec_Clear (camera.angles);
		if (input.key.state[FK_LSHIFT] || input.key.state[FK_RSHIFT])
		{
			/* debug camera position */
			//...
		}
	}

	CameraMovement ();
}
