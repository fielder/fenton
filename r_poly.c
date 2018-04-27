#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

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
DebugDrawMapEdge (int e)
{
	if (e < 0)
		e = -e - 1;
	R_3DLine (	map.vertices[map.edges[e].v[0]].xyz,
			map.vertices[map.edges[e].v[1]].xyz,
			-1);
}


static void
ProcessScanEdges (void)
{
#if 1
	struct scanedge_s *se;
	for (se = scanedge_head.next; se != NULL; se = se->next)
		DebugDrawEdge (se->drawedge);
#else
/* pre-adjust screen x coords so we end up with the correct pixel after shifting down */
// de->u += ((1 << 20) / 2) - 1;

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
static double _clip_1[3];
static double _clip_2[3];
static int lr_status;
static int t_status;
static int b_status;
static double *enter_left, *exit_left;
static double *enter_right, *exit_right;
static double _enter_l[3], _exit_l[3];
static double _enter_r[3], _exit_r[3];


static int
FillDrawEdge (struct drawedge_s *de)
{
	double _u1, _v1;
	double _u2, _v2;

	double u1_f, v1_f;
	double u2_f, v2_f;

	double local[3], out[3];
	double scale;

	Vec_Subtract (clip_v1, camera.pos, local);
	Vec_Transform (camera.xform, local, out);
	scale = camera.dist / out[2];
	_u1 = camera.center_x - scale * out[0];
	_v1 = camera.center_y - scale * out[1];

	Vec_Subtract (clip_v2, camera.pos, local);
	Vec_Transform (camera.xform, local, out);
	scale = camera.dist / out[2];
	_u2 = camera.center_x - scale * out[0];
	_v2 = camera.center_y - scale * out[1];

	/* sort so 1 is top, 2 is bottom */
	if (_v1 <= _v2)
	{
		u1_f = _u1;
		v1_f = _v1;
		u2_f = _u2;
		v2_f = _v2;
	}
	else
	{
		u1_f = _u2;
		v1_f = _v2;
		u2_f = _u1;
		v2_f = _v1;
	}


	/* the pixel containment rule says an edge point exactly on the
	 * center of a pixel vertically will be considered to cover that
	 * pixel */
	int v1_i = ceil (v1_f - 0.5);
	int v2_i = floor (v2_f - 0.5);

	if (v1_i == v2_i)
	{
		/* doesn't cross a pixel center vertically */
		return 0;
	}

	if (v2_i < 0 || v1_i >= video.h)
	{
		/* math imprecision sometimes results in nearly-horizontal
		 * emitted edges just above or just below the screen */
		return 0;
	}

	double du = (u2_f - u1_f) / (v2_f - v1_f);
	de->u = (u1_f + du * (v1_i + 0.5 - v1_f)) * (1 << U_FRACBITS);
	de->du = du * (1 << U_FRACBITS);
	de->top = v1_i;
	de->bottom = v2_i;

	return 1;
}


static struct drawedge_s *
NewExtraEdge (void)
{
	if (extraedges_p == extraedges_end)
		return NULL;
	if (!FillDrawEdge(extraedges_p))
		return NULL;
	return extraedges_p++;
}


static struct drawedge_s *
NewDrawEdge (void)
{
	if (drawedges_p == drawedges_end)
		return NULL;
	if (!FillDrawEdge(drawedges_p))
		return NULL;
	return drawedges_p++;
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
			_clip_2[0] = clip_v1[0] + frac * (clip_v2[0] - clip_v1[0]);
			_clip_2[1] = clip_v1[1] + frac * (clip_v2[1] - clip_v1[1]);
			_clip_2[2] = clip_v1[2] + frac * (clip_v2[2] - clip_v1[2]);
			clip_v2 = _clip_2;
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
			_clip_1[0] = clip_v1[0] + frac * (clip_v2[0] - clip_v1[0]);
			_clip_1[1] = clip_v1[1] + frac * (clip_v2[1] - clip_v1[1]);
			_clip_1[2] = clip_v1[2] + frac * (clip_v2[2] - clip_v1[2]);
			clip_v1 = _clip_1;
			return CLIP_CHOPPED;
		}
	}
	/* shouldn't be reached */
	return CLIP_FRONT;
}


