#include <stdlib.h>
#include <string.h>

#include "bdat.h"
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
	//...

	memset (m, 0, sizeof(*m));
}


void
Map_Unload (void)
{
	FreeMap (&map);
}

/* on-disk structures */

struct dplane_s
{
	double normal[3];
	double dist;
};

struct dvert_s
{
	double xyz[3];
};

struct dedge_s
{
	int v[2];
};

struct dsurf_s
{
	int blah;
};

struct dportal_s
{
	int blah;
};

struct dnode_s
{
	int blah;
};

struct dleaf_s
{
	int blah;
};


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
LoadPlanes (struct map_s *m)
{
	unsigned int sz
	struct dplane_s *dp;
	return 1;
}


static int
LoadVertices (struct map_s *m)
{
	return 1;
}


static int
LoadEdges (struct map_s *m)
{
	return 1;
}


int
Map_Load (const char *name)
{
	struct map_s newmap;

	memset (&newmap, 0, sizeof(newmap));

	if (!LoadPlanes(&newmap))
		goto error;
	if (!LoadVertices(&newmap))
		goto error;
	if (!LoadEdges(&newmap))
		goto error;
	//...

	Map_Unload ();

	map = newmap;

	return 1;

error:
	FreeMap (&newmap);

	return 0;
}
