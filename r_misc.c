#include <string.h>

#include "render.h"
#include "appio.h"
#include "clip.h"
#include "vec.h"


void
R_Clear (void)
{
	memset (video.buf, 0, video.w * video.h * video.bytes_pp);
}


void
R_3DLine (const double v1[3], const double v2[3], int c)
{
	double a[3], b[3], m[3];
	int i;

	Vec_Copy (v1, a);
	Vec_Copy (v2, b);

	Vec_Subtract (a, b, m);
	if (Vec_Length(m) <= 0.05)
	{
		R_3DPoint (a, c);
		return;
	}

	for (i = 0; i < 4; i++)
	{
		struct viewplane_s *p = &camera.vplanes[i];
		double d1, d2;

		d1 = Vec_Dot (a, p->normal) - p->dist;
		d2 = Vec_Dot (b, p->normal) - p->dist;

		if (d1 <= 0 && d2 <= 0)
			return;
		else if (d1 > 0 && d2 > 0)
			continue;
		else if (d1 > 0)
		{
			double frac = d1 / (d1 - d2);
			b[0] = a[0] + frac * (b[0] - a[0]);
			b[1] = a[1] + frac * (b[1] - a[1]);
			b[2] = a[2] + frac * (b[2] - a[2]);
		}
		else if (d2 > 0)
		{
			double frac = d2 / (d2 - d1);
			a[0] = b[0] + frac * (a[0] - b[0]);
			a[1] = b[1] + frac * (a[1] - b[1]);
			a[2] = b[2] + frac * (a[2] - b[2]);
		}
	}

	double local[3];

	Vec_Subtract (a, camera.pos, local);
	Vec_Transform (camera.xform, local, a);
	if (a[2] < 0.01) a[2] = 0.01;
	int sx1 = camera.center_x - camera.dist * (a[0] / a[2]) + 0.5;
	int sy1 = camera.center_y - camera.dist * (a[1] / a[2]) + 0.5;
	if (sx1 < 0) sx1 = 0;
	if (sx1 >= video.w) sx1 = video.w - 1;
	if (sy1 < 0) sy1 = 0;
	if (sy1 >= video.h) sy1 = video.h - 1;

	Vec_Subtract (b, camera.pos, local);
	Vec_Transform (camera.xform, local, b);
	if (b[2] < 0.01) b[2] = 0.01;
	int sx2 = camera.center_x - camera.dist * (b[0] / b[2]) + 0.5;
	int sy2 = camera.center_y - camera.dist * (b[1] / b[2]) + 0.5;
	if (sx2 < 0) sx2 = 0;
	if (sx2 >= video.w) sx2 = video.w - 1;
	if (sy2 < 0) sy2 = 0;
	if (sy2 >= video.h) sy2 = video.h - 1;

	R_DrawLine (sx1, sy1, sx2, sy2, c);
}


void
R_3DLine2 (double x1, double y1, double z1, double x2, double y2, double z2, int c)
{
	double a[3], b[3];

	a[0] = x1; a[1] = y1; a[2] = z1;
	b[0] = x1; b[1] = y1; b[2] = z1;

	R_3DLine (a, b, c);
}


void
R_3DPoint (const double v[3], int c)
{
	double local[3], out[3];

	Vec_Subtract (v, camera.pos, local);
	Vec_Transform (camera.xform, local, out);
	if (out[2] > 0.0)
	{
		int sx = camera.center_x - camera.dist * (out[0] / out[2]) + 0.5;
		int sy = camera.center_y - camera.dist * (out[1] / out[2]) + 0.5;
		if (sx >= 0 && sx < video.w && sy >= 0 && sy < video.h)
		{
			if (video.bpp == 16)
				((unsigned short *)video.rows[sy])[sx] = c;
			else
				((unsigned int *)video.rows[sy])[sx] = c;
		}
	}
}


void
R_3DPoint2 (double x, double y, double z, int c)
{
	double v[3];

	v[0] = x; v[1] = y; v[2] = z;

	R_3DPoint (v, c);
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
R_SimpleDrawPoly (double *xyz, int numverts, int c)
{
	double normal[3];
	double dist;
	int i;

	Vec_MakeNormal (xyz,
			xyz + 3,
			xyz + 3 * (numverts - 1),
			normal,
			&dist);
	if (Vec_Dot(normal, camera.pos) - dist < SURF_BACKFACE_EPSILON)
		return;

	clip_idx = 0;
	for (i = 0; i < numverts; i++)
		Vec_Copy (xyz + i * 3, clip_verts[clip_idx][i]);
	clip_numverts = i;

	for (i = 0; i < 4; i++)
	{
		Clip_WithPlane (camera.vplanes[i].normal, camera.vplanes[i].dist);
		if (clip_numverts <= 0)
			return;
	}

	//TODO: draw filled
	for (i = 0; i < clip_numverts; i++)
	{
		R_3DLine (clip_verts[clip_idx][i],
			clip_verts[clip_idx][(i + 1) % clip_numverts],
			c);
	}
}
