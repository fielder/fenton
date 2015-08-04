#include <math.h>

//#include "rdata.h"
#include "map.h"
#include "appio.h"
#include "vec.h"
#include "render.h"

char * const vplane_names[4] = { "left", "right", "top", "bottom" };
static const double fov_x = 90.0;

struct camera_s camera;

unsigned int r_framenum;


void
R_Init (void)
{
	camera.fov_x = DEG2RAD(fov_x);

	Vec_Clear (camera.pos);
	Vec_Clear (camera.angles);

	R_CalcViewXForm ();

	r_framenum = 0;
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


void
R_Refresh (void)
{
	R_Clear ();

	CalcViewPlanes ();

	R_DrawWorld ();

	if (1)
	{
		int i;
		for (i = 0; i < 100; i++)
		{
			R_3DPoint2(i*0.1,0,0,video.red);
			R_3DPoint2(0,i*0.1,0,video.green);
			R_3DPoint2(0,0,i*0.1,video.blue);
		}
	}

//	R_DrawLine(50,22,277,189,0x00ffffff); // white
//	R_DrawLine(50,30,277,197,0x000000ff); // B - bits 0-7
//	R_DrawLine(50,38,277,205,0x0000ff00); // G - bits 8-15
//	R_DrawLine(50,46,277,213,0x00ff0000); // R - bits 16-23
//
	if (1)
	{
		int i;
		//for (i = 0; i < map.num_vertices; i++)
		//	R_3DPoint(map.vertices[i].xyz);
		for (i = 0; i < map.num_edges; i++)
			R_3DLine (map.vertices[map.edges[i].v[0]].xyz, map.vertices[map.edges[i].v[1]].xyz, 0xffffffff);
	}
	R_Span_DrawGSpans ();

	r_framenum++;
}
