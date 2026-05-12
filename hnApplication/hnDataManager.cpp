#include "hnDataManager.h"
#include <QCoreApplication>
#include <QDir>
#include "..\hnProject\hnProjectManager.h"
#include "..\hnProject\hnFile.h"
#include "..\hnDataTable\hnDBSqliteRoadInfo.h"
#include <QXmlStreamReader>
#include <QDebug>
#include "..\hnConfigService\HnXRSettings.h"
#include <sstream>
#include "..\hnProject\hn3DProject.h"
#include <QSettings>
//using namespace hnDataTable;
#include "../hdPointCloud/SeaPointCloud.h"
#include "..\hnPointCloud\hnPointCloud.h"
#include "..\hnDiseaseService.h"
using namespace hnPtCloud;
using namespace hd;
using namespace hnPro;

namespace hnApp
{
	
	hnDataManager* hnDataManager::m_pDataMagager = NULL;
	
	map<HnProjectEnums::StandardParmTypeEnum, vector<hnDiseaseSetInfo>> hnDataManager::m_mapDiseaseSetInfo;

	map<HnProjectEnums::StandardParmTypeEnum, vector<hnRoadTypeSetInfo>> hnDataManager::m_mapRoadTypeSetInfo;

	hnDataManager::hnDataManager() :m_pProjectManager(NULL), m_pCurProject(NULL), m_bOpenProject(false),m_strPreProName(""), m_bHasProject(false)
	{
		m_xrSetting = HnXRSettings::getInstance();
		m_diseaseService = new hnDiseaseService();
	}


	hnDataManager::~hnDataManager()
	{
		m_strPreProName = "";

		for (int i = 0; i < m_vecPtCloud.size(); i++)
		{
			if (m_vecPtCloud[i])
			{
				delete m_vecPtCloud[i];
				m_vecPtCloud[i] = NULL;
			}
		}

		m_vecPtCloud.clear();
		delete m_diseaseService;
		m_diseaseService = nullptr;
	}

	// 根据传入路径获取所有工程
	bool hnDataManager::getAllProject(QString strFolder,  vector<hnProjectDataInfo>& vecProData, hnCommon::PROJECT_TYPE&nWorkType)
	{

		vector<QString> vecXmlFile;
		vector<QString> vecTxtFile;
		 nWorkType = PROJECT_2D_TYPE;
		if (findFile(strFolder, "ProjectInfo.xml", vecXmlFile))
		{
			nWorkType = PROJECT_23D_TYPE; //暂时默认为23d混合
		}
		//
		//这个地方需要考虑 是否应该根据目录结构自动判断每个项目是 哪种模式
		if (nWorkType == PROJECT_23D_TYPE || nWorkType == PROJECT_JD_3D_TYPE || nWorkType == PROJECT_XD_3D_TYPE) // 作业方式为二三维一体化或三维模式
		{
			// 查找该文件夹下的所有ProjectInfo.xml文件 
			vector<hnProjectDataInfo> vecTempProject;
			for (int i = 0; i < vecXmlFile.size(); i++)
			{
				QString  path_tmp = vecXmlFile[i];
				 
				int index = path_tmp.lastIndexOf('.');
				if (index > 0)
				{
					QString strExp = path_tmp.right(path_tmp.length() - index - 1);
					strExp = strExp.toLower();
					if (!strExp.contains("xml")) // 后缀判断;
					{
						continue;
					}
				}
				if (!analysisXml(vecXmlFile[i], vecTempProject))
				{
					continue;
				}

				vecProData.insert(vecProData.end(), vecTempProject.begin(), vecTempProject.end());
				vecTempProject.clear();
			}

			if (vecProData.size() <= 0)
			{
				return false;
			}
		}
		else if (nWorkType == PROJECT_2D_TYPE) // 作业模式为纯2D模式
		{
			// 查找该文件夹下的所有ProjectInfo.txt文件
			vector<QString> vecTxtFile;
			if (!findFile(strFolder, "ProjectInfo.txt", vecTxtFile))
			{
				return false;
			}
			// 根据

			for (int i = 0; i < vecTxtFile.size(); i++)
			{
				hnProjectDataInfo curProData;

			
				if (!analysisProjectInfo(vecTxtFile[i], curProData))
				{
					continue;
				}
				//解析CamSetting文件
				analysisCamSetting(vecTxtFile[i].replace("ProjectInfo.txt","CamSetting.ini"),curProData.proSetInfo.dRoadWidth);
				curProData.proSetInfo.nWorkType == PROJECT_2D_TYPE;
				vecProData.push_back(curProData);
			}

			if (vecProData.size() <= 0)
			{
				return false;
			}
			
		}

		return true;
	}

	// 打开工程
	bool hnDataManager::initProject(vector<hnProjectDataInfo>& vecProData, QProgressDialog& progress)
	{
		if (!m_pProjectManager)
		{
			m_pProjectManager = new hnProjectManager();

			if (m_pProjectManager->addProject(vecProData,progress))
			{
				this->m_pCurProject = m_pProjectManager->getCurProject();
				this->m_bHasProject = true;
			}
			else
			{
				closeProject();
				this->m_bHasProject = false;
				return false;
			}
		}
		
		//m_bOpenProject = true;
		return true;
	}


	bool hnDataManager::setCurrentProject(const QString& proName)
	{
		if (!m_pProjectManager->setCurProject(proName))
		{
			return false; 
			 
		}  
		this->m_bOpenProject = true;
		this->m_pCurProject = m_pProjectManager->getCurProject();

		if (proName == m_strPreProName)
		{
		
			return true;
		}
		 if (m_pCurProject->getProjectType()!= PROJECT_2D_TYPE)
		 { 
			 // 先清空已有点云数据
			 for (int i = 0; i < m_vecPtCloud.size(); i++)
			 {
				 if (m_vecPtCloud[i])
				 {
					 delete m_vecPtCloud[i];
					 m_vecPtCloud[i] = NULL;
				 }
			 }

			 m_vecPtCloud.clear();

			 // 加载点云数据
			 string strPcdPath = "";
			 QVector<QString> vecPcdPath = m_pProjectManager->getCurProject()->get3DProject()->getJDCloudPath();
			 for (int i = 0; i < vecPcdPath.size(); i++)
			 {
				 strPcdPath = vecPcdPath[i].toLocal8Bit();
				 CSeaPointCloud* ptCloud = new CSeaPointCloud();
				 if (!ptCloud->open(strPcdPath.c_str()))
				 {
					 delete ptCloud;
					 ptCloud = NULL;

					 continue;
				 }

				 m_vecPtCloud.push_back(ptCloud);
			 }
		 }
	

		m_strPreProName = proName;
		setCurRoadTypeName(m_pCurProject->getBaseStandard());
		return true;
	}

	hnPro::hnProject* hnDataManager::getCurrentProject()
	{
		if (!m_pCurProject)
		{
			if (m_pProjectManager)
			{ 
				m_pCurProject = m_pProjectManager->getCurProject(); 
			}
			else
			{
				return nullptr;
			}
			
		}
		/*if (m_pCurProject)
		{
			setCurRoadTypeName(m_pCurProject->getBaseStandard());

		}*/
		return m_pCurProject;
	}

	// 关闭工程
	void hnDataManager::closeProject()
	{
		if (m_pProjectManager)
		{
			m_pProjectManager->closeProject();
			delete m_pProjectManager;
			m_pProjectManager = NULL;

			m_pCurProject = NULL;
			m_bOpenProject = false;
		}

		if (m_pCurProject)
		{
			delete m_pCurProject;
			m_pCurProject = NULL;

			m_bOpenProject = false;
		}
	   
		m_strPreProName = "";

		for (int i = 0; i < m_vecPtCloud.size(); i++)
		{
			if (m_vecPtCloud[i])
			{
				delete m_vecPtCloud[i];
				m_vecPtCloud[i] = NULL;
			}
		}

		m_vecPtCloud.clear();

		m_vecPosInfo.clear();
	}

	void hnDataManager::closeCurrentProject()
	{
		if (this->m_pCurProject!=NULL)
		{
			m_pCurProject = nullptr;
			//m_pCurProject = m_pProjectManager->getCurProject();
			m_bOpenProject = false;
		}
	}

	// 工程是否打开
	bool hnDataManager::isOpenProject()
	{
		return m_bOpenProject;
	}

	bool hnDataManager::isHasProject()
	{
		return m_bHasProject;
	}

	hnDataManager* hnDataManager::getDataManager()
	{
		if (!m_pDataMagager)
		{
			m_pDataMagager = new hnDataManager();
		}

		return m_pDataMagager;
	}

	void hnDataManager::destoryDataManager()
	{
		if (m_pDataMagager)
		{
			delete m_pDataMagager;
			m_pDataMagager = NULL;
		}
	}

