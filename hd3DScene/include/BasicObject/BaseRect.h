
#pragma once
#include "BaseVector2d.h"
#include "hdBasicObject.h"


//! Rectangle template.
/** Mostly used by 2D GUI elements and for 2D drawing methods.
It has 2 positions instead of position and dimension and a fast
method for collision detection with other rectangles and points.

Coordinates are (0,0) for top-left corner, and increasing to the right
and to the bottom.
*/
template <class T>
class CBaseRect:public CHdBasicObject
{
public:

	//! Default constructor creating empty rectangle at (0,0)
	CBaseRect():UpperLeftCorner(0,0), LowerRightCorner(0,0) {m_eType =  E_TID_RECT;}

	//! Constructor with two corners
	CBaseRect(T x, T y, T x2, T y2)
		: UpperLeftCorner(x,y), LowerRightCorner(x2,y2) {m_eType =  E_TID_RECT;}

	//! Constructor with two corners
	CBaseRect(const CBaseVector2d<T>& upperLeft, const CBaseVector2d<T>& lowerRight)
		: UpperLeftCorner(upperLeft), LowerRightCorner(lowerRight) {m_eType =  E_TID_RECT;}

	//! Constructor with upper left corner and dimension
	/*template <class U>
	CHdRect(const CHdPosition2d<T>& pos, const dimension2d<U>& size)
	: UpperLeftCorner(pos), LowerRightCorner(pos.X + size.Width, pos.Y + size.Height) {}*/

	//! move right by given numbers
	CBaseRect<T> operator+(const CBaseVector2d<T>& pos) const
	{
		CBaseRect<T> ret(*this);
		return ret+=pos;
	}

	//! move right by given numbers
	CBaseRect<T>& operator+=(const CBaseVector2d<T>& pos)
	{
		UpperLeftCorner += pos;
		LowerRightCorner += pos;
		return *this;
	}

	//! move left by given numbers
	CBaseRect<T> operator-(const CBaseVector2d<T>& pos) const
	{
		CBaseRect<T> ret(*this);
		return ret-=pos;
	}

	//! move left by given numbers
	CBaseRect<T>& operator-=(const CBaseVector2d<T>& pos)
	{
		UpperLeftCorner -= pos;
		LowerRightCorner -= pos;
		return *this;
	}

	//! equality operator
	bool operator==(const CBaseRect<T>& other) const
	{
		return (UpperLeftCorner == other.UpperLeftCorner &&
			LowerRightCorner == other.LowerRightCorner);
	}

	//! inequality operator
	bool operator!=(const CBaseRect<T>& other) const
	{
		return (UpperLeftCorner != other.UpperLeftCorner ||
			LowerRightCorner != other.LowerRightCorner);
	}

	//! compares size of rectangles
	bool operator<(const CBaseRect<T>& other) const
	{
		return getArea() < other.getArea();
	}

	//! Returns size of rectangle
	T getArea() const
	{
		return getWidth() * getHeight();
	}

	//! Returns if a 2d point is within this rectangle.
	/** \param pos Position to test if it lies within this rectangle.
	\return True if the position is within the rectangle, false if not. */
	bool isPointInside(const CBaseVector2d<T>& pos) const
	{
		return (UpperLeftCorner.X <= pos.X &&
			UpperLeftCorner.Y >= pos.Y &&
			LowerRightCorner.X >= pos.X &&
			LowerRightCorner.Y <= pos.Y);
	}

	//! Check if the rectangle collides with another rectangle.
	/** \param other Rectangle to test collision with
	\return True if the rectangles collide. */
	bool isRectCollided(const CBaseRect<T>& other) const
	{
		return (LowerRightCorner.Y < other.UpperLeftCorner.Y &&
			UpperLeftCorner.Y > other.LowerRightCorner.Y &&
			LowerRightCorner.X > other.UpperLeftCorner.X &&
			UpperLeftCorner.X < other.LowerRightCorner.X);
	}

	// is contain another rect
	bool isContain(const CBaseRect<T>& other) const
	{
		return (UpperLeftCorner.X<=other.UpperLeftCorner.X && 
				UpperLeftCorner.Y>=other.UpperLeftCorner.Y &&
				LowerRightCorner.X>=other.LowerRightCorner.X &&
				LowerRightCorner.Y<=other.LowerRightCorner.Y
				);
	}

	//! Clips this rectangle with another one.
	/** \param other Rectangle to clip with */
	void clipAgainst(const CBaseRect<T>& other)
	{
		if (other.LowerRightCorner.X < LowerRightCorner.X)
			LowerRightCorner.X = other.LowerRightCorner.X;
		if (other.LowerRightCorner.Y < LowerRightCorner.Y)
			LowerRightCorner.Y = other.LowerRightCorner.Y;

		if (other.UpperLeftCorner.X > UpperLeftCorner.X)
			UpperLeftCorner.X = other.UpperLeftCorner.X;
		if (other.UpperLeftCorner.Y > UpperLeftCorner.Y)
			UpperLeftCorner.Y = other.UpperLeftCorner.Y;

		// correct possible invalid CHdRect resulting from clipping
		if (UpperLeftCorner.Y > LowerRightCorner.Y)
			UpperLeftCorner.Y = LowerRightCorner.Y;
		if (UpperLeftCorner.X > LowerRightCorner.X)
			UpperLeftCorner.X = LowerRightCorner.X;
	}

