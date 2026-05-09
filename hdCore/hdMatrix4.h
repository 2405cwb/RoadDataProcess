#ifndef _HD_MATRIX4_H_
#define _HD_MATRIX4_H_

#include "hdDefs.h"
#include "hdMath.h"
#include "hdVector3d.h"

namespace hd
{
	//! 4x4 matrix. Mostly used as transformation matrix for 3d calculations.
	/** The matrix is a D3D style matrix, row major with translations in the 4th row. */
	template <class T>
	class hdMatrix4
	{
		public:

			//! Constructor Flags
			enum eConstructor
			{
				EM4CONST_NOTHING = 0,
				EM4CONST_COPY,
				EM4CONST_IDENTITY,
				EM4CONST_TRANSPOSED,
				EM4CONST_INVERSE,
				EM4CONST_INVERSE_TRANSPOSED
			};

			//! Default constructor
			/** \param constructor Choose the initialization style */
			hdMatrix4( eConstructor constructor = EM4CONST_IDENTITY );
			//! Copy constructor
			/** \param other Other matrix to copy from
			\param constructor Choose the initialization style */
			hdMatrix4( const hdMatrix4<T>& other,eConstructor constructor = EM4CONST_COPY);

			//! Simple operator for directly accessing every element of the matrix.
			T& operator()(const s32 row, const s32 col)
			{ 
#if defined ( USE_MATRIX_TEST )
				definitelyIdentityMatrix=false;
#endif
				return M[ row * 4 + col ];
			}

			//! Simple operator for directly accessing every element of the matrix.
			const T& operator()(const s32 row, const s32 col) const { return M[row * 4 + col]; }

			//! Simple operator for linearly accessing every element of the matrix.
			T& operator[](u32 index)
			{ 
#if defined ( USE_MATRIX_TEST )
				definitelyIdentityMatrix=false; 
#endif
				return M[index];
			}

			//! Simple operator for linearly accessing every element of the matrix.
			const T& operator[](u32 index) const { return M[index]; }

			//! Sets this matrix equal to the other matrix.
			inline hdMatrix4<T>& operator=(const hdMatrix4<T> &other);

			//! Sets all elements of this matrix to the value.
			inline hdMatrix4<T>& operator=(const T& scalar);

			//! Returns pointer to internal array
			const T* pointer() const { return M; }
			T* pointer() 
			{ 
#if defined ( USE_MATRIX_TEST )
				definitelyIdentityMatrix=false;
#endif
				return M;
			}

			//! Returns true if other matrix is equal to this matrix.
			bool operator==(const hdMatrix4<T> &other) const;

			//! Returns true if other matrix is not equal to this matrix.
			bool operator!=(const hdMatrix4<T> &other) const;

			//! Add another matrix.
			hdMatrix4<T> operator+(const hdMatrix4<T>& other) const;

			//! Add another matrix.
			hdMatrix4<T>& operator+=(const hdMatrix4<T>& other);

			//! Subtract another matrix.
			hdMatrix4<T> operator-(const hdMatrix4<T>& other) const;

			//! Subtract another matrix.
			hdMatrix4<T>& operator-=(const hdMatrix4<T>& other);

			//! Set matrix to identity.
			inline hdMatrix4<T>& makeIdentity();

				//! set this matrix to the product of two matrices
			inline hdMatrix4<T>& setbyproduct(const hdMatrix4<T>& other_a,const hdMatrix4<T>& other_b );

			//! Set this matrix to the product of two matrices
			/** no optimization used,
			use it if you know you never have a identity matrix */
			hdMatrix4<T>& setbyproduct_nocheck(const hdMatrix4<T>& other_a,const hdMatrix4<T>& other_b );

			//! Multiply by another matrix.
			hdMatrix4<T> operator*(const hdMatrix4<T>& other) const;

			//! Multiply by another matrix.
			hdMatrix4<T>& operator*=(const hdMatrix4<T>& other);

			//! Multiply by scalar.
			hdMatrix4<T> operator*(const T& scalar) const;

			//! Multiply by scalar.
			hdMatrix4<T>& operator*=(const T& scalar);

			//! Returns the rotation, as set by setRotation().
			/** This code was orginally written by by Chev. */
			CHdVector3d<T> getRotationDegrees() const;

			bool makeInverse();
			

			//! Gets transposed matrix
			hdMatrix4<T> getTransposed() const;