	// 初始化道路规范参数
	bool hnDataManager::initRoadStandardInfo()
	{
		// 道路类型参数数据库根目录
		QString strSettingFolder = QCoreApplication::applicationDirPath();
 
#ifdef DEBUG
		//strSettingFolder = "D:/01Soft/01hnRoadDataProgress/24-hnRoadDataProcess/hnRoadDataProcess/bin/Debug-X64";

#endif // DEBUG

		strSettingFolder = strSettingFolder + "/RoadParamDB";
		QDir dirFolder(strSettingFolder);
		if (!dirFolder.exists())
		{
			return false;
		}

		m_vecRoadStandard.clear();

		// 获取所有db子文件
		vector<QString> vecDBName;
		hnFile fileManager;
		fileManager.getFileType(strSettingFolder, "db", vecDBName);
		if (vecDBName.size() <= 0)
		{
			return false;
		}

		// 
		QStringList listValue;

		// 获取每个文件夹下的参数
		QString strDBPath = "";
		string strPath = "";
		vector<hnRoadTypeSetInfo> vecRoadTypeSetInfo;
		vector<hnDiseaseSetInfo> vecRoadDiseaseInfo;
		for (int i = 0; i < vecDBName.size(); i++)
		{
			vecRoadTypeSetInfo.clear();
			vecRoadDiseaseInfo.clear();

			strDBPath = strSettingFolder + "/" + vecDBName[i];
			strPath = strDBPath.toLocal8Bit();

			// 道路参数数据库
			hnDBSqliteRoadInfo dbSqlitInfo(strPath.c_str());
			if (!dbSqlitInfo.connectDB())
			{
				continue;
			}

			///////////////tst///////////////

			////////////////////////////////////////////////

			// 读取参数
			dbSqlitInfo.m_roadTypeSetTable.readData(vecRoadTypeSetInfo);
			dbSqlitInfo.m_diseaseSetTable.readData(vecRoadDiseaseInfo);

			if (vecRoadDiseaseInfo.size() <= 0 || vecRoadDiseaseInfo.size() <= 0)
			{
				continue;
			}

			//根据病害索引进行排序
			  sort(vecRoadDiseaseInfo.begin(), vecRoadDiseaseInfo.end(), [](const hnDiseaseSetInfo& a,const hnDiseaseSetInfo& b) {
				  return a.nDiseaseIndex < b.nDiseaseIndex;

			});

			listValue = vecDBName[i].split(".");
			strDBPath = listValue[0];
			HnProjectEnums::StandardParmTypeEnum type = HnProjectEnums::roadTypeQStringToEnum(strDBPath);
			m_vecRoadStandard.push_back(type);
			
			m_mapDiseaseSetInfo[type] = vecRoadDiseaseInfo;

			m_mapRoadTypeSetInfo[type] = vecRoadTypeSetInfo;
		}

		if (m_vecRoadStandard.size() <= 0)
		{
			return false;
		}

		setCurRoadTypeName(m_vecRoadStandard[0]);

		return true;
	}

	// 设置当前道路类型
	bool hnDataManager::setCurRoadTypeName(HnProjectEnums::StandardParmTypeEnum strRoadTypeNam)
	{
		// 判断是否存在
		bool bExist = false;
		for (int i = 0; i < m_vecRoadStandard.size(); i++)
		{
			if (strRoadTypeNam != m_vecRoadStandard[i])
			{
				continue;
			}

			bExist = true;
			break;
		}

		if (!bExist)
		{
			return false;
		}

		m_strCurRoadTypeName = strRoadTypeNam;
		m_curRoadTypeSetInfo = m_mapRoadTypeSetInfo[m_strCurRoadTypeName];
		m_curDiseaseSetInfo = m_mapDiseaseSetInfo[m_strCurRoadTypeName];

		// 查找当前道路类型下的道路等级和道路材质类型
		m_vecRoadLevel.clear();
		m_vecRoadSurfaceType.clear();
		QString strValue = "";

		for (int i = 0; i < m_curRoadTypeSetInfo.size(); i++)
		{
			// 道路等级
			if (!findData(m_vecRoadLevel, m_curRoadTypeSetInfo[i].nRoadLevel))
			{
				strValue = QString::fromLocal8Bit(m_curRoadTypeSetInfo[i].nRoadLevel);
				if (std::find(m_vecRoadLevel.begin(),m_vecRoadLevel.end(), strValue) == m_vecRoadLevel.end()
					&& !strValue.isEmpty()) //cwb
				{
					m_vecRoadLevel.push_back(strValue);
				}
				
			}

			if (m_curRoadTypeSetInfo[i].nRSurfaceType == 0)
			{
				strValue = QString::fromLocal8Bit("沥青");
			}
			else if (m_curRoadTypeSetInfo[i].nRSurfaceType == 1)
			{
				strValue = QString::fromLocal8Bit("水泥");
			}
			else
			{
				strValue = QString::fromLocal8Bit("砂石");
			}

			// 道路材质
			if (!findData(m_vecRoadSurfaceType, strValue))
			{
				m_vecRoadSurfaceType.push_back(strValue);
			}
		}

		if (m_vecRoadSurfaceType.size() == 2)
		{
			m_vecRoadSurfaceType.clear();
			m_vecRoadSurfaceType.push_back(QString::fromLocal8Bit("沥青"));
			m_vecRoadSurfaceType.push_back(QString::fromLocal8Bit("水泥"));
			return true;
		}
		else if (m_vecRoadSurfaceType.size() == 3)
		{
			m_vecRoadSurfaceType.clear();
			m_vecRoadSurfaceType.push_back(QString::fromLocal8Bit("沥青"));
			m_vecRoadSurfaceType.push_back(QString::fromLocal8Bit("水泥"));
			m_vecRoadSurfaceType.push_back(QString::fromLocal8Bit("砂石"));
			return true;
		}
		else
		{
			m_vecRoadSurfaceType.clear();
			m_vecRoadSurfaceType.push_back(QString::fromLocal8Bit("沥青"));
			m_vecRoadSurfaceType.push_back(QString::fromLocal8Bit("水泥"));
			return true;
		}

		return false;
	}
 
	void hnDataManager::setDiseaseCalcuteSize( hnRoadDiseaseInfo& dis)
	{
		int drawType = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType;
		HnProjectEnums::StandardParmTypeEnum type = HnProjectEnums::roadTypeQStringToEnum(dis.strRoadStandard);
		hnDiseaseSetInfo setInfo;
		//获得病害设置列表
		if (m_mapDiseaseSetInfo.count(type))
		{
			 
			auto disSettings = m_mapDiseaseSetInfo[type];
			//获得病害属性 
			bool find = false;
			for each (auto disSetting in disSettings)
			{
				if (!find)
				{
					if (std::strcmp( disSetting.strDBTableName ,dis.strDiseaseTableName)==0)
					{
						if (dis.nDrawType == 2|| dis.nDrawType == 3)
						{
							if (disSetting.nLevel == dis.nLevel&& disSetting.nRoadSurfaceType == dis.nRSurfaceType)
							{
								setInfo = disSetting;
								find = true;
							}
						}
						else
						{
							if (disSetting.nDrawType == dis.nDrawType&& disSetting.nLevel == dis.nLevel&& disSetting.nRoadSurfaceType == dis.nRSurfaceType)
							{
								setInfo = disSetting;
								find = true;
							}
						}
						
					}
				}
				else
				{
					break;
				}
			}
			if (find == true)
			{
				calcuteDiseaseSize(drawType,setInfo, dis);
			}
			else
			{
				dis.dArea = 0;
			}
		}

	}

	bool hnDataManager::getRoadTypeSetInfo(HnProjectEnums::StandardParmTypeEnum strRoadStandard, QString strRoadLevel, ROAD_SURFACE_TYPE nRoadSurfaceType, hnRoadTypeSetInfo& roadTypeSetInfo)
	{

		if (!m_mapDiseaseSetInfo.count(strRoadStandard))
		{
			return false;
		}
		vector<hnRoadTypeSetInfo> allRoadTypeSetinfo = 	m_mapRoadTypeSetInfo.at(strRoadStandard);
		// 获取道路设置参数
		for (int i = 0; i <allRoadTypeSetinfo.size(); i++)
		{
			if (strRoadLevel != QString::fromLocal8Bit(allRoadTypeSetinfo[i].nRoadLevel) )
			{
				continue;
			}

			if (nRoadSurfaceType != allRoadTypeSetinfo[i].nRSurfaceType)
			{
				continue;
			}

			roadTypeSetInfo = allRoadTypeSetinfo[i];
			return true;
		}

		return false;
	}

	bool hnDataManager::getRoadTypeSetInfo(const hnProjectSetInfo& settingInfo, hnRoadTypeSetInfo& roadTypeSetInfo)
	{    
		hnCommon::ROAD_WORK_TYPE drawType = (hnCommon::ROAD_WORK_TYPE) settingInfo.nDrawType;
		auto strRoadStandard = HnProjectEnums::roadTypeQStringToEnum(settingInfo.strRoadStandard);
		QString strRoadLevel = QString::fromLocal8Bit(settingInfo.strRoadLevel) ;
		auto  nRoadSurfaceType = settingInfo.nRSurfaceType;
		if (!m_mapDiseaseSetInfo.count(strRoadStandard))
		{
			return false;
		}
		vector<hnRoadTypeSetInfo> allRoadTypeSetinfo = m_mapRoadTypeSetInfo.at(strRoadStandard);
		// 获取道路设置参数
		for (int i = 0; i < allRoadTypeSetinfo.size(); i++)
		{
			if (strRoadLevel != QString::fromLocal8Bit(allRoadTypeSetinfo[i].nRoadLevel))
			{
				continue;
			}

			if (nRoadSurfaceType != allRoadTypeSetinfo[i].nRSurfaceType)
			{
				continue;
			}

			roadTypeSetInfo = allRoadTypeSetinfo[i];
			return true;
		}

		return false;
	}

	// 根据指定道路类型的病害表集合信息
	bool hnDataManager::getTableNameFromRoadType(HnProjectEnums::StandardParmTypeEnum strRoadType, vector<QString>& vecRoadType)
	{
		if (strRoadType != m_strCurRoadTypeName)
		{
			if (!setCurRoadTypeName(strRoadType))
			{
				return false;
			}
		}

		vecRoadType.clear();

		for (int i = 0; i < m_curDiseaseSetInfo.size(); i++)
		{
			if (findData(vecRoadType, m_curDiseaseSetInfo[i].strDBTableName))
			{
				continue;
			}

			vecRoadType.push_back(m_curDiseaseSetInfo[i].strDBTableName);
		}

		if (vecRoadType.size() <= 0)
		{
			return false;
		}

		return true;
	}

