#ifndef __FENTON_H__
#define __FENTON_H__

#define APPNAME "Fenton"

extern void
F_Quit (void);

extern void
F_Error (const char *fmt, ...);

extern void
F_Log (const char *fmt, ...);

extern void
F_AddPak (const char *path);

void
F_LoadMap (const char *name);

extern double frametime;
extern unsigned int elapsedtime_ms;

#endif /* __FENTON_H__ */
