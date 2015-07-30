#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "bdat.h"
#include "appio.h"
#include "render.h"

#define PTR2IDX(PTR) ((int)((PTR) - gspans))
#define IDX2PTR(IDX) (gspans + (IDX))

/* green span */
struct gspan_s
{
	unsigned short previdx, nextidx;
	short left, right;
};

/* global, but point to a temp buffer on the stack during refresh */
struct drawspan_s *r_spans = NULL;
static struct drawspan_s *r_spans_end = NULL;

static struct gspan_s *gspans = NULL;
static unsigned short poolidx;
static int display_w, display_h;


void
R_Span_Init (void)
{
	int i, count;

	R_Span_Cleanup ();

	display_w = video.w;
	display_h = video.h;

	/* estimate a probable max number of spans per row */
	//TODO: should be tested & tweaked
	count = display_h * 24;

	/* we're using shorts to address gspans in the array so ensure
	 * no overflow */
	if (count > 0xffff)
		count = 0xffff;

	gspans = malloc (count * sizeof(*gspans));

	/* index 0 is used as the NULL value */
	memset (&gspans[0], 0, sizeof(gspans[0]));

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
		gspans[i].nextidx = (i == count - 1) ? 0 : i + 1;
		i++;
	}
}


void
R_Span_Cleanup (void)
{
	if (gspans != NULL)
		free (gspans);
	gspans = NULL;
	display_w = 0;
	display_h = 0;
	poolidx = 0;
	r_spans = NULL;
	r_spans_end = NULL;
}


static inline void
PushSpan (short y, short x1, short x2)
{
	if (r_spans != r_spans_end)
	{
		r_spans->u = x1;
		r_spans->v = y;
		r_spans->len = x2 - x1 + 1;
		r_spans++;
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
AllocGSpan ()
{
	struct gspan_s *ret = IDX2PTR(poolidx);
	poolidx = ret->nextidx;
	return ret;
}


static void
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

			if (poolidx == 0)
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
			int next = gs->nextidx;
			UnlinkGSpan (gs);
			FreeGSpan (gs);
			gs = IDX2PTR(next);
		}
		else
		{
			/* the gspan hangs off the right of the span,
			 * split the gspan */

			if (poolidx == 0)
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
R_Span_BeginFrame (void *buf, int buflen)
{
	/* set up draw spans */
	unsigned int cnt;
	r_spans = AlignAllocation (buf, buflen, sizeof(*r_spans), &cnt);
	r_spans_end = r_spans + cnt;

	/* now gspans */
	int i;
	for (i = 0; i < display_h; i++)
	{
		struct gspan_s *head = IDX2PTR(1 + i);
		struct gspan_s *next;

		/* take any gspan still remaining on the row and toss
		 * back into the pool */
		while ((next = IDX2PTR(head->nextidx)) != head)
		{
			UnlinkGSpan (next);
			FreeGSpan (next);
		}

		/* reset the entire row with 1 fresh gspan */

		struct gspan_s *gs = AllocGSpan ();
		gs->left = 0;
		gs->right = display_w - 1;
		gs->previdx = gs->nextidx = PTR2IDX(head);
		head->previdx = head->nextidx = PTR2IDX(gs);
	}
}


/*
 * Draw any remaining gspan on the screen. In normal operation, the
 * screen should be filled by the rendered world so there should never
 * be any gspans visible.
 */
void
R_Span_DrawGSpans (void)
{
#if 0
	const struct gspan_s *gs;
	int i;

	for (i = 0; i < r_vars.h; i++)
	{
		for (gs = r_gspans[i].next; gs != &r_gspans[i]; gs = gs->next)
		{
			memset (r_vars.screen + i * r_vars.pitch + gs->left,
				251,
				gs->right - gs->left + 1);
		}
	}
#endif
}