	// 获取道路类型
	bool hnDataManager::getRoadTypeName(vector<HnProjectEnums::StandardParmTypeEnum>& vecRoadTypeName)
	{


		if (m_vecRoadStandard.size() <= 0)
		{
			return false;
		}

		vecRoadTypeName = m_vecRoadStandard;

		return true;
	}

	QStringList hnDataManager::getRoadStandardNames()
	{
		QStringList roadTypeNames;
		QString roadTypeName;
		for each (auto roadType in m_vecRoadStandard)
		{
			roadTypeName = HnProjectEnums::roadTypeEnumToQString(roadType);
			if (!roadTypeName.isEmpty())
			{
				roadTypeNames.append(roadTypeName);
			}	
		}
		return roadTypeNames;
	}

	// 根据病害类型名称获取病害设置参数
	bool hnDataManager::getDiseaseSetInfo(QString strDiseaseTypeName, 
		HnProjectEnums::StandardParmTypeEnum strRoadType, ROAD_WORK_TYPE nDrawType,
		ROAD_SURFACE_TYPE nRoadSurfaceType,
		hnDiseaseSetInfo& diseaseSetInfo)
	{
		if (strRoadType != m_strCurRoadTypeName)
		{
			if (!setCurRoadTypeName(strRoadType))
			{
				return false;
			}
		}

		for (int i = 0; i < m_curDiseaseSetInfo.size(); i++)
		{
			if (m_curDiseaseSetInfo[i].nDrawType != nDrawType)
			{
				continue;
			}

			if (m_curDiseaseSetInfo[i].nRoadSurfaceType != nRoadSurfaceType)
			{
				continue;
			}
			QString name = QString::fromLocal8Bit(m_curDiseaseSetInfo[i].strDiseaseTypeName);
			if (name != strDiseaseTypeName)
			{
				continue;
			}
			 

			diseaseSetInfo = m_curDiseaseSetInfo[i];
			return true;
		}

		return false;
	}

	bool hnDataManager::getStreetDiseaseSetInfo(QString strDiseaseTypeName, HnProjectEnums::StandardParmTypeEnum strRoadStandard, hnDiseaseSetInfo& diseaseSetInfo)
	{
		if (strRoadStandard != m_strCurRoadTypeName)
		{
			if (!setCurRoadTypeName(strRoadStandard))
			{
				return false;
			}
		}

		for (int i = 0; i < m_curDiseaseSetInfo.size(); i++)
		{ 
			if (m_curDiseaseSetInfo[i].nDiseaseType==0)
			{
				continue;;
			}
			QString name = QString::fromLocal8Bit(m_curDiseaseSetInfo[i].strDiseaseTypeName);
			if (name!= strDiseaseTypeName)
			{
				continue;
			}

			diseaseSetInfo = m_curDiseaseSetInfo[i];
			return true;
		}

		return false;
	}

	// 获取道路类型参数
	bool hnDataManager::getCurrentRoadTypeSetInfo(HnProjectEnums::StandardParmTypeEnum strRoadType, QString strRoadLevel, ROAD_SURFACE_TYPE nRoadSurfaceType, hnRoadTypeSetInfo& roadTypeSetInfo)
	{
		if (strRoadType != m_strCurRoadTypeName)
		{
			if (!setCurRoadTypeName(strRoadType))
			{
				return false;
			}
		}

		// 获取道路设置参数
		for (int i = 0; i < m_curRoadTypeSetInfo.size(); i++)
		{
			if (strRoadLevel != QString::fromLocal8Bit(m_curRoadTypeSetInfo[i].nRoadLevel))
			{
				continue;
			}

			if (nRoadSurfaceType != m_curRoadTypeSetInfo[i].nRSurfaceType)
			{
				continue;
			}

			roadTypeSetInfo = m_curRoadTypeSetInfo[i];
			return true;
		}

		return false;
	}

	vector<QString> hnDataManager::getRoadSurfaceType()
	{
	 
		 return m_vecRoadSurfaceType; 
	}

	QVector<QString> hnDataManager::getTableNames(HnProjectEnums::StandardParmTypeEnum levelType)
	{
		QVector<QString> result;
		std::vector<hnDiseaseSetInfo> infos;
		auto iter = m_mapDiseaseSetInfo.find(levelType);
		if (iter != m_mapDiseaseSetInfo.end())
		{
			infos =  (*iter).second;
		}

		for (auto info : infos)
		{
			 
			if (!result.contains(QString::fromLocal8Bit(info.strDBTableName)))
			{
				result.push_back(QString::fromLocal8Bit(info.strDBTableName));
			}		
		}

		return result;
	}

	QMap<QString, QString> hnDataManager::getTableNamesDiseaseNamesMap(const HnProjectEnums::StandardParmTypeEnum levelType, int drawType)
	{
		QMap<QString, QString> result;
		std::vector<hnDiseaseSetInfo> infos;
		auto iter = m_mapDiseaseSetInfo.find(levelType);
		if (iter != m_mapDiseaseSetInfo.end())
		{
			infos = (*iter).second;
		}

		for (auto info : infos)
		{
			if (!result.contains(QString::fromLocal8Bit(info.strDBTableName)))
			{
				if (info.nDrawType == drawType)
				{
					result.insert(QString::fromLocal8Bit(info.strDBTableName), QString::fromLocal8Bit(info.strDiseaseName));

				}
			}
		}

		return result;
	}


	QMap<QString, std::vector<hnCommon::hnDiseaseSetInfo>> hnDataManager::getTableNamesDiseaseInfoMap(const HnProjectEnums::StandardParmTypeEnum levelType, int drawType)
	{
		QMap<QString, std::vector<hnCommon::hnDiseaseSetInfo>>result;
		std::vector<hnDiseaseSetInfo> infos;
		auto iter = m_mapDiseaseSetInfo.find(levelType);
		if (iter != m_mapDiseaseSetInfo.end())
		{
			infos = (*iter).second;
		}

		for (auto info : infos)
		{
			if (info.nDrawType != drawType)
			{
				continue;
			}
			if (!result.contains(QString::fromLocal8Bit(info.strDBTableName)))
			{ 
				std::vector<hnDiseaseSetInfo> curInfos;
				curInfos.push_back(info);
				result.insert(QString::fromLocal8Bit(info.strDBTableName), curInfos);
			}
			else
			{
				result[QString::fromLocal8Bit(info.strDBTableName)].push_back(info);
			}
		}

		return result;
	}

	hnCommon::ROAD_SURFACE_TYPE hnDataManager::getRoadSurfaceFromStr(QString surface)
	{
		if (surface.contains(QStringLiteral( "沥青")))
		{
			return hnCommon::ROAD_LQ_SURFACE;
		}
		else	if (surface.contains(QStringLiteral("水泥")))
		{
			return hnCommon::ROAD_SN_SURFACE;
		}
		else if (surface.contains(QStringLiteral("砂石")))
		{
			return hnCommon::ROAD_SS_SURFACE;
		}
		else
		{
			return hnCommon::ROAD_LQ_SURFACE;
		}
	}

	QString hnDataManager::getTableName(const QString & diseaseName)
	{
		if (!this->isOpenProject())
		{
			return "";
		}
		std::vector<hnDiseaseSetInfo> infos;
		//获取当前工程的路面标准
		auto levelType = this->m_pCurProject->getBaseStandard();
		auto iter = m_mapDiseaseSetInfo.find(levelType);
		if (iter != m_mapDiseaseSetInfo.end())
		{
			infos = (*iter).second;
		}
		
		QString tableName;
		for (auto info : infos)
		{
			//cwb 20240409修改
			if (diseaseName==QString::fromLocal8Bit(info.strDiseaseTypeName))
			{
				tableName = QString::fromLocal8Bit(info.strDBTableName);
			}
		}

		return tableName;
	}

	QVector<QString> hnDataManager::getRoadLevel(HnProjectEnums::StandardParmTypeEnum strStandard)
	{
		QVector<QString>  roadLevels; 
	
		if (m_mapRoadTypeSetInfo.count(strStandard))
		{
			vector<hnRoadTypeSetInfo> setinfos = m_mapRoadTypeSetInfo[strStandard];
			for each (hnRoadTypeSetInfo settingInfo in setinfos)
			{
				QString tempStr = "";
				tempStr = QString::fromLocal8Bit(settingInfo.nRoadLevel);
				if (roadLevels.contains(tempStr))
				{
					continue;
				}
				else
				{
					roadLevels.append(tempStr);
				}

			}


		}
		else
		{
			qDebug() << QStringLiteral("键不存在") << endl;
		}

	
		std::sort(roadLevels.begin(), roadLevels.end(), compareRoadLevels);
		return roadLevels;
		
	}

	//获得材质
	QVector<QString> hnDataManager::getRoadSurfaceTypes(HnProjectEnums::StandardParmTypeEnum strStandard)
	{
		QVector<QString>  roadSurfaceTypes;
		QString tempStr = "";
		if (m_mapRoadTypeSetInfo.count(strStandard))
		{
			vector<hnRoadTypeSetInfo> setinfos = m_mapRoadTypeSetInfo[strStandard];
			for each (hnRoadTypeSetInfo settingInfo in setinfos)
			{ 
			
				switch (settingInfo.nRSurfaceType)
				{
				case 0:
					tempStr = QStringLiteral("沥青");
					break;
				case  1:
					tempStr = QStringLiteral("水泥");
					break;
				case  2:
					tempStr = QStringLiteral("砂石");
					break;
				default:
					tempStr = QStringLiteral("沥青");
					break;
				}
				
			
				if (roadSurfaceTypes.contains(tempStr))
				{
					continue;
				}
				else
				{
					roadSurfaceTypes.append(tempStr);
				}

			}

		}
		else
		{
			qDebug() << QStringLiteral("键不存在") << endl;
		}
		return roadSurfaceTypes;
	}

