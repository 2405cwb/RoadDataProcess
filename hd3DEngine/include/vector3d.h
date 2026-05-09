// Copyright (C) 2002-2010 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_POINT_3D_H_INCLUDED__
#define __IRR_POINT_3D_H_INCLUDED__

#include "irrMath.h"

namespace irr
{
namespace core
{

	//! 3d vector template class with lots of operators and methods.
	/** The vector3d class is used in Irrlicht for three main purposes:
		1) As a direction vector (most of the methods assume this).
		2) As a position in 3d space (which is synonymous with a direction vector from the origin to this position).
		3) To hold three Euler rotations, where X is pitch, Y is yaw and Z is roll.
	*/
	template <class T>
	class vector3d
	{
	public:
		//! Default constructor (null vector).
		vector3d() : X(0), Y(0), Z(0) {}
		//! Constructor with three different values
		vector3d(T nx, T ny, T nz) : X(nx), Y(ny), Z(nz) {}
		//! Constructor with the same value for all elements
		explicit vector3d(T n) : X(n), Y(n), Z(n) {}
		//! Copy constructor
		vector3d(const vector3d<T>& other) : X(other.X), Y(other.Y), Z(other.Z) {}

		// operators

		vector3d<T> operator-() const { return vector3d<T>(-X, -Y, -Z); }

		vector3d<T>& operator=(const vector3d<T>& other) { X = other.X; Y = other.Y; Z = other.Z; return *this; }

		vector3d<T> operator+(const vector3d<T>& other) const { return vector3d<T>(X + other.X, Y + other.Y, Z + other.Z); }
		vector3d<T>& operator+=(const vector3d<T>& other) { X+=other.X; Y+=other.Y; Z+=other.Z; return *this; }
		vector3d<T> operator+(const T val) const { return vector3d<T>(X + val, Y + val, Z + val); }
		vector3d<T>& operator+=(const T val) { X+=val; Y+=val; Z+=val; return *this; }

		vector3d<T> operator-(const vector3d<T>& other) const { return vector3d<T>(X - other.X, Y - other.Y, Z - other.Z); }
		vector3d<T>& operator-=(const vector3d<T>& other) { X-=other.X; Y-=other.Y; Z-=other.Z; return *this; }
		vector3d<T> operator-(const T val) const { return vector3d<T>(X - val, Y - val, Z - val); }
		vector3d<T>& operator-=(const T val) { X-=val; Y-=val; Z-=val; return *this; }

		vector3d<T> operator*(const vector3d<T>& other) const { return vector3d<T>(X * other.X, Y * other.Y, Z * other.Z); }
		vector3d<T>& operator*=(const vector3d<T>& other) { X*=other.X; Y*=other.Y; Z*=other.Z; return *this; }
		vector3d<T> operator*(const T v) const { return vector3d<T>(X * v, Y * v, Z * v); }
		vector3d<T>& operator*=(const T v) { X*=v; Y*=v; Z*=v; return *this; }

		vector3d<T> operator/(const vector3d<T>& other) const { return vector3d<T>(X / other.X, Y / other.Y, Z / other.Z); }
		vector3d<T>& operator/=(const vector3d<T>& other) { X/=other.X; Y/=other.Y; Z/=other.Z; return *this; }
		vector3d<T> operator/(const T v) const { T i=(T)1.0/v; return vector3d<T>(X * i, Y * i, Z * i); }
		vector3d<T>& operator/=(const T v) { T i=(T)1.0/v; X*=i; Y*=i; Z*=i; return *this; }

		//! sort in order X, Y, Z. Equality with rounding tolerance.
		bool operator<=(const vector3d<T>&other) const
		{
			return 	(X<other.X || core::equals(X, other.X)) ||
					(core::equals(X, other.X) && (Y<other.Y || core::equals(Y, other.Y))) ||
					(core::equals(X, other.X) && core::equals(Y, other.Y) && (Z<other.Z || core::equals(Z, other.Z)));
		}

