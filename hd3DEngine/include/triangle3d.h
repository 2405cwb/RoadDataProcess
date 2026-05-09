// Copyright (C) 2002-2010 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_TRIANGLE_3D_H_INCLUDED__
#define __IRR_TRIANGLE_3D_H_INCLUDED__

#include "vector3d.h"
#include "line3d.h"
#include "plane3d.h"
#include "aabbox3d.h"
#include "vector2d.h"

namespace irr
{
namespace core
{

	//! 3d triangle template class for doing collision detection and other things.
	template <class T>
	class triangle3d
	{
	public:

		//! Constructor for an all 0 triangle
		triangle3d() {}
		//! Constructor for triangle with given three vertices
		triangle3d(vector3d<T> v1, vector3d<T> v2, vector3d<T> v3) : pointA(v1), pointB(v2), pointC(v3) {}

		//! Equality operator
		bool operator==(const triangle3d<T>& other) const
		{
			return other.pointA==pointA && other.pointB==pointB && other.pointC==pointC;
		}

		//! Inequality operator
		bool operator!=(const triangle3d<T>& other) const
		{
			return !(*this==other);
		}

		//! Determines if the triangle is totally inside a bounding box.
		/** \param box Box to check.
		\return True if triangle is within the box, otherwise false. */
		bool isTotalInsideBox(const aabbox3d<T>& box) const
		{
			return (box.isPointInside(pointA) &&
				box.isPointInside(pointB) &&
				box.isPointInside(pointC));
		}

		//! Determines if the triangle is totally outside a bounding box.
		/** \param box Box to check.
		\return True if triangle is outside the box, otherwise false. */
		bool isTotalOutsideBox(const aabbox3d<T>& box) const
		{
			return ((pointA.X > box.MaxEdge.X && pointB.X > box.MaxEdge.X && pointC.X > box.MaxEdge.X) ||

				(pointA.Y > box.MaxEdge.Y && pointB.Y > box.MaxEdge.Y && pointC.Y > box.MaxEdge.Y) ||
				(pointA.Z > box.MaxEdge.Z && pointB.Z > box.MaxEdge.Z && pointC.Z > box.MaxEdge.Z) ||
				(pointA.X < box.MinEdge.X && pointB.X < box.MinEdge.X && pointC.X < box.MinEdge.X) ||
				(pointA.Y < box.MinEdge.Y && pointB.Y < box.MinEdge.Y && pointC.Y < box.MinEdge.Y) ||
				(pointA.Z < box.MinEdge.Z && pointB.Z < box.MinEdge.Z && pointC.Z < box.MinEdge.Z));
		}

		//! Get the closest point on a triangle to a point on the same plane.
		/** \param p Point which must be on the same plane as the triangle.
		\return The closest point of the triangle */
		core::vector3d<T> closestPointOnTriangle(const core::vector3d<T>& p) const
		{
			const core::vector3d<T> rab = line3d<T>(pointA, pointB).getClosestPoint(p);
			const core::vector3d<T> rbc = line3d<T>(pointB, pointC).getClosestPoint(p);
			const core::vector3d<T> rca = line3d<T>(pointC, pointA).getClosestPoint(p);

			const T d1 = rab.getDistanceFrom(p);
			const T d2 = rbc.getDistanceFrom(p);
			const T d3 = rca.getDistanceFrom(p);

			if (d1 < d2)
				return d1 < d3 ? rab : rca;

			return d2 < d3 ? rbc : rca;
		}

		//! Check if a point is inside the triangle (border-points count also as inside)
		/** NOTE: When working with T='int' you should prefer isPointInsideFast, as 
		isPointInside will run into number-overflows already with coordinates in the 3-digit-range.
		\param p Point to test. Assumes that this point is already
		on the plane of the triangle.
		\return True if the point is inside the triangle, otherwise false. */
		bool isPointInside(const vector3d<T>& p) const
		{
			return (isOnSameSide(p, pointA, pointB, pointC) &&
 				isOnSameSide(p, pointB, pointA, pointC) &&
 				isOnSameSide(p, pointC, pointA, pointB));
		}

