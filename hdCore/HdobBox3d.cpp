/*! @CHdobBox3d.cpp
********************************************************************************
<PRE>
模块名       : hdCore
文件名       : HdobBox3d.cpp
相关文件     : HdobBox3d.h
文件实现功能 : 实现三维坐标下obb包围盒，主要实现obb box相交检测功能
顶点顺序说明 :
                  /4---------/5
                 / |        / |
                /  |       /  |
                7---------6   |
                |  /0- - -|- -1
                | /       |  /
                |/        | /
                3---------2/
作者         : 朱旭波
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/06/04   1.0      朱旭波              创建
</PRE>
*******************************************************************************/

#include "StdAfx.h"
#include "HdobBox3d.h"

namespace hd
{
	// 默认构造
	CHdobBox3d::CHdobBox3d(void)
	{
	}

	// 以八个顶点坐标进行构造
	CHdobBox3d::CHdobBox3d( f32* vertex )
	{
		// 顶点赋值
		m_vertex[0].set(vertex[0],vertex[1],vertex[2]);
		m_vertex[1].set(vertex[3],vertex[4],vertex[5]);
		m_vertex[2].set(vertex[6],vertex[7],vertex[8]);
		m_vertex[3].set(vertex[9],vertex[10],vertex[11]);
		m_vertex[4].set(vertex[12],vertex[13],vertex[14]);
		m_vertex[5].set(vertex[15],vertex[16],vertex[17]);
		m_vertex[6].set(vertex[18],vertex[19],vertex[20]);
		m_vertex[7].set(vertex[21],vertex[22],vertex[23]);
	}

	// 析构
	CHdobBox3d::~CHdobBox3d(void)
	{
	}

	// 判断本box与外部传入box是否相交
	int CHdobBox3d::BoxIntersect( CHdobBox3d* box )
	{
		// 定义临时变量值
		int flag = 1;
		float fMin1,fMax1,fMin2,fMax2;

		// 循环比较三个轴线方向
		for (int i = 0;i < 3;i++)
		{
			GetInterval(this,this->GetFaceDir(i),fMin1,fMax1);
			GetInterval(box,this->GetFaceDir(i),fMin2,fMax2);

			// 不相交，直接返回0
			if ((fMax1 < fMin2) || (fMax2 < fMin1))
			{
				return 0;
			}
		}

		// 循环比较，以传入box为中心
		for (int i = 0;i < 3;i++)
		{
			GetInterval(this,box->GetFaceDir(i),fMin1,fMax1);
			GetInterval(box,box->GetFaceDir(i),fMin2,fMax2);

			// 不相交，直接返回
			if ((fMax1 < fMin2) || (fMax2 < fMin1))
			{
				return 0;
			}
		}

		// 三个主要顶点九个边进行比较
		CHdVector3df axis;
		for (int i = 0;i < 3;i++)
		{
			for (int j = 0;j < 3;j++)
			{
				// 求轴线向量
				axis = (this->GetEdgeDir(i)).crossProduct(box->GetEdgeDir(j));
				axis.normalize();

				GetInterval(this,axis,fMin1,fMax1);
				GetInterval(box,axis,fMin2,fMax2);

				// 不相交，直接返回
				if (fMax1 < fMin2 || fMax2 < fMin1)
				{
					return 0;
				}
			}
		}

		// 执行到这一步，表明必定相交
		return flag;
	}

	// 获取边界范围值,传入box，轴线值，返回该轴线方向最大最小值
	void CHdobBox3d::GetInterval( CHdobBox3d* box,CHdVector3df axis,float &fMin,float &fMax )
	{
		// 以索引为0的顶点设置为初始最大最小距离值
		float fValue = 0.0;
		fMin = fMax = Dot(axis,box->m_vertex[0]);

		// 遍历八个顶点求真实最大最小值作为返回值
		for (int i = 0;i < 8;i++)
		{
			fValue = Dot(axis,box->m_vertex[i]);
			fMin = MIN(fMin,fValue);
			fMax = MAX(fMax,fValue);
		}
	}