	 QVector< QString> hnDataManager::getDrawTypes(HnProjectEnums::StandardParmTypeEnum strStandard)
	{
		 QVector<QString>  roadDrawTypes;
		 QString tempStr = "";
		 if (m_mapRoadTypeSetInfo.count(strStandard))
		 {
			 vector<hnRoadTypeSetInfo> setinfos = m_mapRoadTypeSetInfo[strStandard];
			 for each (hnRoadTypeSetInfo settingInfo in setinfos)
			 {

				 switch (settingInfo.nDrawType)
				 {
				 case 0:
					 tempStr = QStringLiteral("人工模式");
					 break;
				 case  1:
					 tempStr = QStringLiteral("自动化模式");
					 break; 
				 default:
					 tempStr = QStringLiteral("自动化模式");
					 break;
				 }
				  
				 if (roadDrawTypes.contains(tempStr))
				 {
					 continue;
				 }
				 else
				 {
					 roadDrawTypes.append(tempStr);
				 } 
			 } 
		 }
		 else
		 {
			 qDebug() << QStringLiteral("键不存在") << endl;
		 }
		 roadDrawTypes.append(QStringLiteral("设计模式"));
		 return roadDrawTypes;
	}

	void hnDataManager::setProjectDiseaseVector(std::vector<hnProjectDataInfo>& vecProData)
	{
		for (int i = 0; i<vecProData.size();++i)
		{
			hnProjectDataInfo& project = vecProData[i];
			//HnProEnums::StandardParmType type = HnProEnums::roadTypeQStringToEnum(QString::fromLocal8Bit(project.proSetInfo.strRoadStandard));
			//vector<hnDiseaseSetInfo> diss = m_mapDiseaseSetInfo[type];
		
			for (auto iter=m_mapDiseaseSetInfo.begin();iter!=m_mapDiseaseSetInfo.end();++iter)
			{
				vector<hnDiseaseSetInfo> diss = iter->second;
				for each (hnDiseaseSetInfo dis in diss)
				{
					if (!std::count(project.vecDiseaseTable.begin(), project.vecDiseaseTable.end(), dis.strDBTableName))
					{
						project.vecDiseaseTable.push_back(dis.strDBTableName);
					}
				}

			}
			
          
		}
	}

	QVector<hnDiseaseSetInfo> hnDataManager::getRoadDisease(const HnProjectEnums::StandardParmTypeEnum& standard, ROAD_WORK_TYPE nDrawType, ROAD_SURFACE_TYPE nRoadSurfaceType
	, int diseaseType)
	{
		QVector<hnDiseaseSetInfo> disVec;
		if (m_mapDiseaseSetInfo.count(standard))
		{
			for each (auto disSetting in m_mapDiseaseSetInfo[standard])
			{
				QString fullName = QString::fromLocal8Bit(disSetting.strDisFullName);
				QString strDrawType = nDrawType == 0 ? QStringLiteral("人工模式") : QStringLiteral("自动化模式");
				QString strSurface = nRoadSurfaceType == 0 ? QStringLiteral("沥青") : nRoadSurfaceType == 1 ? QStringLiteral("水泥") : QStringLiteral("砂石");
				if (fullName.contains(strDrawType) && fullName.contains(strSurface))
				{
					if (disSetting.nDiseaseType == diseaseType)
					{
						if (!disVec.contains(disSetting))
						{
							disVec.append(disSetting);
						}
					}
					
				}
			}
		}
		else
		{
			qDebug() << QStringLiteral("键不存在") << endl;
		}
		return disVec;
	}

	QStringList hnDataManager::getCurrentProjectRoadDiseaseNames()
	{
		auto resultList = QStringList();
		auto info = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
		auto roadStandardQString = QString::fromLocal8Bit(info.strRoadStandard);
		auto standard = HnProjectEnums::roadTypeQStringToEnum(roadStandardQString);
		auto nDrawType = info.nDrawType;
		auto nRoadSurfaceType = info.nRSurfaceType;
		if (m_mapDiseaseSetInfo.count(standard))
		{
			for each (auto disSetting in m_mapDiseaseSetInfo[standard])
			{
				auto fullName = QString::fromLocal8Bit(disSetting.strDisFullName);
				auto strDrawType = nDrawType == 0 ? QStringLiteral("人工模式") : QStringLiteral("自动化模式");
				auto strSurface = nRoadSurfaceType == 0 ? QStringLiteral("沥青") : nRoadSurfaceType == 1 ? QStringLiteral("水泥") : QStringLiteral("砂石");
				if (fullName.contains(strDrawType) && fullName.contains(strSurface))
				{
					resultList.append(fullName);
				}
			}
		}
		return resultList;
	}

 QVector<hnCommon::hnDiseaseSetInfo> hnDataManager::getProjectRoadDiseaseNames(hnPro::hnProject* project, HnProjectEnums::StandardParmTypeEnum userStandard, int roadType/*=-1*/)
	{
		auto resultList = QVector<hnDiseaseSetInfo>();
		auto info = project->getCurProSetInfo();
		auto roadStandardQString = QString::fromLocal8Bit(info.strRoadStandard);
		auto standard = HnProjectEnums::roadTypeQStringToEnum(roadStandardQString);
		if (userStandard != HnProjectEnums::None)
		{
			standard = userStandard;
		}
		auto nDrawType = info.nDrawType;
		int nRoadSurfaceType = info.nRSurfaceType;
		if (roadType != -1)
		{
			nRoadSurfaceType = roadType;
		}

		if (m_mapDiseaseSetInfo.count(standard))
		{
			for each (auto disSetting in m_mapDiseaseSetInfo[standard])
			{
				auto fullName = QString::fromLocal8Bit(disSetting.strDisFullName);
				auto strDrawType = nDrawType == 0 ? QStringLiteral("大框") : QStringLiteral("小框");
				auto strSurface = nRoadSurfaceType == 0 ? QStringLiteral("沥青") : nRoadSurfaceType == 1 ? QStringLiteral("水泥") : QStringLiteral("砂石");
				if (fullName.contains(strDrawType) && fullName.contains(strSurface)&&disSetting.nDiseaseType==0)
				{
					resultList.append(disSetting);
				}
			}
		}
		return resultList;
	}

	QVector<hnDiseaseSetInfo> hnDataManager::getCurrentProjectRoadDiseases(const hnMile& mile)
	{
		QVector<hnDiseaseSetInfo> resultList ;

		auto standard = mile.roadStandard;

		auto nDrawType = mile.drawType;
		auto nRoadSurfaceType = mile.roadType;
		 
		//如果是设计模式，采用人工模式的病害类型 
		if (2 == nDrawType)
		{
			nDrawType = (ROAD_WORK_TYPE)0;
		}
		
		if (m_mapDiseaseSetInfo.count(standard))
		{
			for each (auto disSetting in m_mapDiseaseSetInfo[standard])
			{  
				if (disSetting.nDrawType == nDrawType) //绘制方式
				{
					if (disSetting.nRoadSurfaceType == nRoadSurfaceType) //路面材质  
					{
						if (disSetting.nDiseaseType == 0) //病害类型 普通病害
						{
							resultList.append(disSetting);
						}
					}
				}
			}
		}

		if (mile.drawType == 2)
		{
			//再添加一遍线性病害 20241012
			if (m_mapDiseaseSetInfo.count(standard))
			{
				for each (auto disSetting in m_mapDiseaseSetInfo[standard])
				{
					if (disSetting.nDrawType == mile.drawType) //绘制方式
					{

						if (disSetting.nRoadSurfaceType == nRoadSurfaceType) //路面材质  
						{
							if (disSetting.nDiseaseType == 0) //病害类型 普通病害
							{
								resultList.append(disSetting);
							}
						}
					}
				}
			}
		}

		return resultList;
	}

	QVector<hnDiseaseSetInfo> hnDataManager::getCurrentProjectStreetDiseases(const hnMile& mile,int type)
	{

		QVector<hnDiseaseSetInfo> resultList;

		if (type <=0 || type>=3)
		{
			return resultList;
		}
		auto standard = mile.roadStandard;

		
		if (m_mapDiseaseSetInfo.count(standard))
		{
			for each (auto disSetting in m_mapDiseaseSetInfo[standard])
			{
				
				if (disSetting.nDiseaseType == type) //病害类型 普通病害
				{
				    resultList.append(disSetting);
				}
			}
		}
		return resultList;
	}

	QVector<hnDiseaseSetInfo> hnDataManager::hnDataManager::getCurrentProjectStreetDiseases(HnProjectEnums::StandardParmTypeEnum type, int disType)
	{
		QVector<hnDiseaseSetInfo> resultList;

		if (type <= 0 || type == 2)
		{
			return resultList;
		}
		auto standard =type;


		if (m_mapDiseaseSetInfo.count(standard))
		{
			for each (auto disSetting in m_mapDiseaseSetInfo[standard])
			{

				if (disSetting.nDiseaseType == disType) //病害类型 普通病害
				{
					resultList.append(disSetting);
				}
			}
		}
		return resultList;
	}

