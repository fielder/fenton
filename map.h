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
	int v[2];
};

#if 0

struct msurface_s
{
	/* texture-space vecs */
	double texorg[3];
	int texvecs;

	int plane;
	int firstedge; /* in surfaceedges */
	short numedges;
	short is_backside;

	int texnum;

	char align_padding[4]; /* to 48 bytes */
};

struct mtexvecs_s
{
	double texvec_s[3];
	double texvec_t[3];
};

/* portals are always stored on the front of the plane */
struct mportal_s
{
	int plane;
	int firstedge;
	short numedges;
	char align_padding[2]; /* to 12 bytes */
};

/* nodes are always stored on the front of the plane */
struct mnode_s
{
	int mins[3];
	int maxs[3];
	void *children[2];
	int plane;
	int firstsurf_front;
	int firstsurf_back;
	int firstportal;
	short numsurfs_front;
	short numsurfs_back;
	short numportals;
//	char align_padding[6]; /* to 72 bytes */
};

struct mleaf_s
{
	int mins[3];
	int maxs[3];
	int firstsurf;
	short numsurfs;
	char align_padding[2]; /* to 32 bytes */
};
#endif

struct map_s
{
	char *name;

	struct mplane_s *planes;
	int num_planes;

	struct mvertex_s *vertices;
	int num_vertices;

	struct medge_s *edges;
	int num_edges;

//	struct msurface_s *surfaces;
//	int num_surfaces;

//	int *surfaceedges;
//	int num_surfaceedges;

//	struct mportal_s *portals;
//	int num_portals;

//	struct mnode_s *nodes;
//	int num_nodes;

//	struct mleaf_s *leafs;
//	int num_leafs;

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
