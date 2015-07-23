#include <stdint.h>

#include "vec.h"
#include "map.h"
#include "render.h"

#define SURF_BACKFACE_EPSILON 0.01


extern void
R_GenSpansForSurfaces (unsigned int first, int count, int cplanes[4], int numcplanes);

extern int
R_CheckPortalVisibility (struct mportal_s *portal, int reversewinding);

extern void
R_Surf_BeginFrame (void *surfbuf, int surfbufsize, void *edgebuf, int edgebufsize);

static void
VisitNodeRecursive (void *visit, int cplanes[4], int numcplanes);


static void
VisitLeaf (struct mleaf_s *leaf, int cplanes[4], int numcplanes)
{
	R_GenSpansForSurfaces (leaf->firstsurface, leaf->numsurfaces, cplanes, numcplanes);
}


static void
VisitNode (struct mnode_s *node, int cplanes[4], int numcplanes)
{
	double dist;
	int side;

	/* find which side of the plane the camera is on */
	{
		struct mplane_s *pl = &map.planes[node->plane];
		if (pl->type == NORMAL_X)
			dist = camera.pos[0] - pl->dist;
		else if (pl->type == NORMAL_NEGX)
			dist = -pl->dist - camera.pos[0];
		else if (pl->type == NORMAL_Y)
			dist = camera.pos[1] - pl->dist;
		else if (pl->type == NORMAL_NEGY)
			dist = -pl->dist - camera.pos[1];
		else if (pl->type == NORMAL_Z)
			dist = camera.pos[2] - pl->dist;
		else if (pl->type == NORMAL_NEGZ)
			dist = -pl->dist - camera.pos[2];
		else
			dist = Vec_Dot(camera.pos, pl->normal) - pl->dist;
		side = dist < 0.0;
	}

	int portal_visible = 0;

	if (side == 0)
	{
		VisitNodeRecursive (node->children[0], cplanes, numcplanes);

		if (dist > SURF_BACKFACE_EPSILON)
		{
			struct mportal_s *portal;
			int i;

			R_GenSpansForSurfaces (	node->front_firstsurface,
						node->front_numsurfs,
						cplanes,
						numcplanes );

			for (	i = 0, portal = map.portals + node->firstportal;
				i < node->numportals;
				i++, portal++ )
			{
				if (R_CheckPortalVisibility(portal, 0))
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
			//portal_visible = 1;
		}
	}
	else
	{
		//...
		//...
		//...
	}

	/* go down back side only if a portal is visible */
	if (portal_visible)
		VisitNodeRecursive (node->children[!side], cplanes, numcplanes);
}


static void
VisitNodeRecursive (void *visit, int cplanes[4], int numcplanes)
{
	int planes[4];
	int planecnt = 0;
	unsigned short flags = *(unsigned short *)((uintptr_t)visit + NODEFL_OFFSET);

	if (numcplanes)
	{
		struct viewplane_s *vp;
		double bboxcorner[3];
		int vplaneidx;
		while (numcplanes-- > 0)
		{
			vplaneidx = cplanes[numcplanes];
			vp = &camera.vplanes[vplaneidx];

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
				planes[planecnt++] = vplaneidx;
			}
		}
	}

	if ((flags & NODEFL_LEAF) == NODEFL_LEAF)
		VisitLeaf (visit, planes, planecnt);
	else
		VisitNode (visit, planes, planecnt);
}


void
R_DrawWorld (void)
{
	//char spanbuf[32 * 1024];
	char surfacebuf[32 * 1024];
	char edgebuf[32 * 1024];

	int planes[4] = { 0, 1, 2, 3 };

	R_Surf_BeginFrame (surfacebuf, sizeof(surfacebuf), edgebuf, sizeof(edgebuf));
	//R_Span_BeginFrame (spanbuf, sizeof(spanbuf));

	if (map.num_nodes > 0)
		VisitNodeRecursive (map.nodes, planes, 4);
	else if (map.num_leafs > 0)
		VisitNodeRecursive (map.leafs, planes, 4);
}