		//! sort in order X, Y, Z. Equality with rounding tolerance.
		bool operator>=(const vector3d<T>&other) const
		{
			return 	(X>other.X || core::equals(X, other.X)) ||
					(core::equals(X, other.X) && (Y>other.Y || core::equals(Y, other.Y))) ||
					(core::equals(X, other.X) && core::equals(Y, other.Y) && (Z>other.Z || core::equals(Z, other.Z)));
		}

		//! sort in order X, Y, Z. Difference must be above rounding tolerance.
		bool operator<(const vector3d<T>&other) const
		{
			return 	(X<other.X && !core::equals(X, other.X)) ||
					(core::equals(X, other.X) && Y<other.Y && !core::equals(Y, other.Y)) ||
					(core::equals(X, other.X) && core::equals(Y, other.Y) && Z<other.Z && !core::equals(Z, other.Z));
		}

		//! sort in order X, Y, Z. Difference must be above rounding tolerance.
		bool operator>(const vector3d<T>&other) const
		{
			return 	(X>other.X && !core::equals(X, other.X)) ||
					(core::equals(X, other.X) && Y>other.Y && !core::equals(Y, other.Y)) ||
					(core::equals(X, other.X) && core::equals(Y, other.Y) && Z>other.Z && !core::equals(Z, other.Z));
		}

		//! use weak float compare
		bool operator==(const vector3d<T>& other) const
		{
			return this->equals(other);
		}

		bool operator!=(const vector3d<T>& other) const
		{
			return !this->equals(other);
		}

		// functions

		//! returns if this vector equals the other one, taking floating point rounding errors into account
		bool equals(const vector3d<T>& other, const T tolerance = (T)ROUNDING_ERROR_f32 ) const
		{
			return core::equals(X, other.X, tolerance) &&
				core::equals(Y, other.Y, tolerance) &&
				core::equals(Z, other.Z, tolerance);
		}

		vector3d<T>& set(const T nx, const T ny, const T nz) {X=nx; Y=ny; Z=nz; return *this;}
		vector3d<T>& set(const vector3d<T>& p) {X=p.X; Y=p.Y; Z=p.Z;return *this;}

		//! Get length of the vector.
		T getLength() const { return core::squareroot( X*X + Y*Y + Z*Z ); }

		//! Get squared length of the vector.
		/** This is useful because it is much faster than getLength().
		\return Squared length of the vector. */
		T getLengthSQ() const { return X*X + Y*Y + Z*Z; }

		//! Get the dot product with another vector.
		T dotProduct(const vector3d<T>& other) const
		{
			return X*other.X + Y*other.Y + Z*other.Z;
		}

		//! Get distance from another point.
		/** Here, the vector is interpreted as point in 3 dimensional space. */
		T getDistanceFrom(const vector3d<T>& other) const
		{
			return vector3d<T>(X - other.X, Y - other.Y, Z - other.Z).getLength();
		}

		//! Returns squared distance from another point.
		/** Here, the vector is interpreted as point in 3 dimensional space. */
		T getDistanceFromSQ(const vector3d<T>& other) const
		{
			return vector3d<T>(X - other.X, Y - other.Y, Z - other.Z).getLengthSQ();
		}

		//! Calculates the cross product with another vector.
		/** \param p Vector to multiply with.
		\return Crossproduct of this vector with p. */
		vector3d<T> crossProduct(const vector3d<T>& p) const
		{
			return vector3d<T>(Y * p.Z - Z * p.Y, Z * p.X - X * p.Z, X * p.Y - Y * p.X);
		}

		//! Returns if this vector interpreted as a point is on a line between two other points.
		/** It is assumed that the point is on the line.
		\param begin Beginning vector to compare between.
		\param end Ending vector to compare between.
		\return True if this vector is between begin and end, false if not. */
		bool isBetweenPoints(const vector3d<T>& begin, const vector3d<T>& end) const
		{
			const T f = (end - begin).getLengthSQ();
			return getDistanceFromSQ(begin) <= f &&
				getDistanceFromSQ(end) <= f;
		}

