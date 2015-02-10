//#include <string.h>
#include <stdint.h>
#include <stdlib.h>

//#include "r_defs.h"
#include "appio.h"
#include "render.h"

/* green span */
struct gspan_s
{
	unsigned short previdx, nextidx;
	short left, right;
};

struct drawspan_s *r_spans = NULL;
static struct drawspan_s *r_spans_end = NULL;

static struct gspan_s *r_gspans = NULL;
static struct gspan_s *r_gspans_pool = NULL;


void
R_Span_Init (void)
{
	int i, count;

	R_Span_Cleanup ();

	/* estimate a probable max number of spans per row */
	//TODO: should be tested & tweaked
	count = video.h * 24;

	/* we're using integers rather than pointers, ensure our 'null'
	 * isn't used (0xffff) */
	if (count > 0xffff)
		count = 0xffff;

	r_gspans = malloc (count * sizeof(*r_gspans));

	/* The first handful are reserved as each row's gspan
	 * list head. ie: one element per screen row just for
	 * linked-list management. */
	for (i = 0; i < video.h; i++)
		r_gspans[i].previdx = r_gspans[i].nextidx = i;

	/* set up the pool */
	r_gspans_pool = r_gspans + i;
	while (i < count)
	{
		r_gspans[i].nextidx = (i == count - 1) ? 0xffff : i + 1;
		i++;
	}
}


void
R_Span_Cleanup (void)
{
	if (r_gspans != NULL)
	{
		free (r_gspans);
		r_gspans = NULL;
		r_gspans_pool = NULL;
	}
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
	uintptr_t start, end;

	start = (uintptr_t)surfbuf;
	end = start + surfbuflen;
//	start = (start + cachelinesize - 1) - ((start + cachelinesize - 1) % cachelinesize);
//	end -= (end - start) % sizeof(struct drawsurf_s);
//	surfs = (struct drawsurf_s *)start;
//	surfs_end = (struct drawsurf_s *)end;
#if 0
	/* prepare the given span buffer */
	uintptr_t p = (uintptr_t)buf;

	while ((p % sizeof(struct drawspan_s)) != 0)
	{
		p++;
		buflen--;
	}

	r_spans = (struct drawspan_s *)p;
	r_spans_end = r_spans + (buflen / sizeof(struct drawspan_s));
#endif

	/* now gspans */
	struct gspan_s *gs, *head, *next;
	int i;

	for (i = 0, head = r_gspans; i < video.h; i++, head++)
	{
		/* take any gspan still remaining on the row and toss
		 * back into the pool */
#if 0
		while ((next = head->next) != head)
		{
			head->next = next->next;
			next->next = r_gspans_pool;
			r_gspans_pool = next;
		}

		/* reset the row with 1 fresh gspan */

		gs = r_gspans_pool;
		r_gspans_pool = gs->next;

		gs->left = 0;
		gs->right = video.w - 1;
		gs->prev = gs->next = head;
		head->prev = head->next = gs;
#endif
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
