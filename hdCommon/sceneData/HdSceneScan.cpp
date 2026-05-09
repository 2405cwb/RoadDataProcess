#include "StdAfx.h"
#include <iterator>
#include "HdSceneScan.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	CHdSceneScan::CHdSceneScan()
	{
		//m_eType = ESDT_SCENE_SCAN;
		pScan = NULL;
		transModel.getIdentity();
		m_MCamPicObj = NULL;
	}

	CHdSceneScan::CHdSceneScan(_HD_SCAN_SCAN& hdss)
	{
		//m_eType = ESDT_SCENE_SCAN;
		pScan = new HD_SCAN_SCAN(hdss);
		transModel.getIdentity();
		m_MCamPicObj = NULL;
	}

	CHdSceneScan::CHdSceneScan(CHdSceneScan& hdss)
	{
		//m_eType = ESDT_SCENE_SCAN;

		if (hdss.pScan)
		{
			pScan = new HD_SCAN_SCAN(*(hdss.pScan));
		}

		size_t i;
		size_t nCount = hdss.pPoints.size();
		for (i = 0; i<nCount; i++)
		{
			CHdSxPoint3D* newPoint = new CHdSxPoint3D(*(hdss.pPoints[i]));
			pPoints.push_back(newPoint);
		}

		nCount = hdss.pLabels.size();
		for (i = 0; i<nCount; i++)
		{
			CHdLabel* newLabel = new CHdLabel(*(hdss.pLabels[i]));
			pLabels.push_back(newLabel);
		}

		nCount = hdss.pPolylines.size();
		for (i = 0; i<nCount; i++)
		{
			CHdSxPolyline3D* newPolyline = new CHdSxPolyline3D(*(hdss.pPolylines[i]));
			pPolylines.push_back(newPolyline);
		}

		nCount = hdss.pSpheres.size();
		for (i = 0; i<nCount; i++)
		{
			CHdSphere* newSphere = new CHdSphere(*(hdss.pSpheres[i]));
			pSpheres.push_back(newSphere);
		}

		nCount = hdss.pChessborads.size();
		for (i = 0; i< nCount;i++)
		{
			CHdKeyboard* newChess = new CHdKeyboard(*(hdss.pChessborads[i]));
			pChessborads.push_back(newChess);
		}

		nCount = hdss.pPlanes.size();
		for (i = 0; i< nCount;i++)
		{
			CHdPlane* newPlane = new CHdPlane(*(hdss.pPlanes[i]));
			pPlanes.push_back(newPlane);
		}

		//nCount = hdss.m_MCamPicObjs.size();
		//for (i = 0; i< nCount;i++)
		//{
		//	CHdPicture* newPic = new CHdPicture(*(hdss.m_MCamPicObjs[i]));
		//	m_MCamPicObjs.push_back(newPic);
		//}
		CHdPicture* newPic = new CHdPicture(*(hdss.m_MCamPicObj));

		transModel = hdss.transModel;
	}

	CHdSceneScan::~CHdSceneScan()
	{
		if (pScan)
		{
			delete pScan;
			pScan = NULL;
		}

		size_t i;
		size_t nCount = pPoints.size();
		for (i = 0; i<nCount; i++)
		{
			delete pPoints[i];
			pPoints[i] = NULL;
		}

		nCount = pLabels.size();
		for (i = 0; i<nCount; i++)
		{
			delete pLabels[i];
			pLabels[i] = NULL;
		}

		nCount = pPolylines.size();
		for (i = 0; i<nCount; i++)
		{
			delete pPolylines[i];
			pPolylines[i] = NULL;
		}

		nCount = pSpheres.size();
		for (i = 0; i<nCount; i++)
		{
			delete pSpheres[i];
			pSpheres[i] = NULL;
		}

		nCount = pChessborads.size();
		for (i = 0; i< nCount; i++)
		{
			delete pChessborads[i];
			pChessborads[i] = NULL;
		}

		nCount = pPlanes.size();
		for (i = 0; i< nCount; i++)
		{
			delete pPlanes[i];
			pPlanes[i] = NULL;
		}

		nCount = pFeaturePts.size();
		for (i = 0; i < nCount; i++)
		{
			delete pFeaturePts[i];
			pFeaturePts[i] = NULL;
		}

		//nCount = m_MCamPicObjs.size();
		//for (i = 0; i < nCount; i++)
		//{
		//	delete m_MCamPicObjs[i];
		//	m_MCamPicObjs[i] = NULL;
		//}

		delete m_MCamPicObj;
		m_MCamPicObj = NULL;
	}

	void CHdSceneScan::setTransModel(CBursaWolfModel& model, bool bTrans)
	{
		transModel = model;

	}
	// 添加模型文件
	void  CHdSceneScan::AddModelFile(const char* StrModefile)
	{
		m_ModelStationPath.push_back(StrModefile);
	}

	// 删除模型文件中的第i个元素
	void  CHdSceneScan::DeleteModelFile(int nIndex)
	{
		size_t nCount = m_ModelStationPath.size();
		if (nIndex >= 0 && nIndex < (int)nCount)
		{
			m_ModelStationPath.erase(m_ModelStationPath.begin() + nIndex);
		}
	}


	void CHdSceneScan::SerializeMCam(const char* ipcFile,bool bSave)
	{
		// 从文件到内存
		if (!bSave)
		{
			m_MCamPics.resize(42);

			TiXmlDocument doc(ipcFile);
			if (!doc.LoadFile())
			{
				m_MCamPics.resize(0);
				return;
			}	

			TiXmlElement* rootElement = doc.RootElement();
			if (rootElement)
			{
				string strValue;
				string strText;

				//判断是否为HDScenePicture文件;
				strValue = rootElement->Value();
				if (strValue != "HDScenePicture")
				{
					return;
				}
				int i = 0;
				TiXmlElement* firstElement = rootElement->FirstChildElement();
				TiXmlElement* nextElement = firstElement;
				while (nextElement)
				{
					strValue = nextElement->Value();
					if (strValue == "PicInfo")
					{
						HD_MPIC_PARAM& pic = m_MCamPics.at(i);

						pic.Serialize(nextElement,false);

						//pushbackMCamPic(pic.sImageName.c_str(),0,pic.fHoriAngle,pic.fVertiAngle);
						if (i == 0)
						{
							if (m_MCamPicObj)
							{
								delete m_MCamPicObj;
								m_MCamPicObj = NULL;
							}
							m_MCamPicObj = new CHdPicture(pic.sImageName.c_str());
							m_MCamPicObj->m_bSphere = 0;
							m_MCamPicObj->m_fHoriAngle = pic.fHoriAngle;
							m_MCamPicObj->m_fVertAngle = pic.fVertiAngle;
							// 记录图片文件所在的目录
							string tmpStr = ipcFile;
							tmpStr = tmpStr.substr(0,tmpStr.find_last_of("\\") + 1);
							m_MCamPicObj->m_strFullName = tmpStr;
						}

						i++;
					}
					else if (strValue == "CameraInfo")
					{
						mCamParam.Serialize(nextElement,false);

						// D800 广角镜头
						if (mCamParam.type == 1)
						{
							m_MCamPics.resize(mCamParam.nCount);
							//m_MCamPics.resize(1);
						}
					}
					nextElement = nextElement->NextSiblingElement();
				}

				size_t nCount = m_MCamPics.size();

				vector<HD_MPIC_PARAM> vecJpg;

				for (size_t i = 0; i< nCount;i++)
				{
					HD_MPIC_PARAM prop = m_MCamPics[i];

					if (prop.sImageName != "")
					{
						vecJpg.push_back(prop);
					}
				}

				if (vecJpg.size() > 0)
				{
					m_MCamPics.resize(0);
					// 拷贝
					std::copy(vecJpg.begin(),vecJpg.end(),std::back_inserter(m_MCamPics));

					mCamParam.nCount = m_MCamPics.size();
				}
			}
			else
			{
				return;
			}
		}
		else	// 从内存到文件
		{
			// 写入配置信息
			TiXmlDocument doc;
			TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "UTF-8", "" );
			TiXmlElement* rootElement = new TiXmlElement("HDScenePicture");
			doc.LinkEndChild(decl);
			doc.LinkEndChild(rootElement);

			TiXmlElement* camElement = new TiXmlElement("CameraInfo");
			rootElement->LinkEndChild(camElement);

			mCamParam.Serialize(camElement,true);

			// M-Picture信息
			for (int i = 0;i < m_MCamPics.size();i++)
			{
				HD_MPIC_PARAM& pic = m_MCamPics.at(i);

				TiXmlElement* picElement = new TiXmlElement("PicInfo");
				rootElement->LinkEndChild(picElement);

				pic.Serialize(picElement,true);

			}

			// 保存到文件
			doc.SaveFile(ipcFile);
		}
	}

	void CHdSceneScan::Serialize(const char* strWorkspacePath, TiXmlElement* element, bool bSave)
	{
		if (bSave)
		{
			string strScanName = pScan->strScanFileName.substr(0, pScan->strScanFileName.find_last_of('.'));
			//TiXmlElement* scanName = new TiXmlElement(strScanName.data());
            // 解决点云文件中含有空格导致无法读取问题（张阳 20161229）
            TiXmlElement* scanName = new TiXmlElement("ScanName");
            scanName->SetAttribute("name", strScanName.c_str());
			element->LinkEndChild(scanName);

			TiXmlElement* scanData = new TiXmlElement("ScanData");
			scanName->LinkEndChild(scanData);

			pScan->Serialize(strWorkspacePath, scanData, true);

			//转换模型
			transModel.Serialize(scanName, true);

			//创建的对象
			TiXmlElement* scanObjects = new TiXmlElement("scanObjects");
			scanName->LinkEndChild(scanObjects);

			//点对象
			size_t j;
			size_t nSize = pPoints.size();
			for (j = 0; j<nSize; j++)
			{
				pPoints[j]->Serialize(scanObjects, true);
			}

			//标签对象
			nSize = pLabels.size();
			for (j = 0; j<nSize; j++)
			{
				pLabels[j]->Serialize(scanObjects, true);
			}

			//线对象
			nSize = pPolylines.size();
			for (j = 0; j<nSize; j++)
			{
				pPolylines[j]->Serialize(scanObjects, true);
			}
			//靶球对象
			nSize = pSpheres.size();
			for (j = 0; j < nSize;j++)
			{
				pSpheres[j]->Serialize(scanObjects,true);
			}
			//平面对象
			nSize = pPlanes.size();
			for (j = 0;j< nSize;j++)
			{
				pPlanes[j]->Serialize(scanObjects,true);
			}
			//棋盘格对象
			nSize = pChessborads.size();
			for (j = 0;j< nSize;j++)
			{
				pChessborads[j]->Serialize(scanObjects,true);
			}
			// 特征点对象
			nSize = pFeaturePts.size();
			for (j=0; j < nSize; j++)
			{
				pFeaturePts[j]->Serialize(scanObjects, true);
			}

			// HStarget
			nSize = pHsTargets.size();
			for (j= 0; j < nSize; j++)
			{
				pHsTargets[j]->Serialize(scanObjects, true);
			}
			//定向点对象
			nSize = pOtPoints.size();
			for(j = 0;j <nSize;j++)
			{
				pOtPoints[j]->Serialize(scanObjects,true);
			}

			// 模型 
			TiXmlText* xmlText = NULL;
			TiXmlElement* xmlElement = NULL;
			TiXmlElement *models = new TiXmlElement("Models");
			scanName->LinkEndChild(models);
			for (j = 0; j < m_ModelStationPath.size(); j++)
			{
				xmlElement = new TiXmlElement("Model");
				models->LinkEndChild(xmlElement);	
				xmlText = new TiXmlText(m_ModelStationPath[j].c_str());
				xmlElement->LinkEndChild(xmlText);
			}
			// MCam
			if (m_MCamPicObj)
			{
				TiXmlElement *mcam = new TiXmlElement("MCam");
				scanName->LinkEndChild(mcam);
				string strFull = m_MCamPicObj->m_strFullName;
				string strCut = "";
				string strWorkspace = strWorkspacePath;
				strCut = strFull.substr(strWorkspace.length());
				xmlText = new TiXmlText(strCut.c_str());
				mcam->LinkEndChild(xmlText);
			}
		}
		else
		{
			string strValue;
			string strText;
			TiXmlElement* nextElement = element->FirstChildElement();
			while(nextElement)
			{
				strValue = nextElement->Value();
				if (strValue== "ScanData")
				{
					if (pScan == NULL)
					{
						pScan = new HD_SCAN_SCAN;
					}
					pScan->Serialize(strWorkspacePath, nextElement, false);
				}
				else if (strValue == "TransModel")
				{
					transModel.Serialize(nextElement, false);
				}
				else if (strValue == "scanObjects")
				{
					TiXmlElement* objectElement = nextElement->FirstChildElement();
					while (objectElement)
					{
						strValue = objectElement->Value();
						if (strValue == "Point")
						{
							CHdSxPoint3D* newPoint = new CHdSxPoint3D;
							newPoint->Serialize(objectElement, false);
							pPoints.push_back(newPoint);
						}
						else if (strValue == "Label")
						{
							CHdLabel* newLabel =  new CHdLabel;
							newLabel->Serialize(objectElement, false);
							pLabels.push_back(newLabel);
						}
						else if (strValue == "Polyline")
						{
							CHdSxPolyline3D* newPolyline = new CHdSxPolyline3D;
							newPolyline->Serialize(objectElement, false);
							pPolylines.push_back(newPolyline);
						}
						else if (strValue == "Sphere")
						{
							CHdSphere* newSphere = new CHdSphere;
							newSphere->Serialize(objectElement,false);
							pSpheres.push_back(newSphere);
						}
						else if (strValue == "Chessboard")
						{
							CHdKeyboard* newChess = new CHdKeyboard;
							newChess->Serialize(objectElement,false);
							pChessborads.push_back(newChess);
						}
						else if (strValue == "Plane")
						{
							CHdPlane* newPlane = new CHdPlane;
							newPlane->Serialize(objectElement,false);
							pPlanes.push_back(newPlane);
						}
						else if (strValue == "FeaturePoint")
						{
							CHdFeaturePoint* newFeature = new CHdFeaturePoint;
							newFeature->Serialize(objectElement, false);
							pFeaturePts.push_back(newFeature);
						}
						else if (strValue == "HsTarget")
						{
							CHdHsTarget* newHstarget = new CHdHsTarget;
							newHstarget->Serialize(objectElement, false);
							pHsTargets.push_back(newHstarget);
						}
						else if (strValue == "OrientPoint")
						{
							CHdOrientPoint* newOrtPoint = new CHdOrientPoint;
							newOrtPoint->Serialize(objectElement,false);
							pOtPoints.push_back(newOrtPoint);
						}
						objectElement = objectElement->NextSiblingElement();
					}
				}
				else if (strValue == "Models")
				{
					TiXmlElement* ModelElement = nextElement->FirstChildElement();
					while (ModelElement)
					{
						strValue = ModelElement->Value();
						if (strValue == "Model")
						{
							strText = ModelElement->GetText();
							m_ModelStationPath.push_back(strText);

							ModelElement = ModelElement->NextSiblingElement();
						}
					}		
				}
				else if (strValue == "MCam")
				{	
					strText = nextElement->GetText();
					if (m_MCamPicObj)
					{
						delete m_MCamPicObj;
						m_MCamPicObj = NULL;
					}
					m_MCamPicObj = new CHdPicture();
					string strWork = strWorkspacePath;
					string strFull = strWork + strText;

					m_MCamPicObj->m_strFullName = strFull;
				}
				nextElement = nextElement->NextSiblingElement();
			}
		}
		if (m_MCamPicObj)
		{
			string ipcFile = m_MCamPicObj->m_strFullName + "PointCloud.ipc";
			SerializeMCam(ipcFile.c_str(),bSave);
		}

	}

	void CHdSceneScan::ClearMCam()
	{
		// 清空图片对象
		m_MCamPics.resize(0);

		mCamParam.InitValue();

		if (m_MCamPicObj)
		{
			delete m_MCamPicObj;
			m_MCamPicObj = NULL;
		}
	}

}
