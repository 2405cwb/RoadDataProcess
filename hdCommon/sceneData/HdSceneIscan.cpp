/*! @file
********************************************************************************
<PRE>
模块名       : ProjectManager
文件名       : hdSceneIscan.cpp
相关文件     : hdSceneIscan.h
文件实现功能 : 定义iScan数据节点包含的子节点信息，包括点信息、测量线、标注、
               GPS控制点等，该类对应的参数信息写入iScan-Route.config，函数实现
作者         : 软件部，朱旭波
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/02/14   1.0      朱旭波               创建
</PRE>
*******************************************************************************/

#include "stdafx.h"
#include "HdSceneIscan.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	CHdSceneIscan::CHdSceneIscan(void)
	{
	}

	CHdSceneIscan::CHdSceneIscan( CHdSceneIscan& hdss )
	{
		// 序列化原配置信息

		// 记录入内存
		unsigned int i;
		unsigned int nCount = (unsigned int)hdss.pVecPoints.size();
		for (i = 0; i<nCount; i++)
		{
			CHdSxPoint3D* newPoint = new CHdSxPoint3D(*(hdss.pVecPoints[i]));
			pVecPoints.push_back(newPoint);
		}

		nCount = (unsigned int)hdss.pVecLabels.size();
		for (i = 0; i<nCount; i++)
		{
			CHdLabel* newLabel = new CHdLabel(*(hdss.pVecLabels[i]));
			pVecLabels.push_back(newLabel);
		}

		nCount = (unsigned int)hdss.pVecPolylines.size();
		for (i = 0; i<nCount; i++)
		{
			CHdSxPolyline3D* newPolyline = new CHdSxPolyline3D(*(hdss.pVecPolylines[i]));
			pVecPolylines.push_back(newPolyline);
		}

		nCount = (unsigned int)hdss.pVecGPSPts.size();
		for (i = 0;i < nCount;i++)
		{
			CHdGPSPoint* newGPSpt = new CHdGPSPoint(*(hdss.pVecGPSPts[i]));
			pVecGPSPts.push_back(newGPSpt);
		}
		// 图片
		nCount = (unsigned int)hdss.pVecPictures.size();
		for (i = 0;i < nCount;i++)
		{
			CHdPicture* newImgPt = new CHdPicture(*(hdss.pVecPictures[i]));
			pVecPictures.push_back(newImgPt);
		}
	}

	CHdSceneIscan::~CHdSceneIscan(void)
	{
		// 析构时将指针对象全部清空，释放内存
		unsigned int i;
		unsigned int nCount = (unsigned int)pVecPoints.size();
		for (i = 0; i<nCount; i++)
		{
			delete pVecPoints[i];
			pVecPoints[i] = NULL;
		}

		nCount = (unsigned int)pVecLabels.size();
		for (i = 0; i<nCount; i++)
		{
			delete pVecLabels[i];
			pVecLabels[i] = NULL;
		}

		nCount = (unsigned int)pVecPolylines.size();
		for (i = 0; i<nCount; i++)
		{
			delete pVecPolylines[i];
			pVecPolylines[i] = NULL;
		}

		nCount = (unsigned int)pVecGPSPts.size();
		for (i = 0; i< nCount; i++)
		{
			delete pVecGPSPts[i];
			pVecGPSPts[i] = NULL;
		}

		// 图片
		nCount = (unsigned int)pVecPictures.size();
		for (i = 0;i < nCount;i++)
		{
			delete pVecPictures[i];
			pVecPictures[i] = NULL;
		}
	}

	// 设置图片的张数 fengjing
	void  CHdSceneIscan::SetPictureSize(int index)
	{
		if (index <= 0)
		{
			return;
		}

		try
		{
			pVecPictures.resize(index, NULL);
		}
		catch (...)
		{
			pVecPictures.clear();
			return;
		}		
	}

	// 增加图片 fengjing
	void CHdSceneIscan::AddPicture(int index, const char* strPicPath)
	{
		if (!strPicPath)
		{
			return;
		}

		if (index < pVecPictures.size() && index >= 0)
		{
			// 新建该图片
			if (!pVecPictures[index])
			{
				pVecPictures[index] = new CHdPicture(strPicPath);
			}

			// 修改该图片
			else
			{
				pVecPictures[index]->m_strFullName = strPicPath;
			}
		}
	}

	// 减少图片中的控制点 fengjing
	void CHdSceneIscan::DeletePicture(int index)
	{
		if (index < 0 || index >= pVecPictures.size())
		{
			return;
		}
		if (pVecPictures[index])
		{
			delete pVecPictures[index];
			pVecPictures[index] = NULL;
		}
		pVecPictures.erase(pVecPictures.begin() + index);
	}

	void CHdSceneIscan::Serialize( const char* strWorkspacePath, TiXmlElement* element, bool bSave )
	{
		if (bSave == true)
		{
			// 此处应该增加原工程属性信息

			TiXmlElement* scanObjects = element;
			////创建的对象
			//TiXmlElement* scanObjects = new TiXmlElement("iScanObjects");
			//element->LinkEndChild(scanObjects);

			//点对象
			unsigned int j;
			unsigned int nSize = (unsigned int)pVecPoints.size();
			for (j = 0; j<nSize; j++)
			{
				pVecPoints[j]->Serialize(scanObjects, true);
			}

			//标签对象
			nSize = (unsigned int)pVecLabels.size();
			for (j = 0; j<nSize; j++)
			{
				pVecLabels[j]->Serialize(scanObjects, true);
			}

			//线对象
			nSize = (unsigned int)pVecPolylines.size();
			for (j = 0; j<nSize; j++)
			{
				pVecPolylines[j]->Serialize(scanObjects, true);
			}

			//GPS控制点对象
			nSize = (unsigned int)pVecGPSPts.size();
			for(j = 0;j <nSize;j++)
			{
				pVecGPSPts[j]->Serialize(scanObjects,true);
			}

			//图片控制点对象
			nSize = (unsigned int)pVecPictures.size();
			for(j = 0;j <nSize;j++)
			{
				pVecPictures[j]->Serialize(strWorkspacePath, scanObjects,true);
			}
		}
		else
		{
			string strValue;
			string strText;

			// 从指定节点处进行遍历读取至内存
			//TiXmlElement* nextElement = element->FirstChildElement();
			TiXmlElement* nextElement = element;
			if (nextElement)
			{
				strValue = nextElement->Value();

				// 对象节点
				//if (strValue == "iScans")
				{
					// 遍历迭代
					TiXmlElement* objectElement = nextElement->FirstChildElement();
					while (objectElement)
					{
						strValue = objectElement->Value();
						if (strValue == "Point")
						{
							CHdSxPoint3D* newPoint = new CHdSxPoint3D;
							newPoint->Serialize(objectElement, false);
							pVecPoints.push_back(newPoint);
						}
						else if (strValue == "Label")
						{
							CHdLabel* newLabel =  new CHdLabel;
							newLabel->Serialize(objectElement, false);
							pVecLabels.push_back(newLabel);
						}
						else if (strValue == "Polyline")
						{
							CHdSxPolyline3D* newPolyline = new CHdSxPolyline3D;
							newPolyline->Serialize(objectElement, false);
							pVecPolylines.push_back(newPolyline);
						}
						else if (strValue == "GPSPoint")
						{
							CHdGPSPoint* newGPSpt = new CHdGPSPoint;
							newGPSpt->Serialize(objectElement,false);
							pVecGPSPts.push_back(newGPSpt);
						}
						else if (strValue == "Picture")
						{
							CHdPicture* newImgPt = new CHdPicture;
							newImgPt->Serialize(strWorkspacePath, objectElement, false);
							pVecPictures.push_back(newImgPt);
						}

						objectElement = objectElement->NextSiblingElement();
					}// while (objectElement)
				}// if (strValue == "iScanObjects")

				//nextElement = nextElement->NextSiblingElement();

			}// if (nextElement)
	
		}// if (bSave == false)
	}

}

