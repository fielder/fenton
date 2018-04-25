#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "appio.h"
#include "vec.h"
#include "fenton.h"
#include "bdat.h"
#include "map.h"
#include "render.h"

#define MAX_EMITEDGES 65535

#define MAX_POLY_DRAWEDGES 64

/*
 * Since extra edges aren't cached and we scan a poly right after
 * edge processing, emit edges really are useful only for cached
 * edges. So, store only cached edges in drawedges.
 */

/* as world edges are projected, they're cached for re-use
 * idea is an edge is always shared with at least 2 surfaces
 * emit edges run top->bottom */
struct drawedge_s
{
	unsigned int medgeidx; /* medge this came from */
	int u, du; /* both fixed-point */
	short top, bottom; /* screen coords */
};
#define U_FRACBITS 20

struct drawsurf_s
{
	struct drawspan_s *spans;

	unsigned int msurfidx;

	unsigned short numspans;

	//TODO: texture mapping stuff

	char pad[2]; /* align on 8-byte boundary */
};

/* scanedges are ephemeral, only needed when scan-converting a poly's
 * emitedges to spans
 * scanedges are stored in a single list, sorted by top coord */
struct scanedge_s
{
	struct drawedge_s *drawedge;
	struct scanedge_s *next;
	int top;
};

static struct drawsurf_s *surfs = NULL;
static struct drawsurf_s *surfs_p = NULL;
static struct drawsurf_s *surfs_end = NULL;

static struct drawedge_s *drawedges = NULL;
static struct drawedge_s *drawedges_p = NULL;
static struct drawedge_s *drawedges_end = NULL;

/* uncacheable emitted edges */
struct drawedge_s *extraedges = NULL;
struct drawedge_s *extraedges_p = NULL;
struct drawedge_s *extraedges_end = NULL;

static struct scanedge_s scanedge_head;
static struct scanedge_s *scanedge_p = NULL;
static struct scanedge_s *scanedge_end = NULL;


static void
DebugDrawEdge (struct drawedge_s *de)
{
	int y, u;
	for (y = de->top, u = de->u; y <= de->bottom; y++, u += de->du)
	{
		int x = u >> U_FRACBITS;
		if (y >= 0 && y < video.h && x >= 0 && x < video.w)
		{
			if (video.bpp == 16)
				((unsigned short *)video.rows[y])[x] = 0xffff;
			else
				((unsigned int *)video.rows[y])[x] = 0xffffffff;
		}
	}
}


static void
ProcessScanEdges (void)
{
#if 1
	struct scanedge_s *se;
	for (se = scanedge_head.next; se != NULL; se = se->next)
		DebugDrawEdge (se->drawedge);
#else
	//TODO: do me
	int l_u, l_du, l_bottom;
	int r_u, r_du, r_bottom;
	int v = scanedge_head.next->top;
	int nextv = v;
	while (1)
	{
		while (v < nextv)
		{
			R_Span_ClipAndEmit (v, l_u >> U_FRACBITS, r_u >> U_FRACBITS);
			l_u += l_du;
			r_u += r_du;
			v++;
		}
		// run through scanedge list, emitting spans
//		if (no more scanedges waiting and )
//			break;
	}
#endif
}


static void
EmitScanEdge (struct drawedge_s *drawedge)
{
	if (scanedge_p != scanedge_end)
	{
		struct scanedge_s *p = &scanedge_head;
		struct scanedge_s *n = p->next;;
		while (n != NULL && n->top < drawedge->top)
		{
			n = n->next;
			p = p->next;
		}
		scanedge_p->next = p->next;
		p->next = scanedge_p;
		scanedge_p->top = drawedge->top;
		scanedge_p->drawedge = drawedge;
		scanedge_p++;
	}
}


enum
{
	CLIP_CHOPPED, /* chopped by plane */
	CLIP_SINGLE, /* behind L or behind R */
	CLIP_DOUBLE, /* behind both L and R */
	CLIP_FRONT, /* fully in front of plane */
};

static double *clip_v1;
static double *clip_v2;
static int lr_status;
static int t_status;
static int b_status;
static double *enter_left, *exit_left;
static double *enter_right, *exit_right;
static double _ent_l[3], _ext_l[3];
static double _ent_r[3], _ext_r[3];


static int
FillDrawEdge (struct drawedge_s *de)
{
	/*
	de->u = ;
	de->du = ;
	de->top = ;
	de->bottom = ;
	*/
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
	}
#endif
	return 1;
}


static struct drawedge_s *
NewExtraEdge (void)
{
	struct drawedge_s *de;
	//TODO: ...
	de = NULL;
	return de;
}