			//! Gets transposed matrix
			inline void getTransposed( hdMatrix4<T>& dest ) const;

			//! Sets all matrix data members at once
			hdMatrix4<T>& setM(const T* data);

			//! Set Scale
			hdMatrix4<T>& setScale( const CHdVector3d<T>& scale );

			//! Set Scale
			hdMatrix4<T>& setScale( const T scale ) { return setScale(core::CHdVector3d<T>(scale,scale,scale)); }

			//! Get Scale
			CHdVector3d<T> getScale() const;

				//! Gets the inversed matrix of this one
			/** \param out: where result matrix is written to.
			\return Returns false if there is no inverse matrix. */
			bool getInverse(hdMatrix4<T>& out) const;

		private:
			//! Matrix data, stored in row-major order
			T M[16];
	};

	// Default constructor
	template <class T>
	inline hdMatrix4<T>::hdMatrix4( eConstructor constructor )
	{
		switch ( constructor )
		{
		case EM4CONST_NOTHING:
		case EM4CONST_COPY:
			break;
		case EM4CONST_IDENTITY:
		case EM4CONST_INVERSE:
		default:
			makeIdentity();
			break;
		}
	}

	// Copy constructor
	template <class T>
	inline hdMatrix4<T>::hdMatrix4( const hdMatrix4<T>& other, eConstructor constructor)
	{
		switch ( constructor )
		{
		case EM4CONST_IDENTITY:
			makeIdentity();
			break;
		case EM4CONST_NOTHING:
			break;
		case EM4CONST_COPY:
			*this = other;
			break;
		case EM4CONST_TRANSPOSED:
			other.getTransposed(*this);
			break;
		case EM4CONST_INVERSE:
			if (!other.getInverse(*this))
				memset(M, 0, 16*sizeof(T));
			break;
		case EM4CONST_INVERSE_TRANSPOSED:
			if (!other.getInverse(*this))
				memset(M, 0, 16*sizeof(T));
			else
				*this=getTransposed();
			break;
		}
	}

	//! Add another matrix.
	template <class T>
	inline hdMatrix4<T> hdMatrix4<T>::operator+(const hdMatrix4<T>& other) const
	{
		hdMatrix4<T> temp ( EM4CONST_NOTHING );

		temp[0] = M[0]+other[0];
		temp[1] = M[1]+other[1];
		temp[2] = M[2]+other[2];
		temp[3] = M[3]+other[3];
		temp[4] = M[4]+other[4];
		temp[5] = M[5]+other[5];
		temp[6] = M[6]+other[6];
		temp[7] = M[7]+other[7];
		temp[8] = M[8]+other[8];
		temp[9] = M[9]+other[9];
		temp[10] = M[10]+other[10];
		temp[11] = M[11]+other[11];
		temp[12] = M[12]+other[12];
		temp[13] = M[13]+other[13];
		temp[14] = M[14]+other[14];
		temp[15] = M[15]+other[15];

		return temp;
	}

	template <class T>
	inline hdMatrix4<T>& hdMatrix4<T>::makeIdentity()
	{
		memset(M, 0, 16*sizeof(T));
		M[0] = M[5] = M[10] = M[15] = (T)1;
		return *this;
	}

	// returns transposed matrix
	template <class T>
	inline hdMatrix4<T> hdMatrix4<T>::getTransposed() const
	{
		hdMatrix4<T> t ( EM4CONST_NOTHING );
		getTransposed ( t );
		return t;
	}


	// returns transposed matrix
	template <class T>
	inline void hdMatrix4<T>::getTransposed( hdMatrix4<T>& o ) const
	{
		o[ 0] = M[ 0];
		o[ 1] = M[ 4];
		o[ 2] = M[ 8];
		o[ 3] = M[12];

		o[ 4] = M[ 1];
		o[ 5] = M[ 5];
		o[ 6] = M[ 9];
		o[ 7] = M[13];

		o[ 8] = M[ 2];
		o[ 9] = M[ 6];
		o[10] = M[10];
		o[11] = M[14];

		o[12] = M[ 3];
		o[13] = M[ 7];
		o[14] = M[11];
		o[15] = M[15];

	}

	template <class T>
	inline hdMatrix4<T>& hdMatrix4<T>::operator=(const hdMatrix4<T> &other)
	{
		if (this==&other)
			return *this;
		memcpy(M, other.M, 16*sizeof(T));
		return *this;
	}


