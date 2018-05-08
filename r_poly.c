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

//TODO: figure out if drawedge idx 0 should still be avoided

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
 * When projecting and emitting, don't emit edges that are only 1 pixel
 *   high
 * If <= 1 edges are emitted, winding is not visible.
 * 1 emitted edge is possible due to math imprecision
 *
 * Span emitting is done by a loop working on l_u,l_du,l_top,l_bot,
 *   r_u,r_du,r_top,r_bot variables. Keep an eye on the upcoming edge
 *   to be popped.
 *   Loop is done when we hit/pass bottom of both left/right edges and
 *   scanedge list is empty.
 *   First L and R edges *should* start on the same v. But, math
 *   sometimes leads to odd cases that need to be patched up.
 */

#define MAX_EMITEDGES 65535

#define MAX_POLY_DRAWEDGES 32

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
	short top, bottom; /* screen coords;
		if high bit is set for either, originating edge
		projection is a top-down left edge */
};
#define DRAWEDGE_LEFT(_X) (((_X)->top & 0x8000) == 0x8000)
#define DRAWEDGE_TOP(_X) ((_X)->top & 0x7fff)
#define DRAWEDGE_BOT(_X) ((_X)->bottom & 0x7fff)

#define U_FRACBITS 20

/* scanedges are ephemeral, only needed when scan-converting a poly's
 * emitedges to spans
 * scanedges are stored in a single list, sorted by top coord */
struct scanedge_s
{
	struct drawedge_s *drawedge;
	struct scanedge_s *next;
	int top;
};

struct drawsurf_s *surfs = NULL;
struct drawsurf_s *surfs_p = NULL;
static struct drawsurf_s *surfs_end = NULL;

static struct drawedge_s *drawedges = NULL;
static struct drawedge_s *drawedges_p = NULL;
static struct drawedge_s *drawedges_end = NULL;

/* uncacheable emitted edges */
struct drawedge_s *extraedges = NULL;
struct drawedge_s *extraedges_p = NULL;
struct drawedge_s *extraedges_end = NULL;

static struct scanedge_s scanedges_l;
static struct scanedge_s scanedges_r;
static struct scanedge_s *scanedges = NULL;
static struct scanedge_s *scanedge_p = NULL;
static struct scanedge_s *scanedge_end = NULL;


static void
ProcessScanEdges (void)
{
	struct scanedge_s *se_l = scanedges_l.next;
	struct scanedge_s *se_r = scanedges_r.next;
#if 1
	if (se_l == NULL)
		R_Die ("no L scanedges");
	if (se_r == NULL)
		R_Die ("no R scanedges");
#endif
	int v = (se_l->top < se_r->top) ? se_l->top : se_r->top;
	if (v == 0)
	{
		/* math imprecision, and our vertical containment rules,
		 * can cause a missing scanline when a poly's vertex
		 * lies just on the top vplane
		 * pos = -5.1768 5.49495 -18.2086
		 * angles = 0.198413 0.343612 0
		*/
		if (se_l->top != 0)
		{
			se_l->drawedge->u += se_l->drawedge->du * (0 - se_l->top);
			se_l->top = 0;
			se_l->drawedge->top &= 0x8000;
		}
		else if (se_r->top != 0)
		{
			se_r->drawedge->u += se_r->drawedge->du * (0 - se_r->top);
			se_r->top = 0;
			se_r->drawedge->top &= 0x8000;
		}
	}
	int bot =	(DRAWEDGE_BOT(se_l->drawedge) > DRAWEDGE_BOT(se_r->drawedge)) ?
			DRAWEDGE_BOT(se_l->drawedge) :
			DRAWEDGE_BOT(se_r->drawedge);
	int l_u = 0x7fffffff;
	int l_du = 0;
	int r_u = 0x80000000;
	int r_du = 0;
	while (v < video.h && !(se_l == NULL && se_r == NULL && v > bot))
	{
		if (se_l != NULL && v >= se_l->top)
		{
			l_u = se_l->drawedge->u;
			/* pre-adjust so a simple shift down maintains containment rule */
			l_u += (1 << U_FRACBITS) / 2 - 1;
			l_du = se_l->drawedge->du;
			if (DRAWEDGE_BOT(se_l->drawedge) > bot)
				bot = DRAWEDGE_BOT(se_l->drawedge);
			se_l = se_l->next;
		}
		if (se_r != NULL && v >= se_r->top)
		{
			r_u = se_r->drawedge->u;
			/* pre-adjust so a simple shift down maintains containment rule */
			r_u -= (1 << U_FRACBITS) / 2;
			r_du = se_r->drawedge->du;
			if (DRAWEDGE_BOT(se_r->drawedge) > bot)
				bot = DRAWEDGE_BOT(se_r->drawedge);
			se_r = se_r->next;
		}
		if (l_u <= r_u && v >= 0)
			R_Span_ClipAndEmit (v, l_u >> U_FRACBITS, r_u >> U_FRACBITS);
		l_u += l_du;
		r_u += r_du;
		v++;
	}
}


