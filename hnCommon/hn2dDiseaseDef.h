
#pragma once
#include "stdafx.h" 
namespace hnCommon
{
	template <class T>
	class hn2dDiseaseDef
	{
	public:

		hn2dDiseaseDef(T d0, T d1, T d2, T d3, const char* disesaseName, double diseaseMile)
		{
			this->d0 = d0;
			this->d1 = d1;
			this->d2 = d2;
			this->d3 = d3;
			int length = strlen(disesaseName);
			s_diseaseName = new char[length + 1];
			strcpy(s_diseaseName, disesaseName);

			this->d_diseaseMile = diseaseMile;
		}
	private: hn2dDiseaseDef() {}
	public:
		// 左上坐标
		T d0;

		// 右上坐标
		T d1;

		// 左下坐标
		T d2;

		// 右下坐标
		T d3;

		char* s_diseaseName;
		double d_diseaseMile;
		~hn2dDiseaseDef()
		{
			delete[] s_diseaseName;
		}
	};  
	class hn2dGpsPoint
	{
	public:
		hn2dGpsPoint() { x = 0; y = 0;  z = 0; }
		hn2dGpsPoint(double nx, double ny,double nz)
		{
			x = nx;
			y = ny;
			z = nz;
		}

		hn2dGpsPoint(const hn2dGpsPoint& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
		}

		hn2dGpsPoint & operator=(const hn2dGpsPoint & other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			return *this;
		}

	public:
		// x坐标
		double x;

		// y坐标
		double y;

		double z;
	};
}