	template <class T>
	inline hdMatrix4<T>& hdMatrix4<T>::operator=(const T& scalar)
	{
		for (s32 i = 0; i < 16; ++i)
			M[i]=scalar;


		return *this;
	}

	//! Add another matrix.
	template <class T>
	inline hdMatrix4<T>& hdMatrix4<T>::operator+=(const hdMatrix4<T>& other)
	{
		M[0]+=other[0];
		M[1]+=other[1];
		M[2]+=other[2];
		M[3]+=other[3];
		M[4]+=other[4];
		M[5]+=other[5];
		M[6]+=other[6];
		M[7]+=other[7];
		M[8]+=other[8];
		M[9]+=other[9];
		M[10]+=other[10];
		M[11]+=other[11];
		M[12]+=other[12];
		M[13]+=other[13];
		M[14]+=other[14];
		M[15]+=other[15];

		return *this;
	}

	//! Subtract another matrix.
	template <class T>
	inline hdMatrix4<T> hdMatrix4<T>::operator-(const hdMatrix4<T>& other) const
	{
		hdMatrix4<T> temp ( EM4CONST_NOTHING );

		temp[0] = M[0]-other[0];
		temp[1] = M[1]-other[1];
		temp[2] = M[2]-other[2];
		temp[3] = M[3]-other[3];
		temp[4] = M[4]-other[4];
		temp[5] = M[5]-other[5];
		temp[6] = M[6]-other[6];
		temp[7] = M[7]-other[7];
		temp[8] = M[8]-other[8];
		temp[9] = M[9]-other[9];
		temp[10] = M[10]-other[10];
		temp[11] = M[11]-other[11];
		temp[12] = M[12]-other[12];
		temp[13] = M[13]-other[13];
		temp[14] = M[14]-other[14];
		temp[15] = M[15]-other[15];

		return temp;
	}

	//! Subtract another matrix.
	template <class T>
	inline hdMatrix4<T>& hdMatrix4<T>::operator-=(const hdMatrix4<T>& other)
	{
		M[0]-=other[0];
		M[1]-=other[1];
		M[2]-=other[2];
		M[3]-=other[3];
		M[4]-=other[4];
		M[5]-=other[5];
		M[6]-=other[6];
		M[7]-=other[7];
		M[8]-=other[8];
		M[9]-=other[9];
		M[10]-=other[10];
		M[11]-=other[11];
		M[12]-=other[12];
		M[13]-=other[13];
		M[14]-=other[14];
		M[15]-=other[15];

		return *this;
	}

	//! Multiply by scalar.
	template <class T>
	inline hdMatrix4<T> hdMatrix4<T>::operator*(const T& scalar) const
	{
		hdMatrix4<T> temp ( EM4CONST_NOTHING );

		temp[0] = M[0]*scalar;
		temp[1] = M[1]*scalar;
		temp[2] = M[2]*scalar;
		temp[3] = M[3]*scalar;
		temp[4] = M[4]*scalar;
		temp[5] = M[5]*scalar;
		temp[6] = M[6]*scalar;
		temp[7] = M[7]*scalar;
		temp[8] = M[8]*scalar;
		temp[9] = M[9]*scalar;
		temp[10] = M[10]*scalar;
		temp[11] = M[11]*scalar;
		temp[12] = M[12]*scalar;
		temp[13] = M[13]*scalar;
		temp[14] = M[14]*scalar;
		temp[15] = M[15]*scalar;

		return temp;
	}

	//! Multiply by scalar.
	template <class T>
	inline hdMatrix4<T>& hdMatrix4<T>::operator*=(const T& scalar)
	{
		M[0]*=scalar;
		M[1]*=scalar;
		M[2]*=scalar;
		M[3]*=scalar;
		M[4]*=scalar;
		M[5]*=scalar;
		M[6]*=scalar;
		M[7]*=scalar;
		M[8]*=scalar;
		M[9]*=scalar;
		M[10]*=scalar;
		M[11]*=scalar;
		M[12]*=scalar;
		M[13]*=scalar;
		M[14]*=scalar;
		M[15]*=scalar;

		return *this;
	}

	//! Multiply by another matrix.
	template <class T>
	inline hdMatrix4<T>& hdMatrix4<T>::operator*=(const hdMatrix4<T>& other)
	{
		hdMatrix4<T> temp ( *this );
		return setbyproduct_nocheck( temp, other );

	}