static void
EmitScanEdge (struct drawedge_s *drawedge, int is_left)
{
	if (scanedge_p != scanedge_end)
	{
		struct scanedge_s *p;
		if (is_left)
			p = &scanedges_l;
		else
			p = &scanedges_r;
		struct scanedge_s *n = p->next;
		while (n != NULL && n->top < DRAWEDGE_TOP(drawedge))
		{
			n = n->next;
			p = p->next;
		}
		scanedge_p->next = p->next;
		p->next = scanedge_p;
		scanedge_p->top = DRAWEDGE_TOP(drawedge);
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
	CLIP_CHOPPED_BACK, /* chopped by a LR plane, but not visible;
				needed to gen enter/exit points */
};

static int backwards;
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

	int runsdown;

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
		runsdown = 1;
	}
	else
	{
		u1_f = _u2;
		v1_f = _v2;
		u2_f = _u1;
		v2_f = _v1;
		runsdown = 0;
	}

	/* math imprecision sometimes results in nearly-horizontal
	 * emitted edges just above or just below the screen */
	if (v2_f < 0.5)
		return 0;
	if (v1_f > video.h - 0.5)
		return 0;

	/* the pixel containment rule says an edge point exactly on the
	 * center of a pixel vertically will be considered to cover that
	 * pixel */
	int v1_i = ceil (v1_f - 0.5);
	int v2_i = floor (v2_f - 0.5);

	if (v1_i >= v2_i)
	{
		/* doesn't cross a pixel center vertically */
		return 0;
	}

	if (v1_i < 0) //TODO: shouldn't happen, right? especially if shift viewangle inwards
		R_Die ("top vertex above screen");//v1_i = 0;

	double du = (u2_f - u1_f) / (v2_f - v1_f);
	de->u = (u1_f + du * (v1_i + 0.5 - v1_f)) * (1 << U_FRACBITS);
	de->du = du * (1 << U_FRACBITS);
	de->top = v1_i | (runsdown << 15);
	de->bottom = v2_i | (runsdown << 15);

	return 1;
}


static struct drawedge_s *
NewExtraEdge (int *isleft)
{
	if (extraedges_p == extraedges_end)
		return NULL;
	if (!FillDrawEdge(extraedges_p))
		return NULL;
	*isleft =	(DRAWEDGE_LEFT(extraedges_p) && !backwards) ||
			(!DRAWEDGE_LEFT(extraedges_p) && backwards);
	return extraedges_p++;
}


static struct drawedge_s *
NewCachedDrawEdge (int *isleft)
{
	if (drawedges_p == drawedges_end)
		return NULL;
	if (!FillDrawEdge(drawedges_p))
		return NULL;
	*isleft =	(DRAWEDGE_LEFT(drawedges_p) && !backwards) ||
			(!DRAWEDGE_LEFT(drawedges_p) && backwards);
	return drawedges_p++;
}


/* assumes vertices straddle the plane */
static inline double *
Intersect (	const double v1[3],
		const double v2[3],
		double d1,
		double d2,
		double out[3] )
{
	double frac = d1 / (d1 - d2);
	out[0] = v1[0] + frac * (v2[0] - v1[0]);
	out[1] = v1[1] + frac * (v2[1] - v1[1]);
	out[2] = v1[2] + frac * (v2[2] - v1[2]);
	return out;
}


