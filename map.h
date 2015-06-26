#ifndef __MAP_H__
#define __MAP_H__

enum
{
	NORMAL_X,	/*  1  0  0 */
	NORMAL_NEGX,	/* -1  0  0 */
	NORMAL_Y,	/*  0  1  0 */
	NORMAL_NEGY,	/*  0 -1  0 */
	NORMAL_Z,	/*  0  0  1 */
	NORMAL_NEGZ,	/*  0  0 -1 */
	NORMAL_0,	/* +x +y +z */
	NORMAL_1,	/* -x +y +z */
	NORMAL_2,	/* +x -y +z */
	NORMAL_3,	/* -x -y +z */
	NORMAL_4,	/* +x +y -z */
	NORMAL_5,	/* -x +y -z */
	NORMAL_6,	/* +x -y -z */
	NORMAL_7,	/* -x -y -z */
};

struct mplane_s
{
	double normal[3];
	double dist;
	int type;
	char align_padding[4]; /* to 40 bytes */
};

struct mvertex_s
{
	double xyz[3];
};

struct medge_s
{
	unsigned int v[2];
	unsigned int cachenum;
};

struct msurface_s
{
	/* texture-space vecs */
	//double texorg[3];
	//int texvecs;

	unsigned int plane;
	unsigned int edgeloop_start;
	unsigned short numedges;
	short is_backside;

	unsigned int color;//texnum;

	//char align_padding[4]; /* to 48 bytes */
};

#if 0
struct mtexvecs_s
{
	double texvec_s[3];
	double texvec_t[3];
};
#endif

/* portals are always stored on the front of the node plane */
struct mportal_s
{
	unsigned int edgeloop_start;
	unsigned short numedges;
	char align_padding[2]; /* to 8 bytes */
};

#define NODEFL_LEAF (1<<15)

/* nodes are always stored on the front of the plane */
struct mnode_s
{
	int mins[3];
	int maxs[3];
	unsigned short flags;
	short pad;
	unsigned int plane;
	void *children[2];
	unsigned int front_firstsurface;
	unsigned int back_firstsurface;
	unsigned int firstportal;
	unsigned short front_numsurfs;
	unsigned short back_numsurfs;
	unsigned short numportals;
	char align_padding[6]; /* to 72 bytes */
};

struct mleaf_s
{
	int mins[3];
	int maxs[3];
	unsigned short flags;
	unsigned short numsurfaces;
	unsigned int firstsurface;
};

struct map_s
{
	char *name;

	int allocsz;

	struct mplane_s *planes;
	int num_planes;

	struct mvertex_s *vertices;
	int num_vertices;

	struct medge_s *edges;
	int num_edges;

	int *edgeloops;
	int num_loopedges;

	struct msurface_s *surfaces;
	int num_surfaces;

	struct mportal_s *portals;
	int num_portals;

	struct mnode_s *nodes;
	int num_nodes;

	struct mleaf_s *leafs;
	int num_leafs;

//	struct mtexvecs_s *texvecs;
//	int num_texvecs;

	//TODO: textures
};

extern struct map_s map;

extern void
Map_Unload (void);

extern int
Map_Load (const char *name);

#endif /* __MAP_H__ */
