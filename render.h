#ifndef __RENDER_H__
#define __RENDER_H__

enum
{
	PITCH,
	YAW,
	ROLL,
};

struct viewplane_s
{
	float normal[3];
	float dist;
};

struct camera_s
{
	float center_x;
	float center_y;

	float fov_x; /* radians */
	float fov_y; /* radians */
	float dist;

	float pos[3];
	float angles[3]; /* radians */

	float forward[3];
	float left[3];
	float up[3];

	float xform[3][3]; /* world-to-camera */

	struct viewplane_s vplanes[4];
};

extern struct camera_s camera;

extern void
R_Init (void);

extern void
R_Shutdown (void);

extern void
R_CalcViewXForm (void);

extern void
R_Refresh (void);

#endif /* __RENDER_H__ */