		//! Normalizes the vector.
		/** In case of the 0 vector the result is still 0, otherwise
		the length of the vector will be 1.
		\return Reference to this vector after normalization. */
		vector3d<T>& normalize()
		{
			f64 length = X*X + Y*Y + Z*Z;
			if (length == 0 ) // this check isn't an optimization but prevents getting NAN in the sqrt.
				return *this;
			length = core::reciprocal_squareroot(length);

			X = (T)(X * length);
			Y = (T)(Y * length);
			Z = (T)(Z * length);
			return *this;
		}

		//! Sets the length of the vector to a new value
		vector3d<T>& setLength(T newlength)
		{
			normalize();
			return (*this *= newlength);
		}

		//! Inverts the vector.
		vector3d<T>& invert()
		{
			X *= -1;
			Y *= -1;
			Z *= -1;
			return *this;
		}

		//! Rotates the vector by a specified number of degrees around the Y axis and the specified center.
		/** \param degrees Number of degrees to rotate around the Y axis.
		\param center The center of the rotation. */
		void rotateXZBy(f64 degrees, const vector3d<T>& center=vector3d<T>())
		{
			degrees *= DEGTORAD64;
			f64 cs = cos(degrees);
			f64 sn = sin(degrees);
			X -= center.X;
			Z -= center.Z;
			set((T)(X*cs - Z*sn), Y, (T)(X*sn + Z*cs));
			X += center.X;
			Z += center.Z;
		}

		//! Rotates the vector by a specified number of degrees around the Z axis and the specified center.
		/** \param degrees: Number of degrees to rotate around the Z axis.
		\param center: The center of the rotation. */
		void rotateXYBy(f64 degrees, const vector3d<T>& center=vector3d<T>())
		{
			degrees *= DEGTORAD64;
			f64 cs = cos(degrees);
			f64 sn = sin(degrees);
			X -= center.X;
			Y -= center.Y;
			set((T)(X*cs - Y*sn), (T)(X*sn + Y*cs), Z);
			X += center.X;
			Y += center.Y;
		}

		//! Rotates the vector by a specified number of degrees around the X axis and the specified center.
		/** \param degrees: Number of degrees to rotate around the X axis.
		\param center: The center of the rotation. */
		void rotateYZBy(f64 degrees, const vector3d<T>& center=vector3d<T>())
		{
			degrees *= DEGTORAD64;
			f64 cs = cos(degrees);
			f64 sn = sin(degrees);
			Z -= center.Z;
			Y -= center.Y;
			set(X, (T)(Y*cs - Z*sn), (T)(Y*sn + Z*cs));
			Z += center.Z;
			Y += center.Y;
		}

		void rotateByVector(f64 degrees, vector3d<T> axe)
		{
			vector3d<T> dir(axe);
			dir.normalize();
			degrees *= DEGTORAD64;
			double dCos = cos(degrees);
			double dSin = sin(degrees);

			T m[9];
			m[0] = (T)((dir.X * dir.X) * (1.0f - dCos) + dCos);
			m[1] = (T)((dir.X * dir.Y) * (1.0f - dCos) - dir.Z * dSin);
			m[2] = (T)((dir.X * dir.Z) * (1.0f - dCos) + dir.Y * dSin);

			m[3] = (T)((dir.Y * dir.X) * (1.0f - dCos) + dir.Z * dSin);
			m[4] = (T)((dir.Y * dir.Y) * (1.0f - dCos) + dCos);
			m[5] = (T)((dir.Y * dir.Z) * (1.0f - dCos) - dir.X * dSin);

			m[6] = (T)((dir.Z * dir.X) * (1.0f - dCos) - dir.Y * dSin);
			m[7] = (T)((dir.Z * dir.Y) * (1.0f - dCos) + dir.X * dSin);
			m[8] = (T)((dir.Z * dir.Z) * (1.0f - dCos) + dCos);

			vector3d<T> tmp(*this);
			X = tmp.X*m[0] + tmp.Y*m[3] + tmp.Z*m[6];
			Y = tmp.X*m[1] + tmp.Y*m[4] + tmp.Z*m[7];
			Z = tmp.X*m[2] + tmp.Y*m[5] + tmp.Z*m[8];
		}

