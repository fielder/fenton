#ifndef __BDAT_H__
#define __BDAT_H__

extern int
BDatInit (void); /* returns 0 on little, 1 on big-endianed hosts */

extern int
GetByte (void *bytes);

extern int
GetShort (void *bytes);

extern int
GetInt (void *bytes);

extern float
GetFloat (void *bytes);

extern double
GetDouble (void *bytes);

#endif /* __BDAT_H__ */