	//! Moves this rectangle to fit inside another one.
	/** \return True on success, false if not possible */
	bool constrainTo(const CBaseRect<T>& other)
	{
		if (other.getWidth() < getWidth() || other.getHeight() < getHeight())
			return false;

		T diff = other.LowerRightCorner.X - LowerRightCorner.X;
		if (diff < 0)
		{
			LowerRightCorner.X += diff;
			UpperLeftCorner.X  += diff;
		}

		diff = other.LowerRightCorner.Y - LowerRightCorner.Y;
		if (diff < 0)
		{
			LowerRightCorner.Y += diff;
			UpperLeftCorner.Y  += diff;
		}

		diff = UpperLeftCorner.X - other.UpperLeftCorner.X;
		if (diff < 0)
		{
			UpperLeftCorner.X  -= diff;
			LowerRightCorner.X -= diff;
		}

		diff = UpperLeftCorner.Y - other.UpperLeftCorner.Y;
		if (diff < 0)
		{
			UpperLeftCorner.Y  -= diff;
			LowerRightCorner.Y -= diff;
		}

		return true;
	}

	void insetRect(const CBaseRect<T>& other)
	{
		addInternalPoint(other.UpperLeftCorner);
		addInternalPoint(other.LowerRightCorner);
	}

	//! Get width of rectangle.
	T getWidth() const
	{
		return LowerRightCorner.X - UpperLeftCorner.X;
	}

	//! Get height of rectangle.
	T getHeight() const
	{
		return  UpperLeftCorner.Y - LowerRightCorner.Y ;
	}

	//! If the lower right corner of the CHdRect is smaller then the upper left, the points are swapped.
	void repair()
	{
		if (LowerRightCorner.X < UpperLeftCorner.X)
		{
			T t = LowerRightCorner.X;
			LowerRightCorner.X = UpperLeftCorner.X;
			UpperLeftCorner.X = t;
		}

		if (LowerRightCorner.Y > UpperLeftCorner.Y)
		{
			T t = LowerRightCorner.Y;
			LowerRightCorner.Y = UpperLeftCorner.Y;
			UpperLeftCorner.Y = t;
		}
	}

	//! Returns if the CHdRect is valid to draw.
	/** It would be invalid if the UpperLeftCorner is lower or more
	right than the LowerRightCorner. */
	bool isValid() const
	{
		return ((LowerRightCorner.X >= UpperLeftCorner.X) &&
			(LowerRightCorner.Y >= UpperLeftCorner.Y));
	}

	//! Get the center of the rectangle
	CBaseVector2d<T> getCenter() const
	{
		return CBaseVector2d<T>(
				(UpperLeftCorner.X + LowerRightCorner.X) / 2,
				(UpperLeftCorner.Y + LowerRightCorner.Y) / 2);
	}


	//! Adds a point to the rectangle
	/** Causes the rectangle to grow bigger if point is outside of
	the box
	\param p Point to add to the box. */
	void addInternalPoint(const CBaseVector2d<T>& p)
	{
		addInternalPoint(p.X, p.Y);
	}

	//! Adds a point to the bounding rectangle
	/** Causes the rectangle to grow bigger if point is outside of
	the box
	\param x X-Coordinate of the point to add to this box.
	\param y Y-Coordinate of the point to add to this box. */
	void addInternalPoint(T x, T y)
	{
		if (x>LowerRightCorner.X)
			LowerRightCorner.X = x;
		if (y<LowerRightCorner.Y)
			LowerRightCorner.Y = y;

		if (x<UpperLeftCorner.X)
			UpperLeftCorner.X = x;
		if (y>UpperLeftCorner.Y)
			UpperLeftCorner.Y = y;
	}
	
	T GetMinX() const
	{
		return UpperLeftCorner.X;		
	}

	T GetMaxX() const
	{
		return LowerRightCorner.X;
	}
	
	T GetMinY() const
	{
		return LowerRightCorner.Y;		
	}

	T GetMaxY() const
	{
		return UpperLeftCorner.Y;
	}

	// 设置左下的坐标
	void SetLB(T x, T y)
	{
		UpperLeftCorner.X= x;
		LowerRightCorner.Y = y;
	}

	// 设置右上的坐标
	void SetUR(T x, T y)
	{
		LowerRightCorner.X= x;
		UpperLeftCorner.Y = y;
	}

	//! Upper left corner
	CBaseVector2d<T> UpperLeftCorner;
	//! Lower right corner
	CBaseVector2d<T> LowerRightCorner;
};

//! Rectangle with float values
typedef CBaseRect<float> CHdRectf,Chd2DBoundingBoxf;
//! Rectangle with double values
typedef CBaseRect<double> CHdRectd,Chd2DBoundingBoxd;
//! Rectangle with int values
typedef CBaseRect<int> recti,Chd2DBoundingBoxi;