		void rotateByVector(f64 degrees, const vector3d<T>& axe) const
		{
			vector3d<T> dir(axe);
			dir.normalize();
			degrees *= DEGTORAD64;
			double dCos = cos(degrees);
			double dSin = sin(degrees);

			T m[9];
			m[0] = (T)((dir.X * dir.X) * (1.0f - dCos) + dCos);
			m[1] = (T)((dir.X * dir.Y) * (1.0f - dCos) - dir.Z * dSin);
			m[2] = (T)((dir.X * dir.Z) * (1.0f - dCos) + dir.Y * dSin);

			m[3] = (T)((dir.Y * dir.X) * (1.0f - dCos) + dir.Z * dSin);
			m[4] = (T)((dir.Y * dir.Y) * (1.0f - dCos) + dCos);
			m[5] = (T)((dir.Y * dir.Z) * (1.0f - dCos) - dir.X * dSin);

			m[6] = (T)((dir.Z * dir.X) * (1.0f - dCos) - dir.Y * dSin);
			m[7] = (T)((dir.Z * dir.Y) * (1.0f - dCos) + dir.X * dSin);
			m[8] = (T)((dir.Z * dir.Z) * (1.0f - dCos) + dCos);

			vector3d<T> tmp(*this);
			X = tmp.X*m[0] + tmp.Y*m[3] + tmp.Z*m[6];
			Y = tmp.X*m[1] + tmp.Y*m[4] + tmp.Z*m[7];
			Z = tmp.X*m[2] + tmp.Y*m[5] + tmp.Z*m[8];
		}

		// 根据两个向量获得两向量的叉积（获得的该向量垂直与两个向量构成的平面，即可以得到该平面的法向量）
		// 2014.04.11.add by zhubo
		vector3d<T>& getCrossProduct(vector3d<T> startT,vector3d<T> endT)
		{
			// 根据叉积（向量积）计算公式
			X = startT.Y * endT.Z - startT.Z * endT.Y;
			Y = startT.Z * endT.X - startT.X * endT.Z;
			Z = startT.X * endT.Y - startT.Y * endT.X;

			return *this;
		}

		// [2013/09/23 危迟]
		// 绕经过点center的任意轴vec旋转 右手系 逆时针旋转
		// 参数：degree  旋转角度
		//       center  旋转中心
		//       vec     旋转轴向量
		void RotateByVector(f64 degree,vector3d<T> center,vector3d<T> vec)
		{
			// 单位化
			vector3d<T> dir(vec);
			dir.normalize();
			degree *= DEGTORAD64;
			double dCos = cos(degree);
			double dSin = sin(degree);

			T m[16];
			m[0]  = (T)(dir.X * dir.X + (dir.Y * dir.Y + dir.Z * dir.Z)*dCos);
			m[1]  = (T)((dir.X * dir.Y)*(1 - dCos) + (dir.Z)*dSin);
			m[2]  = (T)((dir.X * dir.Z)*(1 - dCos) - (dir.Y)*dSin);
			m[3]  = (T)(0);

			m[4]  = (T)((dir.X * dir.Y)*(1 - dCos) - (dir.Z)*dSin);
			m[5]  = (T)( dir.Y * dir.Y + (dir.X * dir.X + dir.Z * dir.Z)*dCos);
			m[6]  = (T)((dir.Y * dir.Z)*(1 - dCos) + (dir.X)*dSin);
			m[7]  = (T)(0);

			m[8]  = (T)((dir.X * dir.Z)*(1 - dCos) + (dir.Y)* dSin);
			m[9]  = (T)((dir.Y * dir.Z)*(1 - dCos) - (dir.X)* dSin);
			m[10] = (T)(dir.Z * dir.Z + (dir.X * dir.X + dir.Y * dir.Y) * dCos);
			m[11] = (T)(0);

			m[12] = (T)((center.X * (dir.Y * dir.Y  + dir.Z * dir.Z) - dir.X*(center.Y * dir.Y + center.Z * dir.Z))*(1 - dCos)
				+ (center.Y * dir.Z - center.Z * dir.Y) * dSin);
			m[13] = (T)((center.Y * (dir.X * dir.X  + dir.Z * dir.Z) - dir.Y*(center.X * dir.X + center.Z * dir.Z))*(1 - dCos)  
				+ (center.Z * dir.X - center.X * dir.Z) * dSin);
			m[14] = (T)((center.Z * (dir.X * dir.X  + dir.Y * dir.Y) - dir.Z*(center.X * dir.X + center.Y * dir.Y))*(1 - dCos)  
				+ (center.X * dir.Y - center.Y * dir.X) * dSin);
			m[15] = (T)(1);

			vector3d<T> tmp(*this);

			X = tmp.X*m[0] + tmp.Y*m[4] + tmp.Z*m[8] + m[12];
			Y = tmp.X*m[1] + tmp.Y*m[5] + tmp.Z*m[9] + m[13];
			Z = tmp.X*m[2] + tmp.Y*m[6] + tmp.Z*m[10] + m[14];
		}

