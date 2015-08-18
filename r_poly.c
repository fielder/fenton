#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "appio.h"
#include "vec.h"
#include "fenton.h"
#include "bdat.h"
#include "map.h"
#include "render.h"

#define MAX_DRAWEDGES 65535

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

	unsigned int msurfidx;

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

	short top, bottom;

	unsigned short next; /* in v-sorted list of drawedges for the poly */

	char pad[2]; /* align to 20 bytes */
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
	if (e < 0)
		e = -e - 1;
	R_3DLine (map.vertices[map.edges[e].v[0]].xyz, map.vertices[map.edges[e].v[1]].xyz, c);
}


static void
DrawSurfaceEdges (const struct msurface_s *msurf)
{
	int i, c = ((uintptr_t)msurf >> 4) & 0xffffff;
	for (i = 0; i < msurf->numedges; i++)
		DrawSurfEdge (map.edgeloops[msurf->edgeloop_start + i], c);
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
		pl[i].planeid = cplanes[i];
		pl[i].next = ret;
		ret = &pl[i];
	}

	return ret;
}


static void
ScanEdges (struct drawedge_s *drawedges, int count)
{
	if (count < 2)
	{
		/* shouldn't happen... ?? */
		return;
	}

#define LRBUFH 2048
	int lefts[LRBUFH], rights[LRBUFH];
	int y, u;
	memset (lefts, 0xff, sizeof(lefts));
	memset (rights, 0xff, sizeof(rights));
	for (; count > 0; drawedges++, count--)
	{
		y = drawedges->top;
		u = drawedges->u;
		while (y <= drawedges->bottom)
		{
			if (y >= 0 && y < video.h)
			{
				if (lefts[y] == -1)
				{
					lefts[y] = u;
				}
				else
				{
					if (u >= lefts[y])
					{
						rights[y] = u;
					}
					else
					{
						rights[y] = lefts[y];
						lefts[y] = u;
					}
				}
			}
			y++;
			u += drawedges->du;
		}
	}

	int *l, *r;
	for (y = 0, l = lefts, r = rights; y < LRBUFH; y++, l++, r++)
	{
		if (*l != -1 && *r != -1)
			R_Span_ClipAndEmit (y, *l >> 20, *r >> 20);
	}

#if 0
	int u_l, u_r;
	int du_l, du_r;
	int v;
	int bottom;

	{
		struct drawedge_s *a, *b;
		a = drawedges;
		drawedges = &edges[drawedges->next];
		b = drawedges;
		drawedges = &edges[drawedges->next];

		if (a->top != b->top)
		{
		}

		if (a->u < b->u)
		{
			u_l = a->u;
			du_l = a->du;
			u_r = b->u;
			du_r = b->du;
		}
		else if (a->u > b->u)
		{
		}
		else
		{
		}
	}

	//...
	//...
#endif
}


static int
GenerateDrawEdges (unsigned int edgeloop_start, int numedges, const struct cplane_s *clips)
{
	if (edges_p + numedges + 2 > edges_end)
		return 0;

	{
		edges_p->top = 100;
		edges_p->bottom = 200;
		edges_p->u = 100 * (1<<20);
		edges_p->du = 0;
		edges_p++;
		edges_p->top = 100;
		edges_p->bottom = 150;
		edges_p->u = 100 * (1<<20);
		edges_p->du = 1<<20;
		edges_p++;
		edges_p->top = 150;
		edges_p->bottom = 200;
		edges_p->u = 150 * (1<<20);
		edges_p->du = -(1<<20);
		edges_p++;
		return 3;
	}

	struct drawedge_s *firstemit = edges_p;
	int *eptr = &map.edgeloops[edgeloop_start];

	while (numedges-- > 0)
	{
		int e = *eptr++;

		if (e < 0)
		{
			e = -e - 1;
			struct medge_s *medge = &map.edges[e];
		}
		else
		{
			struct medge_s *medge = &map.edges[e];
			unsigned int cacheidx = medge->cachenum & ~0x80000000;

			if (medge->cachenum & 0x80000000)
			{
				if (cacheidx == r_framenum)
				{
					/* already visited this edge this
					 * frame; it's fully off the screen */
					continue;
				}
			}
			else// if ()
			{
				continue;
			}

			// clip new and maybe cache
			//...
		}
	}

	return edges_p - firstemit;
}


static void
GenerateDrawSurf (struct msurface_s *msurf, const struct cplane_s *clips)
{
	int edgecnt;

	if (surfs_p == surfs_end)
		return;

	surfs_p->firstdrawedge = edges_p - edges;

	if ((edgecnt = GenerateDrawEdges(msurf->edgeloop_start, msurf->numedges, clips)) == 0)
		return;

	/* finish and emit drawsurf */
	surfs_p->msurfidx = msurf - map.surfaces;
	surfs_p->numdrawedges = edgecnt;

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

	/* create spans */
	while (firstemit != surfs_p)
	{
		firstemit->spans = r_spans;
		ScanEdges (&edges[firstemit->firstdrawedge], firstemit->numdrawedges);
		firstemit->numspans = r_spans - firstemit->spans;
		firstemit++;
	}
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
	if (cnt > MAX_DRAWEDGES)
		F_Error ("too mant drawedges (%d)", cnt);

	edges_p++; /* index 0 is used for NULL drawedge */
}


static void
DrawSpan (struct drawspan_s *s, int c)
{
	if (video.bpp == 16)
	{
		unsigned short *dest = (unsigned short *)video.rows[s->v] + s->u;
		unsigned short *end = dest + s->len;
		while (dest != end)
			*dest++ = c;
	}
	else
	{
		unsigned int *dest = (unsigned int *)video.rows[s->v] + s->u;
		unsigned int *end = dest + s->len;
		while (dest != end)
			*dest++ = c;
	}
}


void
R_Surf_DrawDebug ()
{
	struct drawsurf_s *drawsurf;
	int num;

	for (drawsurf = surfs, num = 0; drawsurf != surfs_p; drawsurf++, num++)
	{
		int c = ((uintptr_t)&map.surfaces[drawsurf->msurfidx] >> 4) & 0xffffff;
		int i;
		for (i = 0; i < drawsurf->numspans; i++)
			DrawSpan (&drawsurf->spans[i], c);
	}

	if (0)
		printf("%u: %d surfs\n", r_framenum, num);
}