	//! multiply by another matrix
	// set this matrix to the product of two other matrices
	// goal is to reduce stack use and copy
	template <class T>
	inline hdMatrix4<T>& hdMatrix4<T>::setbyproduct_nocheck(const hdMatrix4<T>& other_a,const hdMatrix4<T>& other_b )
	{
		const T *m1 = other_a.M;
		const T *m2 = other_b.M;

		M[0] = m1[0]*m2[0] + m1[4]*m2[1] + m1[8]*m2[2] + m1[12]*m2[3];
		M[1] = m1[1]*m2[0] + m1[5]*m2[1] + m1[9]*m2[2] + m1[13]*m2[3];
		M[2] = m1[2]*m2[0] + m1[6]*m2[1] + m1[10]*m2[2] + m1[14]*m2[3];
		M[3] = m1[3]*m2[0] + m1[7]*m2[1] + m1[11]*m2[2] + m1[15]*m2[3];

		M[4] = m1[0]*m2[4] + m1[4]*m2[5] + m1[8]*m2[6] + m1[12]*m2[7];
		M[5] = m1[1]*m2[4] + m1[5]*m2[5] + m1[9]*m2[6] + m1[13]*m2[7];
		M[6] = m1[2]*m2[4] + m1[6]*m2[5] + m1[10]*m2[6] + m1[14]*m2[7];
		M[7] = m1[3]*m2[4] + m1[7]*m2[5] + m1[11]*m2[6] + m1[15]*m2[7];

		M[8] = m1[0]*m2[8] + m1[4]*m2[9] + m1[8]*m2[10] + m1[12]*m2[11];
		M[9] = m1[1]*m2[8] + m1[5]*m2[9] + m1[9]*m2[10] + m1[13]*m2[11];
		M[10] = m1[2]*m2[8] + m1[6]*m2[9] + m1[10]*m2[10] + m1[14]*m2[11];
		M[11] = m1[3]*m2[8] + m1[7]*m2[9] + m1[11]*m2[10] + m1[15]*m2[11];

		M[12] = m1[0]*m2[12] + m1[4]*m2[13] + m1[8]*m2[14] + m1[12]*m2[15];
		M[13] = m1[1]*m2[12] + m1[5]*m2[13] + m1[9]*m2[14] + m1[13]*m2[15];
		M[14] = m1[2]*m2[12] + m1[6]*m2[13] + m1[10]*m2[14] + m1[14]*m2[15];
		M[15] = m1[3]*m2[12] + m1[7]*m2[13] + m1[11]*m2[14] + m1[15]*m2[15];

		return *this;
	}

	//! Returns a rotation that is equivalent to that set by setRotationDegrees().
	/** This code was sent in by Chev.  Note that it does not necessarily return
	the *same* Euler angles as those set by setRotationDegrees(), but the rotation will
	be equivalent, i.e. will have the same result when used to rotate a vector or node. */
	template <class T>
	inline CHdVector3d<T> hdMatrix4<T>::getRotationDegrees() const
	{
		const hdMatrix4<T> &mat = *this;
		const CHdVector3d<T> scale = getScale();
		const CHdVector3d<f64> invScale(reciprocal(scale.X),reciprocal(scale.Y),reciprocal(scale.Z));

		f64 Y = -asin(mat[2]*invScale.X);
		const f64 C = cos(Y);
		Y *= RADTODEG64;

		f64 rotx, roty, X, Z;

		if (!iszero(C))
		{
			const f64 invC = reciprocal(C);
			rotx = mat[10] * invC * invScale.Z;
			roty = mat[6] * invC * invScale.Y;
			X = atan2( roty, rotx ) * RADTODEG64;
			rotx = mat[0] * invC * invScale.X;
			roty = mat[1] * invC * invScale.X;
			Z = atan2( roty, rotx ) * RADTODEG64;
		}
		else
		{
			X = 0.0;
			rotx = mat[5] * invScale.Y;
			roty = -mat[4] * invScale.Y;
			Z = atan2( roty, rotx ) * RADTODEG64;
		}

	
		if (X < 0.0) X += 360.0;
		if (Y < 0.0) Y += 360.0;
		if (Z < 0.0) Z += 360.0;

		return CHdVector3d<T>((T)X,(T)Y,(T)Z);
	}

