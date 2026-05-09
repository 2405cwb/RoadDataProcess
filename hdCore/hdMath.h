#pragma once
#include <math.h>
#include "hdDefs.h"
#include "Matrix.h"

namespace hd
{
#undef F32_MAX
#define F32_MAX            +2.0e+37f

#undef F32_MIN
#define F32_MIN            -2.0e+37f

#undef F64_MAX
#define F64_MAX            +2.0e+307
#undef F64_MIN
#define F64_MIN            -2.0e+307

#undef U8_MIN
#define U8_MIN             ((u8)0x0)  // 0
#undef U8_MAX
#define U8_MAX             ((u8)0xFF) // 255
#undef U8_MAX_PLUS_ONE
#define U8_MAX_PLUS_ONE    0x0100     // 256

#undef U16_MIN
#define U16_MIN            ((u16)0x0)    // 0
#undef U16_MAX
#define U16_MAX            ((u16)0xFFFF) // 65535
	#undef U16_MAX_PLUS_ONE
#define U16_MAX_PLUS_ONE   0x00010000    // 65536

#undef U32_MIN
#define U32_MIN            ((u32)0x0)            // 0
#undef U32_MAX
#define U32_MAX            ((u32)0xFFFFFFFF)     // 4294967295
	
#if defined(WIN32)            // 64 byte unsigned int constant under Windows 
#define U32_MAX_PLUS_ONE   0x0000000100000000    // 4294967296
#else                         // 64 byte unsigned int constant elsewhere ... 
#define U32_MAX_PLUS_ONE   0x0000000100000000ull // 4294967296
#endif

#undef I8_MIN
#define I8_MIN             ((s8)0x80) // -128
#undef I8_MAX
#define I8_MAX             ((s8)0x7F) // 127

#undef I16_MIN
#define I16_MIN            ((s16)0x8000) // -32768
#undef I16_MAX
#define I16_MAX            ((s16)0x7FFF) // 32767

#undef I32_MIN
#define I32_MIN            ((s32)0x80000000) // -2147483648
#undef I32_MAX
#define I32_MAX            ((s32)0x7FFFFFFF) //  2147483647

#undef I64_MIN
#define I64_MIN            ((s64)0x8000000000000000)
#undef I64_MAX
#define I64_MAX            ((s64)0x7FFFFFFFFFFFFFFF)

#undef U8_FOLD
#define U8_FOLD(n)      (((n) < U8_MIN) ? (n+U8_MAX_PLUS_ONE) : (((n) > U8_MAX) ? (n-U8_MAX_PLUS_ONE) : (n)))

//#undef I8_CLAMP
//#define I8_CLAMP(n)     (((n) <= I8_MIN) ? I8_MIN : (((n) >= I8_MAX) ? I8_MAX : ((s8)(n))))
//#define I8_CLAMP(n,l,h) (((n) <= l) ? l : (((n) >= h) ? h : ((s8)(n))))
//
//#undef U8_CLAMP
//#define U8_CLAMP(n)     (((n) <= U8_MIN) ? U8_MIN : (((n) >= U8_MAX) ? U8_MAX : ((u8)(n))))
//#define U8_CLAMP(n,l,h) (((n) <= l) ? l : (((n) >= h) ? h : ((u8)(n))))
//
//#undef I16_CLAMP
//#define I16_CLAMP(n)    (((n) <= I16_MIN) ? I16_MIN : (((n) >= I16_MAX) ? I16_MAX : ((s16)(n))))
//#define I16_CLAMP(n,l,h) (((n) <= l) ? l : (((n) >= h) ? h : ((s16)(n))))
//
//#undef U16_CLAMP
//#define U16_CLAMP(n)    (((n) <= U16_MIN) ? U16_MIN : (((n) >= U16_MAX) ? U16_MAX : ((u16)(n))))
//#define U16_CLAMP(n,l,h) (((n) <= l) ? l : (((n) >= h) ? h : ((u16)(n))))
//
//#undef I32_CLAMP
//#define I32_CLAMP(n)    (((n) <= I32_MIN) ? I32_MIN : (((n) >= I32_MAX) ? I32_MAX : ((s32)(n))))
//#define I32_CLAMP(n,l,h)    (((n) <= l) ? l : (((n) >= h) ? h : ((s32)(n))))
//
//#undef U32_CLAMP
//#define U32_CLAMP(n)    (((n) <= U32_MIN) ? U32_MIN : (((n) >= U32_MAX) ? U32_MAX : ((u32)(n))))
//#define U32_CLAMP(n,l,h)    (((n) <= l) ? l : (((n) >= h) ? h : ((u32)(n))))

#undef I8_QUANTIZE
#define I8_QUANTIZE(n) (((n) >= 0) ? (s8)((n)+0.5f) : (s8)((n)-0.5f))

#undef U8_QUANTIZE
#define U8_QUANTIZE(n) (((n) >= 0) ? (u8)((n)+0.5f) : (u8)(0))

#undef I16_QUANTIZE
#define I16_QUANTIZE(n) (((n) >= 0) ? (s16)((n)+0.5f) : (s16)((n)-0.5f))

#undef U16_QUANTIZE
#define U16_QUANTIZE(n) (((n) >= 0) ? (u16)((n)+0.5f) : (u16)(0))

#undef I32_QUANTIZE
#define I32_QUANTIZE(n) (((n) >= 0) ? (s32)((n)+0.5f) : (s32)((n)-0.5f))

#undef U32_QUANTIZE
#define U32_QUANTIZE(n) (((n) >= 0) ? (u32)((n)+0.5f) : (u32)(0))

#undef I64_QUANTIZE
#define I64_QUANTIZE(n) (((n) >= 0) ? (s64)((n)+0.5f) : (s64)((n)-0.5f))
#undef U64_QUANTIZE
#define U64_QUANTIZE(n) (((n) >= 0) ? (u64)((n)+0.5f) : (u64)(0))

#undef I16_FLOOR
#define I16_FLOOR(n) ((((s16)(n)) > (n)) ? (((s16)(n))-1) : ((s16)(n)))
#undef I32_FLOOR
#define I32_FLOOR(n) ((((s32)(n)) > (n)) ? (((s32)(n))-1) : ((s32)(n)))

#undef I64_FLOOR
#define I64_FLOOR(n) ((((s64)(n)) > (n)) ? (((s64)(n))-1) : ((s64)(n)))

#undef I16_CEIL
#define I16_CEIL(n) ((((s16)(n)) < (n)) ? (((s16)(n))+1) : ((s16)(n)))

#undef I32_CEIL
#define I32_CEIL(n) ((((s32)(n)) < (n)) ? (((s32)(n))+1) : ((s32)(n)))
#undef I64_CEIL
#define I64_CEIL(n) ((((s64)(n)) < (n)) ? (((s64)(n))+1) : ((s64)(n)))

#undef I8_FITS_IN_RANGE
#define I8_FITS_IN_RANGE(n) (((n) >= I8_MIN) || ((n) <= I8_MAX) ? TRUE : FALSE)
#undef U8_FITS_IN_RANGE
#define U8_FITS_IN_RANGE(n) (((n) >= U8_MIN) || ((n) <= U8_MAX) ? TRUE : FALSE)
#undef I16_FITS_IN_RANGE
#define I16_FITS_IN_RANGE(n) (((n) >= I16_MIN) || ((n) <= I16_MAX) ? TRUE : FALSE)
#undef U16_FITS_IN_RANGE
#define U16_FITS_IN_RANGE(n) (((n) >= U16_MIN) || ((n) <= U16_MAX) ? TRUE : FALSE)

#undef F32_IS_FINITE
#define F32_IS_FINITE(n) ((F32_MIN < (n)) && ((n) < F32_MAX))
#undef F64_IS_FINITE
#define F64_IS_FINITE(n) ((F64_MIN < (n)) && ((n) < F64_MAX))

#undef U32_ZERO_BIT_0
#define U32_ZERO_BIT_0(n) (((n)&(u32)0xFFFFFFFE))

#undef MAX
#define MAX(a,b)	((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))

const f32 PI32		= 3.14159265359f;

const double PI64		= 3.1415926535897932384626433832795028841971693993751;

//! Constant for 64bit reciprocal of PI.
const f64 RECIPROCAL_PI64 = 1.0/PI64;

//! 32bit Constant for converting from degrees to radians
const f32 DEGTORAD = PI32 / 180.0f;

//! 32bit constant for converting from radians to degrees (formally known as GRAD_PI)
const f32 RADTODEG   = 180.0f / PI32;

//! 64bit constant for converting from degrees to radians (formally known as GRAD_PI2)
const f64 DEGTORAD64 = PI64 / 180.0;

//! 64bit constant for converting from radians to degrees
const f64 RADTODEG64 = 180.0 / PI64;

const s32 ROUNDING_ERROR_S32 = 0;
const f32 ROUNDING_ERROR_f32 = 0.000001f;
const f64 ROUNDING_ERROR_f64 = 0.00000001;

//! returns minimum of two values. Own implementation to get rid of the STL (VS6 problems)
template<class T>
inline const T& min_(const T& a, const T& b)
{
	return a < b ? a : b;
}

//! returns minimum of three values. Own implementation to get rid of the STL (VS6 problems)
template<class T>
inline const T& min_(const T& a, const T& b, const T& c)
{
	return a < b ? min_(a, c) : min_(b, c);
}

//! returns maximum of two values. Own implementation to get rid of the STL (VS6 problems)
template<class T>
inline const T& max_(const T& a, const T& b)
{
	return a < b ? b : a;
}

//! returns maximum of three values. Own implementation to get rid of the STL (VS6 problems)
template<class T>
inline const T& max_(const T& a, const T& b, const T& c)
{
	return a < b ? max_(b, c) : max_(a, c);
}
//! clamps a value between low and high
template <class T>
inline const T clamp (const T& value, const T& low, const T& high)
{
	return min_ (max_(value,low), high);
}

