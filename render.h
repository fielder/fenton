#ifndef __RENDER_H__
#define __RENDER_H__

enum
{
	PITCH,
	YAW,
	ROLL,
};

enum
{
	VPLANE_LEFT,
	VPLANE_RIGHT,
	VPLANE_TOP,
	VPLANE_BOTTOM,
};

#define VPLANE_LEFT_MASK	(1 << VPLANE_LEFT)
#define VPLANE_RIGHT_MASK	(1 << VPLANE_RIGHT)
#define VPLANE_TOP_MASK		(1 << VPLANE_TOP)
#define VPLANE_BOTTOM_MASK	(1 << VPLANE_BOTTOM)
#define VPLANE_ALL_MASK		(VPLANE_LEFT_MASK | \
				VPLANE_RIGHT_MASK | \
				VPLANE_TOP_MASK | \
				VPLANE_BOTTOM_MASK)

struct viewplane_s
{
	double normal[3];
	double dist;
	int accept[3], reject[3];
};

struct camera_s
{
	double center_x;
	double center_y;

	double fov_x; /* radians */
	double fov_y; /* radians */
	double dist;

	double pos[3];
	double angles[3]; /* radians */

	double forward[3];
	double left[3];
	double up[3];

	double xform[3][3]; /* world-to-camera */

	struct viewplane_s vplanes[4];
};

extern struct camera_s camera;
extern unsigned int r_framenum;

extern void
R_Init (void);

extern void
R_Shutdown (void);

extern void
R_CameraSizeChanged (int w, int h);

extern void
R_CalcViewXForm (void);

extern void
R_Refresh (void);

extern void
R_Die (const char *msg);

/* edge processing to spans */

extern void
R_Edge_ProcessSurfaces (unsigned int first,
			int count,
			int planemask,
			int backface_check);

extern int
R_Edge_CheckPortalVisibility (	int portalidx,
				int planemask,
				int reversewinding);

extern void
R_Edge_BeginFrame (void *edgebuf, int edgebufsize);

/* surface draw to framebuffer */

struct drawsurf_s
{
	unsigned short firstspanidx;
	unsigned short numspans;

	unsigned int msurfidx;

	//TODO: texture mapping stuff

	char pad[2]; /* align on 8-byte boundary */
};

extern struct drawsurf_s *surfs;
extern struct drawsurf_s *surfs_p;
extern struct drawsurf_s *surfs_end;

extern void
R_Surf_DrawAll (void);

extern void
R_Surf_BeginFrame (void *surfbuf, int surfbufsize);

/* span stuff */

/* emitted polygon span */
struct drawspan_s
{
	short u, v;
	short len;
};

extern struct drawspan_s *r_spans;
extern struct drawspan_s *r_spans_p;

extern void
R_Span_Cleanup (void);

/* for portal visibility checking */
extern int
R_Span_CheckVisible (int y, int x1, int x2);

extern void
R_Span_ClipAndEmit (int y, int x1, int x2);

extern void
R_Span_DrawGSpans (void);

extern void
R_Span_BeginFrame (void *buf, int buflen, int w, int h);

/* world traverse */

#define SURF_BACKFACE_EPSILON (1.0 / 16.0)

extern void
R_DrawWorld (void);

/* misc */

extern void
R_Clear (void);

extern void
R_3DLine (const double v1[3], const double v2[3], int c);

extern void
R_3DLine2 (double x1, double y1, double z1, double x2, double y2, double z2, int c);

extern void
R_3DPoint (const double v[3], int c);

extern void
R_3DPoint2 (double x, double y, double z, int c);

extern void
R_DrawLine (int x1, int y1, int x2, int y2, int c);

extern void
R_SimpleDrawPoly (double *xyz, int numverts, int c);

#endif /* __RENDER_H__ */
