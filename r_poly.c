#include <stdio.h>
#include <stdint.h>

#include "vec.h"
#include "fenton.h"
#include "bdat.h"
#include "map.h"
#include "render.h"

struct cplane_s
{
	double normal[3];
	double dist;
	struct cplane_s *next;
	int planeid;
};

struct drawsurf_s
{
	struct drawspan_s *spans;

	unsigned int surfnum;

	unsigned short numspans;

	unsigned short firstdrawedge;
	unsigned short numdrawedges;

#if 0
	//TODO: texture mapping stuff
#endif

	char pad[6]; /* align on 8-byte boundary */
};

struct drawedge_s
{
	unsigned int owner; /* medge this drawedge came from */

	int u, du; /* 12.20 fixed-point format */

	//unsigned short next; /* in v-sorted list of drawedges for the poly */

	short top, bottom;

	unsigned char clipflags;

	char pad[3]; /* align to 20 bytes */
};

static struct drawsurf_s *surfs = NULL;
static struct drawsurf_s *surfs_p = NULL;
static struct drawsurf_s *surfs_end = NULL;

static struct drawedge_s *edges = NULL;
static struct drawedge_s *edges_p = NULL;
static struct drawedge_s *edges_end = NULL;


static void
DrawSurfEdge (int e, int c)
{
	R_3DLine (map.vertices[map.edges[e].v[0]].xyz, map.vertices[map.edges[e].v[1]].xyz, c);
}


static void
DrawSurfaceEdges (const struct msurface_s *msurf)
{
	int i;
	for (i = 0; i < msurf->numedges; i++)
	{
		int e = map.edgeloops[msurf->edgeloop_start + i];
		if (e < 0)
			e = -e - 1;
		DrawSurfEdge (e, ((uintptr_t)msurf >> 4) & 0xffffff);
	}
}


static struct cplane_s *
GetCPlanes (int cplanes[4], int numcplanes)
{
	static struct cplane_s pl[4];
	struct cplane_s *ret = NULL;
	int i;

	for (i = 0; i < numcplanes; i++)
	{
		Vec_Copy (camera.vplanes[cplanes[i]].normal, pl[i].normal);
		pl[i].dist = camera.vplanes[cplanes[i]].dist;
		pl[i].next = ret;
		pl[i].planeid = cplanes[i];
		ret = &pl[i];
	}

	return ret;
}


static void
GenerateDrawSpans (struct drawsurf_s *surf)
{
	//int ordered_left[100];
	//int ordered_right[100];
	// store indices (from edges[]) of sorted edges

	surf->spans = r_spans;

	//...

	surf->numspans = r_spans - surf->spans;
}


static int
GenerateDrawEdges (unsigned int edgeloop_start, int numedges, const struct cplane_s *clips)
{
//	if (edges_p + msurf->numedges + 2 > edges_end)
//		return;
	//...

	return 0;
}


static void
GenerateDrawSurf (struct msurface_s *msurf, const struct cplane_s *clips)
{
	int nume;

	if (surfs_p == surfs_end)
		return;

	surfs_p->surfnum = msurf - map.surfaces;
	surfs_p->firstdrawedge = edges_p - edges;

	if (!(nume = GenerateDrawEdges(msurf->edgeloop_start, msurf->numedges, clips)))
		return;

	/* finish and emit drawsurf */
	surfs_p->numdrawedges = nume;

	surfs_p++;
}


void
R_GenSpansForSurfaces (unsigned int first, int count, int cplanes[4], int numcplanes, int backface_check)
{
	struct cplane_s *clips = GetCPlanes (cplanes, numcplanes);
	struct drawsurf_s *firstemit = surfs_p;

	while (count-- > 0)
	{
		struct msurface_s *msurf = &map.surfaces[first++];

		if (!backface_check || Map_DistFromSurface(msurf, camera.pos) >= SURF_BACKFACE_EPSILON)
			GenerateDrawSurf (msurf, clips);
	}

	while (firstemit != surfs_p)
		GenerateDrawSpans (firstemit++);
}


int
R_CheckPortalVisibility (struct mportal_s *portal, int cplanes[4], int numcplanes, int reversewinding)
{
	// run through the edges, emitting drawedges
	// scan over the emitted drawedges; if a span is visible return true
	// the only side-effect of this function will be emitted drawedges
	//TODO: ...
	return 0;
}


void
R_Surf_BeginFrame (void *surfbuf, int surfbufsize, void *edgebuf, int edgebufsize)
{
	unsigned int cnt;

	surfs_p = surfs = AlignAllocation (surfbuf, surfbufsize, sizeof(*surfs), &cnt);
	surfs_end = surfs + cnt;

	edges_p = edges = AlignAllocation (edgebuf, edgebufsize, sizeof(*edges), &cnt);
	edges_end = edges + cnt;
	if (cnt > 65536)
		F_Error ("too mant drawedges (%d)", cnt);
}