static struct drawedge_s *
NewDrawEdge (void)
{
	if (drawedges_p == drawedges_end)
		return NULL;
	if (!FillDrawEdge(drawedges_p))
		return NULL;
	struct drawedge_s *ret = drawedges_p++;
	return ret;
}


static int
ClipWithTB (const struct viewplane_s *p)
{
	double d1 = Vec_Dot (p->normal, clip_v1) - p->dist;
	double d2 = Vec_Dot (p->normal, clip_v2) - p->dist;

	if (d1 >= 0.0)
	{
		if (d2 < 0.0)
		{
			/* edge runs from front -> back */
			double frac = d1 / (d1 - d2);
			clip_v2[0] = clip_v1[0] + frac * (clip_v2[0] - clip_v1[0]);
			clip_v2[1] = clip_v1[1] + frac * (clip_v2[1] - clip_v1[1]);
			clip_v2[2] = clip_v1[2] + frac * (clip_v2[2] - clip_v1[2]);
			return CLIP_CHOPPED;
		}
		else
		{
			/* both vertices on the front side */
			return CLIP_FRONT;
		}
	}
	else
	{
		if (d2 < 0.0)
		{
			/* both vertices behind a plane; the edge is
			 * fully clipped away */
			return CLIP_SINGLE;
		}
		else
		{
			/* edge runs from back -> front */
			double frac = d1 / (d1 - d2);
			clip_v1[0] = clip_v1[0] + frac * (clip_v2[0] - clip_v1[0]);
			clip_v1[1] = clip_v1[1] + frac * (clip_v2[1] - clip_v1[1]);
			clip_v1[2] = clip_v1[2] + frac * (clip_v2[2] - clip_v1[2]);
			return CLIP_CHOPPED;
		}
	}
	/* shouldn't be reached */
	return CLIP_FRONT;
}


static int
ClipWithLR (int planemask)
{
	// note if both LR are active we must check FULL edge against
	// both planes; needed to gen enter/exit points
	// 
	// if only 1 plane active, return CLIP_CHOPPED, CLIP_SINGLE, or CLIP_FRONT

	int l_status = CLIP_FRONT;
	double clipped_l[3];

	// if left active
	//   find left intersect point
	// if not right active:
	//   set clip point if chopped by left
	//   return left clip status
	return CLIP_FRONT;
}


static int
TryEdgeCache (int medgeidx)
{
	struct medge_s *medge = &map.edges[medgeidx];
	unsigned int cacheidx = medge->cachenum & ~0x80000000;
	if (medge->cachenum & 0x80000000)
	{
		if (cacheidx == r_framenum)
		{
			/* already visited this edge this frame and it's
			 * been rejected as behind vplane(s) */
			return 1;
		}
	}
	/* note drawedge index 0 is invalid (used as NULL drawedge) */
	else if (cacheidx > 0 &&
		cacheidx < (drawedges_p - drawedges) &&
		drawedges[cacheidx].medgeidx == medgeidx)
	{
		EmitScanEdge (&drawedges[cacheidx]);
		return 1;
	}
	return 0;
}


