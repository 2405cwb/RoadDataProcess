#pragma once
#pragma  warning(disable:4251)
#include <vector>
#include "HdPicture.h"

using namespace std;
using namespace hd;

//namespace hd
//{
//	namespace ws
//	{
typedef struct _HD_SCAN_PRJINFO 
{
	_HD_SCAN_PRJINFO()
	{
		strprojGUID = "";
		strPrjFileVersion = "";
		strProjName = "";
		strParentProject = "";
		strScanFileBaseName = "";
		nInitialScanNum = 1;
		fPrjLatitude = 30.0;
	}

	string			strprojGUID;		//记录工程的GUID;
	string			strPrjFileVersion;	//记录工程文件的版本;
	string			strProjName;		//记录工程名;
	string			strParentProject;	//记录父工程名;
	string			strScanFileBaseName;//记录扫描文件基础名;
	unsigned int	nInitialScanNum;	//记录扫描文件起始编号;
	double			fPrjLatitude;		//记录扫描工程所在的纬度;
}HD_SCAN_PRJINFO;

typedef struct _HD_SCAN_PARAM
{
	_HD_SCAN_PARAM()
	{
		strScanDate = "";
		strScanTime = "";
		usResolution = 1;
		bHaveColorPics = false;
		nScanSpeed = 1;
		nRows = 0;
		nCols = 0;
		fRowStartAngle = 90.0;
		fRowEndAngle = -45;//-62.5;
		fColStartAngle = 0.0;
		fColEndAngle = 360.0;
	}

	_HD_SCAN_PARAM(const _HD_SCAN_PARAM& hdsp)
	{
		strScanDate = hdsp.strScanDate;
		strScanTime = hdsp.strScanTime;
		usResolution = hdsp.usResolution;
		bHaveColorPics = hdsp.bHaveColorPics;
		nScanSpeed = hdsp.nScanSpeed;
		nRows = hdsp.nRows;
		nCols = hdsp.nCols;
		fRowStartAngle = hdsp.fRowStartAngle;
		fRowEndAngle = hdsp.fRowEndAngle;
		fColStartAngle = hdsp.fColStartAngle;
		fColEndAngle = hdsp.fColEndAngle;
	}

	inline void Serialize(TiXmlElement* element, bool bSave)
	{
		if (bSave)
		{
			//本站扫描的扫描参数;
			char strText[128];
			TiXmlText* xmlText = NULL;
			TiXmlElement* xmlElement = NULL;
			TiXmlElement* scanParams = new TiXmlElement("ScanParams");
			element->LinkEndChild(scanParams);

			xmlElement = new TiXmlElement("scanDate");
			scanParams->LinkEndChild(xmlElement);
			xmlText = new TiXmlText(strScanDate.data());
			xmlElement->LinkEndChild(xmlText);

			xmlElement = new TiXmlElement("scanTime");
			scanParams->LinkEndChild(xmlElement);
			xmlText = new TiXmlText(strScanTime.data());
			xmlElement->LinkEndChild(xmlText);

			xmlElement = new TiXmlElement("resolution");
			scanParams->LinkEndChild(xmlElement);
			sprintf_s(strText,128, "%d", usResolution);
			xmlText = new TiXmlText(strText);
			xmlElement->LinkEndChild(xmlText);

			xmlElement = new TiXmlElement("numRows");
			scanParams->LinkEndChild(xmlElement);
			sprintf_s(strText,128, "%d", nRows);
			xmlText = new TiXmlText(strText);
			xmlElement->LinkEndChild(xmlText);

			xmlElement = new TiXmlElement("numCols");
			scanParams->LinkEndChild(xmlElement);
			sprintf_s(strText,128, "%d", nCols);
			xmlText = new TiXmlText(strText);
			xmlElement->LinkEndChild(xmlText);

			xmlElement = new TiXmlElement("rowStartAngle");
			scanParams->LinkEndChild(xmlElement);
			sprintf_s(strText,128, "%f", fRowStartAngle);
			xmlText = new TiXmlText(strText);
			xmlElement->LinkEndChild(xmlText);

			xmlElement = new TiXmlElement("rowEndAngle");
			scanParams->LinkEndChild(xmlElement);
			sprintf_s(strText,128, "%f", fRowEndAngle);
			xmlText = new TiXmlText(strText);
			xmlElement->LinkEndChild(xmlText);

			xmlElement = new TiXmlElement("colStartAngle");
			scanParams->LinkEndChild(xmlElement);
			sprintf_s(strText,128, "%f", fColStartAngle);
			xmlText = new TiXmlText(strText);
			xmlElement->LinkEndChild(xmlText);

			xmlElement = new TiXmlElement("colEndAngle");
			scanParams->LinkEndChild(xmlElement);
			sprintf_s(strText,128, "%f", fColEndAngle);
			xmlText = new TiXmlText(strText);
			xmlElement->LinkEndChild(xmlText);

			xmlElement = new TiXmlElement("colorPictures");
			scanParams->LinkEndChild(xmlElement);
			sprintf_s(strText,128, "%d", bHaveColorPics);
			xmlText = new TiXmlText(strText);
			xmlElement->LinkEndChild(xmlText);

			xmlElement = new TiXmlElement("scanSpeed");
			scanParams->LinkEndChild(xmlElement);
			sprintf_s(strText,128, "%d", nScanSpeed);
			xmlText = new TiXmlText(strText);
			xmlElement->LinkEndChild(xmlText);
		}
		else
		{
			string strValue;
			string strText;
			TiXmlElement* paramChildElement = element->FirstChildElement();
			while(paramChildElement)
			{
				strValue = paramChildElement->Value();
				if (paramChildElement->GetText() == NULL)
				{
					strText = "";
				}
				else
					strText = paramChildElement->GetText();
				if (strValue == "scanDate")
				{
					strScanDate = strText;
				}
				else if (strValue == "scanTime")
				{
					strScanTime = strText;
				}
				else if (strValue == "resolution")
				{
					usResolution = (unsigned short)(atoi(strText.data()));
				}
				else if (strValue == "numRows")
				{
					nRows = (unsigned int)(atoi(strText.data()));
				}
				else if (strValue == "numCols")
				{
					nCols = (unsigned int)(atoi(strText.data()));
				}
				else if (strValue == "rowStartAngle")
				{
					fRowStartAngle = (float)(atof(strText.data()));
				}
				else if (strValue == "rowEndAngle")
				{
					fRowEndAngle = (float)(atof(strText.data()));
				}
				else if (strValue == "colStartAngle")
				{
					fColStartAngle = (float)(atof(strText.data()));
				}
				else if (strValue == "colEndAngle")
				{
					fColEndAngle = (float)(atof(strText.data()));
				}
				else if (strValue == "colorPictures")
				{
					bHaveColorPics = (atoi(strText.data()) != 0);
				}
				else if (strValue == "scanSpeed")
				{
					nScanSpeed = (short)(atoi(strText.data()));
				}

				paramChildElement = paramChildElement->NextSiblingElement();
			}
		}
	}

	string			strScanDate;		//记录当前扫描的扫描日期;
	string			strScanTime;		//记录当前扫描的扫描结束时间;
	unsigned short	usResolution;		//记录当前扫描测站的扫描分辨率;
	bool			bHaveColorPics;		//记录当前扫描测站是否存在彩色照片;
	short			nScanSpeed;			//记录扫描的扫描质量（扫描速度）;
	unsigned int	nRows;				//记录扫描的行数;
	unsigned int	nCols;				//记录扫描的列数;
	double			fRowStartAngle;		//记录扫描的垂直开始扫描角度;
	double			fRowEndAngle;		//记录扫描的垂直结束扫描角度;
	double			fColStartAngle;		//记录扫描的水平开始扫描角度;
	double			fColEndAngle;		//记录扫描的水平结束扫描角度;
}HD_SCAN_PARAM;