	// sets all matrix data members at once
	template <class T>
	inline hdMatrix4<T>& hdMatrix4<T>::setM(const T* data)
	{
		memcpy(M,data, 16*sizeof(T));
		return *this;
	}

	template <class T>
	inline bool hdMatrix4<T>::makeInverse()
	{
		hdMatrix4<T> temp ( EM4CONST_NOTHING );

		if (getInverse(temp))
		{
			*this = temp;
			return true;
		}

		return false;
	}


		//! Returns the absolute values of the scales of the matrix.
	/** 
	Note that this always returns the absolute (positive) values.  Unfortunately it
	does not appear to be possible to extract any original negative values.  The best
	that we could do would be to arbitrarily make one scale negative if one or three
	of them were negative.
	FIXME - return the original values.
	*/
	template <class T>
	inline CHdVector3d<T> hdMatrix4<T>::getScale() const
	{
		// See http://www.robertblum.com/articles/2005/02/14/decomposing-matrices

		// Deal with the 0 rotation case first
		// Prior to Irrlicht 1.6, we always returned this value.
		if(iszero(M[1]) && iszero(M[2]) &&
			iszero(M[4]) && iszero(M[6]) &&
			iszero(M[8]) && iszero(M[9]))
			return CHdVector3d<T>(M[0], M[5], M[10]);

		// We have to do the full calculation.
		return CHdVector3d<T>(sqrt(M[0] * M[0] + M[1] * M[1] + M[2] * M[2]),
							sqrt(M[4] * M[4] + M[5] * M[5] + M[6] * M[6]),
							sqrt(M[8] * M[8] + M[9] * M[9] + M[10] * M[10]));
	}

	template <class T>
	inline hdMatrix4<T>& hdMatrix4<T>::setScale( const CHdVector3d<T>& scale )
	{
		M[0] = scale.X;
		M[5] = scale.Y;
		M[10] = scale.Z;

		return *this;
	}