static int
ClipTB (const struct viewplane_s *p)
{
	double d1 = Vec_Dot (p->normal, clip_v1) - p->dist;
	double d2 = Vec_Dot (p->normal, clip_v2) - p->dist;
	if (d1 >= 0.0)
	{
		if (d2 < 0.0)
		{
			/* edge runs from front -> back */
			clip_v2 = Intersect (clip_v1, clip_v2, d1, d2, _clip_2);
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
			clip_v1 = Intersect (clip_v1, clip_v2, d1, d2, _clip_1);
			return CLIP_CHOPPED;
		}
	}
	/* shouldn't be reached */
	return CLIP_FRONT;
}


static int
ClipL (double d1, double d2)
{
	if (d1 >= 0.0)
	{
		if (d2 < 0.0)
		{
			/* edge runs from front -> back */
			clip_v2 = exit_left = Intersect (clip_v1, clip_v2, d1, d2, _exit_l);
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
			clip_v1 = enter_left = Intersect (clip_v1, clip_v2, d1, d2, _enter_l);
			return CLIP_CHOPPED;
		}
	}
	/* should not be reached */
	return CLIP_FRONT;
}


static int
ClipR (double d1, double d2)
{
	if (d1 >= 0.0)
	{
		if (d2 < 0.0)
		{
			/* edge runs from front -> back */
			clip_v2 = exit_right = Intersect (clip_v1, clip_v2, d1, d2, _exit_r);
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
			clip_v1 = enter_right = Intersect (clip_v1, clip_v2, d1, d2, _enter_r);
			return CLIP_CHOPPED;
		}
	}
	/* should not be reached */
	return CLIP_FRONT;
}


/* assumes at least 1 of L/R is active */
int
ClipWithLR (int planemask)
{
	const struct viewplane_s *lp = &camera.vplanes[VPLANE_LEFT];
	double l_d1 = Vec_Dot (lp->normal, clip_v1) - lp->dist;
	double l_d2 = Vec_Dot (lp->normal, clip_v2) - lp->dist;

	if ((planemask & VPLANE_LEFT_MASK) && !(planemask & VPLANE_RIGHT_MASK))
		return ClipL (l_d1, l_d2);

	const struct viewplane_s *rp = &camera.vplanes[VPLANE_RIGHT];
	double r_d1 = Vec_Dot (rp->normal, clip_v1) - rp->dist;
	double r_d2 = Vec_Dot (rp->normal, clip_v2) - rp->dist;

	if (!(planemask & VPLANE_LEFT_MASK) && (planemask & VPLANE_RIGHT_MASK))
		return ClipR (r_d1, r_d2);

	/* both active */

	if (l_d1 < 0.0 && l_d2 < 0.0 && r_d1 < 0.0 && r_d2 < 0.0)
		return CLIP_DOUBLE;

	if (l_d1 >= 0.0 && l_d2 >= 0.0)
		return ClipR (r_d1, r_d2);
	else if (l_d1 < 0.0 && l_d2 < 0.0)
	{
		/* behind left vplane */
		int st = ClipR (r_d1, r_d2);
		if (st == CLIP_CHOPPED)
			return CLIP_CHOPPED_BACK;
		/* cacheable */
		return CLIP_SINGLE;
	}

	if (r_d1 >= 0.0 && r_d2 >= 0.0)
		return ClipL (l_d1, l_d2);
	else if (r_d1 < 0.0 && r_d2 < 0.0)
	{
		/* behind right vplane */
		int st = ClipL (l_d1, l_d2);
		if (st == CLIP_CHOPPED)
			return CLIP_CHOPPED_BACK;
		/* cacheable */
		return CLIP_SINGLE;
	}

	/* crosses both planes */

	double l[3], r[3];
	Intersect (clip_v1, clip_v2, l_d1, l_d2, l);
	Intersect (clip_v1, clip_v2, r_d1, r_d2, r);

	if (l_d1 >= 0.0)
	{
		Vec_Copy (l, _exit_l);
		exit_left = _exit_l;
	}
	else
	{
		Vec_Copy (l, _enter_l);
		enter_left = _enter_l;
	}

	if (r_d1 >= 0.0)
	{
		Vec_Copy (r, _exit_r);
		exit_right = _exit_r;
	}
	else
	{
		Vec_Copy (r, _enter_r);
		enter_right = _enter_r;
	}

	if (l_d1 >= 0.0)
	{
		if (r_d2 >= 0.0)
		{
			double d = Vec_Dot (rp->normal, l) - rp->dist;
			if (d >= 0.0)
			{
				/* enter R exit L; in front of viewpoint */
				clip_v1 = enter_right;
				clip_v2 = exit_left;
				return CLIP_CHOPPED;
			}
			else
			{
				/* exit L enter R; behind viewpoint */
				return CLIP_CHOPPED_BACK;
			}
		}
		else
		{
			double d = Vec_Dot (rp->normal, l) - rp->dist;
			if (d >= 0.0)
				clip_v2 = exit_left;
			else
				clip_v2 = exit_right;
			return CLIP_CHOPPED;
		}
	}
	else
	{
		if (r_d2 >= 0.0)
		{
			double d = Vec_Dot (lp->normal, r) - lp->dist;
			if (d >= 0.0)
				clip_v1 = enter_right;
			else
				clip_v1 = enter_left;
			return CLIP_CHOPPED;
		}
		else
		{
			double d = Vec_Dot (lp->normal, r) - lp->dist;
			if (d >= 0.0)
			{
				clip_v1 = enter_left;
				clip_v2 = exit_right;
				return CLIP_CHOPPED;
			}
			else
			{
				/* exit R enter L; behind viewpoint */
				return CLIP_CHOPPED_BACK;
			}
		}
	}

	/* should not be reached */
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
		struct drawedge_s *de = &drawedges[cacheidx];
		int isleft =	(DRAWEDGE_LEFT(de) && !backwards) ||
				(!DRAWEDGE_LEFT(de) && backwards);
		EmitScanEdge (de, isleft);
		return 1;
	}
	return 0;
}


