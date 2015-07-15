//#include <string.h>
#include <stdint.h>
#include <stdlib.h>

//#include "r_defs.h"
#include "bdat.h"
#include "appio.h"
#include "render.h"

/* green span */
#define NULLGSPAN 0xffff
#define PTR2IDX(PTR) ((int)((PTR) - r_gspans))
#define IDX2PTR(IDX) (r_gspans + (IDX))
struct gspan_s
{
	unsigned short previdx, nextidx;
	short left, right;
};

/* globals, but point to a temp buffer on the stack during refresh */
struct drawspan_s *r_spans = NULL;
static struct drawspan_s *r_spans_end = NULL;

static struct gspan_s *r_gspans = NULL;
static struct gspan_s *r_gspans_pool = NULL;

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

	/* we're using integers rather than pointers, ensure our 'null'
	 * isn't used */
	if (count > NULLGSPAN)
		count = NULLGSPAN;

	r_gspans = malloc (count * sizeof(*r_gspans));

	/* The first handful are reserved as each row's gspan
	 * list head. ie: one element per screen row just for
	 * linked-list management. */
	for (i = 0; i < display_h; i++)
		r_gspans[i].previdx = r_gspans[i].nextidx = i;

	/* set up the pool; note that since this is the pool we don't
	 * care about the prev link as it's not needed when popping or
	 * pushing a gspan to the pool */
	r_gspans_pool = r_gspans + i;
	while (i < count)
	{
		r_gspans[i].nextidx = (i == count - 1) ? NULLGSPAN : i + 1;
		i++;
	}
}


void
R_Span_Cleanup (void)
{
	if (r_gspans != NULL)
		free (r_gspans);
	r_gspans = NULL;
	r_gspans_pool = NULL;
	r_spans = NULL;
	r_spans_end = NULL;
}


static void
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
	gs->nextidx = PTR2IDX(r_gspans_pool);
	r_gspans_pool = gs;
}


//FIXME: need to map 0xffff to NULL...
static inline struct gspan_s *
AllocGSpan ()
{
	struct gspan_s *ret = r_gspans_pool;
	r_gspans_pool = IDX2PTR(r_gspans_pool->nextidx);
	return ret;
}


void
R_Span_ClipAndEmit (int y, int x1, int x2)
{
#if 0
	struct gspan_s *head = &r_gspans[y];
	struct gspan_s *gs, *next, *new;

	gs = head->next;
	while (1)
	{
		if (gs == head)
			break;
		else if (gs->right < x1)
			gs = gs->next;
		else if (gs->left > x2)
			break;
		else if (gs->left < x1)
		{
			/* the gspan hangs off the left of the span,
			 * split the gspan */

			if (r_gspans_pool == NULL)
				return;

			new = r_gspans_pool;
			r_gspans_pool = r_gspans_pool->next;
			new->left = x1;
			new->right = gs->right;
			gs->right = x1 - 1;
			new->prev = gs;
			new->next = gs->next;
			new->prev->next = new;
			new->next->prev = new;
			gs = new;
		}
		else if (gs->right <= x2)
		{
			/* the gspan lies entirely within the span,
			 * emit a drawable span */
			PushSpan (y, gs->left, gs->right);
			next = gs->next;
			gs->prev->next = gs->next;
			gs->next->prev = gs->prev;
			gs->next = r_gspans_pool;
			r_gspans_pool = gs;
			gs = next;
		}
		else
		{
			/* the gspan hangs off the right of the span,
			 * split the gspan */

			if (r_gspans_pool == NULL)
				return;

			new = r_gspans_pool;
			r_gspans_pool = r_gspans_pool->next;
			new->left = x2 + 1;
			new->right = gs->right;
			gs->right = x2;
			new->prev = gs;
			new->next = gs->next;
			new->prev->next = new;
			new->next->prev = new;
		}
	}
#endif
}


void
R_Span_BeginFrame (void *buf, int buflen)
{
	unsigned int cnt;

	r_spans = AlignAllocation (buf, buflen, sizeof(*r_spans), &cnt);
	r_spans_end = r_spans + cnt;

	/* now gspans */
	struct gspan_s *head;
	int i;

	for (i = 0, head = r_gspans; i < display_h; i++, head++)
	{
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
