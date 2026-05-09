#pragma once
//出表需要计算的数据列表结构体
namespace MyQtCommon
{
	typedef  struct _HN_EQUIPMENT_
	{
		_HN_EQUIPMENT_()
		{
			     IRI=false;
		 		 
				JUMP=false;
		 			 
				 RUT=false;
		 		 
				ROAD=false;
		 			 
			  STREET=false;
			 
				SMTD = false;
			 
				MPD = false;
		 
				JHXX = false;

				SPEED = false;
				
				GPS = false;
		}
		//平整度
		bool IRI;
		//跳车
		bool JUMP;
		//车辙
		bool RUT;
		//道路破损
		bool ROAD;
		//景观
		bool STREET;
		//构造深度SMTD
		bool SMTD;
		//构造深度MPD
		bool MPD;
		//几何线型
		bool JHXX;
		//车速
		bool SPEED;

		//定位信息
		bool GPS;

	}MyEquipment;
}
