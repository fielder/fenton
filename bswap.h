#ifndef __BSWAP_H__
#define __BSWAP_H__

extern int
SwapInit (void); /* returns 0 on little, 1 on big-endianed hosts */

extern short
SwapShort (short s);

extern int
SwapInt (int i);

extern float
SwapFloat (float f);

extern short
NoSwapShort (short s);

extern int
NoSwapInt (int i);

extern float
NoSwapFloat (float f);

extern short (*LittleShort) (short);
extern int (*LittleInt) (int);
extern float (*LittleFloat) (float);
extern short (*BigShort) (short);
extern int (*BigInt) (int);
extern float (*BigFloat) (float);

#endif /* __BSWAP_H__ */