		//! Check if a point is inside the triangle (border-points count also as inside)
		/** This method uses a barycentric coordinate system. 
		It is faster than isPointInside but is more susceptible to floating point rounding 
		errors. This will especially be noticable when the FPU is in single precision mode 
		(which is for example set on default by Direct3D).
		\param p Point to test. Assumes that this point is already
		on the plane of the triangle.
		\return True if point is inside the triangle, otherwise false. */
		bool isPointInsideFast(const vector3d<T>& p) const
		{
			const vector3d<T> a = pointC - pointA;
			const vector3d<T> b = pointB - pointA;
			const vector3d<T> c = p - pointA;
			
			const f64 dotAA = a.dotProduct( a);
			const f64 dotAB = a.dotProduct( b);
			const f64 dotAC = a.dotProduct( c);
			const f64 dotBB = b.dotProduct( b);
			const f64 dotBC = b.dotProduct( c);
			 
			// get coordinates in barycentric coordinate system
			const f64 invDenom =  1/(dotAA * dotBB - dotAB * dotAB); 
			const f64 u = (dotBB * dotAC - dotAB * dotBC) * invDenom;
			const f64 v = (dotAA * dotBC - dotAB * dotAC ) * invDenom;
		 
			// We count border-points as inside to keep downward compatibility.
			// That's why we use >= and <= instead of > and < as more commonly seen on the web.
			return (u >= 0) && (v >= 0) && (u + v <= 1);

		}


		//! Get an intersection with a 3d line.
		/** \param line Line to intersect with.
		\param outIntersection Place to store the intersection point, if there is one.
		\return True if there was an intersection, false if not. */
		bool getIntersectionWithLimitedLine(const line3d<T>& line,
			vector3d<T>& outIntersection) const
		{
			return getIntersectionWithLine(line.start,
				line.getVector(), outIntersection) &&
				outIntersection.isBetweenPoints(line.start, line.end);
		}


		//! Get an intersection with a 3d line.
		/** Please note that also points are returned as intersection which
		are on the line, but not between the start and end point of the line.
		If you want the returned point be between start and end
		use getIntersectionWithLimitedLine().
		\param linePoint Point of the line to intersect with.
		\param lineVect Vector of the line to intersect with.
		\param outIntersection Place to store the intersection point, if there is one.
		\return True if there was an intersection, false if there was not. */
		bool getIntersectionWithLine(const vector3d<T>& linePoint,
			const vector3d<T>& lineVect, vector3d<T>& outIntersection) const
		{
			if (getIntersectionOfPlaneWithLine(linePoint, lineVect, outIntersection))
				return isPointInside(outIntersection);

			return false;
		}


		//! Calculates the intersection between a 3d line and the plane the triangle is on.
		/** \param lineVect Vector of the line to intersect with.
		\param linePoint Point of the line to intersect with.
		\param outIntersection Place to store the intersection point, if there is one.
		\return True if there was an intersection, else false. */
		bool getIntersectionOfPlaneWithLine(const vector3d<T>& linePoint,
			const vector3d<T>& lineVect, vector3d<T>& outIntersection) const
		{
			const vector3d<T> normal = getNormal().normalize();
			T t2;

			if ( core::iszero ( t2 = normal.dotProduct(lineVect)))
				return false;

			T d = pointA.dotProduct(normal);
			T t = -(normal.dotProduct(linePoint) - d) / t2;

			if (t<0)
			{
				return false;
			}

			outIntersection = linePoint + (lineVect * t);
			return true;
		}

