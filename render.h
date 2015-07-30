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

extern char * const vplane_names[4];
extern struct camera_s camera;

extern void
R_Init (void);

extern void
R_Shutdown (void);

extern void
R_CameraSizeChanged (int w, int h);

extern void
R_CalcViewXForm (void);

extern void
R_Clear (void);

extern void
R_3DPoint (double x, double y, double z);

extern void
R_DrawLine (int x1, int y1, int x2, int y2, int c);

extern void
R_Refresh (void);

/* span stuff */

/* emitted polygon span */
struct drawspan_s
{
	short u, v;
	short len;
};

extern void
R_Span_Init (void);

extern void
R_Span_Cleanup (void);

// for portal visibility checking
extern int
R_Span_CheckVisible (int y, int x1, int x2);

extern void
R_Span_ClipAndEmit (int y, int x1, int x2);

extern void
R_Span_BeginFrame (void *buf, int buflen);

extern void
R_Span_DrawGSpans (void);

/* world traverse */

extern void
R_DrawWorld (void);

#endif /* __RENDER_H__ */
