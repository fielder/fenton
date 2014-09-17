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

extern void
R_Init (void);

extern void
R_Shutdown (void);

extern void
R_CameraChanged (int w, int h);

extern void
R_CalcViewXForm (void);

extern void
R_Refresh (void);

#endif /* __RENDER_H__ */
