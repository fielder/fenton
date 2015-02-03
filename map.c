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
	int sz, cnt, i;
	void *dplanes;
	struct mplane_s *out;
	double *in;

	/* plane on disk:
	 * double normal[3]
	 * double dist
	 */

	if ((dplanes = Get("planes", &sz)) == NULL)
		return 0;
	cnt = sz / (4 * sizeof(double));

	loadmap.planes = malloc(cnt * sizeof(*out));
	loadmap.num_planes = cnt;

	for (	i = 0, in = dplanes, out = loadmap.planes;
		i < cnt;
		i++, in += 4, out++ )
	{
		out->normal[0] = GetDouble (&in[0]);
		out->normal[1] = GetDouble (&in[1]);
		out->normal[2] = GetDouble (&in[2]);
		out->dist = GetDouble (&in[3]);
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
	 * unsigned int v[2]
	 */

	if ((loadmap.edges = Get("edges", &sz)) == NULL)
		return 0;
	loadmap.num_edges = sz / (2 * sizeof(unsigned int));

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
	//...
	return 1;
}


static int
LoadPortals (void)
{
	int sz, cnt, i, insz;
	void *dportals;
	struct mportal_s *out;
	char *in;

	/* portal on disk:
	 * unsigned int edgeloop_start;
	 * unsigned short numedges;
	 */
	insz = sizeof(unsigned int) + sizeof(unsigned short);

	if ((dportals = Get("portals", &sz)) == NULL)
		return 0;
	cnt = sz / insz;

	loadmap.portals = malloc(cnt * sizeof(*out));
	loadmap.num_portals = cnt;
	for (	i = 0, in = dportals, out = loadmap.portals;
		i < cnt;
		i++, in += insz, out++ )
	{
		out->edgeloop_start = GetInt (in);
		out->numedges = GetShort (in + 4);
	}

	free (dportals);

	loadmap.allocsz += cnt * sizeof(*out);

	return 1;
}


static int
LoadNodes (void)
{
	//...
	return 1;
}


static int
LoadLeafs (void)
{
	int sz, cnt, i, insz;
	void *dleafs;
	struct mleaf_s *out;
	char *in;

	/* leaf on disk:
	 * int mins[3]
	 * int maxs[3]
	 * unsigned int firstsurface
	 * unsigned short numsurfaces
	 */
	insz = (6 * sizeof(int)) + sizeof(unsigned int) + sizeof(unsigned short);

	if ((dleafs = Get("leafs", &sz)) == NULL)
		return 0;
	cnt = sz / insz;

	loadmap.leafs = malloc(cnt * sizeof(*out));
	loadmap.num_leafs = cnt;

	for (	i = 0, in = dleafs, out = loadmap.leafs;
		i < cnt;
		i++, in += insz, out++ )
	{
		out->mins[0] = GetInt (in + 0);
		out->mins[1] = GetInt (in + 4);
		out->mins[2] = GetInt (in + 8);
		out->maxs[0] = GetInt (in + 12);
		out->maxs[1] = GetInt (in + 16);
		out->maxs[2] = GetInt (in + 20);
		out->firstsurface = GetInt (in + 24);
		out->numsurfaces = GetShort (in + 28);
	}

	free (dleafs);

	loadmap.allocsz += cnt * sizeof(*out);

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
