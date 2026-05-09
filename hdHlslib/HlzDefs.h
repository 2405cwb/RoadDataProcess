/*! HlzDefs.h
********************************************************************************
<PRE>
模块名       : hdHLSLib
文件名       : HlzDefs.h
相关文件     : 
文件实现功能 : 海达数云点文件hlz读写模块基本类型定义
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              创建
2015/01/08   1.0      龚书林    
</PRE>
*******************************************************************************/

#pragma once
#include <map>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <Windows.h>

#include "..\hdCore\hdMath.h"
#include "..\hdCore\hdBox3d.h"
#include "inc\mydefs.hpp"

using namespace std;
namespace hd
{
#pragma  pack(push,1)

	// 前置声明
	class CHdBlockset;
	class CHdBlock;
	class CHdParcel;
	
	// 坐标格网相对坐标换算参数
	const f32 COORD_REF = 1.0f/65535.0f;
	// 抽稀比例
	const I32 SIMPLE_SCALE = 6;

	//! 数据地址结构体
	struct HdAddr
	{
		// 地址高位
		U16 high;
		// 地址地位
		U32 low;

		HdAddr()
			:high(0xffff),low(0xffffffff){}

		// 判断地址是否有效
		inline bool IsInValid()
		{
			return (high == 0xffff) && (low == 0xffffffff); 
		}
		// 获得绝对地址
		inline U64 GetAddress()
		{
			return (U64)(((U64)high << 32) | low);
		}
		// 拆分解析绝对地址
		inline void Prase(U64 address)
		{
			high = (U16)((address & 0xffff00000000) >> 32);
			low =  (address & 0xffffffff);
		}

		// 根据地址解析文件编号及地址
		inline U16 GetFileNo()
		{
			return (U16)(GetAddress()/((U32)0x80000000));
		}

		// 获取所在文件的地址
		inline U64 GetAddrInFile()
		{
			return (U64)(GetAddress()%((U32)0x80000000));
		}
	};

	
	// 块集结构体
	struct HdLevel
	{
		// 显示比例尺,比例尺分母小于等于dispScale才显示
		U32 dispScale;
		// 有效块集个数
		U32	numBlockset;
		// X方向划分块集数
		U16 numBlocksetX;
		// Y方向划分块集数
		U16 numBlocksetY;
		// Z方向划分块集数
		U16 numBlocksetZ;
		// 块集x方向边长
		F32 sizeX;
		// 块集y方向边长
		F32 sizeY;
		// 块集z方向边长
		F32 sizeZ;
		// 块集首地址
		HdAddr offsetBlockset;

		// 该层内点数
		U64 pointNum;

		HdLevel()
			:numBlockset(0),numBlocksetX(1),numBlocksetY(1),numBlocksetZ(1),dispScale(1),
		     sizeX(64.0f),sizeY(64.0f),sizeZ(64.0f),pointNum(0){}
		
	};

	// 包结构体
	struct HdParcel
	{
		// 当前包点数
		U64 numPoint;
		// 空间范围
		CHdBox3df box;
		// 坐标数据地址
		HdAddr addrCoord;
		// 强度数据地址
		HdAddr addrIntensity;
		// 时间数据地址
		HdAddr addrTime;
		// 颜色数据地址
		HdAddr addrColor;
		// 分类数据地址
		HdAddr addrClass;
		// 压缩后数据大小
		U32 cmpSize;
		HdParcel()
			:numPoint(0), cmpSize(0)
		{
			box.MinEdge = CHdVector3df(F32_MAX, F32_MAX, F32_MAX);
			box.MaxEdge = CHdVector3df(F32_MIN, F32_MIN, F32_MIN);
		}
	};

	// 块集结构体
	struct HdBlockset
		//:public HdParcel
	{
		// 当前包点数
		U64 numPoint;
		// 空间范围
		CHdBox3df box;
		// 坐标数据地址
		HdAddr addrCoord;
		// 强度数据地址
		HdAddr addrIntensity;
		// 时间数据地址
		HdAddr addrTime;
		// 颜色数据地址
		HdAddr addrColor;
		// 分类数据地址
		HdAddr addrClass;

		// 是否包含分块
		U8 hasSubBlock;
		// 划分块规则
		U8 numBlockX;	
		U8 numBlockY;
		U8 numBlockZ;
		
		// 有效块个数
		U16	numBlock;
		// 块索引起始地址
		HdAddr startOfBlock;

		// 压缩后数据大小
		U32 cmpSize;

		HdBlockset()
			:numPoint(0),numBlock(0),hasSubBlock(0),numBlockX(1),numBlockY(1),numBlockZ(1),cmpSize(0)
		{	
			box.MinEdge = CHdVector3df(F32_MAX, F32_MAX, F32_MAX);
			box.MaxEdge = CHdVector3df(F32_MIN, F32_MIN, F32_MIN);
		}
	};
	

