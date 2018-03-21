#include <stdint.h>

#include "vec.h"
#include "map.h"
#include "render.h"


extern void
R_GenSpansForSurfaces (unsigned int first, int count, int cplanes[4], int numcplanes, int backface_check);

extern int
R_CheckPortalVisibility (struct mportal_s *portal, int cplanes[4], int numcplanes, int reversewinding);

extern void
R_Surf_BeginFrame (void *surfbuf, int surfbufsize, void *edgebuf, int edgebufsize);

static void
VisitNodeRecursive (void *visit, int cplanes[4], int numcplanes);


static void
VisitLeaf (struct mleaf_s *leaf, int cplanes[4], int numcplanes)
{
	R_GenSpansForSurfaces (leaf->firstsurface, leaf->numsurfaces, cplanes, numcplanes, 1);
}


static void
VisitNode (struct mnode_s *node, int cplanes[4], int numcplanes)
{
	int portal_visible = 0;
	double dist;
	int side;

	/* find which side of the plane the camera is on */
	dist = Map_DistFromPlane (&map.planes[node->plane], camera.pos);
	side = dist < 0.0;

	if (side == 0)
	{
		VisitNodeRecursive (node->children[0], cplanes, numcplanes);

		if (dist >= SURF_BACKFACE_EPSILON)
		{
			struct mportal_s *portal;
			int i;

			R_GenSpansForSurfaces (	node->front_firstsurface,
						node->front_numsurfs,
						cplanes,
						numcplanes,
						0 /* backface check unnecessary since we're drawing front surfs */
						);

			for (	i = 0, portal = map.portals + node->firstportal;
				i < node->numportals;
				i++, portal++ )
			{
				if (R_CheckPortalVisibility(portal, cplanes, numcplanes, 0))
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

	/* go down back side only if a portal is visible */
	if (portal_visible)
		VisitNodeRecursive (node->children[!side], cplanes, numcplanes);
}


static void
VisitNodeRecursive (void *visit, int cplanes[4], int numcplanes)
{
	int planes[4];
	int planecnt = 0;
	unsigned short flags = NODEFL_FLAGS(visit);

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
	if (0)
	{
		char spanbuf[32 * 1024];
		char surfacebuf[32 * 1024];
		char edgebuf[32 * 1024];

		int cplanes[4] = { 0, 1, 2, 3 };

		R_Surf_BeginFrame (surfacebuf, sizeof(surfacebuf), edgebuf, sizeof(edgebuf));
		R_Span_BeginFrame (spanbuf, sizeof(spanbuf));

		if (map.num_nodes > 0)
		{
			VisitNodeRecursive (map.nodes, cplanes, 4);
		}
		else if (map.num_leafs > 0)
		{
			/* realistically would have only 1 leaf if there are no
			 * nodes, but for dev testing we could have some partial
			 * maps */
			int i;
			for (i = 0; i < map.num_leafs; i++)
				VisitNodeRecursive (&map.leafs[i], cplanes, 4);
		}
		else
		{
			/* for dev purposes just draw all polys */
			R_GenSpansForSurfaces (0, map.num_surfaces, cplanes, 4, 1);
		}

		if (0)
			R_Surf_DrawDebug ();
	}

	if (1)
	{
		if (map.num_surfaces > 0)
			R_SimpleDrawPoly ((double *)map.vertices, map.num_vertices, 0xffffffff);
	}
	//TODO: draw textured surface spans
	//	texturing, lighting, z fill
	//NOTE: we could have emitted surfaces w/o spans; skip them
}
