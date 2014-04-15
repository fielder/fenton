#ifndef __RDATA_H__
#define __RDATA_H__

#include <stdint.h>

typedef uint16_t pixel_t;


#define MIP_NUM_LEVELS 4

struct texlevel_s
{
	int w, h;
	pixel_t *pixels;
	unsigned char *mask; /* uncompressed */
};

struct texture_s
{
	char name[16];
	struct texlevel_s levels[0];
};

extern struct texture_s *
Dat_FreeTexture (struct texture_s *tex);

extern struct texture_s *
Dat_LoadTexture (const char *name);

#endif /* __RDATA_H__ */
