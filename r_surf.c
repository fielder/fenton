#include <string.h>
#include <stdint.h>

#include "bdat.h"
#include "appio.h"
#include "map.h"
#include "render.h"

struct drawsurf_s *surfs = NULL;
struct drawsurf_s *surfs_p = NULL;
struct drawsurf_s *surfs_end = NULL;


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
R_Surf_Emit (struct msurface_s *msurf, int firstspanidx, int numspans)
{
	if (surfs_p == surfs_end)
		return;

	surfs_p->firstspanidx = firstspanidx;
	surfs_p->numspans = numspans;
	surfs_p->msurfidx = msurf - map.surfaces;
	//TODO: texturing n' stuff

	surfs_p++;
}


void
R_Surf_DrawAll (void)
{
	struct drawsurf_s *ds;
	for (ds = surfs; ds != surfs_p; ds++)
	{
		struct drawspan_s *s;
		int i;
		for (i = 0, s = &r_spans[ds->firstspanidx]; i < ds->numspans; i++, s++)
		{
			int c = ((uintptr_t)&map.surfaces[ds->msurfidx] >> 4) & 0xffffff;
			DrawSpan (s, c);
		}
	}
}


void
R_Surf_BeginFrame (void *surfbuf, int surfbufsize)
{
	unsigned int cnt;
	surfs_p = surfs = AlignAllocation (surfbuf, surfbufsize, sizeof(*surfs), &cnt);
	surfs_end = surfs + cnt;
}