	// 查找类型
	bool hnDataManager::findData(const vector<QString>& vecOriData, QString strFindData)
	{
		if (vecOriData.size() <= 0)
		{
			return false;
		}

		for (int i = 0; i < vecOriData.size(); i++)
		{
			if (vecOriData[i] != strFindData)
			{
				continue;
			}

			return true;
		}

		return false;
	}

	// 查找所有类型文件
	bool hnDataManager::findFile(QString strFolder, QString strFileName, vector<QString>& vecRetFile)
	{
		// 设置dirlujing;
		QDir* dir = new QDir(strFolder);
		QStringList filter;
		//filter << QString("*.daq");

		// 获取列表下所有文件信息;
		QList<QFileInfo>* fileInfo = new QList<QFileInfo>(dir->entryInfoList(filter));
		for (int i = 0; i < fileInfo->count(); i++)
		{
			const QFileInfo info_tmp = fileInfo->at(i);
			QString path_tmp = info_tmp.filePath();
			if (info_tmp.fileName() == ".." || info_tmp.fileName() == ".")
			{
				continue;
			}

			if (info_tmp.isFile())
			{
				// 检查文件后缀;
				int index = path_tmp.lastIndexOf('.');
				if (index > 0)
				{
					QString strExp = path_tmp.right(path_tmp.length() - index - 1);
					strExp = strExp.toLower();
					if (path_tmp.contains(strFileName)) // 后缀判断;
					{
						vecRetFile.push_back(path_tmp);
					}
				}
			}
			else if (info_tmp.isDir())// 为文件夹;
			{
				// 迭代搜索;
				findFile(path_tmp, strFileName, vecRetFile);
			}
		}// for (int i = 0;i < fileInfo->count();i++)

		delete fileInfo;
		delete dir;

		if (vecRetFile.size() <= 0)
		{
			return false;
		}

		return true;
	}

	// 解析工程配置文件
	bool hnDataManager::analysisXml(QString strXmlPath, vector<hnProjectDataInfo>& vecProData)
	{

		string strProPath = strXmlPath.toLocal8Bit();
		strProPath = strProPath.substr(0, strProPath.find_last_of("/"));


		string strName = strProPath.substr(strProPath.find_last_of("/") + 1);
		//if (strName.length() < 20)  //|| strName.find_last_of("HN") == -1
		//{
		//	return false;
		//}
	 
		std::vector<std::string> splitResult = subString(strProPath, '/');

		//string strProName = strName;
		string strProName = splitResult[splitResult.size()-2];
		strName = strName.substr(6, 14);
		string proPath = strProPath;
		//strProPath = strProPath + "/" + strName;

		// 检查文件存在;
		QFile file(strXmlPath);
		if (!file.open(QFile::ReadOnly | QFile::Text))
		{
			return false;
		}
		QXmlStreamReader xmlReader(&file);
		xmlReader.readNextStartElement(); 
		xmlReader.readNext();

		// 是否解析工程
		//bool bProjectState = false;

		// 车轮周长、编码器频率
		int nFrequency = 0.0;
		double dWheelPerimeter = 0.0;

		// 当前工程
		hnProjectDataInfo curProjectDataInfo;
		strcpy(curProjectDataInfo.strProjectPath, proPath.c_str());
		strcpy(curProjectDataInfo.str3dProjectPath, strProPath.c_str());
		strcpy(curProjectDataInfo.strProJectName, strProName.c_str());
		string strTemp = "";

		// 当前设置
		hnProjectSetInfo projectSetInfo;

		// 起始编码器值和终止编码器值
		long long nBegEncl, nEndEncl, nEncl;
		nBegEncl = nEndEncl = nEncl = 0;
		int  mileCnt = 1;

		while (!xmlReader.atEnd())
		{
			if (xmlReader.isEndElement())
			{
				QString str = xmlReader.name().toString();
				if (xmlReader.name().contains(QString::fromLocal8Bit("工程信息")))
				{
					// 检查工程是否有效
					QString s2DStr = QString::fromLocal8Bit(strProPath.c_str()) + "/" + QString::fromLocal8Bit(curProjectDataInfo.str2DProName);
					QString s3DStr = QString::fromLocal8Bit(strProPath.c_str()) + "/" + QString::fromLocal8Bit(curProjectDataInfo.str3DProName);
					QDir dir1(s2DStr);
					QDir dir2(s3DStr);
					if (dir1.exists() || dir2.exists())
					{
						
						curProjectDataInfo.proSetInfo = projectSetInfo;
						curProjectDataInfo.proSetInfo.dLength = qAbs(projectSetInfo.dBegMile - projectSetInfo.dEndMile);
						//没有二维工程 纯三维
						if (strlen(curProjectDataInfo.str2DProName) ==0)
						{
							curProjectDataInfo.proSetInfo.nWorkType = PROJECT_TYPE::PROJECT_JD_3D_TYPE;
						}
						else
						{
							curProjectDataInfo.proSetInfo.nWorkType = PROJECT_23D_TYPE;
						}
					
						  
						vecProData.push_back(curProjectDataInfo);
					}

					curProjectDataInfo.vecDiseaseTable.clear();
					curProjectDataInfo.vecMilePile.clear();

					// 更新状态
					//bProjectState = true;
					xmlReader.readNext();
				}
			}

			if (xmlReader.isStartElement())
			{
				QString str = xmlReader.name().toString();

				// 车轮周长
				if (xmlReader.name() == QString::fromLocal8Bit("车轮周长"))
				{
					dWheelPerimeter = xmlReader.readElementText().toDouble();
					curProjectDataInfo.dWheelPerimeter = dWheelPerimeter;
					projectSetInfo.dWheelPerimeter = dWheelPerimeter;
				}
				else if (xmlReader.name() == QString::fromLocal8Bit("编码器参数"))
				{
					nFrequency = xmlReader.readElementText().toInt();
					curProjectDataInfo.nFrequency = nFrequency;
					projectSetInfo.nFrequency = nFrequency;
				}

				// 如果是工程设置，没有开始解析工程状态则开始解析工程，否则则添加继续
				if (xmlReader.name() == QString::fromLocal8Bit("工程信息"))
				{
					xmlReader.readNext();
					continue;
				}
				if (xmlReader.name() == QString::fromLocal8Bit("二维工程名"))
				{
					strTemp = xmlReader.readElementText().toLocal8Bit();

					strcpy(curProjectDataInfo.str2DProName, strTemp.c_str());
				}

				if (xmlReader.name() == QString::fromLocal8Bit("三维工程名"))
				{
					strTemp = xmlReader.readElementText().toLocal8Bit();

					strcpy(curProjectDataInfo.str3DProName, strTemp.c_str());
				}
				// 设置信息配置
				if (xmlReader.name() == QString::fromLocal8Bit("省"))
				{
					strTemp = xmlReader.readElementText().toLocal8Bit();

					strcpy(projectSetInfo.strProvince, strTemp.c_str());
				}

				if (xmlReader.name() == QString::fromLocal8Bit("市"))
				{
					strTemp = xmlReader.readElementText().toLocal8Bit();

					strcpy(projectSetInfo.strCity, strTemp.c_str());
				}

				if (xmlReader.name() == QString::fromLocal8Bit("县"))
				{
					strTemp = xmlReader.readElementText().toLocal8Bit();

					strcpy(projectSetInfo.strCounty, strTemp.c_str());
				}

				if (xmlReader.name() == QString::fromLocal8Bit("车道"))
				{
					strTemp = xmlReader.readElementText().toLocal8Bit();

					strcpy(projectSetInfo.strRoadNO, strTemp.c_str());
				}

				if (xmlReader.name() == QString::fromLocal8Bit("道路编号"))
				{
					strTemp = xmlReader.readElementText().toLocal8Bit();

					strcpy(projectSetInfo.strNumber, strTemp.c_str());
				}

				if (xmlReader.name() == QString::fromLocal8Bit("道路名称"))
				{
					strTemp = xmlReader.readElementText().toLocal8Bit();

					strcpy(projectSetInfo.strRoadName, strTemp.c_str());
				}

				if (xmlReader.name() == QString::fromLocal8Bit("道路类型"))
				{
					strTemp = xmlReader.readElementText().toLocal8Bit();

					strcpy(projectSetInfo.strRoadStandard, strTemp.c_str());
				}
				if (xmlReader.name() == QString::fromLocal8Bit("道路宽度"))
				{
					projectSetInfo.dRoadWidth = xmlReader.readElementText().toDouble();
				}
				if (xmlReader.name() == QString::fromLocal8Bit("病害绘制模式"))
				{
					projectSetInfo.nDrawType = xmlReader.readElementText().toInt();
				}
				if (xmlReader.name() == QString::fromLocal8Bit("道路等级"))
				{
					strTemp = xmlReader.readElementText().toLocal8Bit();

					strcpy(projectSetInfo.strRoadLevel, strTemp.c_str());
				}
				if (xmlReader.name() == QString::fromLocal8Bit("路面材质"))
				{
					strTemp = xmlReader.readElementText().toLocal8Bit();

					if (strTemp == "沥青")
					{
						projectSetInfo.nRSurfaceType = 0;
					}
					else if (strTemp == "水泥")
					{
						projectSetInfo.nRSurfaceType = 1;
					}
					else
					{
						projectSetInfo.nRSurfaceType = 2;
					}
				}

				if (xmlReader.name() == QString::fromLocal8Bit("检测员"))
				{
					strTemp = xmlReader.readElementText().toLocal8Bit();

					strcpy(projectSetInfo.strSurveyor, strTemp.c_str());
				}

				if (xmlReader.name() == QString::fromLocal8Bit("采集天气"))
				{
					strTemp = xmlReader.readElementText().toLocal8Bit();

					strcpy(projectSetInfo.strWeather, strTemp.c_str());
				}

				if (xmlReader.name() == QString::fromLocal8Bit("行车方向"))
				{
					strTemp = xmlReader.readElementText().toLocal8Bit();

					if (strTemp == "上行")
					{
						projectSetInfo.nLineType = 1;
					}
					else
					{
						projectSetInfo.nLineType = -1;
					}
				}

				 //起始桩号
				if (xmlReader.name() == QString::fromLocal8Bit("起点DMI桩号时间"))
				{
					hnMilePile curMilePile;
					strTemp = xmlReader.readElementText().toLocal8Bit();
					analysisMilePile(strTemp, nBegEncl, curMilePile.dTrueMile, curMilePile.dGpsTimer);
					curMilePile.nDMi = nBegEncl;
					curMilePile.dEnclMile = 0.0;
					projectSetInfo.dBegEnclMile = (double)(nBegEncl)* dWheelPerimeter / (double)nFrequency;
					projectSetInfo.dStartDmi = nBegEncl;
					projectSetInfo.dBegMile = curMilePile.dTrueMile;
					curMilePile.nID = 0;
					curProjectDataInfo.vecMilePile.push_back(curMilePile);
				}

				// 终点桩号
				if (xmlReader.name() == QString::fromLocal8Bit("终点DMI桩号时间"))
				{
					hnMilePile curMilePile;
					strTemp = xmlReader.readElementText().toLocal8Bit();
					analysisMilePile(strTemp, nEndEncl, curMilePile.dTrueMile, curMilePile.dGpsTimer);
					curMilePile.nDMi = nEndEncl;
					curMilePile.dEnclMile = (double)(nEndEncl - nBegEncl)* dWheelPerimeter / (double)nFrequency;
					projectSetInfo.dEndEnclMile = curMilePile.dEnclMile;
					projectSetInfo.dEndMile = curMilePile.dTrueMile;
					curMilePile.nID = 1;
					curProjectDataInfo.vecMilePile.push_back(curMilePile);
				}

				// 较桩数据
				if (xmlReader.name() == QString::fromLocal8Bit("DMI桩号时间"))
				{
					hnMilePile curMilePile;
					strTemp = xmlReader.readElementText().toLocal8Bit();
					analysisMilePile(strTemp, nEncl, curMilePile.dTrueMile, curMilePile.dGpsTimer);
					curMilePile.nDMi = nEncl;
					curMilePile.dEnclMile = (double)(nEncl - nBegEncl)* dWheelPerimeter / (double)nFrequency ;  //减去起始点的编码器值
					mileCnt++; 
					curMilePile.nID = mileCnt;
					curProjectDataInfo.vecMilePile.push_back(curMilePile);
				}

				// 
				xmlReader.readNext();

			}
			else
			{
				xmlReader.readNext();
			}
		}//while (!xmlReader.atEnd())

		file.close();


		if (vecProData.size() <= 0)
		{
			return false;
		}


		return true;

	}