static void
GenSpansForEdgeLoop (int edgeloop_start, int numedges, int planemask)
{
	struct scanedge_s scanpool[MAX_POLY_DRAWEDGES];

	scanedge_p = scanpool;
	scanedge_end = scanpool + (sizeof(scanpool) / sizeof(scanpool[0]));
	scanedge_head.drawedge = NULL;
	scanedge_head.next = NULL;
	scanedge_head.top = -9999;

	// set up extra edges
	//TODO: ...

	enter_left = exit_left = NULL;
	enter_right = exit_right = NULL;

	int *eloop_ptr = &map.edgeloops[edgeloop_start];
	while (numedges-- > 0)
	{
		int medgeidx = *eloop_ptr++;
		struct medge_s *medge;

		if (medgeidx < 0)
		{
			if (TryEdgeCache(-medgeidx - 1))
				continue;
			medge = &map.edges[-medgeidx - 1];
			clip_v1 = map.vertices[medge->v[1]].xyz;
			clip_v2 = map.vertices[medge->v[0]].xyz;
		}
		else
		{
			if (TryEdgeCache(medgeidx))
				continue;
			medge = &map.edges[medgeidx];
			clip_v1 = map.vertices[medge->v[0]].xyz;
			clip_v2 = map.vertices[medge->v[1]].xyz;
		}

		lr_status = CLIP_FRONT;
		t_status = CLIP_FRONT;
		b_status = CLIP_FRONT;

		if (planemask & VPLANE_LR_MASK)
		{
			lr_status = ClipWithLR (planemask);
			if (lr_status == CLIP_SINGLE || lr_status == CLIP_DOUBLE)
			{
				/* can cache as non-visible this frame */
				medge->cachenum = r_framenum | 0x80000000;
				continue;
			}
		}

		if (planemask & VPLANE_TOP_MASK)
		{
			t_status = ClipWithTB (&camera.vplanes[VPLANE_TOP]);
			if (t_status == CLIP_SINGLE && lr_status == CLIP_FRONT)
			{
				/* can cache as non-visible this frame */
				medge->cachenum = r_framenum | 0x80000000;
				continue;
			}
		}

		if (planemask & VPLANE_BOTTOM_MASK)
		{
			b_status = ClipWithTB (&camera.vplanes[VPLANE_BOTTOM]);
			if (b_status == CLIP_SINGLE && lr_status == CLIP_FRONT)
			{
				/* can cache as non-visible this frame */
				medge->cachenum = r_framenum | 0x80000000;
				continue;
			}
		}

		if (	lr_status == CLIP_FRONT &&
			t_status == CLIP_FRONT &&
			b_status == CLIP_FRONT )
		{
			/* unclipped edge; cacheable */
			struct drawedge_s *de;
			if ((de = NewDrawEdge()) != NULL)
			{
				medge->cachenum = de - drawedges;
				EmitScanEdge (de);
			}
		}
		else
		{
			struct drawedge_s *de = NewExtraEdge ();
			if (de != NULL)
				EmitScanEdge (de);
		}
	}

	if (enter_left != NULL)
	{
		if (exit_left == NULL)
			R_Die ("L enter, no exit");
		// gen extra edge
		//...
	}
	if (enter_right != NULL)
	{
		if (exit_right == NULL)
			R_Die ("R enter, no exit");
		// gen extra edge
		//...
	}

	if (scanedge_p - scanpool < 2)
	{
		/* must have at least 2 edges
		 * 2 because we omit horizontal drawedges that don't
		 * cross a pixel center vertically */
		R_Die ("too few edges created");
	}

	ProcessScanEdges ();
}


static void
ProcessSurf (struct msurface_s *msurf, int planemask)
{
	if (surfs_p != surfs_end)
	{
		struct drawspan_s *firstspan = r_spans;
		GenSpansForEdgeLoop (msurf->edgeloop_start, msurf->numedges, planemask);
		if (r_spans != firstspan)
		{
			/* spans were created, surf is visible */
			surfs_p->spans = firstspan;
			surfs_p->numspans = r_spans - firstspan;
			surfs_p->msurfidx = msurf - map.surfaces;
			//TODO: texturing n' stuff
			surfs_p++;
		}
	}
}


void
R_GenSpansForSurfaces (unsigned int first, int count, int planemask, int backface_check)
{
	while (count-- > 0)
	{
		struct msurface_s *msurf = &map.surfaces[first++];

		if (!backface_check || Map_DistFromSurface(msurf, camera.pos) >= SURF_BACKFACE_EPSILON)
			ProcessSurf (msurf, planemask);
	}
}


//TODO: probably don't need reversewinding if edge scanning is smart enough to not need explicit winding edge ordering/direction
int
R_CheckPortalVisibility (struct mportal_s *portal, int planemask, int reversewinding)
{
	// clip and emit edges just as usual
	// scan over the emitted edges; if a span is visible return true
	// all spans don't need to be checked; only 1 visible span is sufficient
	// the only side-effect of this function will be emitted edges
	//TODO: ...
	return 0;
}


void
R_Surf_DrawDebug (void)
{
}


void
R_Surf_BeginFrame (void *surfbuf, int surfbufsize, void *edgebuf, int edgebufsize)
{
	unsigned int cnt;

	surfs_p = surfs = AlignAllocation (surfbuf, surfbufsize, sizeof(*surfs), &cnt);
	surfs_end = surfs + cnt;

	drawedges_p = drawedges = AlignAllocation (edgebuf, edgebufsize, sizeof(*drawedges), &cnt);
	cnt = (cnt > MAX_EMITEDGES) ? (MAX_EMITEDGES) : (cnt);
	drawedges_end = drawedges + cnt;

	drawedges_p++; /* index 0 is used for NULL drawedge */
}


/* ================================================================== */
/* ================================================================== */
/* ================================================================== */


#if 0

static int
ClampU (int u)
{
	if (u < 0)
		return 0;
	if (u >= video.w * (1<<20))
		return video.w * (1<<20) - 1;
	return u;
}


#endif


#if 0
static void
EmitEdge (const float v1[3], const float v2[3])
{
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
}
#endif