static void
ProcessEnterExitEdge (double enter[3], double exit[3], int planemask)
{
	clip_v1 = exit;
	clip_v2 = enter;

	if (planemask & VPLANE_TOP_MASK)
	{
		if (ClipTB(&camera.vplanes[VPLANE_TOP]) == CLIP_SINGLE)
		{
			/* fully behind top */
			return;
		}
	}

	/* either chopped by or fully in front of top */

	if (planemask & VPLANE_BOTTOM_MASK)
	{
		if (ClipTB(&camera.vplanes[VPLANE_BOTTOM]) == CLIP_SINGLE)
		{
			/* behind bottom */
			return;
		}
	}

	/* chopped or not, it's visible */

	int isleft;
	struct drawedge_s *de = NewExtraEdge (&isleft);
	if (de != NULL)
		EmitScanEdge (de, isleft);
}


static void
GenSpansForEdgeLoop (int edgeloop_start, int numedges, int planemask)
{
//TODO: pass in scanedges & extraedges
	scanedge_p = scanedges;
	extraedges_p = extraedges;

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
			backwards = 1;
			medgeidx = -medgeidx - 1;
			if (TryEdgeCache(medgeidx))
				continue;
			medge = &map.edges[medgeidx];
			clip_v1 = map.vertices[medge->v[1]].xyz;
			clip_v2 = map.vertices[medge->v[0]].xyz;
		}
		else
		{
			backwards = 0;
			if (TryEdgeCache(medgeidx))
				continue;
			medge = &map.edges[medgeidx];
			clip_v1 = map.vertices[medge->v[0]].xyz;
			clip_v2 = map.vertices[medge->v[1]].xyz;
		}

		lr_status = CLIP_FRONT;
		t_status = CLIP_FRONT;
		b_status = CLIP_FRONT;

		if (planemask & (VPLANE_LEFT_MASK | VPLANE_RIGHT_MASK))
		{
			lr_status = ClipWithLR (planemask);
			if (lr_status == CLIP_SINGLE || lr_status == CLIP_DOUBLE)
			{
				/* can cache as non-visible this frame */
				medge->cachenum = r_framenum | 0x80000000;
				continue;
			}
			else if (lr_status == CLIP_CHOPPED_BACK)
			{
				/* non-visible but it can't be cached as
				 * a later edge may need it to generate
				 * enter/exit points */
				continue;
			}
		}

		if (planemask & VPLANE_TOP_MASK)
		{
			t_status = ClipTB (&camera.vplanes[VPLANE_TOP]);
			if (t_status == CLIP_SINGLE && lr_status == CLIP_FRONT)
			{
				/* can cache as non-visible this frame */
				medge->cachenum = r_framenum | 0x80000000;
				continue;
			}
		}

		if (planemask & VPLANE_BOTTOM_MASK)
		{
			b_status = ClipTB (&camera.vplanes[VPLANE_BOTTOM]);
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
			int isleft;
			if ((de = NewCachedDrawEdge(&isleft)) != NULL)
			{
				medge->cachenum = de - drawedges;
				de->medgeidx = medgeidx;
				EmitScanEdge (de, isleft);
			}
		}
		else
		{
			/* cipped, un-cacheable */
			struct drawedge_s *de;
			int isleft;
			if ((de = NewExtraEdge (&isleft)) != NULL)
				EmitScanEdge (de, isleft);
		}
	}

	if (enter_left != NULL || exit_left != NULL)
	{
#if 1
		if (enter_left == NULL)
			R_Die ("L exit, no enter");
		if (exit_left == NULL)
			R_Die ("L enter, no exit");
#endif
		ProcessEnterExitEdge (enter_left, exit_left, planemask);
	}

	if (enter_right != NULL || exit_right != NULL)
	{
#if 1
		if (enter_right == NULL)
			R_Die ("R exit, no enter");
		if (exit_right == NULL)
			R_Die ("R enter, no exit");
#endif
		ProcessEnterExitEdge (enter_right, exit_right, planemask);
	}

	if (scanedge_p == scanedges)
	{
		/* no scanedges generated; winding not visible */
		return;
	}

	if (scanedge_p - scanedges == 1)
	{
		/* possible to gen only 1 scanedge due to math
		 * imprecision
		 * pos = 50.452744 20.029897 42.966270
		 * angles = 0.198413 5.419247 0.000000 */
		return;
	}

	ProcessScanEdges ();
}


