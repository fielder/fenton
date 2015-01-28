#include <stdlib.h>
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
	if (m->portals != NULL)
		free (m->portals);
	if (m->leafs != NULL)
		free (m->leafs);
	//...

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


static int
LoadPlanes (void)
{
#if 0
struct dplane_s
{
	double normal[3];
	double dist;
};
#endif
#if 0
	int sz, cnt, i;
	struct dplane_s *dplanes, *in;
	struct mplane_s *out;

	/* plane on disk:
	 * double normal[3]
	 * double dist
	 */

	if ((dplanes = Get("planes", &sz)) == NULL)
		return 0;
	cnt = sz / sizeof(*in);

	loadmap.planes = malloc(cnt * sizeof(*out));
	loadmap.num_planes = cnt;
	for (	i = 0, in = dplanes, out = loadmap.planes;
		i < cnt;
		i++, in++, out++)
	{
		out->normal[0] = GetDouble (&in->normal[0]);
		out->normal[1] = GetDouble (&in->normal[1]);
		out->normal[2] = GetDouble (&in->normal[2]);
		out->dist = GetDouble (&in->dist);
		out->type = PlaneType (out->normal);
	}

	free (dplanes);

	loadmap.allocsz += cnt * sizeof(*out);
#endif
	int sz, cnt, i;
	void *dplanes;
	struct mplane_s *out;

	/* plane on disk:
	 * double normal[3]
	 * double dist
	 */

	if ((dplanes = Get("planes", &sz)) == NULL)
		return 0;
	cnt = sz / (4 * sizeof(double));

	loadmap.planes = malloc(cnt * sizeof(*out));
	loadmap.num_planes = cnt;
	for (	i = 0, out = loadmap.planes;
		i < cnt;
		i++, out++ )
	{
		//...
		out->type = PlaneType (out->normal);
	}

	free (dplanes);

	loadmap.allocsz += cnt * sizeof(*out);

	return 1;
}


static int
LoadVertices (void)
{
	int sz, i;

	/* vertex on disk:
	 * double xyz[3]
	 */

	if ((loadmap.vertices = Get("vertices", &sz)) == NULL)
		return 0;
	loadmap.num_vertices = sz / (3 * sizeof(double));

	for (i = 0; i < loadmap.num_vertices; i++)
	{
		loadmap.vertices[i].xyz[0] = GetDouble (&loadmap.vertices[i].xyz[0]);
		loadmap.vertices[i].xyz[1] = GetDouble (&loadmap.vertices[i].xyz[1]);
		loadmap.vertices[i].xyz[2] = GetDouble (&loadmap.vertices[i].xyz[2]);
	}

	loadmap.allocsz += sz;

	return 1;
}


static int
LoadEdges (void)
{
	int sz, i;

	/* edge on disk:
	 * int v[2]
	 */

	if ((loadmap.edges = Get("edges", &sz)) == NULL)
		return 0;
	loadmap.num_edges = sz / (2 * sizeof(int));

	for (i = 0; i < loadmap.num_edges; i++)
	{
		loadmap.edges[i].v[0] = GetInt (&loadmap.edges[i].v[0]);
		loadmap.edges[i].v[1] = GetInt (&loadmap.edges[i].v[1]);
	}

	loadmap.allocsz += sz;

	return 1;
}


static int
LoadEdgeLoops (void)
{
	int sz, i;

#if 1
return 1;
#endif

	/* edgeloops on disk are just int's */

	if ((loadmap.edgeloops = Get("edgeloops", &sz)) == NULL)
		return 0;
	loadmap.num_loopedges = sz / sizeof(*loadmap.edgeloops);

	for (i = 0; i < loadmap.num_loopedges; i++)
		loadmap.edgeloops[i] = GetInt (&loadmap.edgeloops[i]);

	loadmap.allocsz += sz;

	return 1;
}


static int
LoadSurfaces (void)
{
	return 1;
}


static int
LoadPortals (void)
{
#if 0
/* unaligned, so we don't use the struct def directly */
struct dportal_s
{
	int firstedge;
	short numedges;
};
#endif
	return 1;
}


static int
LoadNodes (void)
{
	return 1;
}


static int
LoadLeafs (void)
{
#if 0
/* unaligned, so we don't use the struct def directly */
struct dleaf_s
{
	int mins[3];
	int maxs[3];
	int firstsurface;
	short numsurfaces;
};
#endif
	return 1;
}


static int
LoadTextures (void)
{
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
	//...
	//...
	//...

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
