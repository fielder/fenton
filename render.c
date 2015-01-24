#include <string.h>
#include <math.h>

//#include "rdata.h"
#include "map.h"
#include "appio.h"
#include "vec.h"
#include "render.h"

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


static const int _bboxmap[8][3] = {
	{ 0, 0, 0 },
	{ 1, 0, 0 },
	{ 0, 1, 0 },
	{ 1, 1, 0 },
	{ 0, 0, 1 },
	{ 1, 0, 1 },
	{ 0, 1, 1 },
	{ 1, 1, 1 },
};


static void
CalcAcceptReject (struct viewplane_s *p)
{
	int acc_idx =	((p->normal[0] < 0.0) << 0) +
			((p->normal[1] < 0.0) << 1) +
			((p->normal[2] < 0.0) << 2);

	p->accept[0] = _bboxmap[acc_idx][0];
	p->accept[1] = _bboxmap[acc_idx][1];
	p->accept[2] = _bboxmap[acc_idx][2];

	p->reject[0] = _bboxmap[7 - acc_idx][0];
	p->reject[1] = _bboxmap[7 - acc_idx][1];
	p->reject[2] = _bboxmap[7 - acc_idx][2];
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
R_Refresh (void)
{
	char spanbuf[0x8000];

	R_Clear ();

	CalcViewPlanes ();

	R_Span_BeginFrame (spanbuf, sizeof(spanbuf));

	Render3DPoint(0,0,0);
	Render3DPoint(10,0,0);
	Render3DPoint(0,10,0);
	Render3DPoint(0,0,10);
	if (1)
	{
		int i;
		for (i = 0; i < map.num_vertices; i++)
			Render3DPoint(map.vertices[i].xyz[0],map.vertices[i].xyz[1],map.vertices[i].xyz[2]);
	}
}