		// [2013/09/23 危迟]
		// 根据法向量获取共面的点
		// 参数
		void GetPlanarPoint(vector3d<T> normal,vector3d<T>& memPoint)
		{
			//vector3df tmp;
			if (normal.X != 0)
			{
				memPoint.Y = (T)1;
				memPoint.Z = (T)1;
				memPoint.X = (T)(X  - ((memPoint.Y - Y)*normal.Y + (memPoint.Z - Z)*normal.Z)/normal.X);
			}
			else if (normal.Y != 0)
			{
				memPoint.X = (T)1;
				memPoint.Z = (T)1;
				memPoint.Y = (T)(Y  - ((memPoint.X - X)*normal.X + (memPoint.Z - Z)*normal.Z)/normal.Y);
			}
			else if (normal.Z != 0)
			{
				memPoint.X = (T)1;
				memPoint.Y = (T)1;
				memPoint.Z = (T)(Z  - ((memPoint.X - X)*normal.X + (memPoint.Y - Y)*normal.Y)/normal.Z);
			}
		}
		//! Creates an interpolated vector between this vector and another vector.
		/** \param other The other vector to interpolate with.
		\param d Interpolation value between 0.0f (all the other vector) and 1.0f (all this vector).
		Note that this is the opposite direction of interpolation to getInterpolated_quadratic()
		\return An interpolated vector.  This vector is not modified. */
		vector3d<T> getInterpolated(const vector3d<T>& other, f64 d) const
		{
			const f64 inv = 1.0 - d;
			return vector3d<T>((T)(other.X*inv + X*d), (T)(other.Y*inv + Y*d), (T)(other.Z*inv + Z*d));
		}

		//! Creates a quadratically interpolated vector between this and two other vectors.
		/** \param v2 Second vector to interpolate with.
		\param v3 Third vector to interpolate with (maximum at 1.0f)
		\param d Interpolation value between 0.0f (all this vector) and 1.0f (all the 3rd vector).
		Note that this is the opposite direction of interpolation to getInterpolated() and interpolate()
		\return An interpolated vector. This vector is not modified. */
		vector3d<T> getInterpolated_quadratic(const vector3d<T>& v2, const vector3d<T>& v3, f64 d) const
		{
			// this*(1-d)*(1-d) + 2 * v2 * (1-d) + v3 * d * d;
			const f64 inv = (T) 1.0 - d;
			const f64 mul0 = inv * inv;
			const f64 mul1 = (T) 2.0 * d * inv;
			const f64 mul2 = d * d;

			return vector3d<T> ((T)(X * mul0 + v2.X * mul1 + v3.X * mul2),
					(T)(Y * mul0 + v2.Y * mul1 + v3.Y * mul2),
					(T)(Z * mul0 + v2.Z * mul1 + v3.Z * mul2));
		}

		//! Sets this vector to the linearly interpolated vector between a and b.
		/** \param a first vector to interpolate with, maximum at 1.0f
		\param b second vector to interpolate with, maximum at 0.0f
		\param d Interpolation value between 0.0f (all vector b) and 1.0f (all vector a)
		Note that this is the opposite direction of interpolation to getInterpolated_quadratic()
		*/
		vector3d<T>& interpolate(const vector3d<T>& a, const vector3d<T>& b, f64 d)
		{
			X = (T)((f64)b.X + ( ( a.X - b.X ) * d ));
			Y = (T)((f64)b.Y + ( ( a.Y - b.Y ) * d ));
			Z = (T)((f64)b.Z + ( ( a.Z - b.Z ) * d ));
			return *this;
		}

