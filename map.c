#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "bdat.h"
#include "pak.h"
#include "fdata.h"
#include "map.h"

struct map_s map;


static void
FreeMap (struct map_s *m)
{
	if (m->planes != NULL)
		free (m->planes);
	if (m->vertices != NULL)
		free (m->vertices);
	if (m->edges != NULL)
		free (m->edges);
	if (m->edgeloops != NULL)
		free (m->edgeloops);
	if (m->surfaces != NULL)
		free (m->surfaces);
	if (m->portals != NULL)
		free (m->portals);
	if (m->nodes != NULL)
		free (m->nodes);
	if (m->leafs != NULL)
		free (m->leafs);

	memset (m, 0, sizeof(*m));
}


void
Map_Unload (void)
{
	FreeMap (&map);
}


static struct map_s loadmap;
static struct pak_s *loadpak;
static char *mapdir;


static void *
Get (const char *name, int *size)
{
	void *ret;

	if (loadpak != NULL)
	{
		ret = Pak_ReadEntry (loadpak, name, size);
	}
	else
	{
		/* make the filename: "MAPNAME/vertices" */
		int i = strlen(mapdir);
		mapdir[i] = '/';
		strcpy (mapdir + i + 1, name);
		ret = Data_ReadFile (mapdir, size);
		/* undo filename modification */
		mapdir[i] = '\0';
	}

	return ret;
}


static int
PlaneType (const double normal[3])
{
	if (normal[0] == 1)
		return NORMAL_X;
	else if (normal[0] == -1)
		return NORMAL_NEGX;
	else if (normal[1] == 1)
		return NORMAL_Y;
	else if (normal[1] == -1)
		return NORMAL_NEGY;
	else if (normal[2] == 1)
		return NORMAL_Z;
	else if (normal[2] == -1)
		return NORMAL_NEGZ;
	else
	{
		return	NORMAL_0 +
			((normal[0] < 0) << 0) +
			((normal[1] < 0) << 1) +
			((normal[2] < 0) << 2);
	}
	return 0;
}


struct dplane_s
{
	double normal[3];
	double dist;
} __attribute__ ((packed));

static int
LoadPlanes (void)
{
	int sz, cnt;
	struct dplane_s *dplanes, *in;
	struct mplane_s *out;

	if ((dplanes = Get("planes", &sz)) == NULL)
		return 0;
	cnt = sz / sizeof(*in);

	loadmap.planes = malloc(cnt * sizeof(*out));
	loadmap.allocsz += cnt * sizeof(*out);
	loadmap.num_planes = cnt;

	for (	in = dplanes, out = loadmap.planes;
		cnt > 0;
		cnt--, in++, out++ )
	{
		out->normal[0] = GetDouble (&in->normal[0]);
		out->normal[1] = GetDouble (&in->normal[1]);
		out->normal[2] = GetDouble (&in->normal[2]);
		out->dist = GetDouble (&in->dist);
		out->type = PlaneType (out->normal);
	}

	free (dplanes);

	return 1;
}


static int
LoadVertices (void)
{
	int sz, i;

	/* vertex on disk is the same as what we keep in memory:
	 * double xyz[3]
	 */

	if ((loadmap.vertices = Get("vertices", &sz)) == NULL)
		return 0;
	loadmap.allocsz += sz;
	loadmap.num_vertices = sz / sizeof(*loadmap.vertices);

	for (i = 0; i < loadmap.num_vertices; i++)
	{
		loadmap.vertices[i].xyz[0] = GetDouble (&loadmap.vertices[i].xyz[0]);
		loadmap.vertices[i].xyz[1] = GetDouble (&loadmap.vertices[i].xyz[1]);
		loadmap.vertices[i].xyz[2] = GetDouble (&loadmap.vertices[i].xyz[2]);
	}

	return 1;
}


struct dedge_s
{
	unsigned int v[2];
} __attribute__ ((packed));

static int
LoadEdges (void)
{
	int sz, cnt;
	struct dedge_s *dedges, *in;
	struct medge_s *out;

	if ((dedges = Get("edges", &sz)) == NULL)
		return 0;
	cnt = sz / sizeof(*in);

	loadmap.edges = malloc(cnt * sizeof(*out));
	loadmap.allocsz += cnt * sizeof(*out);
	loadmap.num_edges = cnt;

	for (	in = dedges, out = loadmap.edges;
		cnt > 0;
		cnt--, in++, out++ )
	{
		out->v[0] = GetInt (&in->v[0]);
		out->v[1] = GetInt (&in->v[1]);
		out->cachenum = 0;
	}

	free (dedges);

	return 1;
}


static int
LoadEdgeLoops (void)
{
	int sz, i;

	/* edgeloops on disk are just int's */

	if ((loadmap.edgeloops = Get("edgeloops", &sz)) == NULL)
		return 0;
	loadmap.allocsz += sz;
	loadmap.num_loopedges = sz / sizeof(*loadmap.edgeloops);

	for (i = 0; i < loadmap.num_loopedges; i++)
		loadmap.edgeloops[i] = GetInt (&loadmap.edgeloops[i]);

	return 1;
}