#include <stdio.h>
static int
ClipWithLR (int planemask)
{
	//TODO: lots of duplicated code here

	if (planemask & VPLANE_LEFT_MASK)
	{
		if (planemask & VPLANE_RIGHT_MASK)
		{
			/* if both LR are active we must check FULL edge
			 * against both planes; needed to generate
			 * enter/exit points */
			const struct viewplane_s *lp = &camera.vplanes[VPLANE_LEFT];
			const struct viewplane_s *rp = &camera.vplanes[VPLANE_RIGHT];
			double l_d1 = Vec_Dot (lp->normal, clip_v1) - lp->dist;
			double l_d2 = Vec_Dot (lp->normal, clip_v2) - lp->dist;
			double r_d1 = Vec_Dot (rp->normal, clip_v1) - rp->dist;
			double r_d2 = Vec_Dot (rp->normal, clip_v2) - rp->dist;
			// if cross L back to L front, set L enter, L intersect point
			// if crosses L front to L back, set L exit, L intersect point
			// if cross R back to R front, set R enter, R intersect point
			// if crosses R front to R back, set R exit, R intersect point
			// if behind L, unchopped by R, return CLIP_SINGLE
			// if behind R, unchopped by L, return CLIP_SINGLE
			// if intersect both, need an extra side check for the
			//   fully-behind-camera check
			//TODO: ...
			return CLIP_FRONT;
		}
		else
		{
			/* only 1 vertical plane active
			 * return CLIP_CHOPPED, CLIP_SINGLE, or CLIP_FRONT */
			const struct viewplane_s *p = &camera.vplanes[VPLANE_LEFT];
			double d1 = Vec_Dot (p->normal, clip_v1) - p->dist;
			double d2 = Vec_Dot (p->normal, clip_v2) - p->dist;
			if (d1 >= 0.0)
			{
				if (d2 < 0.0)
				{
					/* edge runs from front -> back */
					double frac = d1 / (d1 - d2);
					_exit_l[0] = clip_v1[0] + frac * (clip_v2[0] - clip_v1[0]);
					_exit_l[1] = clip_v1[1] + frac * (clip_v2[1] - clip_v1[1]);
					_exit_l[2] = clip_v1[2] + frac * (clip_v2[2] - clip_v1[2]);
					clip_v2 = exit_left = _exit_l;
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
					_enter_l[0] = clip_v1[0] + frac * (clip_v2[0] - clip_v1[0]);
					_enter_l[1] = clip_v1[1] + frac * (clip_v2[1] - clip_v1[1]);
					_enter_l[2] = clip_v1[2] + frac * (clip_v2[2] - clip_v1[2]);
					clip_v1 = enter_left = _enter_l;
					return CLIP_CHOPPED;
				}
			}
			/* shouldn't be reached */
			return CLIP_FRONT;
		}
	}

	if (planemask & VPLANE_RIGHT_MASK)
	{
		/* only 1 vertical plane active
		 * return CLIP_CHOPPED, CLIP_SINGLE, or CLIP_FRONT */
		const struct viewplane_s *p = &camera.vplanes[VPLANE_RIGHT];
		double d1 = Vec_Dot (p->normal, clip_v1) - p->dist;
		double d2 = Vec_Dot (p->normal, clip_v2) - p->dist;
		if (d1 >= 0.0)
		{
			if (d2 < 0.0)
			{
				/* edge runs from front -> back */
				double frac = d1 / (d1 - d2);
				_exit_r[0] = clip_v1[0] + frac * (clip_v2[0] - clip_v1[0]);
				_exit_r[1] = clip_v1[1] + frac * (clip_v2[1] - clip_v1[1]);
				_exit_r[2] = clip_v1[2] + frac * (clip_v2[2] - clip_v1[2]);
				clip_v2 = exit_right = _exit_r;
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
				_enter_r[0] = clip_v1[0] + frac * (clip_v2[0] - clip_v1[0]);
				_enter_r[1] = clip_v1[1] + frac * (clip_v2[1] - clip_v1[1]);
				_enter_r[2] = clip_v1[2] + frac * (clip_v2[2] - clip_v1[2]);
				clip_v1 = enter_right = _enter_r;
				return CLIP_CHOPPED;
			}
		}
		/* shouldn't be reached */
		return CLIP_FRONT;
	}

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
ProcessEnterExitEdge (double enter[3], double exit[3], int planemask)
{
	clip_v1 = enter;
	clip_v2 = exit;

	if (planemask & VPLANE_TOP_MASK)
	{
		if (ClipWithTB(&camera.vplanes[VPLANE_TOP]) == CLIP_SINGLE)
		{
			/* fully behind top */
			return;
		}
	}

	/* either chopped by or fully in front of top */

	if (planemask & VPLANE_BOTTOM_MASK)
	{
		if (ClipWithTB(&camera.vplanes[VPLANE_BOTTOM]) == CLIP_SINGLE)
		{
			/* behind bottom */
			return;
		}
	}

	/* chopped or not, it's visible */

	struct drawedge_s *de = NewExtraEdge ();
	if (de != NULL)
		EmitScanEdge (de);
}


static void
GenSpansForEdgeLoop (int edgeloop_start, int numedges, int planemask)
{
	if (0)
	{
		int *eloop_ptr = &map.edgeloops[edgeloop_start];
		while (numedges-- > 0)
			DebugDrawMapEdge (*eloop_ptr++);
		return;
	}

	struct scanedge_s scanpool[MAX_POLY_DRAWEDGES];

	scanedge_p = scanpool;
	scanedge_end = scanpool + (sizeof(scanpool) / sizeof(scanpool[0]));
	scanedge_head.drawedge = NULL;
	scanedge_head.next = NULL;
	scanedge_head.top = -9999;

	struct drawedge_s _ex[MAX_POLY_DRAWEDGES];
	extraedges = extraedges_p = _ex;
	extraedges_end = _ex + (sizeof(_ex) / sizeof(_ex[0]));

	enter_left = NULL;
	exit_left = NULL;
	enter_right = NULL;
	exit_right = NULL;

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

	if (enter_left != NULL || exit_left != NULL)
	{
		if (enter_left == NULL)
			R_Die ("L exit, no enter");
		if (exit_left == NULL)
			R_Die ("L enter, no exit");
		ProcessEnterExitEdge (enter_left, exit_left, planemask);
	}
	if (enter_right != NULL || exit_right != NULL)
	{
		if (enter_right == NULL)
			R_Die ("R exit, no enter");
		if (exit_right == NULL)
			R_Die ("R enter, no exit");
		ProcessEnterExitEdge (enter_right, exit_right, planemask);
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