typedef struct _HD_SCAN_CAMERA_DATA
{
	_HD_SCAN_CAMERA_DATA()
	{
	}

	~_HD_SCAN_CAMERA_DATA()
	{
		Clear();
	}

	_HD_SCAN_CAMERA_DATA(const _HD_SCAN_CAMERA_DATA& hdscd)
	{
		Clear();

		unsigned int i;
		unsigned int nCount = (unsigned int)hdscd.pictures.size();
		for (i = 0; i<nCount; i++)
		{
			CHdPicture* newPic = new CHdPicture;
			*newPic = *(hdscd.pictures[i]);
			pictures.push_back(newPic);
		}
	}

	inline void remove(int nIndex)
	{
		size_t nCount = pictures.size();
		if (nIndex >= 0 && nIndex < (int)nCount)
		{
			delete pictures[nIndex];
			pictures[nIndex] = NULL;
			pictures.erase(pictures.begin() + nIndex);
		}
	}

	inline void insert(int nIndex, const char* strPic, bool bSphere = 0, double fHoriAngle = 0.0, double fVertAngle = 0.0)
	{
		if (nIndex >= 0)
		{
			CHdPicture* newPic = new CHdPicture(strPic);
			newPic->m_bSphere = bSphere;
			newPic->m_fHoriAngle = fHoriAngle;
			newPic->m_fVertAngle = fVertAngle;

			pictures.insert(pictures.begin()+nIndex, newPic);
		}
	}

	inline void pushback(const char* strPic, bool bSphere = 0, double fHoriAngle = 0.0, double fVertAngle = 0.0)
	{
		CHdPicture* newPic = new CHdPicture(strPic);
		newPic->m_bSphere = bSphere;
		newPic->m_fHoriAngle = fHoriAngle;
		newPic->m_fVertAngle = fVertAngle;

		pictures.push_back(newPic);
	}

	inline void Clear()
	{
		size_t i;
		size_t nCount = pictures.size();
		for (i = 0; i<nCount; i++)
		{
			delete pictures[i];
			pictures[i] = NULL;
		}

		pictures.clear();
	}

	inline void Serialize(const char* strScanPath, TiXmlElement* element, bool bSave)
	{
		if (bSave)
		{
			TiXmlElement* scanCameraData = new TiXmlElement("scanCameraData");
			element->LinkEndChild(scanCameraData);

			unsigned int i;
			unsigned int nCount = (unsigned int)pictures.size();
			for (i = 0; i<nCount; i++)
			{
				pictures[i]->Serialize(strScanPath, scanCameraData, true);
			}
		}
		else
		{
			string strValue;
			TiXmlElement* nextElement = element->FirstChildElement();
			while(nextElement)
			{
				strValue = nextElement->Value();
				if (strValue == "Picture")
				{
					CHdPicture* newPic = new CHdPicture;
					newPic->Serialize(strScanPath, nextElement, false);
					pictures.push_back(newPic);
				}

				nextElement = nextElement->NextSiblingElement();
			}
		}
	}

	vector<CHdPicture*> pictures;		//记录测站中所有照片对象
}HD_SCAN_CAMERA_DATA;

