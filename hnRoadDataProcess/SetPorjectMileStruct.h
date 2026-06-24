#pragma once
#include <QString>
struct SetPorjectMileStruct
{ 
	SetPorjectMileStruct()
	{

	}
	SetPorjectMileStruct(long long prt,int roadIndex,int projectIndex , QString name,double sMile,double eMile,bool changed)
	{
		this->changed = changed;
		this->ProjectPtr = prt;
		this->Emile = eMile;
		this->ProjectName = name;
		this->Smile = sMile;
		this->RoadIndex = roadIndex;
		this->ProjectIndex = projectIndex;
	}
	bool operator==(const SetPorjectMileStruct& other) const
	{
		bool equal = RoadIndex == other.RoadIndex &&
			ProjectIndex == other.ProjectIndex && ProjectName == other.ProjectName;
		return equal;
	}
	//工程名称
	QString ProjectName;

	//工程起点桩号
	double Smile;

	//工程终点
	double Emile;

	//工程所在路线在用户导入的所有路段中的index
	int  RoadIndex;

	//工程在路段中的index
	int ProjectIndex;

	long long  ProjectPtr;

	bool changed;
};