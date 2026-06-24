/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdSvMapView
文件名		：hdLonLatParm.h
文件实现功能：定义在线地图（百度地图）的经纬度偏移参数
			  百度地图与iScan的pos解算出来的经纬度有一定的偏差，偏差值是固定的
			  与百度地图叠加时，需要加上偏移量.
作者		：杨峰
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2013/12/22	1.0			杨峰		创建
</PRE>
******************************************************************************************************/
#pragma once//lhq on 2015/12/31
#include <string>
#include <vector>
#include "hdCommon.h"
#include <WinBase.h>
using namespace std;

namespace hd
{
	struct LONLAT_PARM  
	{
		LONLAT_PARM()
		{
			memset(strL, 0, 16);
			memset(strB, 0, 16);
			memset(strLParm, 0, 16);
			memset(strBParm, 0, 16);
		}

		char strL[16];		// 经度
		char strB[16];		// 纬度
		char strLParm[16];	// 经度偏移值
		char strBParm[16];	// 纬度偏移值
	};

	class CHdLonLatParm
	{
	public:
		CHdLonLatParm() {}
		~CHdLonLatParm() {}

	public:
		// 从本地文件读取参数,在本地目录的LonLatParam.lb
		void ReadLonLarParm()
		{
			TCHAR str[1000];
			GetModuleFileName(GetModuleHandle("hdGeoPosition"),str,1000);
			string strCurrenDir = str;
			int pos = strCurrenDir.find_last_of("\\");
			string strName = strCurrenDir.substr(0,pos + 1);

			// 找寻当前目录下的param文件
			string strFile = strName + "LonLatParam.lb";

			FILE* pFile = fopen(strFile.data(), "rt");
			if (pFile == NULL)
			{
				return;
			}

			m_vectParm.clear();

			char strBuf[512];		// 保存每一行字符
			// 首先申请5000个记录,不够时再申请
			m_vectParm.resize(5000);
			size_t nCount = 0;
			while (!feof(pFile))
			{
				memset(strBuf, 0, 512);
				LONLAT_PARM& param = m_vectParm.at(nCount);
				// 读取一行数据 判断属性个数
				fgets(strBuf, 512, pFile);
				sscanf(strBuf,"%s\t%s\t%s\t%s\n",
					param.strL, param.strB, param.strLParm, param.strBParm);

				nCount++;

				if (nCount >= m_vectParm.size())
				{
					m_vectParm.resize(m_vectParm.size() + 5000);
				}
			}

			fclose(pFile);
			m_vectParm.resize(nCount);

			// 
			FILE *pf = fopen("D:\\out.txt", "w");
			for (int i = 0; i < nCount; i++)
			{
				fprintf(pf, "%s,%s,%s,%s\n", m_vectParm[i].strL, m_vectParm[i].strB, m_vectParm[i].strLParm,
					m_vectParm[i].strBParm);
			}
			fclose(pf);
		}

		// 查询指定经纬度的偏移值
		void Query(double dL, double dB, double& dlParm, double& dbParm)
		{
			size_t nSize = m_vectParm.size();
			for (size_t i=0; i<nSize; i++)
			{
				// 保留经纬度1位小数，进行查询
				char strL[16] = {0};
				char strB[16] = {0};

				// 四舍五入到小数点后一位，即0.1
				if (abs(dL + 0.05 - int(dL+0.05))>=0.1 )
				{
					sprintf(strL, "%.1f", dL+0.05);
				}
				else
				{
					sprintf(strL, "%.0f", dL+0.05);
				}

				// 当与其整数的差距小于0.1时,此时转换后应该是一个整数,所以需要转换成没有小数点的值，否则找不到坐标偏移值
				if (abs(dB + 0.05 - int(dB+0.05))>=0.1 )
				{
					sprintf(strB, "%.1f", dB+0.05);
				}
				else
				{
					sprintf(strB, "%.0f", dB+0.05);
				}

				if (strcmp(strL, m_vectParm[i].strL) == 0 &&
					strcmp(strB, m_vectParm[i].strB) == 0)
				{
					dlParm = (double)atof(m_vectParm[i].strLParm);
					dbParm = (double)atof(m_vectParm[i].strBParm);

					return;
				}
			}
		}

	private:
		vector<LONLAT_PARM> m_vectParm;			// 保存参数
	};
}