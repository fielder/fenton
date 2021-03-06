#include <stdint.h>

#include "vec.h"
#include "map.h"
#include "appio.h"
#include "render.h"

static void
VisitNodeRecursive (void *visit, int planemask);


static void
VisitLeaf (struct mleaf_s *leaf, int planemask)
{
	R_Edge_ProcessSurfaces (leaf->firstsurface, leaf->numsurfaces, planemask, 1);
}


static void
VisitNode (struct mnode_s *node, int planemask)
{
	int portal_visible = 0;
	double dist;
	int side;

	/* find which side of the plane the camera is on */
	dist = Map_DistFromPlane (&map.planes[node->plane], camera.pos);
	side = dist < 0.0;

#if 0
	if (side == 0)
	{
		VisitNodeRecursive (node->children[0], cplanes, numcplanes);

		if (dist >= SURF_BACKFACE_EPSILON)
		{
			struct mportal_s *portal;
			int i;

#if 0
			R_GenSpansForSurfaces (	node->front_firstsurface,
						node->front_numsurfs,
						cplanes,
						numcplanes,
						0 /* backface check unnecessary since we're drawing front surfs */
						);
			TODO: if no map portals; dev map; always descend down back side
#endif

			for (	i = 0, portal = map.portals + node->firstportal;
				i < node->numportals;
				i++, portal++ )
			{
#if 0
				if (R_Edge_CheckPortalVisibility(portal, cplanes, numcplanes, 0))
#endif
				{
					portal_visible = 1;
					break;
				}
			}
		}
		else
		{
		//FIXME: Probaly will have cases where we don't scan
		//out portals because we're right on it, and therefore
		//never descend down the far side of the node
		//May have to always descend the far side if we're on
		//the node.
		//play with this...
			portal_visible = 1;
		}
	}
	else
	{
		//...
		//...
		//...
	}
#endif

	/* go down back side only if a portal is visible */
	if (portal_visible)
		VisitNodeRecursive (node->children[!side], planemask);
}


static void
VisitNodeRecursive (void *visit, int planemask)
{
	int newplanemask = 0x0;

	/* check node/leaf bbox against active viewplanes and
	 * update active viewplane set */
	if (planemask != 0x0)
	{
		int vplaneidx;
		for (vplaneidx = 0; vplaneidx < 4; vplaneidx++)
		{
			if (planemask & (1 << vplaneidx))
			{
				struct viewplane_s *vp = &camera.vplanes[vplaneidx];
				double bboxcorner[3];

				bboxcorner[0] = ((int *)visit)[vp->reject[0]];
				bboxcorner[1] = ((int *)visit)[vp->reject[1]];
				bboxcorner[2] = ((int *)visit)[vp->reject[2]];
				if (Vec_Dot(bboxcorner, vp->normal) - vp->dist <= 0.0)
				{
					/* bbox is entirely behind a viewplane */
					return;
				}

				bboxcorner[0] = ((int *)visit)[vp->accept[0]];
				bboxcorner[1] = ((int *)visit)[vp->accept[1]];
				bboxcorner[2] = ((int *)visit)[vp->accept[2]];
				if (Vec_Dot(bboxcorner, vp->normal) - vp->dist >= 0.0)
				{
					/* bbox is entirely infront of a viewplane;
					 * won't get added to the active plane set */
				}
				else
				{
					/* bbox intersects the plane, keep it
					 * in the clip plane list */
					newplanemask |= 1 << vplaneidx;
				}
			}
		}
	}

	if ((NODEFL_FLAGS(visit) & NODEFL_LEAF) == NODEFL_LEAF)
		VisitLeaf (visit, newplanemask);
	else
		VisitNode (visit, newplanemask);
}


void
R_DrawWorld (void)
{
	char spanbuf[32 * 1024];
	char surfbuf[32 * 1024];
	char edgebuf[32 * 1024];
	//TODO: keep stats and tweak buf sizes
	// - spans max, min, avg created
	// - surfs max, min, avg created
	// maybe bin numbers
	// - edges: cached per frame; cache hit, cache miss
	// - edges: number of uncached extraedges per poly
	// allow bug sizes config-settable

	R_Edge_BeginFrame (edgebuf, sizeof(edgebuf));
	R_Surf_BeginFrame (surfbuf, sizeof(surfbuf));
	R_Span_BeginFrame (spanbuf, sizeof(spanbuf), video.w, video.h);

	if (map.num_nodes > 0)
		VisitNodeRecursive (map.nodes, VPLANE_ALL_MASK);
	else if (map.num_leafs > 0)
		VisitNodeRecursive (&map.leafs[0], VPLANE_ALL_MASK);
	else
		/* no geometry */ ;

	R_Surf_DrawAll ();
}
