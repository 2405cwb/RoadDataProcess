/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：HdOriImageByHdiStruct.h
相关文件	: HdOriImageByHdiStruct.cpp
文件实现功能：定义原始影像与全景影像的对应关系的结构
作者		：冯晶
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2014/11/25	1.0			冯晶		创建
</PRE>
******************************************************************************************************/

#pragma once
#include "hdCommon.h"
#include "hdConstDef.h"
#include "..\hdCore\tinyxml.h"
#include <vector>

using namespace std;
namespace hd
{
	struct  HD_ORIIMAGEBYHDI_INFO	
	{
		HD_ORIIMAGEBYHDI_INFO()
			: hdiIndex(0)
		{
			memset(strImageName, 0, OBJECT_ID_LEN);
		}

		char				strImageName[PANO_ID_LEN];	// 影像唯一标识
		int					hdiIndex;					// 对应于hdi的索引编号
		vector<string>		Cam_OriName;				// 当前全景影像对应的原始影像相对路径
	};

	// 定义原始影像与全景影像的对应关系的结构
	struct HD_ORIIMAGEBYHDI_INFOS	
	{
		// 构造函数，赋初值
		HD_ORIIMAGEBYHDI_INFOS()
			: pOriImage(NULL)
			, nCount(0)
			, m_strImgPath("")
		{
		}

		HD_ORIIMAGEBYHDI_INFOS(HD_ORIIMAGEBYHDI_INFO* pImge, int ncount)
		{
			pOriImage = pImge;
			nCount = ncount;
			m_strImgPath = "";
		}

		~HD_ORIIMAGEBYHDI_INFOS()
		{
			if (pOriImage)
			{
				delete[] pOriImage;
				pOriImage = NULL;
			}
			nCount = 0;
		}

		bool OpenFile(const char* filePath, int nCount)
		{
			// 表示索引文件已经加载进来
			if (pOriImage && nCount > 0)
			{
				return true;
			}

			try
			{
				pOriImage = new HD_ORIIMAGEBYHDI_INFO[nCount];
				this->nCount = nCount;
			}
			catch (...)
			{
				return false;
			}

			TiXmlDocument doc(filePath);
			if (!doc.LoadFile())
			{
				return false;
			}
			int j = 0;
			TiXmlElement* rootElement = doc.RootElement();

			string strValue;
			string strText;

			rootElement = rootElement->FirstChildElement("ImagePath");
			strValue = rootElement->Value();
			strText = rootElement->GetText();
			m_strImgPath = strText;

			rootElement = rootElement->NextSiblingElement("PanoName");
			while (rootElement)
			{
				pOriImage[j].hdiIndex = j;

				// 读取全景影像照片
				string str = rootElement->Attribute("name");
				memcpy(pOriImage[j].strImageName, str.c_str(), PANO_ID_LEN);


				// 读该全景影像对应的原始图像
				TiXmlElement* childElement = rootElement->FirstChildElement("OriImageName");
				if (!childElement)
				{
					return false;
				}

				while(childElement)
				{
					strValue = childElement->Value();
					strText = childElement->GetText();
					pOriImage[j].Cam_OriName.push_back(strText);

					childElement = childElement->NextSiblingElement("OriImageName");
				}

				rootElement = rootElement->NextSiblingElement("PanoName");
				j++;
			}
		}

		// 序列化，从文件中的一行解析数据
		bool Serialize(const char* strLine)
		{
			// 判断可用性
			if (!pOriImage || !nCount)
			{
				return false;
			}

			TiXmlDocument doc;
			TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "gbk", "" );
			TiXmlElement* rootElement = new TiXmlElement("HD_3LS_SCENE_ISCAN");

			doc.LinkEndChild(decl);
			doc.LinkEndChild(rootElement);

			// 保存原始影像目录
			TiXmlElement* ImagePath = new TiXmlElement("ImagePath");
			rootElement->LinkEndChild(ImagePath);
			TiXmlText* ImagePathText = new TiXmlText(m_strImgPath.c_str());
			ImagePath->LinkEndChild(ImagePathText);

			int i,j;
			for (i = 0; i < nCount; i++)
			{
				// 保存原始影像名称
				TiXmlElement* panoname= new TiXmlElement("PanoName");
				panoname->SetAttribute("name", pOriImage[i].strImageName);
				rootElement->LinkEndChild(panoname);

				// 保存原始影像的相对路径
				for (j = 0; j < pOriImage[i].Cam_OriName.size(); j ++)
				{
					TiXmlElement* OriImageName = new TiXmlElement("OriImageName");
					panoname->LinkEndChild(OriImageName);
					TiXmlText* OriImageNameText = new TiXmlText(pOriImage[i].Cam_OriName[j].c_str());
					OriImageName->LinkEndChild(OriImageNameText);
				}
			}

			doc.SaveFile(strLine);
		}

		// 获取原始影像存储的目录
		string GetOriImageFolder() { return m_strImgPath;}

		HD_ORIIMAGEBYHDI_INFO*	pOriImage;		// 原始影像与全景影像对应列表
		int						nCount;			// 全景影像的数量
		string					m_strImgPath;	// 原始影像存储的目录
	};
}