		// 获取三角形与共面三维直线的交点 [2014/04/18 危迟]
		// 参数：
		// linePoint-三维直线上一点
		// lineVect-三维直线的方向向量
		// outIntersectionStart-三维交线起始点
		// outIntersectionEnd-三维交线终止点
		// nInterCount-交点个数 0表示无交点 1表示有一个交点 一般是三个顶点 2表示有两个交点
		bool GetIntersectionWithCoPlananrLine(const vector3d<T>& linePoint,const vector3d<T>& lineVect, 
				vector3d<T>& outIntersectionStart,vector3d<T>& outIntersectionEnd,int& nInterCount) const
		{
			//1.判断直线在平面上的投影直线（就是它本身，但此时它是二维直线）是否与三角形有相交
			//  1.1以pointA为原点，pointA、pointB直线方向作为二维平面的x轴方向，垂直方向为y轴方向构建二维平面直角坐标系
			//  1.2以直线上一点LA到pointA、pointB的距离值作为y值，到lineAB的垂线的距离值为x值，获取其二维平面坐标
			//      以直线上一点LB到pointA、pointB的距离值作为y值，到lineAB的垂线的距离值为x值，获取其二维平面坐标
			//		通过两个二维平面坐标，即可构建出直线在AB上的投影直线

			// 构建二维平面直角坐标系
			T disAB = pointB.getDistanceFrom(pointA);

			// 三角形第一点A的平面坐标 表示二维平面坐标系原点
			core::vector2d<T> pA(0,0);

			// 三角形第二点B的平面坐标
			core::vector2d<T> pB(disAB,0);

			// 计算二维平面直角坐标系的X轴的三维直线方程
			core::line3d<T> AxisX(pointA,pointB);

			// 计算二维平面直角坐标系的Y轴的三维直线方程
			core::vector3d<T> normY = AxisX.getVector().crossProduct(getPlane().Normal);
			core::line3d<T> AxisY(pointA,pointA - normY);

			// 计算直线上一点LA的二维平面坐标
			core::vector2d<T> coorStart;
			core::vector2d<T> coorEnd;
			core::vector2d<T> pC;

			// 计算三维直线上一点A的二维平面坐标
			CalPlanarCoordinate(coorStart,linePoint,AxisX,AxisY,pointA);

			// 计算三维直线上一点B的二维平面坐标
			CalPlanarCoordinate(coorEnd,linePoint + lineVect,AxisX,AxisY,pointA);

			// 计算三角形第三点C
			CalPlanarCoordinate(pC,pointC,AxisX,AxisY,pointA);

			// 根据coorA和coorB构建二维直线
			core::line2d<T> projLine(coorStart,coorEnd);

			// 在直线上取两个点保证其范围要与超过三角形的范围
			// 这样在进行判断的时候会避免因为线段长度有限而应该相交直线判断为不相交
			core::vector2d<T> projLineVec = projLine.getVector();

			// 比较三个坐标点的最大最小x、y值
			T xmin = min_(pA.X,pB.X,pC.X);
			T xmax = max_(pA.X,pB.X,pC.X);

			T ymin = min_(pA.Y,pB.Y,pC.Y);
			T ymax = max_(pA.Y,pB.Y,pC.Y);

			T x_inter = xmax - xmin;
			T y_inter = ymax - ymin;

			xmin = xmin - x_inter;
			xmax = xmax + x_inter;
			ymin = ymin - y_inter;
			ymax = ymax + y_inter;

			core::vector2d<T> o_coorTmp(coorStart);

			// 计算坐标时 不用考虑直线的方向向量正负关系 因为不管正负关系如何 直线可以通过两点确定 
			if (projLineVec.X != 0)
			{
				coorStart.X = xmin;

				if ((o_coorTmp.X - xmin)*projLineVec.X != 0)
				{
					coorStart.Y = o_coorTmp.Y - (o_coorTmp.X - xmin)*projLineVec.Y/projLineVec.X;
				}
				else
				{
					coorStart.Y = o_coorTmp.Y;
				}

				o_coorTmp.set(coorEnd);

				coorEnd.X = xmax;

				if ((o_coorTmp.X - xmax)*projLineVec.X != 0)
				{
					coorEnd.Y = o_coorTmp.Y - (o_coorTmp.X - xmax)*projLineVec.Y/projLineVec.X;
				}
				else
				{
					coorEnd.Y = o_coorTmp.Y;
				}
			}
			else if (projLineVec.Y != 0)
			{
				coorStart.Y = ymin;

				if ((o_coorTmp.Y - ymin)*projLineVec.Y != 0)
				{
					coorStart.X = o_coorTmp.X - (o_coorTmp.Y - ymin)*projLineVec.X/projLineVec.Y;
				}
				else
				{
					coorStart.X = o_coorTmp.X;
				}

				o_coorTmp.set(coorEnd);

				coorEnd.Y = ymax;
				if ((o_coorTmp.Y - ymax)*projLineVec.Y != 0)
				{
					coorEnd.X = o_coorTmp.X - (o_coorTmp.Y - ymax)*projLineVec.X/projLineVec.Y;
				}
				else
				{
					coorEnd.X = o_coorTmp.X;
				}
			}

			// 获取此时的投影直线的起始终止点坐标
			projLine.start = coorStart;
			projLine.end = coorEnd;

			// 通过判断三个点与直线的关系来判断三角形是否与直线相交
			if ((projLine.getPointOrientation(pA) > 0 
				&& projLine.getPointOrientation(pB) > 0 
				&& projLine.getPointOrientation(pC) > 0 )
				|| (projLine.getPointOrientation(pA) < 0 
				&& projLine.getPointOrientation(pB) < 0 
				&& projLine.getPointOrientation(pC) < 0 ))
			{
				return false;
			}

			//   1.3 通过比较投影直线与三角形的二维直线是否有交点即可判断
			core::line2d<T> projLineAB(pA,pB);
			core::line2d<T> projLineAC(pA,pC);
			core::line2d<T> projLineBC(pB,pC);

			// 判断是会否与AB边相交
			bool bInterAB = false;
			bool bInterAC = false;
			bool bInterBC = false;

			// 对应三条边的交点
			core::vector2d<T> interAB;
			core::vector2d<T> interAC;
			core::vector2d<T> interBC;
			
			nInterCount = 0;

			if (projLine.intersectWith(projLineAB,interAB) && projLineAB.isPointBetweenStartAndEnd(interAB))
			{
				bInterAB = true;
				nInterCount++;
			}
			if (projLine.intersectWith(projLineAC,interAC) && projLineAC.isPointBetweenStartAndEnd(interAC))
			{
				bInterAC = true;
				nInterCount++;
			}
			if (projLine.intersectWith(projLineBC,interBC) && projLineBC.isPointBetweenStartAndEnd(interBC))
			{
				bInterBC = true;
				nInterCount++;
			}

			if (!bInterAB && !bInterAC && !bInterBC)
			{
				return false;
			}
			else
			{
				core::vector2d<T> interStart;
				core::vector2d<T> interEnd;
				// 最多有两个交点，如果大于2个交点，则存在重复点
				if (nInterCount >2)
				{
					// 设定第一个点为起始点
					interStart = interAB;

					// 如果第一个点与第二个点相同 则终止点为第三个点
					if (interAB == interBC)
					{
						interEnd = interAC;
					}
					// 如果第一个点与第三个点相同 则终止点为第二个点
					else if (interAB == interAC)
					{
						interEnd = interBC;
					}
					// 如果第二个点与第三个点相同 则终止点为任意一点均可
					else if (interBC == interAC)
					{
						interEnd = interBC;
					}
					nInterCount = 2;
				}
				// 如果相交为两个点 需要判断两个点是否为相同一点
				if (nInterCount == 2)
				{
					if (bInterAB && bInterBC)
					{
						interStart = interAB;
						interEnd = interBC;
					}
					else if (bInterAB && bInterAC)
					{
						interStart = interAB;
						interEnd = interAC;
					}
					else if (bInterBC && bInterAC)
					{
						interStart = interBC;
						interEnd = interAC;
					}

					// 如果存在两点相同 则合并同一点
					if (interStart == interEnd)
					{
						Cal3DCoordinate(interStart,outIntersectionStart);
						outIntersectionEnd = outIntersectionStart;
						nInterCount = 1;
					}
					else
					{
						Cal3DCoordinate(interStart,outIntersectionStart);
						Cal3DCoordinate(interEnd,outIntersectionEnd);

						nInterCount = 2;
					}
				}

				/*if (nInterCount == 1)
				{
					if (bInterAB)
					{
						interStart = interAB;
					}
					else if(bInterBC)
					{
						interStart = interBC;
					}
					else if (bInterAC)
					{
						interStart = interAC;
					}
					Cal3DCoordinate(interStart,outIntersectionStart);
				}*/

				return true;
			}

			return true;
		}

