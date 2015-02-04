#include "vec.h"
#include "map.h"
#include "render.h"


#if 0
static void
RecursiveBSP (void *node, int active_viewplanes)
{
	int i;

	if (active_viewplanes != 0x0)
	{
		// update, unset a bit if the node is fully front
		// return if node is entirely behind an active plane
	}

	if (((struct mnode_s *)node)->is_leaf)
	{
		// make view plane list
		// for all surfs: draw()
	}
	else
	{
		int node_seethrough = 0;
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
		if (node_seethrough)
			; // recurse back
	}
}
#endif


#if 0
- load map
  vertices
  planes
  edges
- draw edges
#endif


#if 0
static void
DrawSurfaces (struct msurface_s *surfs, int num, int clips[4], int numclips)
{
	for (; num > 0; num--, surfs++)
	{
	}
}
#endif


static int
IsLeaf (void *node)
{
	//...
	return 1;
}


static void
VisitNode (void *node, int active_vplanes)
{
	int clips[4];
	int numclips = 0;

	if (active_vplanes)
	{
		struct viewplane_s *vp;
		int *bounds = node;
		double p[3];
		int i;
		for (i = 0, vp = camera.vplanes; i < 4; i++, vp++)
		{
			if (active_vplanes & (1 << i))
			{
				p[0] = bounds[vp->reject[0]];
				p[1] = bounds[vp->reject[1]];
				p[2] = bounds[vp->reject[2]];
				if (Vec_Dot(p, vp->normal) - vp->dist <= 0.0)
					return;

				p[0] = bounds[vp->accept[0]];
				p[1] = bounds[vp->accept[1]];
				p[2] = bounds[vp->accept[2]];
				if (Vec_Dot(p, vp->normal) - vp->dist >= 0.0)
					active_vplanes &= ~(1 << i);
				else
					clips[numclips++] = i;
			}
		}
	}

	if (IsLeaf(node))
	{
		struct mleaf_s *leaf = node;
		DrawSurfaces (map.surfaces + leaf->firstsurface, leaf->numsurfaces, clips, numclips);
		return;
	}

	//...
}


void
R_DrawWorld (void)
{
	//char spanbuf[32 * 1024];
	//char surfacebuf[32 * 1024];
	//char edgebuf[32 * 1024];

	//R_Span_BeginFrame (spanbuf, sizeof(spanbuf));

	if (map.num_nodes > 0)
		VisitNode (map.nodes, 0xf);
	else if (map.num_leafs > 0)
		VisitNode (map.leafs, 0xf);
}