#if 0

static int
GenerateDrawEdges (unsigned int edgeloop_start, int numedges, const struct cplane_s *clips)
{
	if (drawedges_p + numedges + 2 > drawedges_end)
		return 0;

	struct drawedge_s *firstemit = drawedges_p;
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
			else if (	cacheidx > 0 && // note drawedge index 0 is invalid (used as NULL drawedge)
					cacheidx < (drawedges_p - drawedges) &&
					drawedges[cacheidx].medgeidx == e )
			{
				EmitCached (&drawedges[cacheidx]);
				continue;
			}

			clip_v1 = map.vertices[medge->v[0]].xyz;
			clip_v2 = map.vertices[medge->v[1]].xyz;

			enter_left = NULL;
			exit_left = NULL;
			enter_right = NULL;
			exit_right = NULL;

			int fl = ClipEdge (clips);

			if (fl == CLIPFL_REJECT_LR)
			{
				medge->cachenum = 0x80000000 | r_framenum;
				continue;
			}
			// clip new and maybe cache
			//...
		}
	}

	//TODO: when the better scanline generation algo
	// is in place we'll need to set up each drawedge's next
	// so the emitted drawedge cluster is sorted by v

	return drawedges_p - firstemit;
}
#endif


#if 0
static void
DrawSurfEdge (int e, int c)
{
	if (e < 0)
		e = -e - 1;
	R_3DLine (map.vertices[map.edges[e].v[0]].xyz, map.vertices[map.edges[e].v[1]].xyz, c);
}
#endif


#if 0
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
#endif





#if 0

	OLD JUNK


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


struct cplane_s
{
	double normal[3];
	double dist;
	struct cplane_s *next;
	int vplaneidx;
};


/* for (i = VPLANE_ITER_MIN; i <= VPLANE_ITER_MAX; i <<= 1) { } */
#define VPLANE_ITER_MIN VPLANE_LEFT_MASK
#define VPLANE_ITER_MAX VPLANE_BOTTOM_MASK


	struct drawsurf_s *firstemitsurf = surfs_p;

	while (count-- > 0)
	{
		struct msurface_s *msurf = &map.surfaces[first++];

		if (!backface_check || Map_DistFromSurface(msurf, camera.pos) >= SURF_BACKFACE_EPSILON)
			GenerateDrawSurf (msurf, planemask);
	}

	/* create spans */
	while (firstemitsurf != surfs_p)
	{
		firstemitsurf->spans = r_spans;
		ScanEdges (&drawedges[firstemitsurf->firstdrawedgeidx], firstemitsurf->numdrawedges);
		firstemitsurf->numspans = r_spans - firstemitsurf->spans;
		firstemitsurf++;
	}

#endif

#if 0
/*
 * A chopped edge is never cached.
 * Clip against LR first
 *   If LR are not active, skip LR clipping altogether. There are no
 *     enter/exit points and no extra edges created in this case.
 *   Enter/exit points should be saved when clipping against LR
 *   If only 1 LR vplane is active, edges behind it can be cached as
 *     non-visible.
 *   If both LR are active, edges behind one vplane must still be
 *     clipped against the other vplane as it could contribute to that
 *     vplane's enter/exit points.
 *   If both are active, edge is unchopped, and it's behind 1 or both LR
 *     vplanes, cache as non-visible.
 *   If an enter/exit point is generated, BOTH enter and exit points
 *     must be created. Die & debug if only an enter or only an exit was
 *     made.
 * If an edge is unchopped by LR, and rejected by TB, cache as
 *   non-visible
 * After LR, clip against TB
 *   If TB are not active, skip
 * After edges are run through, clip extra LR edges against TB and
 *   emit. Emitted extra LR edges are never cached.
 * If less than 2 edges emitted, die and debug. 2 because we ignore
 *   horizontal edges. eg: simple square emits only 2 vertical edges.
 * When projecting and emitting, don't emit edges that are only 1 pixel
 *   high
 *
 * Emitted edges are stored on a single list, sorted by edge's top v.
 * When adding to list, start searching at bottom as it's more likely
 *   a quick insert.
 * Span emitting is done by a loop working on l_u,l_du,l_top,l_bot,
 *   r_u,r_du,r_top,r_bot variables. Keep an eye on the upcoming edge
 *   to be popped. When next edge is needed, determine whether it's left
 *   or right by checking it's u.
 *   Loop is done when we hit/pass bottom of both left/right edges and
 *   scanedge list is empty.
 */
#endif
#if 0
		if (de == NULL)
		{
			/* no drawedges available */
			//TODO: probably check earlier for enough available
			return;
		}
#endif