	template <class T>
	inline bool hdMatrix4<T>::getInverse(hdMatrix4<T>& out) const
	{
		/// Calculates the inverse of this Matrix
		/// The inverse is calculated using Cramers rule.
		/// If no inverse exists then 'false' is returned.

		const hdMatrix4<T> &m = *this;

		f32 d = (f32)((m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0)) * (m(2, 2) * m(3, 3) - m(2, 3) * m(3, 2)) -
			(m(0, 0) * m(1, 2) - m(0, 2) * m(1, 0)) * (m(2, 1) * m(3, 3) - m(2, 3) * m(3, 1)) +
			(m(0, 0) * m(1, 3) - m(0, 3) * m(1, 0)) * (m(2, 1) * m(3, 2) - m(2, 2) * m(3, 1)) +
			(m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1)) * (m(2, 0) * m(3, 3) - m(2, 3) * m(3, 0)) -
			(m(0, 1) * m(1, 3) - m(0, 3) * m(1, 1)) * (m(2, 0) * m(3, 2) - m(2, 2) * m(3, 0)) +
			(m(0, 2) * m(1, 3) - m(0, 3) * m(1, 2)) * (m(2, 0) * m(3, 1) - m(2, 1) * m(3, 0)));

		if( iszero ( d ) )
			return false;

		d = reciprocal ( d );

		out(0, 0) = d * (m(1, 1) * (m(2, 2) * m(3, 3) - m(2, 3) * m(3, 2)) +
			m(1, 2) * (m(2, 3) * m(3, 1) - m(2, 1) * m(3, 3)) +
			m(1, 3) * (m(2, 1) * m(3, 2) - m(2, 2) * m(3, 1)));
		out(0, 1) = d * (m(2, 1) * (m(0, 2) * m(3, 3) - m(0, 3) * m(3, 2)) +
			m(2, 2) * (m(0, 3) * m(3, 1) - m(0, 1) * m(3, 3)) +
			m(2, 3) * (m(0, 1) * m(3, 2) - m(0, 2) * m(3, 1)));
		out(0, 2) = d * (m(3, 1) * (m(0, 2) * m(1, 3) - m(0, 3) * m(1, 2)) +
			m(3, 2) * (m(0, 3) * m(1, 1) - m(0, 1) * m(1, 3)) +
			m(3, 3) * (m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1)));
		out(0, 3) = d * (m(0, 1) * (m(1, 3) * m(2, 2) - m(1, 2) * m(2, 3)) +
			m(0, 2) * (m(1, 1) * m(2, 3) - m(1, 3) * m(2, 1)) +
			m(0, 3) * (m(1, 2) * m(2, 1) - m(1, 1) * m(2, 2)));
		out(1, 0) = d * (m(1, 2) * (m(2, 0) * m(3, 3) - m(2, 3) * m(3, 0)) +
			m(1, 3) * (m(2, 2) * m(3, 0) - m(2, 0) * m(3, 2)) +
			m(1, 0) * (m(2, 3) * m(3, 2) - m(2, 2) * m(3, 3)));
		out(1, 1) = d * (m(2, 2) * (m(0, 0) * m(3, 3) - m(0, 3) * m(3, 0)) +
			m(2, 3) * (m(0, 2) * m(3, 0) - m(0, 0) * m(3, 2)) +
			m(2, 0) * (m(0, 3) * m(3, 2) - m(0, 2) * m(3, 3)));
		out(1, 2) = d * (m(3, 2) * (m(0, 0) * m(1, 3) - m(0, 3) * m(1, 0)) +
			m(3, 3) * (m(0, 2) * m(1, 0) - m(0, 0) * m(1, 2)) +
			m(3, 0) * (m(0, 3) * m(1, 2) - m(0, 2) * m(1, 3)));
		out(1, 3) = d * (m(0, 2) * (m(1, 3) * m(2, 0) - m(1, 0) * m(2, 3)) +
			m(0, 3) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) +
			m(0, 0) * (m(1, 2) * m(2, 3) - m(1, 3) * m(2, 2)));
		out(2, 0) = d * (m(1, 3) * (m(2, 0) * m(3, 1) - m(2, 1) * m(3, 0)) +
			m(1, 0) * (m(2, 1) * m(3, 3) - m(2, 3) * m(3, 1)) +
			m(1, 1) * (m(2, 3) * m(3, 0) - m(2, 0) * m(3, 3)));
		out(2, 1) = d * (m(2, 3) * (m(0, 0) * m(3, 1) - m(0, 1) * m(3, 0)) +
			m(2, 0) * (m(0, 1) * m(3, 3) - m(0, 3) * m(3, 1)) +
			m(2, 1) * (m(0, 3) * m(3, 0) - m(0, 0) * m(3, 3)));
		out(2, 2) = d * (m(3, 3) * (m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0)) +
			m(3, 0) * (m(0, 1) * m(1, 3) - m(0, 3) * m(1, 1)) +
			m(3, 1) * (m(0, 3) * m(1, 0) - m(0, 0) * m(1, 3)));
		out(2, 3) = d * (m(0, 3) * (m(1, 1) * m(2, 0) - m(1, 0) * m(2, 1)) +
			m(0, 0) * (m(1, 3) * m(2, 1) - m(1, 1) * m(2, 3)) +
			m(0, 1) * (m(1, 0) * m(2, 3) - m(1, 3) * m(2, 0)));
		out(3, 0) = d * (m(1, 0) * (m(2, 2) * m(3, 1) - m(2, 1) * m(3, 2)) +
			m(1, 1) * (m(2, 0) * m(3, 2) - m(2, 2) * m(3, 0)) +
			m(1, 2) * (m(2, 1) * m(3, 0) - m(2, 0) * m(3, 1)));
		out(3, 1) = d * (m(2, 0) * (m(0, 2) * m(3, 1) - m(0, 1) * m(3, 2)) +
			m(2, 1) * (m(0, 0) * m(3, 2) - m(0, 2) * m(3, 0)) +
			m(2, 2) * (m(0, 1) * m(3, 0) - m(0, 0) * m(3, 1)));
		out(3, 2) = d * (m(3, 0) * (m(0, 2) * m(1, 1) - m(0, 1) * m(1, 2)) +
			m(3, 1) * (m(0, 0) * m(1, 2) - m(0, 2) * m(1, 0)) +
			m(3, 2) * (m(0, 1) * m(1, 0) - m(0, 0) * m(1, 1)));
		out(3, 3) = d * (m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) +
			m(0, 1) * (m(1, 2) * m(2, 0) - m(1, 0) * m(2, 2)) +
			m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0)));

		return true;
	}
	//! Typedef for f32 matrix
	typedef hdMatrix4<f32> matrix4;

	//! Typedef for f64 matrix
	typedef hdMatrix4<f64> matrix4d;

}

#endif