 
#pragma once
#include "stdafx.h" 
namespace hnCommon
{
	template <class T>
	class hn3dDiseaseDef
	{
	public: 
	
		hn3dDiseaseDef(T d0, T d1, T d2,T d3 , const char* disesaseName,double diseaseMile)
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
	private: hn3dDiseaseDef() {}
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
		~hn3dDiseaseDef()
		{
			delete[] s_diseaseName;
		}
	};
	//定义double对象

	//线状病害
	template <class T>
	class hn3dDiseaseLineDef
	{
	public:

		hn3dDiseaseLineDef(std::vector<T>  ps, const char* disesaseName, double diseaseMile,bool isLine)
		{
			this->ps = ps;
			this->isLine = isLine;
			int length = strlen(disesaseName);
			s_diseaseName = new char[length + 1];
			strcpy(s_diseaseName, disesaseName);

			this->d_diseaseMile = diseaseMile;
		}
	private: hn3dDiseaseLineDef() {}
	public:
		//点集合
		std::vector<T>  ps;
		bool isLine;
		char* s_diseaseName;
		double d_diseaseMile;
		~hn3dDiseaseLineDef()
		{
			delete[] s_diseaseName;
		}
	};

} 

