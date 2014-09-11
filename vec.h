#ifndef __VEC_H__
#define __VEC_H__

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279
#endif /* M_PI */

#define DEG2RAD(_X) ((_X) * (M_PI / 180.0))
#define RAD2DEG(_X) ((_X) * (180.0 / M_PI))

extern void
Vec_Clear (double v[3]);

extern void
Vec_Copy (const double src[3], double out[3]);

extern void
Vec_Scale (double v[3], double s);

extern void
Vec_Add (const double a[3], const double b[3], double out[3]);

extern void
Vec_Subtract (const double a[3], const double b[3], double out[3]);

extern double
Vec_Dot (const double a[3], const double b[3]);

extern void
Vec_Cross (const double a[3], const double b[3], double out[3]);

extern void
Vec_Normalize (double v[3]);

extern double
Vec_Length (const double v[3]);

extern void
Vec_Transform (double xform[3][3], const double v[3], double out[3]);

extern void
Vec_MakeNormal (const double v1[3],
		const double v2[3],
		const double v3[3],
		double normal[3],
		double *dist);

extern void
Vec_IdentityMatrix (double mat[3][3]);

extern void
Vec_MultMatrix (double a[3][3], double b[3][3], double out[3][3]);

extern void
Vec_AnglesMatrix (const double angles[3], double out[3][3], const char *order);

#endif /* __VEC_H__ */