	__inline float
		hd_round (float number)
	{
		return (number < 0.0f ? ceil(number - 0.5f) : floor(number + 0.5f));
	}

	__inline double
		hd_round (double number)
	{
		return (number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5));
	}

	__inline int 
		hd_round32(float number)
	{
		return (int)hd_round(number);
	}

	__inline int 
		hd_round32(double number)
	{
#if defined _MSC_VER && defined _M_IX86
		int t;
		__asm
		{
			fld number;
			fistp t;
		}
		return t;
#else
		return (int)(number + (number >= 0 ? 0.5 : -0.5));
#endif
	}

	//! returns if a equals b, taking possible rounding errors into account
	inline bool equals(const f64 a, const f64 b, const f64 tolerance = ROUNDING_ERROR_f64)
	{
		return (a + tolerance >= b) && (a - tolerance <= b);
	}

	//! returns if a equals b, taking possible rounding errors into account
	inline bool equals(const f32 a, const f32 b, const f32 tolerance = ROUNDING_ERROR_f32)
	{
		return (a + tolerance >= b) && (a - tolerance <= b);
	}
#if 0
	//! returns if a equals b, not using any rounding tolerance
	inline bool equals(const s32 a, const s32 b)
	{
		return (a == b);
	}

	//! returns if a equals b, not using any rounding tolerance
	inline bool equals(const u32 a, const u32 b)
	{
		return (a == b);
	}