	// 取得box的面的法向量,传入参数为顶点的索引值，范围为0-7
	hd::CHdVector3df CHdobBox3d::GetFaceDir( int indeID )
	{
		// 定义三点确定面的法向量
		CHdVector3df normalLine;

		// 定义临时变量，记录两点构成的向量
		CHdVector3df tmpLine1;
		CHdVector3df tmpLine2;

		// 求面的法向量
		switch (indeID)
		{
		case 0:
			{
				// 前后面
				tmpLine1 = m_vertex[3] - m_vertex[2];
				tmpLine2 = m_vertex[6] - m_vertex[2];
				tmpLine1.normalize();
				tmpLine2.normalize();

				// 得到的向量叉积即为该面的法向量
				normalLine = tmpLine1.crossProduct(tmpLine2);
				normalLine.normalize();
			}
			break;
		case 1:
			{
				// 左右面
				tmpLine1 = m_vertex[2] - m_vertex[1];
				tmpLine2 = m_vertex[5] - m_vertex[1];
				tmpLine1.normalize();
				tmpLine2.normalize();

				// 得到的向量叉积即为该面的法向量
				normalLine = tmpLine1.crossProduct(tmpLine2);
				normalLine.normalize();
			}
			break;
		case 2:
			{
				// 上下面
				tmpLine1 = m_vertex[0] - m_vertex[1];
				tmpLine2 = m_vertex[2] - m_vertex[1];
				tmpLine1.normalize();
				tmpLine2.normalize();

				// 得到的向量叉积即为该面的法向量
				normalLine = tmpLine1.crossProduct(tmpLine2);
				normalLine.normalize();
			}
			break;
		}

		return normalLine;
	}

	// 取得边的方向值，传入参数为顶点索引值
	hd::CHdVector3df CHdobBox3d::GetEdgeDir( int indexID )
	{
		// 定义返回方向值点
		CHdVector3df pt;

		// 定义向量值(向量)
		CHdVector3df tmpLine;

		// 根据假定的轴线遍历
		switch (indexID)
		{
		case 0:
			{
				// x轴线方向
				tmpLine = m_vertex[1] - m_vertex[0];
				pt = tmpLine.normalize();
			}
			break;
		case 1:
			{
				// y轴线方向
				tmpLine = m_vertex[4] - m_vertex[0];
				pt = tmpLine.normalize();
			}
			break;
		case 2:
			{
				// z轴线方向
				tmpLine = m_vertex[3] - m_vertex[0];
				pt = tmpLine.normalize();
			}
			break;
		}

		return pt;
	}

	// 设置box的八个顶点坐标
	void CHdobBox3d::SetVertex( f32* vertex )
	{
		// 顶点赋值
		m_vertex[0].set(vertex[0],vertex[1],vertex[2]);
		m_vertex[1].set(vertex[3],vertex[4],vertex[5]);
		m_vertex[2].set(vertex[6],vertex[7],vertex[8]);
		m_vertex[3].set(vertex[9],vertex[10],vertex[11]);
		m_vertex[4].set(vertex[12],vertex[13],vertex[14]);
		m_vertex[5].set(vertex[15],vertex[16],vertex[17]);
		m_vertex[6].set(vertex[18],vertex[19],vertex[20]);
		m_vertex[7].set(vertex[21],vertex[22],vertex[23]);

		//初始化成员参数值
		SetValue();
	}

	//判断点是否在包围盒内
	bool CHdobBox3d::isPointInside(CHdVector3df pt)
	{
		CHdVector3df vd = pt /*- m_center*/;
        /*
		dot方法为求点积
		由于_xAxis为单位矢量
		vd与_xAxis的点击即为在_xAxis方向的投影
		*/
       float d = Dot(m_xAxis,vd); //计算x方向投影d

	   //判断投影是否大于x正方向的半长或小于x负方向半长
	   if (d > m_vecMax.X || d < m_vecMin.X)
	   {
		   return false;//满足条件说明不在包围盒内
	   }

	   d = Dot(m_yAxis,vd); //计算y方向投影

	   //同理
		if (d > m_vecMax.Y || d < m_vecMin.Y)
		{
			return false;
		}

		d = Dot(m_zAxis,vd);//计算z方向投影

	    if (d > m_vecMax.Z || d < m_vecMin.Z)
		{
			return false;
		}

		return true;
	}

	//设置参数值
	void CHdobBox3d::SetValue()
	{
		m_xAxis = GetEdgeDir(0);
		m_yAxis = GetEdgeDir(2);
		m_zAxis = GetEdgeDir(1);

		//求出每个方向上的最大值和最小值	
		CHdVector3df vecMax,vecMin;
		GetInterval(this,m_xAxis,vecMin.X,vecMax.X);
		GetInterval(this,m_yAxis,vecMin.Y,vecMax.Y);
		GetInterval(this,m_zAxis,vecMin.Z,vecMax.Z);
		m_vecMax = vecMax;
		m_vecMin = vecMin;
	}

	//获取包围盒内的顶点
	void CHdobBox3d::GetBoxVertex(vector<hd::CHdVector3df>& vecVertex)
	{
		for (int i = 0; i<8;i++)
		{
			vecVertex.push_back(m_vertex[i]);
		}
	}

}
