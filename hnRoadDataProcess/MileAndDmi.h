#pragma once
#include "..\hnProject\hnProject.h"
struct MileAndDmi
{

	MileAndDmi()
	{

	}
	MileAndDmi(hnPro::hnProject * project,double curShowMile,double realMile, double CurDmi )
	{
		this->ShowMile = curShowMile;
		if (realMile!=ShowMile)
		{
			//不相等说明是插入的  取整桩号+实际里程 仅用作显示的
			//计算真实桩号是多少 需要根据里程来计算
			ReadMile = project->enclToTrueMile(CurDmi);
		}
		else
		{
			ReadMile = ShowMile;
		}
		this->Dmi = CurDmi;
	}

	//乾通出表要求  桩号(取整桩号+实际里程) 用做显示出表
	double ShowMile;
	 
	double ReadMile;
	//真实对应的里程
	double Dmi;

};