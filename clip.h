#ifndef __CLIP_H__
#define __CLIP_H__

#define CLIP_MAX_VERTS 64

extern double clip_verts[2][CLIP_MAX_VERTS][3];
extern int clip_idx;
extern int clip_numverts;

extern void
Clip_WithPlane (const double normal[3], double dist);

#endif /* __CLIP_H__ */