	std::vector<std::string> hnDataManager::subString(const std::string&s, char delimiter)
	{
		std::vector<std::string> tokens;
		std::stringstream ss(s);
		std::string token;
		while (getline(ss,token,delimiter))
		{
			tokens.push_back(token);
		}
		return tokens;
	}

	// 解析工程配置文件
	bool hnDataManager::analysisProjectInfo(QString strIni, hnProjectDataInfo& outProData)
	{
		QStringList strList = strIni.split("/");
		if (strList.size() < 2)
		{
			//?
			return false;
		}
		string strName = strList[strList.size() - 3].toLocal8Bit();
		string str2DName = strList[strList.size() - 2].toLocal8Bit();
		
	     
		strcpy(outProData.str2DProName, str2DName.c_str());
		strcpy(outProData.strProJectName, str2DName.c_str());
		strcpy(outProData.str3DProName, "");
		
		// 工程路径
		QString strProjectPath = "";
		for (int i = 0; i < strList.size() - 1; i++)
		{
			if (i == 0)
			{
				strProjectPath = strProjectPath + strList[i];
			}
			else
			{
				strProjectPath = strProjectPath + "/" + strList[i];
			}
		}

		strcpy(outProData.strProjectPath, strProjectPath.toLocal8Bit());
		


		// 设置参数
		QFile setFile;
		
		setFile.setFileName(strIni);
		if (!setFile.open(QIODevice::ReadOnly))
		{
			return false;
		}

		QString strData = "";
		QStringList listData;
		string strValue = "";
	/*	QTextStream in(&setFile);
		while (!in.atEnd())
		{
			 
		}*/
		while (!setFile.atEnd())
		{
			listData.clear();
			strData = setFile.readLine(1024);

			listData = strData.split(QStringLiteral("："));
			bool hasValue = false;
			if (listData.size() <= 1)
			{
				hasValue = false;
				listData = strData.split(QStringLiteral(":"));
				if (listData.size()<=1)
				{
					hasValue = false;
				}
				else
				{
					hasValue = true;
				}
				
			}
			else
			{
				hasValue = true;
			}
			if (!hasValue)
			{
				continue;
			}

			strValue = listData[1].toLocal8Bit(); 
			std::size_t found = strValue.find("\r\n");
			if (found != std::string::npos)
			{
				strValue = strValue.substr(0, found);
			}

			std::size_t found2 = strValue.find("\n");
			if (found2 != std::string::npos)
			{
				strValue = strValue.substr(0, found2);
			}
			if (listData[0].contains(QString::fromLocal8Bit("省")))
			{
				strcpy(outProData.proSetInfo.strProvince, strValue.c_str());
			}
			else if (listData[0].contains(QString::fromLocal8Bit("市")))
			{
				strcpy(outProData.proSetInfo.strCity, strValue.c_str());
			}
			else if (listData[0].contains(QString::fromLocal8Bit("县")))
			{
				strcpy(outProData.proSetInfo.strCounty, strValue.c_str());
			}
			else if (listData[0].contains(QString::fromLocal8Bit("工程起点道路编号")))
			{
				strcpy(outProData.proSetInfo.strNumber, strValue.c_str());
			}
			else if (listData[0].contains(QString::fromLocal8Bit("工程起点道路名称")))
			{
				strcpy(outProData.proSetInfo.strRoadName, strValue.c_str());
			}
			else if (listData[0].contains(QString::fromLocal8Bit("工程起点桩号")))
			{
				listData[1].replace("K", "");
				listData[1].replace("+", "");
				outProData.proSetInfo.dBegMile = listData[1].toDouble(); 
				hnMilePile curMilePile; 
				curMilePile.dEnclMile = 0;
				curMilePile.dTrueMile = outProData.proSetInfo.dBegMile;
				outProData.vecMilePile.push_back(curMilePile);
			}
			
			else if (listData[0].contains(QString::fromLocal8Bit("行车方向")))
			{

				if (strcmp(strValue.c_str(), "上行") ==0 )
				{
					outProData.proSetInfo.nLineType = 1;
				}
				else
				{
					outProData.proSetInfo.nLineType = -1;
				}
			}
			else if (listData[0].contains(QString::fromLocal8Bit("公路等级")))
			{
				strcpy(outProData.proSetInfo.strRoadLevel, strValue.c_str());
			}
			else if (listData[0].contains(QString::fromLocal8Bit("车道")))
			{
				strcpy(outProData.proSetInfo.strRoadNO, strValue.c_str());
			}
			else if (listData[0].contains(QString::fromLocal8Bit("采集日期")))
			{
				strcpy(outProData.proSetInfo.strDate, strValue.c_str()); 
			}
			else if (listData[0].contains(QString::fromLocal8Bit("工程开始时刻")))
			{
				strcpy(outProData.proSetInfo.strTimer, strValue.c_str());
			}
			else if (listData[0].contains(QString::fromLocal8Bit("检测员")))
			{
				strcpy(outProData.proSetInfo.strSurveyor, strValue.c_str());
			}
			else if (listData[0].contains(QString::fromLocal8Bit("检测天气")))
			{
				strcpy(outProData.proSetInfo.strWeather, strValue.c_str());
			}
			else if (listData[0].contains(QString::fromLocal8Bit("路面材质")))
			{

				if (strcmp(strValue.c_str(), "沥青") == 0) 
				{
					outProData.proSetInfo.nRSurfaceType = 0;
				}
				else 	if (strcmp(strValue.c_str(), "水泥") == 0) 
				{
					outProData.proSetInfo.nRSurfaceType = 1;
				}
				else
				{
					outProData.proSetInfo.nRSurfaceType = 2;
				}
			}
			else if (listData[0].contains(QString::fromLocal8Bit("工程终点道路标识桩号")))
			{
				listData[1].replace("K", "");
				listData[1].replace("+", "");
				outProData.proSetInfo.dEndMile = listData[1].toDouble();
			
			}
			else if (listData[0].contains(QString::fromLocal8Bit("工程总里程数")))
			{
			
				listData[1].replace("K", "");
				listData[1].replace("+", "");
				outProData.proSetInfo.dLength = listData[1].toDouble();
				outProData.proSetInfo.dEndEnclMile = listData[1].toDouble();
			 
			}
			else if (listData[0].contains(QString::fromLocal8Bit("道路类型")))
			{
				strcpy(outProData.proSetInfo.strRoadStandard, strValue.c_str());
			}
			else if (listData[0].contains(QString::fromLocal8Bit("道路宽度")))
			{
				outProData.proSetInfo.dRoadWidth = stod(strValue); 
			}
			else if (listData[0].contains(QString::fromLocal8Bit("病害绘制模式")))
			{
				outProData.proSetInfo.nDrawType = stoi( strValue);
			}
		}

	
		//读取校桩文件
		QString MileStoneInfoPath = strProjectPath + "\\MileStoneCaliInfo.txt";
		QFile mileStoneFile(MileStoneInfoPath);
	 
		if (mileStoneFile.exists())
		{
			if (mileStoneFile.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				QTextStream in(&mileStoneFile);

				while (!in.atEnd())
				{
					QString line = in.readLine();
					QStringList dmiMileValue = line.split(' ');
					if (dmiMileValue.size() != 2)
					{
						continue;
					}
					double dmi = dmiMileValue[0].toDouble();
					if (dmi == 0 || dmi == outProData.proSetInfo.dLength)
					{
						//解决打标信息 中重复的 起点终点打标信息
						continue;
					}
					double mile = dmiMileValue[1].toDouble();
					hnMilePile curMilePile;
					curMilePile.dEnclMile = dmi;
					curMilePile.dTrueMile = mile;
					outProData.vecMilePile.push_back(curMilePile);
				}
			}

			hnMilePile curMilePile;
			curMilePile.dEnclMile = outProData.proSetInfo.dLength;
			curMilePile.dTrueMile = outProData.proSetInfo.dEndMile;
			outProData.vecMilePile.push_back(curMilePile);
			}
			
	}

	

	
	bool hnDataManager::analysisCamSetting(QString camPath, double& width)
	{
		QFile file(camPath);
		if (!file.exists())
		{
			return true;
		}
		QSettings setting(camPath, QSettings::IniFormat);
		setting.beginGroup("RoadConfig");
		bool ok;
		double value =  setting.value("jz_with").toDouble(&ok);
		if (ok&&value !=0)
		{
			width = value;
		}
		return true;
	}

