#include "bdat.h"

static int endianess = 0;


int
BDatInit (void)
{
	union
	{
		int i;
		short s;
		char c;
	} u;

	u.i = 0;
	u.c = 1;
	if (u.s == 1)
		endianess = 0;
	else
		endianess = 1;

	return endianess;
}


int
GetByte (void *bytes)
{
	return ((unsigned char *)bytes)[0];
}


int
GetShort (void *bytes)
{
	if (endianess == 0)
	{
		return	(((unsigned char *)bytes)[0] << 0) |
			(((unsigned char *)bytes)[1] << 8);
	}
	else
	{
		return	(((unsigned char *)bytes)[1] << 0) |
			(((unsigned char *)bytes)[0] << 8);
	}
}


int
GetInt (void *bytes)
{
	if (endianess == 0)
	{
		return	(((unsigned char *)bytes)[0] << 0) |
			(((unsigned char *)bytes)[1] << 8) |
			(((unsigned char *)bytes)[2] << 16) |
			(((unsigned char *)bytes)[3] << 24);
	}
	else
	{
		return	(((unsigned char *)bytes)[3] << 0) |
			(((unsigned char *)bytes)[2] << 8) |
			(((unsigned char *)bytes)[1] << 16) |
			(((unsigned char *)bytes)[0] << 24);
	}
}


float
GetFloat (void *bytes)
{
	union {
		float f;
		unsigned char b[4];
	} x;

	if (endianess == 0)
	{
		x.b[0] = ((unsigned char *)bytes)[0];
		x.b[1] = ((unsigned char *)bytes)[1];
		x.b[2] = ((unsigned char *)bytes)[2];
		x.b[3] = ((unsigned char *)bytes)[3];
	}
	else
	{
		x.b[0] = ((unsigned char *)bytes)[3];
		x.b[1] = ((unsigned char *)bytes)[2];
		x.b[2] = ((unsigned char *)bytes)[1];
		x.b[3] = ((unsigned char *)bytes)[0];
	}

	return x.f;
}


double
GetDouble (void *bytes)
{
	union {
		double d;
		unsigned char b[8];
	} x;

	if (endianess == 0)
	{
		x.b[0] = ((unsigned char *)bytes)[0];
		x.b[1] = ((unsigned char *)bytes)[1];
		x.b[2] = ((unsigned char *)bytes)[2];
		x.b[3] = ((unsigned char *)bytes)[3];
		x.b[4] = ((unsigned char *)bytes)[4];
		x.b[5] = ((unsigned char *)bytes)[5];
		x.b[6] = ((unsigned char *)bytes)[6];
		x.b[7] = ((unsigned char *)bytes)[7];
	}
	else
	{
		x.b[0] = ((unsigned char *)bytes)[7];
		x.b[1] = ((unsigned char *)bytes)[6];
		x.b[2] = ((unsigned char *)bytes)[5];
		x.b[3] = ((unsigned char *)bytes)[4];
		x.b[4] = ((unsigned char *)bytes)[3];
		x.b[5] = ((unsigned char *)bytes)[2];
		x.b[6] = ((unsigned char *)bytes)[1];
		x.b[7] = ((unsigned char *)bytes)[0];
	}

	return x.d;
}


#if 0

#include <stdio.h>

int
main (int argc, const char **argv)
{
	unsigned char b[] = { 	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
				0x7f, 0x80, 0x81, 0xfe, 0xff, 0x00, 0x00, 0x00 };
	const char *s[] = { "little", "big" };
	int i;

	printf ("%s endian\n", s[BDatInit()]);

	for (i = 0; i < sizeof(b); i++)
		printf ("%d\n", GetByte(b + i));
	printf ("\n");

	for (i = 0; i < sizeof(b); i += 2)
		printf ("%d\n", GetShort(b + i));
	printf ("\n");

	for (i = 0; i < sizeof(b); i += 4)
		printf ("%d\n", GetInt(b + i));
	printf ("\n");

	//TODO: the random bytes above aren't valid floaters

	for (i = 0; i < sizeof(b); i += 4)
		printf ("%f\n", GetFloat(b + i));
	printf ("\n");

	for (i = 0; i < sizeof(b); i += 8)
		printf ("%f\n", GetDouble(b + i));
	printf ("\n");

	return 0;
}

#endif
