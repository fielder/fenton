#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "bdat.h"
#include "render.h"

#define PTR2IDX(PTR) ((int)((PTR) - gspans))
#define IDX2PTR(IDX) (gspans + (IDX))
#define NULLIDX 0

/* green span */
struct gspan_s
{
	unsigned short previdx, nextidx;
	short left, right;
};

/* global, but point to a temp buffer on the stack during refresh */
struct drawspan_s *r_spans = NULL;
struct drawspan_s *r_spans_p = NULL;
static struct drawspan_s *r_spans_end = NULL;

static struct gspan_s *gspans = NULL;
static unsigned short poolidx = NULLIDX;
static int display_w = 0;
static int display_h = 0;


static void
Init (int w, int h)
{
	int i, count;

	R_Span_Cleanup ();

	if (w <= 0 || h <= 0)
		return;

	display_w = w;
	display_h = h;

	/* estimate a probable max number of spans per row */
	//TODO: should be tested & tweaked
	count = display_h * 24;

	/* we're using shorts to address gspans in the array so ensure
	 * no overflow */
	if (count > 0xffff)
		count = 0xffff;

	gspans = malloc (count * sizeof(*gspans));

	/* index 0 is used as the NULL value */
	memset (&gspans[NULLIDX], 0, sizeof(gspans[NULLIDX]));

	/* The first handful are reserved as each row's gspan
	 * list head. ie: one element per screen row just for
	 * linked-list management. */
	for (i = 1; i < display_h + 1; i++)
		gspans[i].previdx = gspans[i].nextidx = i;

	/* set up the pool; note that since this is the pool we don't
	 * care about the prev link as it's not needed when popping or
	 * pushing a gspan to the pool */
	poolidx = i;
	while (i < count)
	{
		gspans[i].nextidx = (i == count - 1) ? NULLIDX : i + 1;
		i++;
	}
}


void
R_Span_Cleanup (void)
{
	if (gspans != NULL)
	{
		free (gspans);
		gspans = NULL;
	}
	display_w = 0;
	display_h = 0;
	poolidx = NULLIDX;
	r_spans = NULL;
	r_spans_p = NULL;
	r_spans_end = NULL;
}


static inline void
PushSpan (short y, short x1, short x2)
{
	if (r_spans_p != r_spans_end)
	{
		r_spans_p->u = x1;
		r_spans_p->v = y;
		r_spans_p->len = x2 - x1 + 1;
		r_spans_p++;
	}
}


static inline void
UnlinkGSpan (struct gspan_s *gs)
{
	IDX2PTR(gs->previdx)->nextidx = gs->nextidx;
	IDX2PTR(gs->nextidx)->previdx = gs->previdx;
}


static inline void
FreeGSpan (struct gspan_s *gs)
{
	gs->nextidx = poolidx;
	poolidx = PTR2IDX(gs);
}


/* this assumes the pool isn't empty */
static inline struct gspan_s *
AllocGSpan (void)
{
	struct gspan_s *ret = IDX2PTR(poolidx);
	poolidx = ret->nextidx;
	return ret;
}


static inline void
LinkAfter (struct gspan_s *prev, struct gspan_s *linkin)
{
	linkin->previdx = PTR2IDX(prev);
	linkin->nextidx = prev->nextidx;
	IDX2PTR(linkin->previdx)->nextidx = PTR2IDX(linkin);
	IDX2PTR(linkin->nextidx)->previdx = PTR2IDX(linkin);
}


/* assumed y is on the screen and x2 >= x1 */
int
R_Span_CheckVisible (int y, int x1, int x2)
{
	/* basically do the same as clipandemit, but stop and return
	 * when a fragment is determined to be visible */

	struct gspan_s *head = IDX2PTR(1 + y);
	struct gspan_s *gs;

	gs = IDX2PTR(head->nextidx);
	while (1)
	{
		if (gs == head)
			break;
		else if (gs->right < x1)
			gs = IDX2PTR(gs->nextidx);
		else if (gs->left > x2)
			break;
		else
			return 1;
	}

	return 0;
}


/* assumed y is on the screen and x2 >= x1 */
void
R_Span_ClipAndEmit (int y, int x1, int x2)
{
	struct gspan_s *head = IDX2PTR(1 + y);
	struct gspan_s *gs;

	gs = IDX2PTR(head->nextidx);
	while (1)
	{
		if (gs == head)
			break;
		else if (gs->right < x1)
			gs = IDX2PTR(gs->nextidx);
		else if (gs->left > x2)
			break;
		else if (gs->left < x1)
		{
			/* the gspan hangs off the left of the span,
			 * split the gspan */

			if (poolidx == NULLIDX)
				return;

			struct gspan_s *new = AllocGSpan ();
			new->left = x1;
			new->right = gs->right;
			gs->right = x1 - 1;
			LinkAfter (gs, new);
			gs = new;
		}
		else if (gs->right <= x2)
		{
			/* the gspan lies entirely within the span,
			 * emit a drawable span */

			PushSpan (y, gs->left, gs->right);
			unsigned short nextidx = gs->nextidx;
			UnlinkGSpan (gs);
			FreeGSpan (gs);
			gs = IDX2PTR(nextidx);
		}
		else
		{
			/* the gspan hangs off the right of the span,
			 * split the gspan */

			if (poolidx == NULLIDX)
				return;

			struct gspan_s *new = AllocGSpan ();
			new->left = x2 + 1;
			new->right = gs->right;
			gs->right = x2;
			LinkAfter (gs, new);
			/* stay on gs; next iteration we'll hit the
			 * entirely-within case */
		}
	}
}


void
R_Span_BeginFrame (void *buf, int buflen, int w, int h)
{
	unsigned int cnt;
	int i;

	if (display_w != w || display_h != h)
		Init (w, h);

	/* set up draw spans */
	r_spans = r_spans_p = AlignAllocation (buf, buflen, sizeof(*r_spans), &cnt);
	r_spans_end = r_spans + cnt;

	/* take any gspan still remaining on each row and toss back into
	 * the pool */
	for (i = 1; i < display_h + 1; i++)
	{
		if (gspans[i].nextidx != i)
		{
			/* gspans still remain on this row */
			unsigned short firstidx = gspans[i].nextidx;
			unsigned short lastidx = gspans[i].previdx;
			gspans[lastidx].nextidx = poolidx;
			poolidx = firstidx;
			gspans[i].previdx = gspans[i].nextidx = i;
		}
	}

	/* reset all rows with a gspan covering the entire row */
	for (i = 1; i < display_h + 1; i++)
	{
		struct gspan_s *gs = AllocGSpan ();
		gs->left = 0;
		gs->right = display_w - 1;
		LinkAfter (IDX2PTR(i), gs);
	}
}


/*
 * Draw any remaining gspan on the screen. In normal operation, the
 * screen should be filled by the rendered world so there should never
 * be any gspans visible.
 */
#if 0
void
R_Span_DrawGSpans (void)
{
}
#else
#include "appio.h"
void
R_Span_DrawGSpans (void)
{
	struct gspan_s *head;
	struct gspan_s *next;
	int i, x;
	for (i = 0; i < display_h; i++)
	{
		head = IDX2PTR(1 + i);
		for (next = IDX2PTR(head->nextidx); next != head; next = IDX2PTR(next->nextidx))
		{
			for (x = next->left; x <= next->right; x++)
			{
				if (video.bpp == 16)
					((unsigned short *)video.rows[i])[x] = 0x7e0;
				else
					((unsigned int *)video.rows[i])[x] = 0xff00;
			}
		}
	}
}
#endif