	// 解析时间桩号  str   编码器值      真实里程 gps 
	bool hnDataManager::analysisMilePile(string strMilePile, long long& nEncl,  double& dTrueMileage,double& dGpsTimer )
	{
		// 解析数据
		std::vector<string> vecStr;
		std::vector<string> vecStr1;
		const char *d = ",";
		char *p;
		p = strtok(const_cast<char*>(strMilePile.c_str()), d);

		while (p)
		{
			vecStr.push_back(p);
			p = strtok(NULL, d);
		}

		if (vecStr.size() != 3)
		{
			return false;
		}

		// 编码器
		nEncl = atoll(vecStr[0].c_str());

		// 真实里程
		dTrueMileage = atof(vecStr[1].c_str());

		// 分割时间
		const char *d1 = ".";
		char *p1;
		p1 = strtok(const_cast<char*>(vecStr[2].c_str()), ".");
		while (p1)
		{
			vecStr1.push_back(p1);
			p1 = strtok(NULL, d1);
		}

		if (vecStr1.size() != 4)
		{
			return false;
		}

		// GPS时间
		dGpsTimer = atof(vecStr1[1].substr(0, vecStr1[1].length() - 1).c_str()) + atof(vecStr1[2].substr(0, vecStr1[2].length() - 2).c_str())*0.001 +
			atof(vecStr1[3].substr(0, vecStr1[3].length() - 2).c_str())*0.000001;
		//delete [] cstr;
		vecStr1.clear();
		vecStr.clear();
		return true;
	}

	void hnDataManager::calcuteDiseaseSize(int drawType, const hnDiseaseSetInfo& setInfo, hnRoadDiseaseInfo& dis)
	{ 
		double& Area = dis.dArea;
		double& calcwidth = dis.dReaWidth;
		double&  calcheight = dis.dRealLen;
		double & realwidth = dis.dWidth;
		double & realheight = dis.dLength;

		//保留位数
		dis.dReaWidth = std::round(dis.dReaWidth*100.0) / 100.0;
		dis.dRealLen = std::round(dis.dRealLen*100.0) / 100.0;
		dis.dWidth = std::round(dis.dWidth*100.0) / 100.0;
		dis.dLength = std::round(dis.dLength*100.0) / 100.0;
		dis.dArea = std::round(dis.dArea*10000.0) / 10000.0;

		 if (dis.nDrawType == 3)//线性病害
		{
			 calcwidth = setInfo.fEffectWid;
			 dis.dReaWidth = setInfo.fEffectWid;
			 dis.dWidth = setInfo.fEffectWid;
		} 
		else if (drawType == 1)
		{
			//自动化模式
			dis.dArea = dis.vec2dRect.size() * 0.1*0.1;
		} 
		else
		{	 ///面积公式：
			 ///0.框的面积长X宽，
			 ///1.框的对角线X影响宽度，
			 ///2.一个框1m2，
			 ///3.框的长边X影响宽度，
			 ///4.框沿路的方向的边长X影响宽度
			 ///5.板块长度X板块宽度
			switch (setInfo.nAreaFormula)
			{
			case 0: //直接就是框的面积
			{
				
					calcwidth = realwidth;
					calcheight = realheight;
					Area = calcwidth * calcheight;
					if (setInfo.fValidArea != 0)
					{
						if (Area < setInfo.fValidArea)
						{
							calcwidth = 0;
							calcheight = 0;
							Area = calcwidth * calcheight;
						}
					}
				
				break;
			}
			case 1: //框的对角线 X 影响宽度
			{
				double tlen = std::sqrt(realwidth * realwidth + realheight * realheight);
				if (setInfo.fValidLen != 0)
				{
					if (tlen <static_cast<double>( setInfo.fValidLen))
					{
						calcwidth = 0;
						calcheight = 0;
					}
					else
					{
						calcwidth = static_cast<double>(setInfo.fEffectWid);
						calcheight = tlen;
					}
				}
				else
				{
					calcwidth = static_cast<double>(setInfo.fEffectWid);
					calcheight = tlen;
				}
				Area = calcwidth * calcheight;
				break;
			}
			case 2://框的个数
			{
				calcwidth = 1.0;
				calcheight = static_cast<double>(setInfo.fEffectWid);
				Area = calcwidth * calcheight;
				break;
			}
			case 3://框的长边X影响宽度
			{
				calcwidth = static_cast<double>(setInfo.fEffectWid);
				calcheight = qMax(realwidth, realheight);
				Area = calcwidth * calcheight;
				break;
			}
			case 4://框沿路的方向的边长X影响宽度
			{
				calcwidth = static_cast<double>(setInfo.fEffectWid);
				calcheight = realheight;
				Area = calcwidth * calcheight;
				break;
			}
			case 5://板块长度X宽度
			{
				if (m_xrSetting->BrokenPlatetype == 0)
				{
					calcwidth = realwidth;
					calcheight = realheight;
				}
				else if (m_xrSetting->BrokenPlatetype == 1)
				{
					calcwidth = m_xrSetting->PlateWidth;
					calcheight = m_xrSetting->PlateLength;
				}
				Area = calcwidth * calcheight;
				break;
			}
			default:
				break;
			}

		}
		
		//保留位数
		dis.dReaWidth = std::round(dis.dReaWidth*100.0) / 100.0;
		dis.dRealLen = std::round(dis.dRealLen*100.0) / 100.0;
		dis.dWidth = std::round(dis.dWidth*100.0) / 100.0;
		dis.dLength = std::round(dis.dLength*100.0) / 100.0;
		dis.dArea = std::round(dis.dArea*10000.0) / 10000.0;
	}

	vector<HnProjectEnums::StandardParmTypeEnum> hnDataManager::m_vecRoadStandard;

	// 根据左上、左下以及右上三个坐标获取病害深度信息
	bool hnDataManager::getDiseaseDepth(hn3dPointWithMileI ptLU, hn3dPointWithMileI ptLD, hn3dPointWithMileI ptRU, double& dDepth)
	{
		if (!m_pCurProject)
		{
			return false;
		}

		if (!m_pCurProject->get3DProject())
		{
			return false;
		}

		// 定义三个三维坐标
		hn3dPointD pt3dLU, pt3dLD, pt3dRU;

		// 根据里程获取左上角点所在的三维图像名称
		QString strImageName = m_pCurProject->get3DProject()->getImageByMile(ptLU.bottomEncoderMile + 1);
		hn2dPointI pt;

		// 转换左上角坐标
		pt.x = ptLU.x;
		pt.y = ptLU.y;
		m_pCurProject->get3DProject()->get3DCoord(strImageName, pt, pt3dLU);

		// 根据里程获取左下角点所在的三维图像名称
		strImageName = m_pCurProject->get3DProject()->getImageByMile(ptLD.bottomEncoderMile + 1);

		// 转换左下角坐标
		pt.x = ptLD.x;
		pt.y = ptLD.y;
		m_pCurProject->get3DProject()->get3DCoord(strImageName, pt, pt3dLD);

		// 根据里程获取右上角点所在的三维图像名称
		strImageName = m_pCurProject->get3DProject()->getImageByMile(ptRU.bottomEncoderMile + 1);

		// 转换右上角坐标
		pt.x = ptRU.x;
		pt.y = ptRU.y;
		m_pCurProject->get3DProject()->get3DCoord(strImageName, pt, pt3dRU);

		// 获取深度信息
		hnPointCloud pointCloud;
		return pointCloud.getDiseaseDepth(pt3dLU, pt3dLD, pt3dRU, m_vecPtCloud, dDepth);
	}

	bool hnDataManager::getDisease3DPoint(hn3dPointWithMileI pt, hn3dPointD& pt3D)
	{
		if (!m_pCurProject)
		{
			return false;
		}

		if (!m_pCurProject->get3DProject())
		{
			return false;
		} 
		// 根据里程获取左上角点所在的三维图像名称
		QString strImageName = m_pCurProject->get3DProject()->getImageByMile(pt.bottomEncoderMile + 1);
		hn2dPointI pt2d; 

		// 转换左上角坐标
		pt2d.x = pt.x;
		pt2d.y = pt.y;
		m_pCurProject->get3DProject()->get3DCoord(strImageName, pt2d, pt3D);

		return true;
	}