		// 计算三维坐标点的二维平面坐标 [2014/04/18 危迟]
		// 参数：
		// planarCoor-表示返回的二维平面坐标
		// linePoint-表示传入的三维坐标点
		// AxisX-表示坐标轴X轴的三维直线
		// AxisY-表示坐标轴Y轴的三维直线
		// oriPoint-表示坐标原点的三维坐标
		void CalPlanarCoordinate(core::vector2d<T>& planarCoor,const core::vector3d<T>& linePoint,
			const core::line3d<T>& AxisX,const core::line3d<T>& AxisY,
			const core::vector3d<T>& oriPoint) const
		{
			T dis_AY = AxisX.getDistanceToPointP(linePoint);
			dis_AY = dis_AY > 0 ? dis_AY : -dis_AY;
			T dis_AX = AxisY.getDistanceToPointP(linePoint);
			dis_AX = dis_AX > 0 ? dis_AX : -dis_AX;

			// 知道距离之后需要知道方向 通过该点与原点的连线与坐标轴的夹角来判断正负号
			core::vector3d<T> VecA = linePoint - pointA;

			T dpX = VecA.dotProduct(AxisX.getVector());
			T dpY = VecA.dotProduct(AxisY.getVector());

			if ( dpX>= 0 && dpY >= 0)
			{
				planarCoor.set(dis_AX,dis_AY);
			}
			else if(dpX <0 && dpY >=0)
			{
				planarCoor.set(-dis_AX,dis_AY);
			}
			else if (dpX < 0 && dpY < 0)
			{
				planarCoor.set(-dis_AX,-dis_AY);
			}
			else if (dpX >= 0 && dpY < 0)
			{
				planarCoor.set(dis_AX,-dis_AY);
			}
		}

