#include <stdint.h>

#include "vec.h"
#include "map.h"
#include "render.h"


static void
DrawSurface (struct msurface_s *s, int cplanes[4], int numcplanes)
{
}


static void
VisitNode (void *node, int cplanes[4], int numcplanes)
{
	int planes[4];
	int planecnt = 0;
	unsigned short flags = ((unsigned short *)node)[24];

	if (numcplanes)
	{
		struct viewplane_s *vp;
		int *bounds = node;
		double p[3];
		int vplaneidx;
		while (numcplanes-- > 0)
		{
			vplaneidx = cplanes[numcplanes];
			vp = &camera.vplanes[vplaneidx];

			p[0] = bounds[vp->reject[0]];
			p[1] = bounds[vp->reject[1]];
			p[2] = bounds[vp->reject[2]];
			if (Vec_Dot(p, vp->normal) - vp->dist <= 0.0)
				return;

			p[0] = bounds[vp->accept[0]];
			p[1] = bounds[vp->accept[1]];
			p[2] = bounds[vp->accept[2]];
			if (Vec_Dot(p, vp->normal) - vp->dist < 0.0)
			{
				/* bbox intersects the plane, keep it
				 * in the clip plane list */
				planes[planecnt++] = vplaneidx;
			}
		}
	}

	if ((flags & NODEFL_LEAF) == NODEFL_LEAF)
	{
		struct mleaf_s *leaf = node;
		struct msurface_s *s;
		int i;
		for (	i = 0, s = map.surfaces + leaf->firstsurface;
			i < leaf->numsurfaces;
			i++, s++ )
		{
			DrawSurface (s, planes, planecnt);
		}
		return;
	}

	//int node_visible = 0;

	//int node_seethrough = 0;
	// check plane side
	// recurse front
	// if node surfaces:
	// 	make plane list
	// 	for all surfs:
	// 		if backfacing continue
	// 		if portal:
	// 			if !node_seethrough: node_seethrough = checkportal()
	// 		else:
	// 			draw surface
	//if (node_seethrough)
	//	; // recurse back
}


void
R_DrawWorld (void)
{
	int planes[4] = { 0, 1, 2, 3 };

	char spanbuf[32 * 1024];
	//char surfacebuf[32 * 1024];
	//char edgebuf[32 * 1024];

	R_Span_BeginFrame (spanbuf, sizeof(spanbuf));

	if (map.num_nodes > 0)
		VisitNode (map.nodes, planes, 4);
	else if (map.num_leafs > 0)
		VisitNode (map.leafs, planes, 4);
}