	// 块结构体
	struct HdBlock
	{
		// 当前包点数
		U64 numPoint;
		// 空间范围
		CHdBox3df box;
		// 坐标数据地址
		HdAddr addrCoord;
		// 强度数据地址
		HdAddr addrIntensity;
		// 时间数据地址
		HdAddr addrTime;
		// 颜色数据地址
		HdAddr addrColor;
		// 分类数据地址
		HdAddr addrClass;

		// 是否包含分包
		U8 hasSubParcel;
		// 划分包规则
		U8 numParcelX;
		U8 numParcelY;
		U8 numParcelZ;
		
		// 有效包个数
		U16	numParcel;
		
		// 包索引起始地址
		HdAddr startOfParcel;

		// 压缩后数据大小
		U32 cmpSize;

		HdBlock()
			:numPoint(0),numParcel(0),hasSubParcel(0),numParcelX(1),numParcelY(1),numParcelZ(1),cmpSize(0)
		{		
			box.MinEdge = CHdVector3df(F32_MAX, F32_MAX, F32_MAX);
			box.MaxEdge = CHdVector3df(F32_MIN, F32_MIN, F32_MIN);
		}
	};
		
	// 网格位置大小信息,位置是相对文件头偏移量坐标
	struct HdRefPoint
	{
		// 网格左下角点
		F32 x;
		F32 y;
		F32 z;
		// 网格分辨率,即网格内部坐标1个单位代表距离
		F32 rx;
		F32 ry;
		F32 rz;
		// 网格分辨率倒数
		F32 rxRcp;
		F32 ryRcp;
		F32 rzRcp;

		HdRefPoint()
			:x(0.0f),y(0.0f),z(0.0f),rx(0.0f),ry(0.0f),rz(0.0f),
			rxRcp(0.0f),ryRcp(0.0f),rzRcp(0.0f){}

		void FromBox(const CHdBox3df& box)
		{
			x = box.MinEdge.X;
			y = box.MinEdge.Y;
			z = box.MinEdge.Z;

			const CHdVector3df& ext = box.getExtent();
			rx = ext.X * COORD_REF;
			ry = ext.Y * COORD_REF;
			rz = ext.Z * COORD_REF;

			rxRcp = 1.0f / rx;
			ryRcp = 1.0f / ry;
			rzRcp = 1.0f / rz;
		}
	};

	struct HdRefPointd
	{
		//网格左下角点坐标
		F64 x;
		F64 y;
		F64 z;

		//格网分辨率
		F64 rx;
		F64 ry;
		F64 rz;

		//格网分辨率倒数
		F64 rxRcp;
		F64 ryRcp;
		F64 rzRcp;

		HdRefPointd()
			:x(0.0), y(0.0), z(0.0), rx(0.0), ry(0.0), rz(0.0),
			rxRcp(0.0), ryRcp(0.0), rzRcp(0.0){}

		void FromBox(const CHdBox3dd& box)
		{
			x = box.MinEdge.X;
			y = box.MinEdge.Y;
			z = box.MinEdge.Z;

			const CHdVector3dd& ext = box.getExtent();
			rx = ext.X * COORD_REF;
			ry = ext.Y * COORD_REF;
			rz = ext.Z * COORD_REF;

			rxRcp = 1.0 / rx;
			ryRcp = 1.0 / ry;
			rzRcp = 1.0 / rz;
		}
	};

	// HLZ文件头信息
	struct HLS_API HLZheader
	{
		CHAR file_signature[4];		 // 文件标识,固定为"HLZF"
		U32 project_ID_GUID_data_1;	 // 工程唯一ID 1
		U16 project_ID_GUID_data_2;
		U16 project_ID_GUID_data_3;
		U8  project_ID_GUID_data_4[8];
		U16 file_source_id;			 // 文件源ID