	// 获取经纬度
	bool hnDataManager::getDiseaseLoction(hn3dPointWithMileI pt, double& dLatitude, double& dLongtitude, double& dHeight)
	{
		// 获取工程路径
	//	QString strProPath = m_pCurProject->get3DProject()->getProjectPath();
		QString strProPath = 	m_pCurProject->getAbsulotelyPath();
		// 获取POS路径
		QDir dirTemp(strProPath);
		QString parentDir = dirTemp.absolutePath();
		if (dirTemp.cdUp())
		{
			strProPath = dirTemp.absolutePath();
		}
		strProPath.left(strProPath.lastIndexOf("/"));
		strProPath = strProPath + "/POS/IE";
		QDir dir;
		if (!dir.exists(strProPath))
		{
			return false;
		}

		// 获取当前路劲下所有pos
		hnFile pFile;
		vector<QString> vecPosPath;
		QVector<QString> QvecPosPath;
		pFile.getFilePathType(strProPath, QString::fromLocal8Bit("pos"), QvecPosPath);
		vecPosPath = QvecPosPath.toStdVector();
		if (vecPosPath.size() <= 0)
		{
			return false;
		}

		// 读取POS数据
		if (m_vecPosInfo.size() <= 0)
		{
			loadPosData(vecPosPath[0].toLocal8Bit(), m_vecPosInfo);
		}

		if (m_vecPosInfo.size() <= 0)
		{
			return false;
		}

		// 根据里程获取角点所在的三维图像名称
		QString strImageName = m_pCurProject->get3DProject()->getImageByMile(pt.bottomEncoderMile + 1);

		// 获取gps
		double dGpsTimer = m_pCurProject->get3DProject()->getGpsTimer(strImageName, pt.y);

		// 根据时间获取经纬度
		// 先将pos文件分成200分
		int nGridCnt = 200;
		int nStep = m_vecPosInfo.size() / nGridCnt;
		if (m_vecPosInfo.size() % nGridCnt != 0)
		{
			nGridCnt += 1;
		}

		int nBegIndex = 0;
		int nEndIndex = 0;
		for (int i = 0; i < nGridCnt; i++)
		{
			nBegIndex = i * nStep;
			nEndIndex = (i + 1) * nStep;

			if (i == nGridCnt - 1)
			{
				nEndIndex = m_vecPosInfo.size()-1;
			}

			if ((dGpsTimer - m_vecPosInfo[nBegIndex].dGpsSecond) *
				(dGpsTimer - m_vecPosInfo[nEndIndex].dGpsSecond) <= 0)
			{
				break;
			}
		}

		if (nBegIndex - 20 > 0)
		{
			nBegIndex = nBegIndex - 20;
		}

		if (nEndIndex + 20 < m_vecPosInfo.size())
		{
			nEndIndex = nEndIndex + 20;
		}

		double dRoll = 0.0;
		int nIndex = -1;

		//
		for (int i = nBegIndex; i < nEndIndex; i++)
		{
			if ((dGpsTimer - m_vecPosInfo[i].dGpsSecond) *
				(dGpsTimer - m_vecPosInfo[i + 1].dGpsSecond) > 0)
			{
				continue;
			}

			nIndex = i;
			break;
		}

		if (nIndex == -1)
		{
			return false;
		}

		dLatitude = m_vecPosInfo[nIndex].dLatitude;
		dLongtitude = m_vecPosInfo[nIndex].dLongitude;
		dHeight = m_vecPosInfo[nIndex].dHeight;

		return true;
	}

	bool hnDataManager::getDiseaseLoction(hn3dPointWithMileI pt, double& dLatitude, double& dLongtitude, double& dHeight, hnPro::hnProject* project)
	{
		// 获取工程路径
		//	QString strProPath = m_pCurProject->get3DProject()->getProjectPath();
		QString strProPath = project->getAbsulotelyPath();
		// 获取POS路径
		QDir dirTemp(strProPath);
		QString parentDir = dirTemp.absolutePath();
		if (dirTemp.cdUp())
		{
			strProPath = dirTemp.absolutePath();
		}
		strProPath.left(strProPath.lastIndexOf("/"));
		strProPath = strProPath + "/POS/IE";
		QDir dir;
		if (!dir.exists(strProPath))
		{
			return false;
		}

		// 获取当前路劲下所有pos
		hnFile pFile;
		vector<QString> vecPosPath;
		QVector<QString> QvecPosPath;
		pFile.getFilePathType(strProPath, QString::fromLocal8Bit("pos"), QvecPosPath);
		vecPosPath = QvecPosPath.toStdVector();
		if (vecPosPath.size() <= 0)
		{
			return false;
		}

		// 读取POS数据
		if (m_vecPosInfo.size() <= 0)
		{
			loadPosData(vecPosPath[0].toLocal8Bit(), m_vecPosInfo);
		}

		if (m_vecPosInfo.size() <= 0)
		{
			return false;
		}

		// 根据里程获取角点所在的三维图像名称
		QString strImageName = project->get3DProject()->getImageByMile(pt.bottomEncoderMile + 1);

		// 获取gps
		double dGpsTimer = project->get3DProject()->getGpsTimer(strImageName, pt.y);

		// 根据时间获取经纬度
		// 先将pos文件分成200分
		int nGridCnt = 200;
		int nStep = m_vecPosInfo.size() / nGridCnt;
		if (m_vecPosInfo.size() % nGridCnt != 0)
		{
			nGridCnt += 1;
		}

		int nBegIndex = 0;
		int nEndIndex = 0;
		for (int i = 0; i < nGridCnt; i++)
		{
			nBegIndex = i * nStep;
			nEndIndex = (i + 1) * nStep;

			if (i == nGridCnt - 1)
			{
				nEndIndex = m_vecPosInfo.size();
			}

			if ((dGpsTimer - m_vecPosInfo[nBegIndex].dGpsSecond) *
				(dGpsTimer - m_vecPosInfo[nEndIndex].dGpsSecond) <= 0)
			{
				break;
			}
		}

		if (nBegIndex - 20 > 0)
		{
			nBegIndex = nBegIndex - 20;
		}

		if (nEndIndex + 20 < m_vecPosInfo.size())
		{
			nEndIndex = nEndIndex + 20;
		}

		double dRoll = 0.0;
		int nIndex = -1;

		//
		for (int i = nBegIndex; i < nEndIndex; i++)
		{
			if ((dGpsTimer - m_vecPosInfo[i].dGpsSecond) *
				(dGpsTimer - m_vecPosInfo[i + 1].dGpsSecond) > 0)
			{
				continue;
			}

			nIndex = i;
			break;
		}

		if (nIndex == -1)
		{
			return false;
		}

		dLatitude = m_vecPosInfo[nIndex].dLatitude;
		dLongtitude = m_vecPosInfo[nIndex].dLongitude;
		dHeight = m_vecPosInfo[nIndex].dHeight;

		return true;
	}

	hnDiseaseService* hnDataManager::getDiseaseService()
	{
		return m_diseaseService;
	}

	//1.读POS得到经纬度// 加载POS数据至内存
	bool hnDataManager::loadPosData(const char* strPosPath, std::vector<hnPosInfo>& vecInfo)
	{
		// 检查文件是否存在
		if (_access(strPosPath, 0) != 0)
		{
			return 0;
		}

		// 中间文件用于读取数据
		char strData[1024];
		memset(strData, 0, 1024);

		// 读取文件
		int file_line_count = 0;
		bool bFindData = false;
		FILE* ptrFile = fopen(strPosPath, "rt");
		while (!feof(ptrFile))
		{
			fgets(strData, 1024, ptrFile);
			file_line_count++;
		}
		fclose(ptrFile);
		ptrFile = fopen(strPosPath, "rt");

		// 读取第一行数据
		fgets(strData, 1024, ptrFile);
		string strLine = strData;
		int nPos = strLine.find_first_of('.');
		if (nPos > 0 && nPos <= 10)
		{
			bFindData = true;
		}

		// 迭代剔除前面的n行数据
		while (!bFindData)
		{
			// 读取一行数据
			memset(strData, 0, 1024);
			fgets(strData, 1024, ptrFile);
			strLine = strData;
			nPos = strLine.find_first_of('.');
			if (nPos > 0 && nPos <= 10)
			{
				bFindData = true;
			}
		}

		// 定义存储数据的vector
		int nPerSize = 50000;
		int nCount = 0;
		vecInfo.resize(nPerSize);

		// 找到后，进行解析
		hnPosInfo infoTmp;
		bool nSize = infoTmp.serialize(strData);
		if (!nSize)
		{
			fclose(ptrFile);
			return 0;
		}

		// 第一条记录也要存储
		vecInfo[nCount] = infoTmp;
		nCount++;

		// 读取获取全部数据
		while (!feof(ptrFile) /*&& m_is_running*/)
		{
			// 读取数据
			memset(strData, 0, 1024);
			fgets(strData, 1024, ptrFile);

			// 解析数据
			hnPosInfo info;
			nSize = info.serialize(strData);
			if (nSize)
			{
				vecInfo[nCount] = info;
				nCount++;

				// 容器逐渐扩大
				if (nCount >= vecInfo.size())
				{
					vecInfo.resize(vecInfo.size() + nPerSize);
				}
			}

		}

		vecInfo.resize(nCount);
		fclose(ptrFile);

		return true;
	}

	 	 
	

}
