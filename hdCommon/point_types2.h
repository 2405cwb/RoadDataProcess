/*! point_types2.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : point_types2.h
相关文件     : 
文件实现功能 : hlz点类型定义
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/01/8    1.0      龚书林    

</PRE>
*******************************************************************************/

#ifndef __HD_POINT_TYPE2_H_INCLUDED__
#define __HD_POINT_TYPE2_H_INCLUDED__

// 设置结构体成员对齐1byte
#pragma  pack(push,1)

#include <math.h>
#include <vector>
#include "..\hdCore\hdDefs.h"
#include "point_types.h"

namespace hd
{
	 // 强度属性
	 typedef u8 HdIntensity;
	 // 时间属性
	 typedef u16 HdPtTime;
	 // 颜色属性
	 union HdPtColor;
	 // 分类属性
	 struct HdPtClass;

	 
	// 包含x,y,z属性的点,64格网大小精度1/1024,对应物理文件结构
	struct HdPointXYZ
	{
		hd::u16  x,y,z;
		HdPointXYZ()
			:x(0),y(0),z(0){}

		// b起始坐标,s比例系数
		inline hd::f32 getX(hd::f32 b,hd::f32 s)
		{
			return b + x * s;
		}

		// b起始坐标,s比例系数
		inline hd::f32 getY(hd::f32 b,hd::f32 s)
		{
			return b + y * s;
		}

		// b起始坐标,s比例系数
		inline hd::f32 getZ(hd::f32 b,hd::f32 s)
		{
			return b + z * s;
		}

		// 依次按照x、y、z的优先级进行排序
// 		inline bool operator< ( const HdPointXYZ& pt)
// 		{
// 			if (x <= pt.x)
// 			{
// 				return true;
// 			}
// 			else if (x == pt.x)
// 			{
// 				if (y < pt.y)
// 				{
// 					return true;
// 				}
// 				else if (y == pt.y)
// 				{
// 					if (z > pt.z)
// 					{
// 						return true;
// 					}
// 					else if (z == pt.z)
// 					{
// 						return true;
// 					}
// 					else if (z > pt.z)
// 					{
// 						return false;
// 					}
// 				}
// 				else if (y > pt.y)
// 				{
// 					return false;
// 				}
// 
// 			}
// 			else if (x > pt.x)
// 			{
// 				return false;
// 			}
// 		}
 	};

	//包含x,y,z属性的点，坐标值使用1byte存储
	struct XYZ_S
	{
		hd::u8 x, y, z;
		XYZ_S():x(0), y(0), z(0)
		{}
	};
	
	// 颜色属性,内存结构和物理文件结构相同
	union HdPtColor
	{
		//struct
		//{
		//	hd::u8 r:5;	// R占用5bit
		//	hd::u8 g:6; // G占用6bit
		//	hd::u8 b:5; // B占用5bit
		//};
		hd::u16 color16;

		HdPtColor()
			:color16(0){}

		// 获取红色值
		inline hd::u32 getRed()
		{
			//return (b & 0xff) << 3;
			//return (color16 & 0x001F) << 3;
			return (color16 & 0xF800) >> 8;   //取高5位为Red分量  袁亮   20160625
		}
		

		// 获取绿色值	
		inline hd::u32 getGreen()
		{
			//return (g & 0xff) << 2;
			return (color16 & 0x07E0) >> 3; //第6到11位为Green分量  袁亮  20160625
		}

		// 获取蓝色值
		inline hd::u32 getBlue()
		{
			//return (color16 & 0xF800) >> 8;
			return (color16 & 0x001F) << 3; //第12到16位为Blue分量  袁亮  20160625
		}

		// 设置RGB颜色,A8R8G8B8转R5G6B5
		inline void setColor(u32 color)
		{
			color16 = (u16)(( color & 0x00F80000) >> 8 |
				( color & 0x0000FC00) >> 5 |
				( color & 0x000000F8) >> 3);
		}

		// 获取RGB颜色,A8R8G8B8
		inline u32 getColor()
		{
			return 0xFF000000 |
				((color16 & 0xF800) << 8)|
				((color16 & 0x07E0) << 5)|
				((color16 & 0x001F) << 3);
		}

		// 设置RGB颜色,A8R8G8B8转R5G6B5
		inline void setColor(u32 r,u32 g,u32 b)
		{
			/*color16 = (u16)((r & 0xF8) << 7 |
				            (g & 0xF8) << 2 |
				            (b & 0xF8) >> 3);*/    //与setColor(u32 color)等价  袁亮  20160625
			color16 = (u16)((r & 0xF8) << 8 |
							(g & 0xFC) << 3 |
							(b & 0xF8) >> 3);
		}
	};// 颜色属性end

	// 分类属性,占用8bit
	struct HdPtClass
	{
		// 选择标记1bit
		u8 isSelected:1;	
		// 删除标记1bit
		u8 isDeleted:1;
		// 保留位1bit
		u8 reserve:1;
		// 分类属性5bit
		u8 ptclass:5;
		// 构造函数
		HdPtClass()
			:isSelected(0),isDeleted(0),reserve(0),ptclass(0){}
	};

	// Hlz点结构体,对应解析后内存结构
	 struct HlzPoint
	 {
		 hd::f32  x,y,z;		// 坐标
		 hd::u8 intensity;      // 反射强度
		 hd::u16 gpsTime;		// GPS时间
		 HdPtColor color;		// 颜色
		 HdPtClass prop;		// 分类及编辑属性

		 HlzPoint()
			 :x(0.0f),y(0.0f),z(0.0f),intensity(0),
		 gpsTime(0){}

		 void setCoord(
			 const HdPointXYZ& coordi,	// 数据包中网格相对坐标
			 const PointXYZ& ll,		// 网格起始点			    
			 hd::f32 s)
		 {
			 x = ll.x + coordi.x * s;
			 x = ll.y + coordi.y * s;
			 x = ll.z + coordi.z * s;
		 }

		 bool operator< (const struct PointXYZINormal& rhs) const
		 {
			 return intensity < rhs.intensity;
		 }

		 bool operator> (const struct PointXYZINormal& rhs) const
		 {
			 return intensity > rhs.intensity;
		 }

		 // 根据坐标判读是否有效
		 inline bool isValid()
		 {
			 return !(x == 0.0f && y == 0.0f && z == 0.0f) && !prop.isDeleted;
		 }
		 // 根据强度判断是否有效
		 inline bool isValid(int min,int max) 
		 {
			 return intensity >= min && intensity <= max && !prop.isDeleted;
		 }
		 // 分类
		 inline u8 getClass()
		 {
			 return prop.ptclass;
		 }

		 //判断点是否选中
		 inline bool isSelected()const
		 {
			 return prop.isSelected;	// 选中点标记
		 }
		 //判断点是否删除
		 inline bool isDeleted()const
		 {
			 return prop.isDeleted;	// 删除点标记
		 }
		 // 设置选中
		 inline void setSelected()
		 {
			 prop.isSelected = 1;
		 }
		 // 设置反选
		 inline void setXorSelected()
		 {
			 prop.isSelected = (prop.isSelected ^ 1);
		 }
		 // 设置取消选中
		 inline void setUnSelected()
		 {
			 prop.isSelected = 0;
		 }

		 // 设置删除
		 inline void setDeleted()
		 {
			 prop.isDeleted = 1;
		 }
	 };

}

//恢复默认结构体对齐
#pragma  pack(pop)

#endif