		U8 version_major;			 // 格式主版本号
		U8 version_minor;			 // 格式次版本号
		CHAR system_identifier[32];	 // 系统标识符
		CHAR generating_software[32];// 生成软件，如hdScan
		U16 file_creation_day;		 // 文件生成日期，一年中的第几天
		U16 file_creation_year;		 // 文件生成年份
		U16 header_size;			 // 文件头大小,目前是256byte
		U32 offset_to_level_index;	 // 层索引记录偏移量
		U64 number_of_point_records; // 点记录数
		U32 number_of_row;			 // 每扫描圈点数
		U32 number_of_col;			 // 统计第0层块集以及块的数量

		F32 max_x;					 // 偏移后的相对坐标范围
		F32 min_x;
		F32 max_y;
		F32 min_y;
		F32 max_z;
		F32 min_z;

		//! 转换坐标7参数
		F64 offsetX;			// 偏移量X,最终坐标是 X = offsetX + x;
		F64 offsetY;			// 偏移量Y
		F64 offsetZ;			// 偏移量Z
		F64 rotateX;			// 旋转X ,phi,注意这里是绕Y轴旋转
		F64 rotateY;			// 旋转Y,omega,注意这里是绕x轴旋转
		F64 rotateZ;			// 旋转Z,kappa,绕Z轴旋转
		F64 scale;			    // 比例系数

		U8 number_of_level;	    // 层数
		U8 length_of_level;		// 层记录长度
		U8 length_of_blockset;  // 块集记录长度
		U8 length_of_block;     // 块记录长度
		U8 length_of_parcel;    // 包记录长度

		//F32 grid_sx;			// 网格起始坐标X
		//F32 grid_sy;			// 网格起始坐标Y
		//F32 grid_sz;			// 网格起始坐标Z

		F32 renderMinX;			// 按坐标渲染增强相关变量
		F32 renderMaxX;
		F32 renderMinY;
		F32 renderMaxY;
		F32 renderMinZ;
		F32 renderMaxZ;

		U8  intensityMin;		// 强度渲染增强变量
		U8  intensityMax;		// 强度渲染增强变量

		U8 isCompress;        // 是否压缩：0 不压缩， 1 压缩

		U8 iZoneID;           // 投影带号 

		CHAR Reversed[32];	    // 保留位

		HLZheader()
		{			
			clean_header();
		}

		//! 将内存流解析到文件头结构体
		inline void prase(char* hdData)
		{
			memcpy(file_signature,hdData,4);
			hdData += 4;

			memcpy(&project_ID_GUID_data_1,hdData,4);
			hdData += 4;
			memcpy(&project_ID_GUID_data_2,hdData,2);
			hdData += 2;
			memcpy(&project_ID_GUID_data_3,hdData,2);
			hdData += 2;
			memcpy(project_ID_GUID_data_4,hdData,8);
			hdData += 8;

			memcpy(&file_source_id,hdData,2);
			hdData += 2;
			memcpy(&version_major,hdData,1);
			hdData += 1;
			memcpy(&version_minor,hdData,1);
			hdData += 1;
			memcpy(&system_identifier,hdData,32);
			hdData += 32;
			memcpy(generating_software,hdData,32);
			hdData += 32;
			memcpy(&file_creation_day,hdData,2);
			hdData += 2;
			memcpy(&file_creation_year,hdData,2);
			hdData += 2;
			memcpy(&header_size,hdData,2);
			hdData += 2;

			memcpy(&offset_to_level_index,hdData,4);
			hdData += 4;
			memcpy(&number_of_point_records,hdData,8);
			hdData += 8;
			memcpy(&number_of_row,hdData,4);
			hdData += 4;
			memcpy(&number_of_col,hdData,4);
			hdData += 4;
			//读取范围
			memcpy(&max_x,hdData,4);
			hdData += 4;
			memcpy(&min_x,hdData,4);
			hdData += 4;
			memcpy(&max_y,hdData,4);
			hdData += 4;
			memcpy(&min_y,hdData,4);
			hdData += 4;
			memcpy(&max_z,hdData,4);
			hdData += 4;
			memcpy(&min_z,hdData,4);
			hdData += 4;

			//读取7参数
			memcpy(&offsetX,hdData,8);
			hdData += 8;
			memcpy(&offsetY,hdData,8);
			hdData += 8;
			memcpy(&offsetZ,hdData,8);
			hdData += 8;

			memcpy(&rotateX,hdData,8);
			hdData += 8;
			memcpy(&rotateY,hdData,8);
			hdData += 8;
			memcpy(&rotateZ,hdData,8);
			hdData += 8;

			memcpy(&scale,hdData,8);
			hdData += 8;

			memcpy(&number_of_level, hdData, 1);
			hdData += 1;

			memcpy(&length_of_level, hdData, 1);
			hdData += 1;

			memcpy(&length_of_blockset, hdData, 1);
			hdData += 1;
			
			memcpy(&length_of_block, hdData, 1);
			hdData += 1;

			memcpy(&length_of_parcel, hdData, 1);
			hdData += 1;
			// 格网起点
			//memcpy(&grid_sx, hdData, 4);
			//hdData += 4;
			//(&grid_sy, hdData, 4);
			//hdData += 4;
			//memcpy(&grid_sz, hdData, 4);
			//hdData += 4;
			// 渲染统计坐标
			memcpy(&renderMinX, hdData, 4);
			hdData += 4;

			memcpy(&renderMaxX, hdData, 4);
			hdData += 4;

			memcpy(&renderMinY, hdData, 4);
			hdData += 4;
			memcpy(&renderMaxY, hdData, 4);
			hdData += 4;

			memcpy(&renderMinZ, hdData, 4);
			hdData += 4;

			memcpy(&renderMaxZ, hdData, 4);
			hdData += 4;

			memcpy(&intensityMin, hdData, 1);
			hdData += 1;

			memcpy(&intensityMax, hdData, 1);
			hdData += 1;

			memcpy(&isCompress, hdData, 1);
			hdData += 1;

			memcpy(&iZoneID, hdData, 1);
			hdData += 1;
			

			// 解决旧的数据没有7参数
			if (scale == 0.0)
			{
				scale = 1.0;
			}
		}