struct dsurface_s
{
	unsigned int planenum;
	unsigned short is_backside;
	unsigned int edgeloop_start;
	unsigned short numedges;
} __attribute__ ((packed));

static int
LoadSurfaces (void)
{
	int sz, cnt;
	struct dsurface_s *dsurfaces, *in;
	struct msurface_s *out;

	if ((dsurfaces = Get("surfaces", &sz)) == NULL)
		return 0;
	cnt = sz / sizeof(*in);

	loadmap.surfaces = malloc(cnt * sizeof(*out));
	loadmap.allocsz += cnt * sizeof(*out);
	loadmap.num_surfaces = cnt;

	for (	in = dsurfaces, out = loadmap.surfaces;
		cnt > 0;
		cnt--, in++, out++ )
	{
		out->plane = GetInt (&in->planenum);
		out->is_backside = GetShort (&in->is_backside);
		out->edgeloop_start = GetInt (&in->edgeloop_start);
		out->numedges = GetShort (&in->numedges);
		out->color = ((uintptr_t)out >> 4) & 0x00ffffff;
	}

	free (dsurfaces);

	return 1;
}


struct dportal_s
{
	unsigned int edgeloop_start;
	unsigned short numedges;
} __attribute__ ((packed));

static int
LoadPortals (void)
{
	int sz, cnt;
	struct dportal_s *dportals, *in;
	struct mportal_s *out;

	if ((dportals = Get("portals", &sz)) == NULL)
		return 0;
	cnt = sz / sizeof(*in);

	loadmap.portals = malloc(cnt * sizeof(*out));
	loadmap.allocsz += cnt * sizeof(*out);
	loadmap.num_portals = cnt;

	for (	in = dportals, out = loadmap.portals;
		cnt > 0;
		cnt--, in++, out++ )
	{
		out->edgeloop_start = GetInt (&in->edgeloop_start);
		out->numedges = GetShort (&in->numedges);
	}

	free (dportals);

	return 1;
}


static int
LoadNodes (void)
{
	//...
	return 1;
}


struct dleaf_s
{
	int mins[3];
	int maxs[3];
	unsigned int firstsurface;
	unsigned short numsurfaces;
} __attribute__ ((packed));

static int
LoadLeafs (void)
{
	int sz, cnt;
	struct dleaf_s *dleafs, *in;
	struct mleaf_s *out;

	if ((dleafs = Get("leafs", &sz)) == NULL)
		return 0;
	cnt = sz / sizeof(*in);

	loadmap.leafs = malloc(cnt * sizeof(*out));
	loadmap.allocsz += cnt * sizeof(*out);
	loadmap.num_leafs = cnt;

	for (	in = dleafs, out = loadmap.leafs;
		cnt > 0;
		cnt--, in++, out++ )
	{
		out->mins[0] = GetInt (&in->mins[0]);
		out->mins[1] = GetInt (&in->mins[1]);
		out->mins[2] = GetInt (&in->mins[2]);
		out->maxs[0] = GetInt (&in->maxs[0]);
		out->maxs[1] = GetInt (&in->maxs[1]);
		out->maxs[2] = GetInt (&in->maxs[2]);
		out->flags = NODEFL_LEAF;
		out->firstsurface = GetInt (&in->firstsurface);
		out->numsurfaces = GetShort (&in->numsurfaces);
	}

	free (dleafs);

	return 1;
}


static int
LoadTextures (void)
{
	//...
	return 1;
}


int
Map_Load (const char *name)
{
	int direxists;
	char path[1024];

	/* save room on end for filenames, "vertices", "nodes", etc */
	if (strlen(name) > sizeof(path) - 100)
		return 0;

	if (Data_IsDir(name, &direxists) && direxists)
	{
		/* first, try a map in the filesystem */

		strcpy (path, name);
		mapdir = path;

		loadpak = NULL;
	}
	else
	{
		/* no directory with the map name; try a pak */

		strcpy (path, name);
		strcpy (path + strlen(path), ".pak");

		if ((loadpak = Pak_Open(path)) == NULL)
			return 0;

		mapdir = NULL;
	}

	if (!LoadPlanes())
		goto error;
	if (!LoadVertices())
		goto error;
	if (!LoadEdges())
		goto error;
	if (!LoadEdgeLoops())
		goto error;
	if (!LoadSurfaces())
		goto error;
	if (!LoadPortals())
		goto error;
	if (!LoadNodes())
		goto error;
	if (!LoadLeafs())
		goto error;
	if (!LoadTextures())
		goto error;

	if (loadpak != NULL)
		loadpak = Pak_Close (loadpak);
	mapdir = NULL;

	/* success; get rid of current map & switch over */

	Map_Unload ();
	map = loadmap;
	memset (&loadmap, 0, sizeof(loadmap));

	return 1;

error:
	/* failure; keep the currently-loaded map running */

	if (loadpak != NULL)
		loadpak = Pak_Close (loadpak);
	mapdir = NULL;

	FreeMap (&loadmap);

	return 0;
}