void
R_GenSpansForSurfaces (unsigned int first, int count, int planemask, int backface_check)
{
	struct scanedge_s _scanpool[MAX_POLY_DRAWEDGES];
	scanedges = _scanpool;
	scanedge_end = _scanpool + (sizeof(_scanpool) / sizeof(_scanpool[0]));
	scanedges_l.drawedge = NULL;
	scanedges_l.next = NULL;
	scanedges_l.top = -9999;
	scanedges_r.drawedge = NULL;
	scanedges_r.next = NULL;
	scanedges_r.top = -9999;

	struct drawedge_s _extra[MAX_POLY_DRAWEDGES];
	extraedges = _extra;
	extraedges_end = _extra + (sizeof(_extra) / sizeof(_extra[0]));

	while (count-- > 0)
	{
		struct msurface_s *msurf = &map.surfaces[first++];
		if (!backface_check || Map_DistFromSurface(msurf, camera.pos) >= SURF_BACKFACE_EPSILON)
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


#if 0
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
DebugDrawEdge (struct drawedge_s *de, int c)
{
	int y, u;
	for (y = DRAWEDGE_TOP(de), u = de->u; y <= DRAWEDGE_BOT(de); y++, u += de->du)
	{
		int x = u >> U_FRACBITS;
		if (y >= 0 && y < video.h && x >= 0 && x < video.w)
		{
			if (video.bpp == 16)
				((unsigned short *)video.rows[y])[x] = (unsigned short)c;
			else
				((unsigned int *)video.rows[y])[x] = (unsigned int)c;
		}
	}
}
#endif