		//! 将文件头结构体,序列化到内存流
		inline void serialize(char* hdData) const
		{
			memcpy(hdData,file_signature,4);
			hdData += 4;

			memcpy(hdData,&project_ID_GUID_data_1,4);
			hdData += 4;
			memcpy(hdData,&project_ID_GUID_data_2,2);
			hdData += 2;
			memcpy(hdData,&project_ID_GUID_data_3,2);
			hdData += 2;
			memcpy(hdData,project_ID_GUID_data_4,8);
			hdData += 8;

			memcpy(hdData,&file_source_id,2);
			hdData += 2;
			memcpy(hdData,&version_major,1);
			hdData += 1;
			memcpy(hdData,&version_minor,1);
			hdData += 1;
			memcpy(hdData,&system_identifier,32);
			hdData += 32;
			memcpy(hdData,generating_software,32);
			hdData += 32;
			memcpy(hdData,&file_creation_day,2);
			hdData += 2;
			memcpy(hdData,&file_creation_year,2);
			hdData += 2;
			memcpy(hdData,&header_size,2);
			hdData += 2;

			memcpy(hdData,&offset_to_level_index,4);
			hdData += 4;
			memcpy(hdData,&number_of_point_records,8);
			hdData += 8;
			memcpy(hdData,&number_of_row,4);
			hdData += 4;
			memcpy(hdData,&number_of_col,4);
			hdData += 4;

			//读取范围
			memcpy(hdData,&max_x,4);
			hdData += 4;
			memcpy(hdData,&min_x,4);
			hdData += 4;
			memcpy(hdData,&max_y,4);
			hdData += 4;
			memcpy(hdData,&min_y,4);
			hdData += 4;
			memcpy(hdData,&max_z,4);
			hdData += 4;
			memcpy(hdData,&min_z,4);
			hdData += 4;
						
			//读取7参数
			memcpy(hdData,&offsetX,8);
			hdData += 8;
			memcpy(hdData,&offsetY,8);
			hdData += 8;
			memcpy(hdData,&offsetZ,8);
			hdData += 8;

			memcpy(hdData,&rotateX,8);
			hdData += 8;
			memcpy(hdData,&rotateY,8);
			hdData += 8;
			memcpy(hdData,&rotateZ,8);
			hdData += 8;

			memcpy(hdData,&scale,8);
			hdData += 8;

			memcpy(hdData,&number_of_level,  1);
			hdData += 1;

			memcpy(hdData,&length_of_level,  1);
			hdData += 1;

			memcpy(hdData,&length_of_blockset,  1);
			hdData += 1;

			memcpy(hdData, &length_of_block, 1);
			hdData += 1;

			memcpy(hdData, &length_of_parcel, 1);
			hdData += 1;

			// 格网起始坐标
			//memcpy(hdData,&grid_sx,  4);
			//hdData += 4;
			//memcpy(hdData,&grid_sy,  4);
			//hdData += 4;
			//memcpy(hdData,&grid_sz,  4);
			//hdData += 4;

			// 渲染统计坐标
			memcpy(hdData,&renderMinX,  4);
			hdData += 4;
			memcpy(hdData,&renderMaxX,  4);
			hdData += 4;
			memcpy(hdData,&renderMinY,  4);
			hdData += 4;
			memcpy(hdData,&renderMaxY,  4);
			hdData += 4;
			memcpy(hdData,&renderMinZ,  4);
			hdData += 4;
			memcpy(hdData,&renderMaxZ,  4);
			hdData += 4;
			memcpy(hdData,&intensityMin,1);
			hdData += 1;
			memcpy(hdData,&intensityMax,1);
			hdData += 1;
			memcpy(hdData, &isCompress, 1);
			hdData += 1;
			memcpy(hdData, &iZoneID, 1);
			hdData += 1;

		}

