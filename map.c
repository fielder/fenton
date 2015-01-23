#include <stdlib.h>
#include <string.h>

#include "fdata.h"
#include "map.h"

struct map_s map;


void
Map_Unload (void)
{
	if (map.raw != NULL)
	{
		free (map.raw);
		memset (&map, 0, sizeof(map));
	}
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


int
Map_Load (const char *name)
{
	struct map_s newmap;

	//...

	Map_Unload ();

	map = newmap;

	return 1;
}
