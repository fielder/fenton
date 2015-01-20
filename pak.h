#ifndef __PAK_H__
#define __PAK_H__

struct pak_s;

extern struct pak_s *
Pak_Open (const char *path);

extern void *
Pak_Close (struct pak_s *pak);

extern void *
Pak_ReadEntry (struct pak_s *pak, const char *name, unsigned int *size);

extern void *
Pak_FreeEntry (void *dat);

#endif /* __PAK_H__ */
