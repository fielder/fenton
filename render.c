#include <string.h>
#include <math.h>
#include <stdint.h>

//#include "rdata.h"
#include "map.h"
#include "appio.h"
#include "vec.h"
#include "render.h"

char * const vplane_names[4] = { "left", "right", "top", "bottom" };
static const double fov_x = 90.0;

struct camera_s camera;


void
R_Init (void)
{
	camera.fov_x = DEG2RAD(fov_x);

	Vec_Clear (camera.pos);
	Vec_Clear (camera.angles);

	R_CalcViewXForm ();
}


void
R_Shutdown (void)
{
	R_Span_Cleanup ();
}


void
R_CameraSizeChanged (int w, int h)
{
	camera.center_x = w / 2.0;
	camera.center_y = h / 2.0;

	camera.dist = (w / 2.0) / tan(camera.fov_x / 2.0);
	camera.fov_y = 2.0 * atan((h / 2.0) / camera.dist);

	R_Span_Init ();
}


void
R_CalcViewXForm (void)
{
	double v[3];

	/* make transformation matrix */
	Vec_Copy (camera.angles, v);
	Vec_Scale (v, -1.0);
	Vec_AnglesMatrix (v, camera.xform, "xyz");

	/* get view vectors */
	Vec_Copy (camera.xform[0], camera.left);
	Vec_Copy (camera.xform[1], camera.up);
	Vec_Copy (camera.xform[2], camera.forward);
}


/*
 * At the start of each frame render we classify each viewplane by
 * the direction of its normal. This classification tells which bbox
 * corner the plane points to. This is used to check for bboxes that
 * are fully in front of a viewplane (trivially accept nodes and reduce
 * clip checks). The opposite bbox corner is used to check when the bbox
 * is fully behind the plane (trivially rejected nodes/leafs terminate
 * tree traversal for that node).
 */

/* indices of a bbox, taken as a straight array of 6 values */
#define MINX 0
#define MINY 1
#define MINZ 2
#define MAXX 3
#define MAXY 4
#define MAXZ 5
static const int _bboxmap[8][3] = {
	{ MINX, MINY, MINZ },
	{ MAXX, MINY, MINZ },
	{ MINX, MAXY, MINZ },
	{ MAXX, MAXY, MINZ },
	{ MINX, MINY, MAXZ },
	{ MAXX, MINY, MAXZ },
	{ MINX, MAXY, MAXZ },
	{ MAXX, MAXY, MAXZ },
};
#undef MINX
#undef MINY
#undef MINZ
#undef MAXX
#undef MAXY
#undef MAXZ


static void
CalcAcceptReject (struct viewplane_s *p)
{
	int planeclass =	((p->normal[0] < 0.0) << 0) +
				((p->normal[1] < 0.0) << 1) +
				((p->normal[2] < 0.0) << 2);

	/* The fully-accept corner. If this corner is in front of a
	 * plane then the whole bbox (and its contents) are in front of
	 * the plane.
	 */
	p->accept[0] = _bboxmap[planeclass][0];
	p->accept[1] = _bboxmap[planeclass][1];
	p->accept[2] = _bboxmap[planeclass][2];

	/* The fully-reject corner. If this corner is behind a plane
	 * then the whole bbox (and its contents) are behind the plane.
	 */
	p->reject[0] = _bboxmap[7 - planeclass][0];
	p->reject[1] = _bboxmap[7 - planeclass][1];
	p->reject[2] = _bboxmap[7 - planeclass][2];
}


static void
CalcViewPlanes (void)
{
	double cam2world[3][3];
	double v[3];
	struct viewplane_s *p;
	double ang;

	/* view to world transformation matrix */
	Vec_AnglesMatrix (camera.angles, cam2world, "zyx");

	p = &camera.vplanes[VPLANE_LEFT];
	ang = (camera.fov_x / 2.0);
	v[0] = -cos (ang);
	v[1] = 0.0;
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, camera.pos);
	CalcAcceptReject (p);

	p = &camera.vplanes[VPLANE_RIGHT];
	ang = (camera.fov_x / 2.0);
	v[0] = cos (ang);
	v[1] = 0.0;
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, camera.pos);
	CalcAcceptReject (p);

	p = &camera.vplanes[VPLANE_TOP];
	ang = (camera.fov_y / 2.0);
	v[0] = 0.0;
	v[1] = -cos (ang);
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, camera.pos);
	CalcAcceptReject (p);

	p = &camera.vplanes[VPLANE_BOTTOM];
	ang = (camera.fov_y / 2.0);
	v[0] = 0.0;
	v[1] = cos (ang);
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, camera.pos);
	CalcAcceptReject (p);
}


