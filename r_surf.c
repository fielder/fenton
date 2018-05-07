#include <stdint.h>

#include "appio.h"
#include "map.h"
#include "render.h"


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
R_DrawSurfs (void)
{
	struct drawsurf_s *ds;
	for (ds = surfs; ds != surfs_p; ds++)
	{
		struct drawspan_s *s;
		int i;
		for (i = 0, s = ds->spans; i < ds->numspans; i++, s++)
		{
			int c = ((uintptr_t)&map.surfaces[ds->msurfidx] >> 4) & 0xffffff;
			DrawSpan (s, c);
		}
	}
}