		//! 返回角度值在0-180度之间 
		T getAngleTo(const vector3d<T>& a)
		{
			T numerator = dotProduct(a);
			T denominator = getLength()*(a.getLength());

			const f64 angle = acos((f64)numerator/(f64)denominator) * RADTODEG64;

			return (T)angle;
		}
		//! Get the rotations that would make a (0,0,1) direction vector point in the same direction as this direction vector.
		/** Thanks to Arras on the Irrlicht forums for this method.  This utility method is very useful for
		orienting scene nodes towards specific targets.  For example, if this vector represents the difference
		between two scene nodes, then applying the result of getHorizontalAngle() to one scene node will point
		it at the other one.
		Example code:
		// Where target and seeker are of type ISceneNode*
		const vector3df toTarget(target->getAbsolutePosition() - seeker->getAbsolutePosition());
		const vector3df requiredRotation = toTarget.getHorizontalAngle();
		seeker->setRotation(requiredRotation);

		\return A rotation vector containing the X (pitch) and Y (raw) rotations (in degrees) that when applied to a
		+Z (e.g. 0, 0, 1) direction vector would make it point in the same direction as this vector. The Z (roll) rotation
		is always 0, since two Euler rotations are sufficient to point in any given direction. */
		// 求取将+Z如(0,0,1)向量旋转至任意向量，需要两个角度，这两个角度并非都是绕相应坐标轴的角度
		// 函数返回一个vector3df,第三个角度Z值总是0 [2013/10/22 危迟]
		vector3d<T> getHorizontalAngle() const
		{
			vector3d<T> angle;

			const f64 tmp = (atan2((f64)X, (f64)Z) * RADTODEG64);
			angle.Y = (T)tmp;

			if (angle.Y < 0)
				angle.Y += 360;
			if (angle.Y >= 360)
				angle.Y -= 360;

			const f64 z1 = core::squareroot(X*X + Z*Z);

			angle.X = (T)(atan2((f64)z1, (f64)Y) * RADTODEG64 - 90.0);

			if (angle.X < 0)
				angle.X += 360;
			if (angle.X >= 360)
				angle.X -= 360;

			return angle;
		}

		vector3d<T> getHorizontalAngleOld() const
		{
			vector3d<T> angle;

			const f64 tmp = (atan2((f64)X, (f64)Z) * RADTODEG64);
			angle.Y = (T)tmp;

			if (angle.Y < 0)
				angle.Y += 360;
			if (angle.Y >= 360)
				angle.Y -= 360;

			const f64 z1 = core::squareroot(X*X + Z*Z);

			angle.X = (T)(atan2((f64)z1, (f64)Y) * RADTODEG64 - 90.0);

			if (angle.X < 0)
				angle.X += 360;
			if (angle.X >= 360)
				angle.X -= 360;
			//gsl 2012/07/12 增加与Z轴夹角
			const f64 z2 = core::squareroot(Y*Y + X*X);
			angle.Z = (T)(atan2((f64)z2, (f64)Z) * RADTODEG64 - 90.0);

			if (angle.Z < 0)
				angle.Z += 360;
			if (angle.Z >= 360)
				angle.Z -= 360;

			return angle;
		}
		//! Get the spherical coordinate angles
		/** This returns Euler degrees for the point represented by
		this vector.  The calculation assumes the pole at (0,1,0) and
		returns the angles in X and Y.
		*/
		vector3d<T> getSphericalCoordinateAngles()
		{
			vector3d<T> angle;
			const f64 length = X*X + Y*Y + Z*Z;

			if (length)
			{
				if (X!=0)
				{
					angle.Y = (T)(atan2((f64)Z,(f64)X) * RADTODEG64);
				}
				else if (Z<0)
					angle.Y=180;

				angle.X = (T)(acos(Y * core::reciprocal_squareroot(length)) * RADTODEG64);
			}
			return angle;
		}

