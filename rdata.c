//#include <stdint.h>
#include <stdlib.h>
//#include <string.h>

//#include "fenton.h"
//#include "pak.h"
//#include "rdata.h"

#define TEXTURE_VERSION 2

struct dtextureheader_s
{
	char id[8];
	unsigned int version;
	char wadname[8];
	unsigned int width;
	unsigned int height;
	unsigned int original_width;
	unsigned int original_height;
	unsigned int masked;
	unsigned int numlevels;
};


#if 0
static unsigned int
getUInt (const void *ptr)
{
	const unsigned char *bytes = ptr;
	return	((unsigned int)bytes[0] << 0) | ((unsigned int)bytes[1] << 8) |
		((unsigned int)bytes[2] << 16) | ((unsigned int)bytes[3] << 24);
}


static pixel_t
RGB24to16 (const unsigned char *rgb)
{
	return (((pixel_t)rgb[0] << 8) & 0xf800) |
		(((pixel_t)rgb[1] << 3) & 0x07e0) |
		(((pixel_t)rgb[2] >> 3) & 0x001f);
}
#endif


struct texture_s *
Dat_FreeTexture (struct texture_s *tex)
{
	if (tex != NULL)
		free (tex);
	return NULL;
}


#if 0

//FIXME: mask data should really be aligned by 2 or 4 so to misalign
//	off following multi-byte pixels

static struct texture_s *
ParseTexture (const uint8_t *bytes, int sz)
{
	struct texture_s *ret;
	struct dtextureheader_s hdr;
	const uint8_t *mipdat;
	uint8_t *dest;
	int off, w, h, allocsz;
	int numlevels;
	int i, j;

#define MAX_LEVELS 16

	int widths[MAX_LEVELS];
	int heights[MAX_LEVELS];
	int pixelofs[MAX_LEVELS];
	int maskofs[MAX_LEVELS];

	if (sz <= sizeof(hdr))
		return NULL;

//	memcpy (&hdr.id, bytes + 0, sizeof(hdr.id));
//	memcpy (&hdr.version, bytes + 8, sizeof(hdr.version));
//	memcpy (&hdr.wadname, bytes + 10, sizeof(hdr.wadname));
//	memcpy (&hdr.width, bytes + 18, sizeof(hdr.width));
//	memcpy (&hdr.height, bytes + 20, sizeof(hdr.height));
//	memcpy (&hdr.original_width, bytes + 22, sizeof(hdr.original_width));
//	memcpy (&hdr.original_height, bytes + 24, sizeof(hdr.original_height));
//	memcpy (&hdr.masked, bytes + 26, sizeof(hdr.masked));
//	memcpy (&hdr.datalen, bytes + 28, sizeof(hdr.datalen));

//	hdr.version = getUShort (&hdr.version);
//	hdr.width = getUShort (&hdr.width);
//	hdr.height = getUShort (&hdr.height);
//	hdr.original_width = getUShort (&hdr.original_width);
//	hdr.original_height = getUShort (&hdr.original_height);
//	hdr.masked = getUShort (&hdr.masked);
//	hdr.datalen = getUInt (&hdr.datalen);

	if (memcmp(hdr.id, "TEXMIPS\0", 8) != 0 || hdr.version != TEXTURE_VERSION)
		return NULL;

	/* first, figure out what/where mip levels we have available */

	w = hdr.width;
	h = hdr.height;
	numlevels = 0;
	off = 0;
	mipdat = bytes + sizeof(hdr);
	while (off < hdr.datalen)
	{
		if (numlevels >= MAX_LEVELS)
			F_Error ("too many mip levels: %d", numlevels);

		widths[numlevels] = w;
		heights[numlevels] = h;

		pixelofs[numlevels] = off;
		off += w * h * 3;

		if (hdr.masked)
		{
			maskofs[numlevels] = off;
			off += (w * h + 7) / 8;
		}

		w /= 2;
		h /= 2;
		numlevels++;
	}

	/* find total size needed to allocate */

	allocsz = sizeof(struct texture_s) + sizeof(struct texlevel_s) * numlevels;
	for (i = 0; i < numlevels; i++)
	{
		allocsz += widths[i] * heights[i] * sizeof(pixel_t);
		if (hdr.masked)
			allocsz += widths[i] * heights[i];
	}

	/* allocate and read */

	ret = malloc (allocsz);

	memcpy (ret->name, hdr.wadname, sizeof(hdr.wadname));
	ret->name[sizeof(hdr.wadname)] = '\0';

	dest = (uint8_t *)ret + sizeof(struct texture_s) + sizeof(struct texlevel_s) * numlevels;
	for (i = 0; i < numlevels; i++)
	{
		ret->levels[i].w = widths[i];
		ret->levels[i].h = heights[i];

		ret->levels[i].pixels = (void *)dest;
		dest += widths[i] * heights[i] * sizeof(pixel_t);

		for (j = 0; j < ret->levels[i].w * ret->levels[i].h; j++)
			ret->levels[i].pixels[j] = RGB24to16 (mipdat + pixelofs[i] + j * 3);

		if (hdr.masked)
		{
			ret->levels[i].mask = (void *)dest;
			dest += widths[i] * heights[i];

			for (j = 0; j < ret->levels[i].w * ret->levels[i].h; j++)
				ret->levels[i].mask[j] = ((mipdat + maskofs[i])[j >> 3] & (1 << (j & 0x7))) != 0x0;
		}
		else
		{
			ret->levels[i].mask = NULL;
		}
	}

	return ret;
}
#endif


struct texture_s *
Dat_LoadTexture (const char *name)
{
	/*
	uint8_t *bytes;
	unsigned int sz;
	*/
	struct texture_s *ret = NULL;

	/*
	if ((bytes = Pak_Read(name, &sz)) != NULL)
	{
		ret = ParseTexture(bytes, sz);
		bytes = Pak_Free (bytes);
	}
	*/

	return ret;
}