#endif
	//! returns if a equals b, taking an explicit rounding tolerance into account
	inline bool equals(const s32 a, const s32 b, const s32 tolerance = ROUNDING_ERROR_S32)
	{
		return (a + tolerance >= b) && (a - tolerance <= b);
	}

	//! returns if a equals b, taking an explicit rounding tolerance into account
	inline bool equals(const u32 a, const u32 b, const s32 tolerance = ROUNDING_ERROR_S32)
	{
		return (a + tolerance >= b) && (a - tolerance <= b);
	}

	// calculate: sqrt ( x )
	inline f32 squareroot(const f32 f)
	{
		return sqrtf(f);
	}

	// calculate: sqrt ( x )
	inline f64 squareroot(const f64 f)
	{
		return sqrt(f);
	}	

	// calculate: sqrt ( x )
	inline s32 squareroot(const s32 f)
	{
		return static_cast<s32>(squareroot(static_cast<f32>(f)));
	}
		// calculate: 1 / sqrt ( x )
	inline f64 reciprocal_squareroot(const f64 x)
	{
		return 1.0 / sqrt(x);
	}

	// calculate: 1 / sqrtf ( x )
	inline f32 reciprocal_squareroot(const f32 f)
	{
		return 1.f / sqrtf(f);
	}

	// calculate: 1 / sqrtf( x )
	inline s32 reciprocal_squareroot(const s32 x)
	{
		return static_cast<s32>(reciprocal_squareroot(static_cast<f32>(x)));
	}


	//! returns if a equals zero, taking rounding errors into account
	inline bool iszero(const f64 a, const f64 tolerance = ROUNDING_ERROR_f64)
	{
		return fabs(a) <= tolerance;
	}

	//! returns if a equals zero, taking rounding errors into account
	inline bool iszero(const f32 a, const f32 tolerance = ROUNDING_ERROR_f32)
	{
		return fabsf(a) <= tolerance;
	}

	//! returns if a equals not zero, taking rounding errors into account
	inline bool isnotzero(const f32 a, const f32 tolerance = ROUNDING_ERROR_f32)
	{
		return fabsf(a) > tolerance;
	}

	//! returns if a equals zero, taking rounding errors into account
	inline bool iszero(const s32 a, const s32 tolerance = 0)
	{
		return ( a & 0x7ffffff ) <= tolerance;
	}

	//! returns if a equals zero, taking rounding errors into account
	inline bool iszero(const u32 a, const u32 tolerance = 0)
	{
		return a <= tolerance;
	}

	// calculate: 1 / x
	inline f32 reciprocal( const f32 f )
	{
		return 1.f / f;
	}

	// calculate: 1 / x
	inline f64 reciprocal ( const f64 f )
	{
		return 1.0 / f;
	}