		//! 根据7参数获取转换矩阵
		inline void computeMatrix(double* M)
		{
			double cr = cos( rotateX );
			double sr = sin( rotateX );
			double cp = cos( rotateY );
			double sp = sin( rotateY );
			double cy = cos( rotateZ );
			double sy = sin( rotateZ );

			double a1,a2,a3,b1,b2,b3,c1,c2,c3;

			a1 = cr * cy - sr * sp * sy; //cos(m_header.rotateX)*cos(m_header.rotateZ) - sin(m_header.rotateX)*sin(m_header.rotateY)*sin(m_header.rotateZ);
			a2 = -cr * sy - sr * sp * cy;//-cos(m_header.rotateX)*sin(m_header.rotateZ) - sin(m_header.rotateX)*sin(m_header.rotateY)*cos(m_header.rotateZ);
			a3 = -sr * cp;			     //-sin(m_header.rotateX)*cos(m_header.rotateY);
			b1 = cp * sy;				 //cos(m_header.rotateY)*sin(m_header.rotateZ);
			b2 = cp * cy;				 //cos(m_header.rotateY)*cos(m_header.rotateZ);
			b3 = -sp;					 //-sin(m_header.rotateY);
			c1 = sr * cy + cr * sp * sy; //sin(m_header.rotateX)*cos(m_header.rotateZ) + cos(m_header.rotateX)*sin(m_header.rotateY)*sin(m_header.rotateZ);
			c2 = -sr * sy + cr * sp * cy;//-sin(m_header.rotateX)*sin(m_header.rotateZ) + cos(m_header.rotateX)*sin(m_header.rotateY)*cos(m_header.rotateZ);
			c3 = cr * cp;				 //cos(m_header.rotateX)*cos(m_header.rotateY);
			// 旋转矩阵
			M[0] = a1 * scale; 
			M[1] = a2 * scale; 
			M[2] = a3 * scale;

			M[4] = b1 * scale; 
			M[5] = b2 * scale; 
			M[6] = b3 * scale;

			M[8] = c1 * scale; 
			M[9] = c2 * scale; 
			M[10] = c3 * scale;

			// 偏移量
			M[3] = offsetX;
			M[7] = offsetY;
			M[11] = offsetZ;

			// fill the bottom row of the 4x4 matrix,固定0.0,0.0,0.0,1.0，与scale无关
			M[12] = 0.0;
			M[13] = 0.0;
			M[14] = 0.0;
			M[15] = 1.0;
		}