		// 计算二维坐标点的三维坐标 [2014/04/18 危迟]
		// 参数
		//  planarCoor-输入的平面坐标
		//  outPoint-输出的三维坐标值
		//  备注：以PointA为原点构建二维平面直角坐标系
		void Cal3DCoordinate(const core::vector2d<T>& planarCoor,core::vector3d<T>& outPoint) const
		{
			// 计算二维平面的上的交点与原点构成的直线与x轴的夹角
			// 角度在范围[0,90]
			f64 angle = planarCoor.getAngleWith(core::vector2d<T>(9999.f,0));

			// 规划角度至[-180,180]
			if (planarCoor.X < 0 && planarCoor.Y >= 0)
			{
				angle = 180.f - angle;
			}
			else if (planarCoor.X < 0 && planarCoor.Y < 0)
			{
				angle = 180.f + angle;
			}
			else if (planarCoor.X >= 0 && planarCoor.Y < 0)
			{
				angle = 360.f - angle;
			}

			// 获取交点与原点之间的距离
			f32 dis = planarCoor.getDistanceFrom(core::vector2df(0.f,0.f));

			// 获取其在x轴上的距离点三维坐标
			core::vector3df tmpPoint = pointA + (pointB - pointA).normalize()*dis;

			core::vector3df normal = getPlane().Normal;

			// 将该点绕经过点A的垂直平面的轴顺时逆时针旋转angle角度 即可获取三维坐标
			tmpPoint.RotateByVector(angle,pointA,normal);

			outPoint = tmpPoint;
		}

		//! Get the normal of the triangle.
		/** Please note: The normal is not always normalized. */
		vector3d<T> getNormal() const
		{
			return (pointB - pointA).crossProduct(pointC - pointA);
		}

		//! Test if the triangle would be front or backfacing from any point.
		/** Thus, this method assumes a camera position from which the
		triangle is definitely visible when looking at the given direction.
		Do not use this method with points as it will give wrong results!
		\param lookDirection Look direction.
		\return True if the plane is front facing and false if it is backfacing. */
		bool isFrontFacing(const vector3d<T>& lookDirection) const
		{
			const vector3d<T> n = getNormal().normalize();
			const f32 d = (f32)n.dotProduct(lookDirection);
			return F32_LOWER_EQUAL_0(d);
		}

		//! Get the plane of this triangle.
		plane3d<T> getPlane() const
		{
			return plane3d<T>(pointA, pointB, pointC);
		}

		//! Get the area of the triangle
		T getArea() const
		{
			return (pointB - pointA).crossProduct(pointC - pointA).getLength() * 0.5f;

		}

		//! sets the triangle's points
		void set(const core::vector3d<T>& a, const core::vector3d<T>& b, const core::vector3d<T>& c)
		{
			pointA = a;
			pointB = b;
			pointC = c;
		}

		//! the three points of the triangle
		vector3d<T> pointA;
		vector3d<T> pointB;
		vector3d<T> pointC;

	private:
		bool isOnSameSide(const vector3d<T>& p1, const vector3d<T>& p2,
			const vector3d<T>& a, const vector3d<T>& b) const
		{
			vector3d<T> bminusa = b - a;
			vector3d<T> cp1 = bminusa.crossProduct(p1 - a);
			vector3d<T> cp2 = bminusa.crossProduct(p2 - a);
			return (cp1.dotProduct(cp2) >= 0.0f);
		}
	};


	//! Typedef for a f32 3d triangle.
	typedef triangle3d<f32> triangle3df;

	//! Typedef for an integer 3d triangle.
	typedef triangle3d<s32> triangle3di;

} // end namespace core
} // end namespace irr

#endif

