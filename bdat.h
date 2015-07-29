#ifndef __BDAT_H__
#define __BDAT_H__

extern void *
AlignAllocation (void *buf, unsigned int bufsz, int align, unsigned int *count);

extern int
GetByte (const void *bytes);

extern int
GetShort (const void *bytes);

extern int
GetBigShort (const void *bytes);

extern int
GetInt (const void *bytes);

extern int
GetBigInt (const void *bytes);

extern float
GetFloat (const void *bytes);

extern float
GetBigFloat (const void *bytes);

extern double
GetDouble (const void *bytes);

extern double
GetBigDouble (const void *bytes);

#endif /* __BDAT_H__ */