static void
Render3DPoint (double x, double y, double z)
{
	double v[3], local[3], out[3];

	v[0] = x; v[1] = y; v[2] = z;

	Vec_Subtract (v, camera.pos, local);
	Vec_Transform (camera.xform, local, out);
	if (out[2] > 0.0)
	{
		int sx = camera.center_x - camera.dist * (out[0] / out[2]) + 0.5;
		int sy = camera.center_y - camera.dist * (out[1] / out[2]) + 0.5;
		if (sx >= 0 && sx < video.w && sy >= 0 && sy < video.h)
		{
			if (video.bpp == 16)
				((unsigned short *)video.rows[sy])[sx] = 0xffff;
			else
				((unsigned int *)video.rows[sy])[sx] = 0x00ffffff;
		}
	}
}


void
R_Clear (void)
{
	memset (video.buf, 0, video.w * video.h * video.bytes_pp);
}


void
R_DrawLine (int x1, int y1, int x2, int y2, int c)
{
	int x, y;
	int dx, dy;
	int sx, sy;
	int ax, ay;
	int d;

	if (0)
	{
		if (	x1 < 0 || x1 >= video.w ||
			x2 < 0 || x2 >= video.w ||
			y1 < 0 || y1 >= video.h ||
			y2 < 0 || y2 >= video.h )
		{
			return;
		}
	}

	dx = x2 - x1;
	ax = 2 * (dx < 0 ? -dx : dx);
	sx = dx < 0 ? -1 : 1;

	dy = y2 - y1;
	ay = 2 * (dy < 0 ? -dy : dy);
	sy = dy < 0 ? -1 : 1;

	x = x1;
	y = y1;

	if (video.bpp == 16)
	{
		if (ax > ay)
		{
			d = ay - ax / 2;
			while (1)
			{
				((unsigned short *)video.rows[y])[x] = c;
				if (x == x2)
					break;
				if (d >= 0)
				{
					y += sy;
					d -= ax;
				}
				x += sx;
				d += ay;
			}
		}
		else
		{
			d = ax - ay / 2;
			while (1)
			{
				((unsigned short *)video.rows[y])[x] = c;
				if (y == y2)
					break;
				if (d >= 0)
				{
					x += sx;
					d -= ay;
				}
				y += sy;
				d += ax;
			}
		}
	}
	else
	{
		if (ax > ay)
		{
			d = ay - ax / 2;
			while (1)
			{
				((unsigned int *)video.rows[y])[x] = c;
				if (x == x2)
					break;
				if (d >= 0)
				{
					y += sy;
					d -= ax;
				}
				x += sx;
				d += ay;
			}
		}
		else
		{
			d = ax - ay / 2;
			while (1)
			{
				((unsigned int *)video.rows[y])[x] = c;
				if (y == y2)
					break;
				if (d >= 0)
				{
					x += sx;
					d -= ay;
				}
				y += sy;
				d += ax;
			}
		}
	}
}


void
R_Refresh (void)
{
	R_Clear ();

	CalcViewPlanes ();

	R_DrawWorld ();

	Render3DPoint(0,0,0);
	Render3DPoint(10,0,0);
	Render3DPoint(0,10,0);
	Render3DPoint(0,0,10);
	R_DrawLine(50,22,277,189,0x00ffffff);
	R_DrawLine(50,30,277,197,0x000000ff);
	R_DrawLine(50,38,277,205,0x0000ff00);
	R_DrawLine(50,46,277,213,0x00ff0000);
	if (1)
	{
		int i;
		for (i = 0; i < map.num_vertices; i++)
			Render3DPoint(map.vertices[i].xyz[0],map.vertices[i].xyz[1],map.vertices[i].xyz[2]);
	}
}
