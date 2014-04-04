#ifndef __PAK_H__
#define __PAK_H__

extern int
Pak_AddFile (const char *path);

extern void
Pak_CloseFile (const char *path);

extern void
Pak_CloseAll (void);

extern void *
Pak_Read (const char *name, unsigned int *size);

extern void *
Pak_Free (void *dat);

#endif /* __PAK_H__ */