typedef struct HDCOMMON_API _HD_LAS_DATA
{
	_HD_LAS_DATA()
	{
	}
	~_HD_LAS_DATA()
	{
	}
	_HD_LAS_DATA(const _HD_LAS_DATA& las)
	{
		lasPath = las.lasPath;
	}

	inline void Serialize(TiXmlElement* element, bool bSave)
	{
		if (bSave)
		{			
			TiXmlElement* lasData = new TiXmlElement("LasPointCloud");
			lasData->SetAttribute("path",lasPath.c_str());
			element->LinkEndChild(lasData);
		}
		else
		{
			lasPath = element->Attribute("path");
		}
	}

	string lasPath;
}HD_LAS_DATA;

typedef struct _HD_MLS_DATA
{
	_HD_MLS_DATA()
	{
	}

	~_HD_MLS_DATA()
	{
		Clear();
	}

	_HD_MLS_DATA(const _HD_MLS_DATA& hdscd)
	{
		Clear();

		unsigned int i;
		unsigned int nCount = (unsigned int)hdscd.pictures.size();
		for (i = 0; i<nCount; i++)
		{
			CHdPicture* newPic = new CHdPicture;
			*newPic = *(hdscd.pictures[i]);
			pictures.push_back(newPic);
		}
	}

	inline void remove(int nIndex)
	{
		size_t nCount = pictures.size();
		if (nIndex >= 0 && nIndex < (int)nCount)
		{
			delete pictures[nIndex];
			pictures[nIndex] = NULL;
			pictures.erase(pictures.begin() + nIndex);
		}
	}

	inline void insert(int nIndex, const char* strPic, bool bSphere = 0, double fHoriAngle = 0.0, double fVertAngle = 0.0)
	{
		if (nIndex >= 0)
		{
			CHdPicture* newPic = new CHdPicture(strPic);
			newPic->m_bSphere = bSphere;
			newPic->m_fHoriAngle = fHoriAngle;
			newPic->m_fVertAngle = fVertAngle;

			pictures.insert(pictures.begin()+nIndex, newPic);
		}
	}

	inline void pushback(const char* strPic, bool bSphere = 0, double fHoriAngle = 0.0f, double fVertAngle = 0.0f)
	{
		CHdPicture* newPic = new CHdPicture(strPic);
		newPic->m_bSphere = bSphere;
		newPic->m_fHoriAngle = fHoriAngle;
		newPic->m_fVertAngle = fVertAngle;

		pictures.push_back(newPic);
	}

	inline void Clear()
	{
		unsigned int i;
		unsigned int nCount = pictures.size();
		for (i = 0; i<nCount; i++)
		{
			delete pictures[i];
			pictures[i] = NULL;
		}

		pictures.clear();
	}

	inline void Serialize(TiXmlElement* element, bool bSave)
	{
		if (bSave)
		{			
			TiXmlElement* mlsData = new TiXmlElement("mlsPointCloud");
			mlsData->SetAttribute("path",mlsPath.c_str());
			element->LinkEndChild(mlsData);

			unsigned int i;
			unsigned int nCount = pictures.size();
			for (i = 0; i<nCount; i++)
			{
				pictures[i]->Serialize(mlsPath.c_str(), mlsData, true);
			}
		}
		else
		{
			mlsPath = element->Attribute("path");
			string strValue;
			TiXmlElement* nextElement = element->FirstChildElement();
			while(nextElement)
			{
				strValue = nextElement->Value();
				if (strValue == "Picture")
				{
					CHdPicture* newPic = new CHdPicture;
					newPic->Serialize(mlsPath.c_str(), nextElement, false);
					pictures.push_back(newPic);
				}

				nextElement = nextElement->NextSiblingElement();
			}
		}
	}

	string				mlsPath;		//mls点云路径
	vector<CHdPicture*> pictures;		//记录所有照片对象
}HD_MLS_DATA;

