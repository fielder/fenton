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
	unsigned int medgeidx; /* medge this drawedge came from */

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
DrawSurfaceEdges (const struct msurface_s *msurf, int c)
{
	int i;
	for (i = 0; i < msurf->numedges; i++)
		DrawSurfEdge (map.edgeloops[msurf->edgeloop_start + i], c);
}


static void
DrawSurfaceEdges2 (const struct msurface_s *msurf)
{
	DrawSurfaceEdges (msurf, ((uintptr_t)msurf >> 4) & 0xffffff);
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


static int
ClampU (int u)
{
	if (u < 0) return 0;
	if (u >= video.w * (1<<20)) return video.w * (1<<20) - 1;
	return u;
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
	memset (lefts, 0xff, sizeof(lefts));
	memset (rights, 0xff, sizeof(rights));
	for (; count > 0; drawedges++, count--)
	{
		int y = drawedges->top;
		int u = drawedges->u;
		while (y <= drawedges->bottom)
		{
			if (y >= 0 && y < video.h)
			{
				if (lefts[y] == -1)
				{
					lefts[y] = ClampU (u);
				}
				else
				{
					if (u >= lefts[y])
					{
						rights[y] = ClampU (u);
					}
					else
					{
						rights[y] = lefts[y];
						lefts[y] = ClampU (u);
					}
				}
			}
			y++;
			u += drawedges->du;
		}
	}

	int y, *l, *r;
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


static void
EmitEdge (const float v1[3], const float v2[3], int cacheable)
{
#if 0
	float u1_f, v1_f;
	int v1_i;

	float u2_f, v2_f;
	int v2_i;

	float du;

	float local[3], out[3];
	float scale;

	/* the pixel containment rule says an edge point exactly on the
	 * center of a pixel vertically will be considered to cover that
	 * pixel */

	Vec_Subtract (v1, camera.pos, local);
	Vec_Transform (camera.xform, local, out);
	scale = camera.dist / out[2];
	u1_f = camera.center_x - scale * out[0];
	v1_f = camera.center_y - scale * out[1];

	Vec_Subtract (v2, camera.pos, local);
	Vec_Transform (camera.xform, local, out);
	scale = camera.dist / out[2];
	u2_f = camera.center_x - scale * out[0];
	v2_f = camera.center_y - scale * out[1];

	v1_i = ceil (v1_f - 0.5);
	v2_i = ceil (v2_f - 0.5);

	if (v1_i == v2_i)
	{
		/* doesn't cross a pixel center vertically */
		return 0;
	}
	else if (v1_i < v2_i)
	{
		/* left-side edge, running down the screen */

		if (v2_i <= 0 || v1_i >= video.h)
		{
			/* math imprecision sometimes results in nearly-horizontal
			 * emitted edges just above or just below the screen */
			return 0;
		}

		du = (u2_f - u1_f) / (v2_f - v1_f);
		e->u = (u1_f + du * (v1_i + 0.5 - v1_f)) * 0x100000;
		e->u += ((1 << 20) / 2) - 1; /* pre-adjust screen x coords so we end up with the correct pixel after shifting down */
		e->du = du * 0x100000;
		e->top = v1_i;
		e->bottom = v2_i - 1; /* this edge's last row is 1 pixel above the top pixel of the next edge */
		r_edges_left = LinkEdge (e, r_edges_left);
	}
	else
	{
		/* right-side edge, running up the screen */

		if (v1_i <= 0 || v2_i >= video.h)
		{
			/* math imprecision sometimes results in nearly-horizontal
			 * emitted edges just above or just below the screen */
			return 0;
		}

		du = (u1_f - u2_f) / (v1_f - v2_f);
		e->u = (u2_f + du * (v2_i + 0.5 - v2_f)) * 0x100000;
		e->u -= ((1 << 20) / 2); /* pre-adjust screen x coords so we end up with the correct pixel after shifting down */
		e->du = du * 0x100000;
		e->top = v2_i;
		e->bottom = v1_i - 1; /* this edge's last row is 1 pixel above the top pixel of the next edge */
		r_edges_right = LinkEdge (e, r_edges_right);
	}

	return 1;
#endif
}


static void
EmitCached (const struct drawedge_s *e)
{
	edges_p->medgeidx = e->medgeidx;
	edges_p->u = e->u;
	edges_p->du = e->du;
	edges_p->top = e->top;
	edges_p->bottom = e->bottom;
	edges_p++;
}


static int
GenerateDrawEdges (unsigned int edgeloop_start, int numedges, const struct cplane_s *clips)
{
	if (edges_p + numedges + 2 > edges_end)
		return 0;

	if (0)
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
	int *eloop_ptr = &map.edgeloops[edgeloop_start];

	while (numedges-- > 0)
	{
		int e = *eloop_ptr++;

		/* NOTE: need to know edge direction to know enter/exit
		 * when edges hit L/R planes */

		if (e < 0)
		{
			e = -e - 1;
			struct medge_s *medge = &map.edges[e];

			//TODO: pretty much copy from the else
			// this needed only w/ maps w/ polys that
			// share edges
			//...
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
			else if (	cacheidx > 0 &&
					cacheidx < (edges_p - edges) &&
					edges[cacheidx].medgeidx == e )
			{
				/* note drawedge index 0 is invalid (used as NULL drawedge) */
				EmitCached (&edges[cacheidx]);
				continue;
			}

			// clip new and maybe cache
			//...
		}
	}

	//TODO: when the better scanline generation algo
	// is in place we'll need to set up each drawedge's next
	// so the emitted drawedge cluster is sorted by v

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
		DrawSurfaceEdges (&map.surfaces[drawsurf->msurfidx], 0);
	}

	if (0)
		printf("%u: %d surfs\n", r_framenum, num);
}
