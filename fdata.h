#ifndef __FDATA_H__
#define __FDATA_H__

extern int
Data_AddPath (const char *path);

extern void
Data_RemovePath (const char *path);

extern void
Data_CloseAll (void);

extern void *
Data_Free (void *dat);

extern void *
Data_Fetch (const char *name, unsigned int *size);

#endif /* __FDATA_H__ */

