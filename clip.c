#include "vec.h"
#include "clip.h"

#define CLIP_EPSILON (1.0 / 16.0)

enum
{
	SIDE_ON,
	SIDE_FRONT,
	SIDE_BACK,
};

double clip_verts[2][CLIP_MAX_VERTS][3];
int clip_idx;
int clip_numverts;


static int
ClassifyDist (double d)
{
	if (d < -CLIP_EPSILON)
		return SIDE_BACK;
	else if (d > CLIP_EPSILON)
		return SIDE_FRONT;
	else
		return SIDE_ON;
}


void
Clip_WithPlane (const double normal[3], double dist)
{
	int sides[CLIP_MAX_VERTS + 1];
	double dots[CLIP_MAX_VERTS + 1];
	int counts[3];
	int i;

	counts[SIDE_ON] = counts[SIDE_FRONT] = counts[SIDE_BACK] = 0;

	for (i = 0; i < clip_numverts; i++)
	{
		dots[i] = Vec_Dot (clip_verts[clip_idx][i], normal) - dist;
		sides[i] = ClassifyDist (dots[i]);
		counts[sides[i]]++;
	}
	dots[i] = dots[0]; /* make wrap-around case easier */
	sides[i] = sides[0]; /* make wrap-around case easier */

	if (!counts[SIDE_FRONT])
	{
		/* all verts on or behind the plane */
		clip_numverts = 0;
		return;
	}
	if (!counts[SIDE_BACK])
	{
		/* all verts on the front, some possibly touching the
		 * plane */
		return;
	}

	int numout = 0;

	for (i = 0; i < clip_numverts; i++)
	{
		double *v = clip_verts[clip_idx][i];
		double *next = clip_verts[clip_idx][(i + 1 < clip_numverts) ? (i + 1) : (0)];

		if (sides[i] == SIDE_FRONT || sides[i] == SIDE_ON)
			Vec_Copy (v, clip_verts[clip_idx ^ 1][numout++]);

		if (	(sides[i] == SIDE_FRONT && sides[i + 1] == SIDE_BACK) ||
			(sides[i] == SIDE_BACK && sides[i + 1] == SIDE_FRONT) )
		{
			double frac = dots[i] / (dots[i] - dots[i + 1]);
			clip_verts[clip_idx ^ 1][numout][0] = v[0] + frac * (next[0] - v[0]);
			clip_verts[clip_idx ^ 1][numout][1] = v[1] + frac * (next[1] - v[1]);
			clip_verts[clip_idx ^ 1][numout][2] = v[2] + frac * (next[2] - v[2]);
			numout++;
		}
	}

	if (numout < 3)
	{
		clip_numverts = 0;
	}
	else
	{
		clip_idx ^= 1;
		clip_numverts = numout;
	}
}