#define hd_lrint(x) ((long int) hd_round(x))
#define hd_lrintf(x) ((long int) hd_round(x))

# define hd_isnan(x)    _isnan(x)
# define hd_isfinite(x) (_finite(x) != 0)
# define hd_isinf(x)    (_finite(x) == 0)

#ifndef DEG2RAD
#define DEG2RAD(x) ((x)*0.0174532925199433)
#endif

#ifndef RAD2DEG
#define RAD2DEG(x) ((x)*57.29577951308233)
#endif

	static inline void VecZero3fv(float v[3])
	{
		v[0] = 0.0f;
		v[1] = 0.0f;
		v[2] = 0.0f;
	}

	static inline void VecSubtract3fv(float v[3], float a[3], float b[3])
	{
		v[0] = a[0] - b[0];
		v[1] = a[1] - b[1];
		v[2] = a[2] - b[2];
	}

	static inline void VecSelfAdd3fv(float v[3], const float a[3])
	{
		v[0] += a[0];
		v[1] += a[1];
		v[2] += a[2];
	}

	static inline void VecSet3fv(float v[3], float x, float y, float z)
	{
		v[0] = x;
		v[1] = y;
		v[2] = z;
	}

	static void VecCrossProd3fv(float v[3], float a[3], float b[3]) // c = a X b
	{ 
		VecSet3fv(v, a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]);
	}

	static inline float VecDotProd3fv(const float a[3], const float b[3])
	{
		return( a[0]*b[0] + a[1]*b[1] + a[2]*b[2] );
	}

	static inline float VecLength3fv(float v[3])
	{
		return( (float)sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]) );
	}

	static inline bool VecSelfNormalize3fv(float v[3])
	{
		float l = VecLength3fv(v);
		if (l)
		{
			v[0] /= l;
			v[1] /= l;
			v[2] /= l;
			return true;
		}
		return false;
	}

	static inline bool VecCcwNormNormal3fv(float n[3], float a[3], float b[3], float c[3])
	{
		float ab[3], ac[3];
		VecSubtract3fv(ab,b,a);
		VecSubtract3fv(ac,c,a);
		VecCrossProd3fv(n,ab,ac);
		return VecSelfNormalize3fv(n);
	}

	//----------------------------------------------------------------------------
	// Given a unit vector 'x', find two other unit vectors 'y' and 'z' which
	// which form an orthonormal set.
	inline void Perpendiculars(const double x[3], double y[3], double z[3],
		double theta)
	{
		int dx,dy,dz;

		double x2 = x[0]*x[0];
		double y2 = x[1]*x[1];
		double z2 = x[2]*x[2];
		double r = sqrt(x2 + y2 + z2);

		// transpose the vector to avoid divide-by-zero error
		if (x2 > y2 && x2 > z2)
		{
			dx = 0; dy = 1; dz = 2;
		}
		else if (y2 > z2)
		{
			dx = 1; dy = 2; dz = 0;
		}
		else
		{
			dx = 2; dy = 0; dz = 1;
		}

		double a = x[dx]/r;
		double b = x[dy]/r;
		double c = x[dz]/r;

		double tmp = sqrt(a*a+c*c);

		if (theta != 0)
		{
			double sintheta = sin(theta);
			double costheta = cos(theta);

			if (y)
			{
				y[dx] = (c*costheta - a*b*sintheta)/tmp;
				y[dy] = sintheta*tmp;
				y[dz] = (-a*costheta - b*c*sintheta)/tmp;
			}

			if (z)
			{
				z[dx] = (-c*sintheta - a*b*costheta)/tmp;
				z[dy] = costheta*tmp;
				z[dz] = (a*sintheta - b*c*costheta)/tmp;
			}
		}
		else
		{
			if (y)
			{
				y[dx] = c/tmp;
				y[dy] = 0;
				y[dz] = -a/tmp;
			}

			if (z)
			{
				z[dx] = -a*b/tmp;
				z[dy] = tmp;
				z[dz] = -b*c/tmp;
			}
		}
	}