		//! Builds a direction vector from (this) rotation vector.
		/** This vector is assumed to be a rotation vector composed of 3 Euler angle rotations, in degrees.
		The implementation performs the same calculations as using a matrix to do the rotation.

		\param[in] forwards  The direction representing "forwards" which will be rotated by this vector.
		If you do not provide a direction, then the +Z axis (0, 0, 1) will be assumed to be forwards.
		\return A direction vector calculated by rotating the forwards direction by the 3 Euler angles
		(in degrees) represented by this vector. */
		vector3d<T> rotationToDirection(const vector3d<T> & forwards = vector3d<T>(0, 0, 1)) const
		{
			const f64 cr = cos( core::DEGTORAD64 * X );
			const f64 sr = sin( core::DEGTORAD64 * X );
			const f64 cp = cos( core::DEGTORAD64 * Y );
			const f64 sp = sin( core::DEGTORAD64 * Y );
			const f64 cy = cos( core::DEGTORAD64 * Z );
			const f64 sy = sin( core::DEGTORAD64 * Z );

			const f64 srsp = sr*sp;
			const f64 crsp = cr*sp;

			const f64 pseudoMatrix[] = {
				( cp*cy ), ( cp*sy ), ( -sp ),
				( srsp*cy-cr*sy ), ( srsp*sy+cr*cy ), ( sr*cp ),
				( crsp*cy+sr*sy ), ( crsp*sy-sr*cy ), ( cr*cp )};

			return vector3d<T>(
				(T)(forwards.X * pseudoMatrix[0] +
					forwards.Y * pseudoMatrix[3] +
					forwards.Z * pseudoMatrix[6]),
				(T)(forwards.X * pseudoMatrix[1] +
					forwards.Y * pseudoMatrix[4] +
					forwards.Z * pseudoMatrix[7]),
				(T)(forwards.X * pseudoMatrix[2] +
					forwards.Y * pseudoMatrix[5] +
					forwards.Z * pseudoMatrix[8]));
		}

		//! Fills an array of 4 values with the vector data (usually floats).
		/** Useful for setting in shader constants for example. The fourth value
		will always be 0. */
		void getAs4Values(T* array) const
		{
			array[0] = X;
			array[1] = Y;
			array[2] = Z;
			array[3] = 0;
		}

		//! X coordinate of the vector
		T X;

		//! Y coordinate of the vector
		T Y;

		//! Z coordinate of the vector
		T Z;
	};

	//! partial specialization for integer vectors
	// Implementor note: inline keyword needed due to template specialization for s32. Otherwise put specialization into a .cpp
	template <>
	inline vector3d<s32> vector3d<s32>::operator /(s32 val) const {return core::vector3d<s32>(X/val,Y/val,Z/val);}
	template <>
	inline vector3d<s32>& vector3d<s32>::operator /=(s32 val) {X/=val;Y/=val;Z/=val; return *this;}

	template <>
	inline vector3d<s32> vector3d<s32>::getSphericalCoordinateAngles()
	{
		vector3d<s32> angle;
		const f64 length = X*X + Y*Y + Z*Z;

		if (length)
		{
			if (X!=0)
			{
				angle.Y = round32((f32)(atan2((f64)Z,(f64)X) * RADTODEG64));
			}
			else if (Z<0)
				angle.Y=180;

			angle.X = round32((f32)(acos(Y * core::reciprocal_squareroot(length)) * RADTODEG64));
		}
		return angle;
	}

	//! Typedef for a f32 3d vector.
	typedef vector3d<f32> vector3df;

	//! Typedef for an integer 3d vector.
	typedef vector3d<s32> vector3di;

	//! 定义double型的三维向量
	typedef vector3d<f64> vector3dd;

	//! Function multiplying a scalar and a vector component-wise.
	template<class S, class T>
	vector3d<T> operator*(const S scalar, const vector3d<T>& vector) { return vector*scalar; }

} // end namespace core
} // end namespace hd

#endif

