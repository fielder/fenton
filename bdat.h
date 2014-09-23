#ifndef __BDAT_H__
#define __BDAT_H__

extern int
GetByte (void *bytes);

extern int
GetShort (void *bytes);

extern int
GetBigShort (void *bytes);

extern int
GetInt (void *bytes);

extern int
GetBigInt (void *bytes);

extern float
GetFloat (void *bytes);

extern float
GetBigFloat (void *bytes);

extern double
GetDouble (void *bytes);

extern double
GetBigDouble (void *bytes);

#endif /* __BDAT_H__ */
