#include <stdio.h>
#include <stdint.h>

#include "map.h"
#include "render.h"

struct drawsurf_s
{
	unsigned int surfnum;
	char blah[28];
#if 0
	struct drawedge_s *edges[2]; /* left/right on the screen */

	struct drawspan_s *spans;
	int num_spans;

	struct mpoly_s *mpoly;

	//TODO: texture mapping stuff
#endif
};

struct drawedge_s
{
#if 0
	struct medge_s *owner;
	struct drawedge_s *next; /* in v-sorted list of edges for the poly */
#endif

	int top, bottom;
	int u, du; /* 12.20 fixed-point format */

#if 0
	int is_right;
#endif
};

static struct drawsurf_s *surfs = NULL;
static struct drawsurf_s *surfs_end = NULL;

static struct drawedge_s *edges = NULL;
static struct drawedge_s *edges_end = NULL;


void
R_GenSpansForSurfaces (unsigned int first, int count, int cplanes[4], int numcplanes)
{
	// stage 1: emit drawpolys and drawedges for those drawpolys
	// stage 2: scan over the emitted drawpolys, emitting spans
}


int
R_CheckPortalVisibility (struct mportal_s *portal, int reversewinding)
{
	// run through the edges, emitting drawedges
	// scan over the emitted drawedges; if a span is visible return true
	// the only side-effect of this function will be emitted drawedges
	return 0;
}


void
R_Surf_BeginFrame (void *surfbuf, int surfbufsize, void *edgebuf, int edgebufsize)
{
	const int cachelinesize = 32;
	uintptr_t start, end;

	/* tweak the buffers to align on cacheline boundaries */

	/* surfaces */
	start = (uintptr_t)surfbuf;
	end = start + surfbufsize;
	start = (start + cachelinesize - 1) - ((start + cachelinesize - 1) % cachelinesize);
	end -= (end - start) % sizeof(struct drawsurf_s);
	surfs = (struct drawsurf_s *)start;
	surfs_end = (struct drawsurf_s *)end;

	/* edges */
	start = (uintptr_t)edgebuf;
	end = start + edgebufsize;
	start = (start + cachelinesize - 1) - ((start + cachelinesize - 1) % cachelinesize);
	end -= (end - start) % sizeof(struct drawedge_s);
	edges = (struct drawedge_s *)start;
	edges_end = (struct drawedge_s *)end;
}