typedef struct _HD_SCAN_SCAN 
{
	_HD_SCAN_SCAN()
	{
		strScanPath = "";
		strScanFileName = "";
		pHdScanParam = NULL;
		pHdScanCameraData = NULL;
	}

	_HD_SCAN_SCAN(const _HD_SCAN_SCAN& hdss)
	{
		strScanPath = hdss.strScanPath;
		strScanFileName = hdss.strScanFileName;
		pHdScanParam = new HD_SCAN_PARAM(*(hdss.pHdScanParam));
		pHdScanCameraData = new HD_SCAN_CAMERA_DATA(*(hdss.pHdScanCameraData));
	}

	~_HD_SCAN_SCAN()
	{
		if (pHdScanParam)
		{
			delete pHdScanParam;
			pHdScanParam = NULL;
		}

		if (pHdScanCameraData)
		{
			delete pHdScanCameraData;
			pHdScanCameraData = NULL;
		}
	}

	inline void Serialize(const char* strWorkspacePath, TiXmlElement* element, bool bSave)
	{
		if (bSave)
		{
			string strScanName = strScanFileName.substr(0, strScanFileName.find_last_of('.'));

			//本站扫描的扫描数据路径;
			TiXmlElement* scanPath = new TiXmlElement("ScanFile");
			element->LinkEndChild(scanPath);
			string strPath = "Scans\\"+ strScanName + "\\" + strScanFileName;		//  WorkspacePath\Scans\ScanName\ScanName.jpg
			TiXmlText* xmlText = new TiXmlText(strPath.data());
			scanPath->LinkEndChild(xmlText);

			pHdScanParam->Serialize(element, true);
			pHdScanCameraData->Serialize(strScanPath.data(), element, true);
		}
		else
		{
			string strValue;
			string strText;
			TiXmlElement* nextElement = element->FirstChildElement();
			while(nextElement)
			{
				strValue = nextElement->Value();
				if (strValue==  "ScanFile")
				{
					strText = nextElement->GetText();
					strScanPath = strWorkspacePath;
					strScanPath += strText.substr(0,strText.find_last_of('\\') + 1);
					strScanFileName = strText.substr(strText.find_last_of('\\') + 1);
				}
				else if (strValue == "ScanParams")
				{
					if (pHdScanParam == NULL)
					{
						pHdScanParam = new HD_SCAN_PARAM;
					}
					pHdScanParam->Serialize(nextElement, false);
				}
				else if (strValue == "scanCameraData")
				{
					if (pHdScanCameraData == NULL)
					{
						pHdScanCameraData = new HD_SCAN_CAMERA_DATA;
					}
					pHdScanCameraData->Serialize(strScanPath.data(), nextElement, false);
				}

				nextElement = nextElement->NextSiblingElement();
			}
		}
	}

	string					strScanPath;		//记录本站扫描文件夹路径;
	string					strScanFileName;	//记录扫描文件名;
	HD_SCAN_PARAM*			pHdScanParam;		//记录本站扫描参数;
	HD_SCAN_CAMERA_DATA*	pHdScanCameraData;	//记录本站相机数据;
}HD_SCAN_SCAN;

enum EHD_SCAN_FILE
{
	EHDSF_CAMERA_PARAM = 0,				//扫描相机参数文件;
	EHDSF_PICTURES,						//扫描相片文件，如果GetScanFile中传入此参数，则只返回照片文件夹路径;
	EHDSF_PARAM,						//扫描参数文件;
	EHDSF_DATA,							//扫描数据文件;
	EHDSF_LOG,							//扫描日志文件;
	EHDSF_GREY_PICTURE,					//扫描灰度图;
	EHDSF_COUNT							//类型总数;
};


//	}
//}