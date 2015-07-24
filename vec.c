/*
 * Slow, crusty, archaic math "library"
 */

#include <math.h>

#include "vec.h"


void
Vec_Clear (double v[3])
{
	v[0] = 0.0;
	v[1] = 0.0;
	v[2] = 0.0;
}


void
Vec_Copy (const double src[3], double out[3])
{
	out[0] = src[0];
	out[1] = src[1];
	out[2] = src[2];
}


void
Vec_Scale (double v[3], double s)
{
	v[0] *= s;
	v[1] *= s;
	v[2] *= s;
}


void
Vec_Add (const double a[3], const double b[3], double out[3])
{
	out[0] = a[0] + b[0];
	out[1] = a[1] + b[1];
	out[2] = a[2] + b[2];
}


void
Vec_Subtract (const double a[3], const double b[3], double out[3])
{
	out[0] = a[0] - b[0];
	out[1] = a[1] - b[1];
	out[2] = a[2] - b[2];
}


double
Vec_Dot (const double a[3], const double b[3])
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}


void
Vec_Cross (const double a[3], const double b[3], double out[3])
{
	out[0] = a[1] * b[2] - a[2] * b[1];
	out[1] = a[2] * b[0] - a[0] * b[2];
	out[2] = a[0] * b[1] - a[1] * b[0];
}


void
Vec_Normalize (double v[3])
{
	double len;

	len = sqrt (v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	if (len == 0.0)
	{
		v[0] = 0.0;
		v[1] = 0.0;
		v[2] = 0.0;
	}
	else
	{
		v[0] /= len;
		v[1] /= len;
		v[2] /= len;
	}
}


double
Vec_Length (const double v[3])
{
	return sqrt (v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}


void
Vec_Transform (double xform[3][3], const double v[3], double out[3])
{
	out[0] = xform[0][0] * v[0] + xform[0][1] * v[1] + xform[0][2] * v[2];
	out[1] = xform[1][0] * v[0] + xform[1][1] * v[1] + xform[1][2] * v[2];
	out[2] = xform[2][0] * v[0] + xform[2][1] * v[1] + xform[2][2] * v[2];
}


void
Vec_MakeNormal (const double v1[3],
		const double v2[3],
		const double v3[3],
		double normal[3],
		double *dist)
{
	double a[3], b[3];

	Vec_Subtract (v2, v1, a);
	Vec_Subtract (v3, v1, b);

	Vec_Cross (a, b, normal);

	Vec_Normalize (normal);

	*dist = Vec_Dot (normal, v1);
}


void
Vec_IdentityMatrix (double mat[3][3])
{
	mat[0][0] = 1.0; mat[0][1] = 0.0; mat[0][2] = 0.0;
	mat[1][0] = 0.0; mat[1][1] = 1.0; mat[1][2] = 0.0;
	mat[2][0] = 0.0; mat[2][1] = 0.0; mat[2][2] = 1.0;
}


void
Vec_MultMatrix (double a[3][3], double b[3][3], double out[3][3])
{
	out[0][0] = a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0];
	out[0][1] = a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1];
	out[0][2] = a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2];

	out[1][0] = a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0];
	out[1][1] = a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1];
	out[1][2] = a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2];

	out[2][0] = a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0];
	out[2][1] = a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1];
	out[2][2] = a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2];
}


static int
CmpOrder (const char *a, const char *b)
{
	return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}


void
Vec_AnglesMatrix (const double angles[3], double out[3][3], const char *order)
{
	double cx, sx;
	double cy, sy;
	double cz, sz;

	double x[3][3];
	double y[3][3];
	double z[3][3];
	double temp[3][3];

	cx = cos (angles[0]);
	sx = sin (angles[0]);
	Vec_IdentityMatrix (x);
	x[1][1] = cx;
	x[1][2] = -sx;
	x[2][1] = sx;
	x[2][2] = cx;

	cy = cos (angles[1]);
	sy = sin (angles[1]);
	Vec_IdentityMatrix (y);
	y[0][0] = cy;
	y[0][2] = sy;
	y[2][0] = -sy;
	y[2][2] = cy;

	cz = cos (angles[2]);
	sz = sin (angles[2]);
	Vec_IdentityMatrix (z);
	z[0][0] = cz;
	z[0][1] = -sz;
	z[1][0] = sz;
	z[1][1] = cz;

	if (CmpOrder(order, "xyz"))
	{
		Vec_MultMatrix (x, y, temp);
		Vec_MultMatrix (temp, z, out);
	}
	else if (CmpOrder(order, "xzy"))
	{
		Vec_MultMatrix (x, z, temp);
		Vec_MultMatrix (temp, y, out);
	}
	else if (CmpOrder(order, "yxz"))
	{
		Vec_MultMatrix (y, x, temp);
		Vec_MultMatrix (temp, z, out);
	}
	else if (CmpOrder(order, "zxy"))
	{
		Vec_MultMatrix (z, x, temp);
		Vec_MultMatrix (temp, y, out);
	}
	else if (CmpOrder(order, "yzx"))
	{
		Vec_MultMatrix (y, z, temp);
		Vec_MultMatrix (temp, x, out);
	}
	else if (CmpOrder(order, "zyx"))
	{
		Vec_MultMatrix (z, y, temp);
		Vec_MultMatrix (temp, x, out);
	}
	else
	{
		Vec_IdentityMatrix (out);
	}
}