#define HD_LANDMARK_RIGIDBODY 6		// 刚体变换
#define HD_LANDMARK_SIMILARITY 7	// 相似变换
#define HD_LANDMARK_AFFINE 12		// 仿射变换
#define HD_LANDMARK_XYZYAW  4		// 退化模型变换
#define HD_ROTATE(a,i,j,k,l) g=a[i][j];h=a[k][l];a[i][j]=g-s*(h+g*tau);\
	a[k][l]=h+s*(g-h*tau)

#define HD_MAX_ROTATIONS 20

	//#undef VTK_MAX_ROTATIONS

	//#define VTK_MAX_ROTATIONS 50

	// Jacobi iteration for the solution of eigenvectors/eigenvalues of a nxn
	// real symmetric matrix. Square nxn matrix a; size of matrix in n;
	// output eigenvalues in w; and output eigenvectors in v. Resulting
	// eigenvalues/vectors are sorted in decreasing order; eigenvectors are
	// normalized.
	inline int JacobiN(double **a, int n, double *w, double **v)
	{
		int i, j, k, iq, ip, numPos;
		double tresh, theta, tau, t, sm, s, h, g, c, tmp;
		double bspace[4], zspace[4];
		double *b = bspace;
		double *z = zspace;

		// only allocate memory if the matrix is large
		if (n > 4)
		{
			b = new double[n];
			z = new double[n];
		}

		// initialize
		for (ip=0; ip<n; ip++)
		{
			for (iq=0; iq<n; iq++)
			{
				v[ip][iq] = 0.0;
			}
			v[ip][ip] = 1.0;
		}
		for (ip=0; ip<n; ip++)
		{
			b[ip] = w[ip] = a[ip][ip];
			z[ip] = 0.0;
		}

		// begin rotation sequence
		for (i=0; i<HD_MAX_ROTATIONS; i++)
		{
			sm = 0.0;
			for (ip=0; ip<n-1; ip++)
			{
				for (iq=ip+1; iq<n; iq++)
				{
					sm += fabs(a[ip][iq]);
				}
			}
			if (sm == 0.0)
			{
				break;
			}

			if (i < 3)                                // first 3 sweeps
			{
				tresh = 0.2*sm/(n*n);
			}
			else
			{
				tresh = 0.0;
			}

			for (ip=0; ip<n-1; ip++)
			{
				for (iq=ip+1; iq<n; iq++)
				{
					g = 100.0*fabs(a[ip][iq]);

					// after 4 sweeps
					if (i > 3 && (fabs(w[ip])+g) == fabs(w[ip])
						&& (fabs(w[iq])+g) == fabs(w[iq]))
					{
						a[ip][iq] = 0.0;
					}
					else if (fabs(a[ip][iq]) > tresh)
					{
						h = w[iq] - w[ip];
						if ( (fabs(h)+g) == fabs(h))
						{
							t = (a[ip][iq]) / h;
						}
						else
						{
							theta = 0.5*h / (a[ip][iq]);
							t = 1.0 / (fabs(theta)+sqrt(1.0+theta*theta));
							if (theta < 0.0)
							{
								t = -t;
							}
						}
						c = 1.0 / sqrt(1+t*t);
						s = t*c;
						tau = s/(1.0+c);
						h = t*a[ip][iq];
						z[ip] -= h;
						z[iq] += h;
						w[ip] -= h;
						w[iq] += h;
						a[ip][iq]=0.0;

						// ip already shifted left by 1 unit
						for (j = 0;j <= ip-1;j++)
						{
							HD_ROTATE(a,j,ip,j,iq);
						}
						// ip and iq already shifted left by 1 unit
						for (j = ip+1;j <= iq-1;j++)
						{
							HD_ROTATE(a,ip,j,j,iq);
						}
						// iq already shifted left by 1 unit
						for (j=iq+1; j<n; j++)
						{
							HD_ROTATE(a,ip,j,iq,j);
						}
						for (j=0; j<n; j++)
						{
							HD_ROTATE(v,j,ip,j,iq);
						}
					}
				}
			}

			for (ip=0; ip<n; ip++)
			{
				b[ip] += z[ip];
				w[ip] = b[ip];
				z[ip] = 0.0;
			}
		}

		//// this is NEVER called
		if ( i >= HD_MAX_ROTATIONS )
		{
			//vtkGenericWarningMacro(
			//	"vtkMath::Jacobi: Error extracting eigenfunctions");
			return 0;
		}

		// sort eigenfunctions                 these changes do not affect accuracy
		for (j=0; j<n-1; j++)                  // boundary incorrect
		{
			k = j;
			tmp = w[k];
			for (i=j+1; i<n; i++)                // boundary incorrect, shifted already
			{
				if (w[i] >= tmp)                   // why exchage if same?
				{
					k = i;
					tmp = w[k];
				}
			}
			if (k != j)
			{
				w[k] = w[j];
				w[j] = tmp;
				for (i=0; i<n; i++)
				{
					tmp = v[i][j];
					v[i][j] = v[i][k];
					v[i][k] = tmp;
				}
			}
		}
		// insure eigenvector consistency (i.e., Jacobi can compute vectors that
		// are negative of one another (.707,.707,0) and (-.707,-.707,0). This can
		// reek havoc in hyperstreamline/other stuff. We will select the most
		// positive eigenvector.
		int ceil_half_n = (n >> 1) + (n & 1);
		for (j=0; j<n; j++)
		{
			for (numPos=0, i=0; i<n; i++)
			{
				if ( v[i][j] >= 0.0 )
				{
					numPos++;
				}
			}
			//    if ( numPos < ceil(double(n)/double(2.0)) )
			if ( numPos < ceil_half_n)
			{
				for(i=0; i<n; i++)
				{
					v[i][j] *= -1.0;
				}
			}
		}

		if (n > 4)
		{
			delete [] b;
			delete [] z;
		}
		return 1;
	}
	//! 坐标转换根据4*4矩阵,转换坐标.gsl-2013/6/5
	inline double vtkHomogeneousTransformPoint(double M[4][4],
		double& x,double& y,double& z)
	{
		double tmpX = x;
		double tmpY = y;
		double tmpZ = z;

		x = M[0][0]*tmpX + M[0][1]*tmpY + M[0][2]*tmpZ + M[0][3];
		y = M[1][0]*tmpX + M[1][1]*tmpY + M[1][2]*tmpZ + M[1][3];
		z = M[2][0]*tmpX + M[2][1]*tmpY + M[2][2]*tmpZ + M[2][3];
		double w = M[3][0]*tmpX + M[3][1]*tmpY + M[3][2]*tmpZ + M[3][3];

		double f = 1.0/w;
		x = static_cast<double>(x*f);
		y = static_cast<double>(y*f);
		z = static_cast<double>(z*f);

		return f;
	}
	//! 坐标转换根据4*4矩阵,转换坐标.gsl-2013/6/5
	inline double vtkHomogeneousTransformPoint(double M[4][4],
		float& x,float& y,float& z)
	{
		double tmpX = x;
		double tmpY = y;
		double tmpZ = z;

		x = (float)(M[0][0]*tmpX + M[0][1]*tmpY + M[0][2]*tmpZ + M[0][3]);
		y = (float)(M[1][0]*tmpX + M[1][1]*tmpY + M[1][2]*tmpZ + M[1][3]);
		z = (float)(M[2][0]*tmpX + M[2][1]*tmpY + M[2][2]*tmpZ + M[2][3]);
		double w = M[3][0]*tmpX + M[3][1]*tmpY + M[3][2]*tmpZ + M[3][3];

		double f = 1.0/w;
		x = static_cast<float>(x*f);
		y = static_cast<float>(y*f);
		z = static_cast<float>(z*f);

		return f;
	}
	//! 坐标转换根据4*4矩阵,转换坐标.gsl-2013/6/5
	inline double hdHomogeneousTransformPoint(double M[16],
		double& x,double& y,double& z)
	{
		double tmpX = x;
		double tmpY = y;
		double tmpZ = z;

		x = M[0]*tmpX + M[1]*tmpY + M[2]*tmpZ + M[3];
		y = M[4]*tmpX + M[5]*tmpY + M[6]*tmpZ + M[7];
		z = M[8]*tmpX + M[9]*tmpY + M[10]*tmpZ + M[11];
		double w = M[12]*tmpX + M[13]*tmpY + M[14]*tmpZ + M[15];

		double f = 1.0/w;
		x = static_cast<double>(x*f);
		y = static_cast<double>(y*f);
		z = static_cast<double>(z*f);

		return f;
	}

	//! 多项式拟合.gsl-2013/9/24
	inline void PolynomialFitting( 
		int n,					/* 多项式指数 */ 
		double *pObsArgument,	/* 为观测值向量 */ 
		double *pRealArgument,	/* 真实值向量 */ 
		int num,				/* 观测值个数 */ 
		double offset,			/* 偏移因子（防止数据泄露）*/ 
		double *pPara )			// 为系数向量,长度为指数加1
	{
		if(n < 3 || n > 32 || pObsArgument == NULL || pRealArgument == NULL || pPara == NULL)
			return;
		int row = num ;
		int col = n+1 ;

		double *ptemp = new double[row];
		for (int i = 0 ; i<row; i++)
		{
			ptemp[i] = pObsArgument[i] - offset;
		}

		double * pArgumentMat = new double [row * col];	//自变量矩阵
		int k = 0;
		for ( int i = 0; i < row; i++)
		{
			for( int j = col-1; j >= 0; j--)
			{
				if (j !=0 )
				{
					pArgumentMat[k] = pow(ptemp[i],j);
					k++;
				}
				else
				{
					pArgumentMat[k] = 1;
					k++;
				}
			}
		}

		CMatrix Matrix_Xtemp(row,col,pArgumentMat);
		CMatrix Matrix_X = Matrix_Xtemp;
		CMatrix Matrix_Y(row,1,pRealArgument);
		CMatrix Matrix_XT = Matrix_Xtemp.getTranspose();
		CMatrix Matrix_P = Matrix_XT * Matrix_X;
		CMatrix Matrix_Q = Matrix_XT * Matrix_Y;
		CMatrix Matrix_Para = Matrix_P.getInverse() * Matrix_Q;

		//得到拟合方程系数
		for (int i = 0 ; i < col; i++)
		{
			pPara[i] = Matrix_Para.ptr[i];
		}

		delete[] ptemp;
		delete[] pArgumentMat;
	}

	//! 根据多项式,计算目标值.gsl-2013/9/24
	inline void PolynomialFittingValue( 
		double  obs,			// 观测值
		double* pPara,			// 多项式参数
		int	    n,			    // 多项式参数个数
		double& fitValue)		// 拟合结果值
	{		
		fitValue = 0.0;
		for (int i = 0;i <= n;i++)
		{
			fitValue += (pPara[n - i] * pow(obs,i));
		}
	}
	

#undef HD_ROTATE
#undef HD_MAX_ROTATIONS

}