		inline void getGlobalExtent(double& xmin,double& ymin,double& zmin,double& xmax,double& ymax,double& zmax)
		{
			double m[16];
			computeMatrix(m);

			// 底平面4个点
			double x1 = min_x,y1 = min_y,z1 = min_z;
			double x2 = max_x,y2 = min_y,z2 = min_z;
			double x3 = max_x,y3 = max_y,z3 = min_z;
			double x4 = min_x,y4 = max_y,z4 = min_z;
			// 顶平面4个点
			double x5 = min_x,y5 = min_y,z5 = max_z;
			double x6 = max_x,y6 = min_y,z6 = max_z;
			double x7 = max_x,y7 = max_y,z7 = max_z;
			double x8 = min_x,y8 = max_y,z8 = max_z;

			hdHomogeneousTransformPoint(m,x1,y1,z1);
			hdHomogeneousTransformPoint(m,x2,y2,z2);
			hdHomogeneousTransformPoint(m,x3,y3,z3);
			hdHomogeneousTransformPoint(m,x4,y4,z4);

			hdHomogeneousTransformPoint(m,x5,y5,z5);
			hdHomogeneousTransformPoint(m,x6,y6,z6);
			hdHomogeneousTransformPoint(m,x7,y7,z7);
			hdHomogeneousTransformPoint(m,x8,y8,z8);

			xmin = MIN(MIN(MIN(MIN(x1,x2),x3),x4),MIN(MIN(MIN(x5,x6),x7),x8));
			ymin = MIN(MIN(MIN(MIN(y1,y2),y3),y4),MIN(MIN(MIN(y5,y6),y7),y8));
			zmin = MIN(MIN(MIN(MIN(z1,z2),z3),z4),MIN(MIN(MIN(z5,z6),z7),z8));

			xmax = MAX(MAX(MAX(MAX(x1,x2),x3),x4),MAX(MAX(MAX(x5,x6),x7),x8));
			ymax = MAX(MAX(MAX(MAX(y1,y2),y3),y4),MAX(MAX(MAX(y5,y6),y7),y8));
			zmax = MAX(MAX(MAX(MAX(z1,z2),z3),z4),MAX(MAX(MAX(z5,z6),z7),z8));
		}

		// set bounding box
		void set_bounding_box(F64 min_x, F64 min_y, F64 min_z, F64 max_x, F64 max_y, F64 max_z)
		{
			this->min_x = (F32)min_x;
			this->min_y = (F32)min_y;
			this->min_z = (F32)min_z;
			this->max_x = (F32)max_x;
			this->max_y = (F32)max_y;
			this->max_z = (F32)max_z;
		};

		// clean functions
		void clean_header()
		{
			memset((void*)this, 0, sizeof(HLZheader));
			file_signature[0] = 'H'; file_signature[1] = 'L'; file_signature[2] = 'Z'; file_signature[3] = 0;
			version_major = 3;
			version_minor =0;

			header_size = 256;
			offset_to_level_index = 256;
			number_of_point_records = 0;
			number_of_row = 0;
			number_of_col = 0;
			number_of_level = 0;

			length_of_level = sizeof(HdLevel);
			length_of_blockset = sizeof(HdBlockset);
			length_of_block = sizeof(HdBlock);
			length_of_parcel = sizeof(HdParcel);
			scale = 1.0;

			set_bounding_box(F32_MAX,F32_MAX,F32_MAX,F32_MIN,F32_MIN,F32_MIN);

			renderMinX = F32_MAX;
			renderMaxX = F32_MIN;
			renderMinY = F32_MAX;
			renderMaxX = F32_MIN;
			renderMinZ = F32_MAX;
			renderMaxZ = F32_MIN;

			isCompress = FALSE;
			iZoneID = 0;

		};

		void clean()
		{
			clean_header();
		};

		~HLZheader()
		{
			clean();
		};
	};

	// 根据xyz方向编号转换为总序号
	inline I32 GridNo2Index(
		U16 gSizeX,U16 gSizeY,U16 gSizeZ,	// 格网大小
		U16 xNo,U16 yNo,U16 zNo)			// 网格编号
	{
		if (xNo < gSizeX -1 || yNo < gSizeY - 1 || zNo < gSizeZ - 1)
		{
			return -1;
		}

		return xNo + yNo * gSizeX + zNo * gSizeX * gSizeY;
	}

	// 根据总序号转换为xyz方向编号
	inline void Index2GridNo(
		U16 gSizeX,U16 gSizeY,U16 gSizeZ,	// 格网大小
		I32 index,							// 总序号
		U16& xNo,U16& yNo,U16& zNo)			// 网格编号
	{
		xNo = 0;
		yNo = 0;
		zNo = 0;
		if (index < 0)
		{
			return ;
		}

		zNo = index / (gSizeX * gSizeY);
		yNo = (index - zNo * gSizeX * gSizeY) / gSizeX;
		xNo = index - zNo * gSizeX * gSizeY - yNo * gSizeX;
	}

	// 前置申明层、块集、块、包管理类
	class CHdLevel;
	class CHdBlockset;
	class CHdBlock;
	class CHdParcel;

	typedef pair<U32,CHdLevel*> HdLevelPair;
	typedef pair<U32,CHdBlockset*> HdBlockSetPair;
	typedef pair<U16,CHdBlock*> HdBlockPair;
	typedef pair<U16,CHdParcel*> HdParcelPair;
#pragma  pack(pop)
}