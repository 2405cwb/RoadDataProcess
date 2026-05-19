#include "hnProject.h"
#include <QDir>
#include "hn2DProject.h"
#include "hn3DProject.h"
#include "hnFile.h"
#include "..\hnCommon\hnCompare.h"
#include <QSettings>
#include <QImage>
#include <algorithm>
#include <QTextCodec>
#include "..\hnQtCommon\MyCommonMethods.h"
#include "configService.h"
#include <QApplication>
#include <QDomDocument>
#include <QDomElement>
#include "../hnDataTable/hnDBDefine.h" 
#include <stdexcept>
#include <QHostInfo>
#include "../hnQtCommon/BaseException.h"
#include <QMessageBox>
#include <QElapsedTimer>
namespace hnPro
{
	hnProject::hnProject() :m_pDbSqlite(NULL), m_p2DProject(NULL), m_p3DProject(NULL), m_current3dDmi(0), m_bNeedImportFieldMilePilesToResultDb(false), m_bNeedImportFieldMarksToResultDb(false)
	{
	}

	hnProject::~hnProject()
	{

	}

	// 打开工程
	bool hnProject::openProject(hnProjectDataInfo& curProDataInfo)
	{
		m_roadSpace = 2;
		m_bNeedImportFieldMilePilesToResultDb = false;
		m_bNeedImportFieldMarksToResultDb = false;
		//景观图像间距
		m_leftStreetSpce = 10;
		m_rightStreetSpce = 10;
		// 工程路径
		string strProPath = curProDataInfo.strProjectPath;
		
		if (strProPath.find_last_of("/") == -1)
		{
			return false;
		} 
		m_strProjectPath = QString::fromLocal8Bit(strProPath.c_str());

		// 3D工程路径和名称
		m_str3DProName = QString::fromLocal8Bit(curProDataInfo.str3DProName);
		m_str3DProPath = QString::fromLocal8Bit(curProDataInfo.str3dProjectPath);

		// 2D工程路径和名称
		m_str2DProName = QString::fromLocal8Bit(curProDataInfo.str2DProName);
		m_str2DProPath = m_strProjectPath + "/" + m_str2DProName;
		// 确定工程类型
		bool bExist2D = true;
		bool bExist3D = true;
		QDir dir2D(m_str2DProPath);
		QDir dir3D(m_str3DProPath);
		//cwb 
		//单独二维工程的时候 m_str2DProPath不用在处理
		if (!dir2D.exists())
		{
			 
			m_str2DProPath = m_strProjectPath;
			dir2D.setPath (m_str2DProPath);
		}
	
		
		if (!dir2D.exists() || m_str2DProName.isEmpty())
		{
			bExist2D = false;
			m_str2DProName = "";
			m_str2DProPath = "";
			strcpy(curProDataInfo.str2DProName, "");
		}

		if (!dir3D.exists() || m_str3DProPath.isEmpty())
		{
			bExist3D = false;
			m_str3DProName = "";
			m_str3DProPath = "";
		}

		if ((!bExist2D) && (!bExist3D))
		{
			THROW_RUNTIME(QString::fromLocal8Bit("未检测到任何二维或三维工程。")); 
		}

		if (bExist2D && (!bExist3D))
		{
			m_nProjectType = PROJECT_2D_TYPE;
			m_strProjectName = QString::fromLocal8Bit( curProDataInfo.str2DProName); 
		}
		else if ((!bExist2D) && bExist3D)
		{
			m_nProjectType = PROJECT_XD_3D_TYPE;
			m_strProjectName = QString::fromLocal8Bit(curProDataInfo.strProJectName);
			// 
		}
		else
		{
			m_nProjectType = PROJECT_23D_TYPE;
			m_strProjectName = QString::fromLocal8Bit(curProDataInfo.strProJectName);
		}  
		 
		if (!getOrCreateResultDb(m_strProjectPath, curProDataInfo, m_strDbDirPath, m_strDbFilePath))
		{
			return false;
			//THROW_FILE(m_str2DProName + QString::fromLocal8Bit("创建成果数据文件失败。"));
		} 
		// 3D工程
		if (m_p3DProject)
		{
			delete m_p3DProject;
			m_p3DProject = NULL;
		}

		// 2D工程
		if (m_p2DProject)
		{
			delete m_p2DProject;
			m_p2DProject = NULL;
		}

		if (bExist3D)
		{
			m_p3DProject = new hn3DProject();
			//设置数据库文件夹路径
			//设置数据库文件路径
			if (!m_p3DProject->init(QString::fromLocal8Bit(curProDataInfo.str3dProjectPath), m_str3DProName))
			{
				delete m_p3DProject;
				m_p3DProject = NULL;
				return false;
			}
			else
			{
			}
			if (m_p3DProject)
			{
				m_p3DProject->setCurProject();
			}
		}

		if (bExist2D)
		{
			m_p2DProject = new hn2DProject();
			//设置数据库文件夹路径
			//设置数据库文件路径
			if (!m_p2DProject->init(m_strProjectPath, m_str2DProName,m_projectInfo, m_nProjectType))
			{
				delete m_p2DProject;
				m_p2DProject = NULL;
				return false;
			}
			QString projectinfoPath = m_p2DProject->get2dProjectinfoPath();
			//读取配置文件
			read2dProjectinfo(projectinfoPath);
			if (read2dSetting(m_p2DProject->getSettingPath()))
			{
				m_projectInfo.dRoadLength = m_roadSpace;
				QStringList picPathList = m_p2DProject->getRoadPicturePath();
				if (picPathList.count() > 0)
				{
					QString basePath = m_p2DProject->getBasePath();
					QString path = picPathList[0];
					QFileInfo dir1(path);
					QString path1 = dir1.absoluteDir().path();
					getPixResoluion(path1);
				}
				else
				{
					//
				}
				if (m_p2DProject)
				{
					//首次创建成果库时，将外业文本导入成果库；后续打开以成果库为准。
					import2DFieldDataToResultDb();
					initMileList();
				}
			}
			else
			{
				qDebug() << QStringLiteral("读取二维工程setting.ini配置文件失败");
			}

			//读取多工程用户桩号配置数据
			
			QFile userMileFile( m_p2DProject->getUserMilePath()) ;
			if (userMileFile.exists())
			{
				 QStringList txts =  MyCommonMethods::ReadAllLines(m_p2DProject->getUserMilePath());
				 for (QString line : txts)
				 {
					 QStringList split =  line.split("=");
					 if (split.at(0).contains("UserSmile"))
					 {
						 if (split.size()>1)
						 {
							 m_projectInfo.dUserBegMile = split.at(1).toDouble(); 
						 }
					 }
					 if (split.at(0).contains("UserEmile"))
					 {
						 if (split.size() > 1)
						 {
							 m_projectInfo.dUserEndMile = split.at(1).toDouble();
						 }
					 }
				 }
			}

		}

		// 初始化三三维编码器里程差
		this->init2d3dMileDiff();

		return true;
	}

	bool hnProject::writeProjectSettingInfo(const hnProjectDataInfo& curProDataInfo)
	{
		//	m_pDbSqlite->m_projectSetTable.writeData(curProDataInfo.proSetInfo);
		return false;
	}

	// 关闭工程
	void hnProject::closeProject()
	{
		// 3D工程
		if (m_p3DProject)
		{
			delete m_p3DProject;
			m_p3DProject = NULL;
		}

		// 2D工程
		if (m_p2DProject)
		{
			delete m_p2DProject;
			m_p2DProject = NULL;
		}

		if (m_pDbSqlite)
		{
			delete m_pDbSqlite;
			m_pDbSqlite = NULL;
		}
	}

	// 设置当前工程
	void hnProject::setCurProject()
	{
		if (m_p2DProject)
		{
			//首次创建成果库时，将外业文本导入成果库；后续打开以成果库为准。
			import2DFieldDataToResultDb();
			initMileList();
		}

		if (m_p3DProject)
		{
			m_p3DProject->setCurProject();
		}
		
	}

	QString hnProject::getAbsulotelyPath()
	{
		return this->m_strProjectPath;
	}




	 QString hnProject::getRoadPicturePath(double mile)
	{
		 QString picturePath = "";
		 if (m_vecMile.size()>0)
		 {
			 double value = qAbs(mile - m_vecMile.at(0).dTrueMile);

			 for (int i = 0; i < m_vecMile.size(); ++i)
			 {
				 auto cMile = m_vecMile.at(i);
				 double nowValue = qAbs(cMile.dTrueMile - mile);
				 if (nowValue <= value)
				 {
					 picturePath = cMile.picturePath;
					 value = nowValue;
				 }
			 }
		 }
		 return picturePath;
	}

	  QString hnProject::getStreetPicturePath(double mile)
	 {
		  QString picturePath = "";
		  if (m_vecMile.size() > 0)
		  {
			  double value = qAbs(mile - m_vecMile.at(0).dTrueMile);

			  for (int i = 0; i < m_vecMile.size(); ++i)
			  {
				  auto cMile = m_vecMile.at(i);
				  double nowValue = qAbs(cMile.dTrueMile - mile);
				  if (nowValue <= value)
				  {
					  picturePath = cMile.leftStreetPicPath;
					  value = nowValue;
				  }
			  }
		  }
		  return picturePath;
	 }

	  ROAD_WORK_TYPE hnProject::getBaseDrawType()
	  {
		  return   (ROAD_WORK_TYPE)m_projectInfo.nDrawType;
	  }

	 HnProjectEnums::StandardParmTypeEnum hnProject::getBaseStandard()
	{
		return HnProjectEnums::roadTypeQStringToEnum(m_projectInfo.strRoadStandard);
	}

	hnCommon::ROAD_SURFACE_TYPE hnProject::getBaseSurface()
	{
		return (hnCommon::ROAD_SURFACE_TYPE)m_projectInfo.nRSurfaceType;
	}

	void hnProject::init2d3dMileDiff()
	{
		if (m_p2DProject && m_p3DProject)
		{
			QString fileName = m_str2DProPath + "/2d3dDiffSetting.ini";
			this->checkMileDiffFile(fileName);

			QSettings settings(fileName, QSettings::IniFormat);
			settings.beginGroup("CONFIG");
			double value = settings.value("2D3D_DIFF", 0).toDouble();
			settings.endGroup();

			m_2d3dDiff =  value;
		}
		else
		{
			m_2d3dDiff = 0;
		}


	}

	double hnProject::get2d3dMileDiff()
	{
		return m_2d3dDiff;
	}
		

	void hnProject::set2d3dMileDiff(const double diff)
	{
		if (m_p2DProject && m_p3DProject)
		{
			QString fileName = m_str2DProPath + "/2d3dDiffSetting.ini";
			this->checkMileDiffFile(fileName);

			QSettings settings(fileName, QSettings::IniFormat);
			settings.beginGroup("CONFIG");
			settings.setValue("2D3D_DIFF", diff);
			settings.endGroup();
			m_2d3dDiff = diff;
		}

	}

	bool  hnProject::getOrCreateResultDb(QString projectPath,hnProjectDataInfo& curProDataInfo, QString& dbDirPath, QString & dbFilePath)
	{
		// 成果路径 
		QString strResultPath = projectPath + "/" + QString::fromLocal8Bit("成果数据");
		 
			QDir dir(strResultPath);
			if (!dir.exists())
			{
				if (!dir.mkdir(strResultPath))
				{
					THROW_FILE(QString::fromLocal8Bit("创建成果数据文件夹失败。"));
				}
			}
			QString oldResultPath;
			if (m_nProjectType == PROJECT_TYPE::PROJECT_23D_TYPE || m_nProjectType == PROJECT_TYPE::PROJECT_2D_TYPE)
			{
				//更新代码 之前的代码可能导致数据库过长
				QString roadNumber = QString::fromLocal8Bit(curProDataInfo.proSetInfo.strNumber);
				QString roadStartMile = QString::number(curProDataInfo.proSetInfo.dBegMile, 'f', 2);
				QString roadEndMile = QString::number(curProDataInfo.proSetInfo.dEndMile, 'f', 2);
				if (curProDataInfo.proSetInfo.nLineType != 1 && curProDataInfo.proSetInfo.nLineType != -1)
				{ 
					QMessageBox::critical(QApplication::activeWindow(), QStringLiteral("错误"), m_str2DProName + QString::fromLocal8Bit("是否未设置道路上下行信息,请检查!"),QMessageBox::Ok);
					return false;
				}
				QString line = curProDataInfo.proSetInfo.nLineType == 1 ? QString::fromLocal8Bit("上行") : QString::fromLocal8Bit("下行");
				QString tempName;
				if (roadNumber.isEmpty())
				{
					//if (strcmp(curProDataInfo.proSetInfo.strRoadStandard, "低等级农村公路") == 0)
					//{
					//	//THROW_RUNTIME(m_str2DProName + QString::fromLocal8Bit("道路编号为空无法导入,请检查!"));
					//	QMessageBox::critical(QApplication::activeWindow(), QStringLiteral("错误"), m_str2DProName + QString::fromLocal8Bit("道路编号为空无法导入,请检查!"), QMessageBox::Ok);
					//	return false;
					//}
					//else
					{
						dbDirPath = strResultPath + "/" + m_str2DProName;
						oldResultPath = strResultPath + "/" + m_str2DProName;
					}
				}
				else
				{
					dbDirPath = strResultPath + "/" + roadNumber + "_" + line + "_" + roadStartMile + "_" + roadEndMile;
					oldResultPath = strResultPath + "/" + m_str2DProName;
				}
			
			}
			else
			{
				dbDirPath = strResultPath + "/" + m_str3DProName;
				oldResultPath = strResultPath + "/" + m_str3DProName;
			}
			QDir dirResult(dbDirPath);
			if (!dirResult.exists())
			{
				if (!dirResult.mkdir(dbDirPath))
				{
					return false;
				}
			}
#pragma region 兼容旧工程
			QDir oldDirResult(oldResultPath);
			QString oldDirPath = oldResultPath;
			if (oldDirResult.exists() )
			{
				if (dbDirPath == oldResultPath)
				{
					if (getResultDB(oldDirPath, oldResultPath, true))
					{
						QString targetFilePath = dbDirPath + "/" + QString::fromLocal8Bit("成果.db");
						bool success = QFile::rename(oldResultPath, targetFilePath);
					}

				}
				else
				{
					//说明是旧工程 旧工程把数据库移动一下
					//获取旧工程的数据库
					QString oldDbPath;
					if (!getResultDB(oldDirPath, oldResultPath, true))
					{
						return false;
					}
					else
					{
						QString targetFilePath = dbDirPath + "/" + QString::fromLocal8Bit("成果.db");
						if (QFile::exists(targetFilePath))
						{
							if (!QFile::remove(targetFilePath))
							{
								return false;
							}
						}
						//移动并重命名
						bool success = QFile::rename(oldResultPath, targetFilePath);
						if (success)
						{
							if (dbDirPath != oldResultPath)
							{
								//移除旧的文件夹
								oldDirResult.rmdir(oldDirPath);
							}

						}
						else
						{
							return false;
						}

					}
				}
				
			}
#pragma endregion 

			// 数据库信息
			string strTemp = "";

			// 是否存在成果db
			//成果db
			if (getResultDB(dbDirPath, dbFilePath, false))
			{
				const QString importFlagPath = dbDirPath + "/FieldSourceImported.flag";
				const bool hasImportFlag = QFile::exists(importFlagPath);
				strTemp = dbFilePath.toLocal8Bit();

				// 读取数据库
				if (!m_pDbSqlite)
				{
					m_pDbSqlite = new hnDBSqlite(strTemp.c_str());
					if (!m_pDbSqlite->connectDB(curProDataInfo.vecDiseaseTable))
					{
						delete m_pDbSqlite;
						m_pDbSqlite = NULL;
						return false;
					}
				}

				if (!m_pDbSqlite->m_projectSetTable.readData(m_projectInfo))
				{
					//如果db文件存在但是找不到有效工程信息 则重新写入一次
					m_pDbSqlite->m_projectSetTable.clearData();
					if (m_pDbSqlite->m_projectSetTable.writeData(curProDataInfo.proSetInfo))
					{
						m_projectInfo = curProDataInfo.proSetInfo;

					}
					else
					{
						return false;
					}

				}
				if (!m_pDbSqlite->m_markerInfoTable.readData(m_vecMarkInfo))
				{
					return false;

				}

				bool milePileLoadedFromDb = m_pDbSqlite->m_milePileTable.readData(m_vecMileagePile);
				if (!milePileLoadedFromDb)
				{
					//如果db文件存在但是找不到有效工程信息 则重新写入一次
					m_pDbSqlite->m_milePileTable.clearData();
					m_vecMileagePile = curProDataInfo.vecMilePile; //得到xml文件里面的 较桩数据 
					if (!m_pDbSqlite->m_milePileTable.writeData(m_vecMileagePile))
					{
						return false;
					}
				}
				if (!hasImportFlag)
				{
					if (m_vecMarkInfo.empty())
					{
						m_bNeedImportFieldMarksToResultDb = true;
					}
					if (!milePileLoadedFromDb)
					{
						m_bNeedImportFieldMilePilesToResultDb = true;
					}
					if (m_bNeedImportFieldMarksToResultDb || m_bNeedImportFieldMilePilesToResultDb)
					{
						int ret = QMessageBox::question(QApplication::activeWindow(), QStringLiteral("旧成果库迁移"),
							QStringLiteral("当前成果库没有外业导入标记，是否从外业 Dmi2Mile.txt / RoadStatuMarkInfo.txt 补充缺失的较桩或打标工作副本？\n选择否将保留当前成果库内容。"),
							QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
						if (ret != QMessageBox::Yes)
						{
							m_bNeedImportFieldMarksToResultDb = false;
							m_bNeedImportFieldMilePilesToResultDb = false;
						}
					}
					if (!m_bNeedImportFieldMarksToResultDb && !m_bNeedImportFieldMilePilesToResultDb)
					{
						QFile importFlag(importFlagPath);
						if (importFlag.open(QIODevice::WriteOnly | QIODevice::Text))
						{
							importFlag.close();
						}
					}
				}
				/*	if (!m_pDbSqlite->m_diseaseTable.readAllData(m_vecDisData))
				{
				return false;
				}*/

				//如果不存在控制点的表，则创建控制点的表
				HN_CREATE_RESULT_TABLE tableCmd;
				tableCmd.createCtrlPointTableCmd;
				m_pDbSqlite->executeDB(tableCmd.createCtrlPointTableCmd.c_str());


				//判断 配置信息是否有更新 有更新则写入 
				if (curProDataInfo.proSetInfo.dRoadWidth != m_projectInfo.dRoadWidth
					|| strcmp(curProDataInfo.proSetInfo.strRoadStandard, m_projectInfo.strRoadStandard) != 0
					|| curProDataInfo.proSetInfo.nDrawType != m_projectInfo.nDrawType
					|| strcmp(curProDataInfo.proSetInfo.strRoadLevel, m_projectInfo.strRoadLevel) != 0
					)
				{
					m_pDbSqlite->m_projectSetTable.writeData(curProDataInfo.proSetInfo);
					m_projectInfo = curProDataInfo.proSetInfo;
				}
			}
			else
			{
				// 不存在则创建数据库
				if (m_pDbSqlite)
				{
					delete m_pDbSqlite;
					m_pDbSqlite = NULL;
				}
				QString strDBPath;
				if (m_nProjectType == PROJECT_TYPE::PROJECT_23D_TYPE || m_nProjectType == PROJECT_TYPE::PROJECT_2D_TYPE)
				{
					strDBPath = dbDirPath + "/" + QString::fromLocal8Bit("成果.db");
				}
				else
				{
					strDBPath = dbDirPath + "/" + QString::fromLocal8Bit("成果.db");
				}
				dbFilePath = strDBPath;
				strTemp = dbFilePath.toLocal8Bit();
				m_pDbSqlite = new hnDBSqlite(strTemp.c_str());

				if (!m_pDbSqlite->connectDB(curProDataInfo.vecDiseaseTable))
				{
					delete m_pDbSqlite;
					m_pDbSqlite = NULL;
					return false;
				}
				// 写入设置参数到数据库

				m_pDbSqlite->m_projectSetTable.writeData(curProDataInfo.proSetInfo);
				m_projectInfo = curProDataInfo.proSetInfo;
				m_bNeedImportFieldMilePilesToResultDb = true;
				m_bNeedImportFieldMarksToResultDb = true;
				std::sort(curProDataInfo.vecMilePile.begin(), curProDataInfo.vecMilePile.end(), [=](const hnMilePile&a, const hnMilePile& b)
				{
					if (curProDataInfo.proSetInfo.nLineType < 0)
					{
						return a.dEnclMile > b.dEnclMile;
					}
					else
					{
						return a.dEnclMile < b.dEnclMile;
					}
				}
				);
				m_vecMileagePile = curProDataInfo.vecMilePile; //得到xml文件里面的 较桩数据

				if (!m_pDbSqlite->m_milePileTable.writeData(m_vecMileagePile))
				{
					return false;
				}

			}
		
	
	
	}

	void hnProject::checkMileDiffFile(const QString & fileName)
	{
		QFile file(fileName);
		if (!file.exists())
		{
			file.open(QIODevice::WriteOnly | QIODevice::Text);
			QTextStream stream(&file);
			stream << "[CONFIG]" << endl;
			stream << "2D3D_DIFF=0" << endl;
			file.close();
		}
	}

	// 相对里程转绝对里程
	double hnProject::enclToTrueMile(double dEnclMile)
	{
		if (m_vecMileagePile.size() <= 0)
		{
			return dEnclMile;
		}

		double dTrueMile = 0.0;

		if (m_vecMileagePile.size() == 1)
		{
			// 只有一个里程桩
			if (m_projectInfo.nLineType == 1) // 上行
			{
				dTrueMile = m_vecMileagePile[0].dTrueMile - m_vecMileagePile[0].dEnclMile + dEnclMile;
			}
			else
			{
				dTrueMile = m_vecMileagePile[0].dTrueMile - (dEnclMile - m_vecMileagePile[0].dEnclMile);
			}
		}
		else
		{
			// 按照相对里程排序
			sort(m_vecMileagePile.begin(), m_vecMileagePile.end(), compareMilepileByEnclMile);

			bool bToBigMile = true;
			if (m_vecMileagePile[0].dTrueMile > m_vecMileagePile[1].dTrueMile)
			{
				bToBigMile = false;
			}

			if (dEnclMile < m_vecMileagePile[0].dEnclMile)
			{
				if (bToBigMile)
				{
					dTrueMile = m_vecMileagePile[0].dTrueMile - m_vecMileagePile[0].dEnclMile + dEnclMile;
				}
				else
				{
					dTrueMile = m_vecMileagePile[0].dTrueMile - (dEnclMile - m_vecMileagePile[0].dEnclMile);
				}
			}
			else if (dEnclMile > m_vecMileagePile[m_vecMileagePile.size() - 1].dEnclMile)
			{
				if (bToBigMile)
				{
					dTrueMile = m_vecMileagePile[m_vecMileagePile.size() - 1].dTrueMile - m_vecMileagePile[m_vecMileagePile.size() - 1].dEnclMile + dEnclMile;
				}
				else
				{
					dTrueMile = m_vecMileagePile[m_vecMileagePile.size() - 1].dTrueMile - (dEnclMile - m_vecMileagePile[m_vecMileagePile.size() - 1].dEnclMile);
				}
			}
			else
			{
				for (int i = 0; i < m_vecMileagePile.size() - 1; i++)
				{
					if (dEnclMile < m_vecMileagePile[i].dEnclMile || dEnclMile > m_vecMileagePile[i + 1].dEnclMile)
					{
						continue;
					}

					dTrueMile = (m_vecMileagePile[i + 1].dTrueMile - m_vecMileagePile[i].dTrueMile) / (m_vecMileagePile[i + 1].dEnclMile - m_vecMileagePile[i].dEnclMile)*
						(dEnclMile - m_vecMileagePile[i].dEnclMile) + m_vecMileagePile[i].dTrueMile;
				}
			}
		}

		return dTrueMile;
	}
	// 绝对里程桩相对里程
	double hnProject::trueMileToEncl(double dTrueMile)
	{
		if (m_vecMileagePile.size() <= 0)
		{
			return dTrueMile;
		}

		double dEnclMile = 0.0;

		if (m_vecMileagePile.size() == 1)
		{
			// 只有一个里程桩
			if (m_projectInfo.nLineType == 1) // 上行
			{
				dEnclMile = dTrueMile - (m_vecMileagePile[0].dTrueMile - m_vecMileagePile[0].dEnclMile);
			}
			else
			{
				dEnclMile = m_vecMileagePile[0].dEnclMile - (dTrueMile - m_vecMileagePile[0].dTrueMile);
			}
		}
		else
		{
			// 按照绝对里程排序
			sort(m_vecMileagePile.begin(), m_vecMileagePile.end(), compareMilepileByTrueMile);

			bool bToBigMile = true;
			if (m_vecMileagePile[0].dEnclMile > m_vecMileagePile[1].dEnclMile)
			{
				bToBigMile = false;
			}

			if (dTrueMile < m_vecMileagePile[0].dTrueMile)
			{
				if (bToBigMile)
				{
					dEnclMile = dTrueMile - (m_vecMileagePile[0].dTrueMile - m_vecMileagePile[0].dEnclMile);
				}
				else
				{
					dEnclMile = m_vecMileagePile[0].dEnclMile - (dTrueMile - m_vecMileagePile[0].dTrueMile);
				}
			}
			else if (dTrueMile > m_vecMileagePile[m_vecMileagePile.size() - 1].dTrueMile)
			{
				if (bToBigMile)
				{
					dEnclMile = dTrueMile - (m_vecMileagePile[m_vecMileagePile.size() - 1].dTrueMile - m_vecMileagePile[m_vecMileagePile.size() - 1].dEnclMile);
				}
				else
				{
					dEnclMile = m_vecMileagePile[m_vecMileagePile.size() - 1].dEnclMile - (dTrueMile - m_vecMileagePile[m_vecMileagePile.size() - 1].dTrueMile);
				}
			}
			else
			{
				for (int i = 0; i < m_vecMileagePile.size() - 1; i++)
				{
					if (dTrueMile < m_vecMileagePile[i].dTrueMile || dTrueMile > m_vecMileagePile[i + 1].dTrueMile)
					{
						continue;
					}

					dEnclMile = (m_vecMileagePile[i + 1].dEnclMile - m_vecMileagePile[i].dEnclMile) / (m_vecMileagePile[i + 1].dTrueMile - m_vecMileagePile[i].dTrueMile)*
						(dTrueMile - m_vecMileagePile[i].dTrueMile) + m_vecMileagePile[i].dEnclMile;
					if (dEnclMile<0.1)
					{
						dEnclMile = 0;
					}
				}
			}
		}
		return dEnclMile;
		//return round(dEnclMile);//cwb 20231027
	}

	// 获取道路类型
	/*ROAD_SURFACE_TYPE hnProject::getRoadType()
	{
		ROAD_SURFACE_TYPE nRoadSurfaceType = ROAD_LQ_SURFACE;
		return nRoadSurfaceType;
	}*/



	bool hnProject::read2dSetting(const QString& path)
	{
		QSettings settings(path, QSettings::IniFormat);

		//设置要读取的组名 
		settings.beginGroup(QString("Parm"));

		m_roadSpace = settings.value("RoadDis").toInt();
		m_leftStreetSpce = settings.value("StreetDis").toInt();

		m_rightStreetSpce = settings.value("StreetDis2").toInt();

		if (m_roadSpace <= 0)
		{
			m_roadSpace = 2;
		}
		if (m_leftStreetSpce <= 0)
		{
			m_leftStreetSpce = 20;
		}
		if (m_rightStreetSpce<=0)
		{
			m_rightStreetSpce = m_leftStreetSpce;
		}

		//读取具体参数的值
		return true;
	}

	int hnProject::getPixResoluion(const QString &pixDirName)
	{
		QDir dir(pixDirName);
		if (!dir.exists())
		{
			return -1;
		}
		QStringList filiter;
		filiter << "*.jpg" << "*.jpeg";
		QFileInfoList infolist = dir.entryInfoList(filiter);
		if (infolist.size() == 0)
		{
			return -2;
		}
		QString pictureName = infolist.first().absoluteFilePath();
		QImage image(pictureName);
		if (image.isNull() || image.width() <= 0 || image.height() <= 0)
		{
			return -3;
		}
		m_projectInfo.picPixelX = image.width();
		m_projectInfo.picPixelY = image.height();
		m_projectInfo.dRadioX = m_projectInfo.dRoadWidth / m_projectInfo.picPixelX;
		m_projectInfo.dRadioY = m_projectInfo.dRoadLength / m_projectInfo.picPixelY;
		return 0;
	}

	



	void hnProject::read2dProjectinfo(const QString& filePath)
	{
		QStringList list;
		QFile file(filePath);
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream in(&file);
	 
			in.setCodec(QTextCodec::codecForName("utf-8"));
			while (!in.atEnd())
			{
				QString line = in.readLine();
				list.append(line);
			}
			file.close();
		}
		for each (QString line in list)
		{    
			QStringList strSplit = line.split(":");
			if (strSplit.size()>1)
			{
				if (line.contains(QStringLiteral("车道")))
				{
					string s1 = strSplit[1].toLocal8Bit();
					if (strlen( m_projectInfo.strRoadNO)==0)
					{
						strcpy(m_projectInfo.strRoadNO, s1.c_str());

					}
					
				}
				if (line.contains(QStringLiteral("采集日期")))
				{
					string s1 = strSplit[1].toLocal8Bit();
					strcpy(m_projectInfo.strDate, s1.c_str());
					
				}
				if (line.contains(QStringLiteral("工程开始时刻")))
				{
					string s1 = strSplit[1].toLocal8Bit();
					strcpy(m_projectInfo.strTimer, s1.c_str());

				}
			}
		}
		
	}

	void hnProject::initMileList(bool NotNeedUpdateMileVector)
	{
		if (m_projectInfo.nLineType < 0)
		{
			std::sort(m_vecMarkInfo.begin(), m_vecMarkInfo.end(), [](const hnMarkInfo &a, const hnMarkInfo &b)
			{
				return a.dTrueMile > b.dTrueMile;
			}); //从大到小排序
		}
		else
		{
			std::sort(m_vecMarkInfo.begin(), m_vecMarkInfo.end(), [](const hnMarkInfo &a, const hnMarkInfo &b)
			{
				return a.dTrueMile < b.dTrueMile;
			});
		}
		//注意这个地方 
		if (m_vecMile.size() > 0&& NotNeedUpdateMileVector) //Qvector<hnMile> 不为空代表已经加载过了
		{
			
			return;
		}
		m_vecMile.clear();
		currentMileVec.clear();
		currentLeftStreetMileVec.clear();
		currentRightStreetMileVec.clear();
		/*for (int i = 0 ; i<m_vecMarkInfo.size();++i)
		{
			hnMarkInfo& mark = m_vecMarkInfo[i];
			mark.nID = i;
		}*/
		// initMileList 只生成运行时 m_vecMile，不写成果库，也不覆盖外业文本。

		QVector<hnMile> miles;
		QStringList picPathList = m_p2DProject->getRoadPicturePath();
		QStringList streetPicPathList0 = m_p2DProject->getLeftStreetPicturePath();
		QStringList streetPicPathList1 = m_p2DProject->getRightStreetPicturePath();
		double sMile = m_projectInfo.dBegMile;
		double eMile = m_projectInfo.dEndMile;

		int imgnum = picPathList.count();
		 


		if (imgnum < 7)
		{
			// 可能是简易工程；如果成果库没有里程表，则按工程长度生成无图片里程。
			if (m_pDbSqlite)
			{
				m_pDbSqlite->m_mileInfoTable.readData(m_vecMile);
			}
			 
			if (m_vecMile.size() > 0)
			{
				m_currentMile = m_vecMile.at(0);
				return;
			}

			double totalDmi = m_projectInfo.dEndEnclMile;
			if (totalDmi <= 0)
			{
				totalDmi = m_projectInfo.dLength;
			}
			if (totalDmi <= 0)
			{
				totalDmi = qAbs(m_projectInfo.dEndMile - m_projectInfo.dBegMile);
			}

			for (int i = 0; ; ++i)
			{
				double tdmi = m_roadSpace * i;
				if (totalDmi > 0 && tdmi > totalDmi)
				{
					break;
				}

				hnMile mile;
				double tmile = enclToTrueMile(tdmi);
				mile.dTrueMile = qRound(tmile);
				mile.dEnclMile = tdmi;
				mile.nDMi = m_projectInfo.dWheelPerimeter == 0 ? 0 : m_projectInfo.nFrequency * tdmi / m_projectInfo.dWheelPerimeter;
				mile.roadGradStr = QString::fromLocal8Bit(m_projectInfo.strRoadLevel);
				mile.roadStandard = HnProjectEnums::roadTypeQStringToEnum(QString::fromLocal8Bit(m_projectInfo.strRoadStandard));
				mile.drawType = (hnCommon::ROAD_WORK_TYPE)m_projectInfo.nDrawType;
				mile.roadWidth = m_projectInfo.dRoadWidth;
				mile.roadType = (hnCommon::ROAD_SURFACE_TYPE)m_projectInfo.nRSurfaceType;
				mile.roadGrad = mile.GradStrToGrad(QString::fromLocal8Bit(m_projectInfo.strRoadLevel));
				mile.nID = i;
				miles.push_back(mile);

				if (totalDmi <= 0)
				{
					break;
				}
			}
		}
		else
		{
			double tdmi = 0, tmile = 0;

			for (int i = 0; i < imgnum; ++i)
			{
				hnMile mile;
				tdmi = m_roadSpace*i;
				tmile = enclToTrueMile(tdmi);
				if ((m_projectInfo.nLineType < 0 && tmile <= eMile)
					|| (m_projectInfo.nLineType > 0 && tmile >= eMile))
				{
					break;
				}

				mile.dTrueMile = qRound(tmile);
				mile.dEnclMile = tdmi;
				mile.nDMi = m_projectInfo.dWheelPerimeter == 0 ? 0 : m_projectInfo.nFrequency*tdmi / m_projectInfo.dWheelPerimeter;
				mile.roadGradStr = QString::fromLocal8Bit(m_projectInfo.strRoadLevel);
				mile.roadStandard = HnProjectEnums::roadTypeQStringToEnum(QString::fromLocal8Bit(m_projectInfo.strRoadStandard));
				mile.drawType = (hnCommon::ROAD_WORK_TYPE) m_projectInfo.nDrawType;
				mile.roadWidth = m_projectInfo.dRoadWidth;
				mile.roadType = (hnCommon::ROAD_SURFACE_TYPE) m_projectInfo.nRSurfaceType;
				mile.roadGrad = mile.GradStrToGrad(QString::fromLocal8Bit(m_projectInfo.strRoadLevel));
				if (picPathList.count() > i)
				{
					QFileInfo leftPicFile(picPathList.at(i));
					mile.picturePath = leftPicFile.fileName();
				}
				int leftStreetIndex = i / (m_leftStreetSpce / m_roadSpace);
				if (streetPicPathList0.count() > leftStreetIndex)
				{
					QFileInfo leftPicFile(streetPicPathList0.at(leftStreetIndex));

					mile.leftStreetPicPath = leftPicFile.fileName();
				}
				int rightStreetIndex = i / (m_rightStreetSpce / m_roadSpace);
				if (streetPicPathList1.count() > rightStreetIndex)
				{
					QFileInfo rightPicFile(streetPicPathList1.at(rightStreetIndex));
					mile.rightStreetPicPath = rightPicFile.fileName();
				}
				mile.nID = i;
				miles.push_back(mile);
			}
		}
		
		//根据打标文件将 miles 的路面类型 道路标准 等进行重新赋值

		QVector<double > mileValueVec;
		int picIndex  = 0;
		for (auto& mile : miles)
		{
			mileValueVec.push_back(mile.dTrueMile);

			int dirNum = picIndex  / 1000;
			QString dirName = QString("%1").arg(dirNum, 4, 10, QChar('0'));
			QString basePath = m_p2DProject->getBasePath() + "/RoadImg/Camera0/Image_" + dirName;
			if (!mile.picturePath.isEmpty())
			{
				mile.picturePath = basePath + "/" + mile.picturePath;
			}
			picIndex++;
		}

		 

		for (const auto& mark: m_vecMarkInfo)
		{
			 
			int startIndx = MyCommonMethods::findFirstGreaterOrEqual(m_projectInfo.nLineType ,mileValueVec, mark.dTrueMile);
			QString message = QString::fromLocal8Bit(mark.strMark);
			if (mark.nType == 1 || mark.nType == 4)
			{
				//auto& mile = miles[startIndx];
				//switch (mark.nType)
				//{
				// 
				//case 1: //路面单元 
				//	mile.roadUnitStr = message;
				//	break; 
				//case 4:
				//	mile.roadUnitStr += message+"\n";
				//	break;
				//default:
				//	break;
				//}
			}
			else
			{
				for (int j = startIndx; j < mileValueVec.size(); j++)
				{
					auto& mile = miles[j]; 
					//				//判断以下当前标准是否有这种材质？暂时不做判断
					HnProjectEnums::StandardParmTypeEnum nowStandard = mile.roadStandard;
					switch (mark.nType)
					{
					case 0: //路面材质

						if (message.contains(QStringLiteral("沥青")))
						{
							mile.roadType = hnCommon::ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE;
						}
						if (message.contains(QStringLiteral("水泥")))
							mile.roadType = hnCommon::ROAD_SURFACE_TYPE::ROAD_SN_SURFACE;

						if (message.contains(QStringLiteral("砂石")))
						{
							mile.roadType = hnCommon::ROAD_SURFACE_TYPE::ROAD_SS_SURFACE;
						}
						break; 
					case 2: //路面等级
						mile.roadGradStr = message;
						mile.roadGrad = mile.GradStrToGrad(message);
						break;
					case 3: //路面标准
						mile.roadStandard = HnProjectEnums::roadTypeQStringToEnum(message);
						break;
					default:
						break;
					}
				}
			}
	

		} 
		m_vecMile = miles;
	/*	if (changeMark)
		{
			m_pDbSqlite->m_mileInfoTable.clearData();
			m_pDbSqlite->m_mileInfoTable.writeData(m_vecMile);
		}
		 int dbMileCount = m_pDbSqlite->m_mileInfoTable.getMaxID(); 
		 if (dbMileCount==1)
		 {
			 m_pDbSqlite->m_mileInfoTable.writeData(m_vecMile);
		 }*/
		if (miles.length() > 0)
		{
			m_currentMile = miles.at(0);
		}
#ifdef DEBUG
		//QStringList mileList;
		//for (auto mile : miles)
		//{
		//	//qDebug() <<
		//	QString mileInfo = mile;
		//	mileList.push_back(mileInfo);
		//}

		//QString MilePath = m_p2DProject->getMilesTextPath();

		//MyCommonMethods::writeAllLines(MilePath, mileList, QTextCodec::codecForName("utf-8"));
#endif // CWB_测试
	}

	double hnProject::getRoadSpace()
	{
		return m_roadSpace;

	}

	QVector<hnMile> hnProject::getCurrentMileVector()
	{
		 /*if (currentMileVec.size()>0)
		 {
			 return currentMileVec;
		 }
		for (int i = 0 ; i<m_vecMile.size();++i)
		{
			int dirNum = i / 1000;
			QString dirName = QString("%1").arg(dirNum,4,10,QChar('0'));
			QString basePath = m_p2DProject->getBasePath() + "/RoadImg/Camera0/Image_" + dirName; 
			hnMile curMile(m_vecMile[i]);
			curMile.picturePath = basePath + "/" + m_vecMile[i].picturePath;
			currentMileVec.push_back(curMile);
		} */
		//如果m_vecMile.size() = 0 ;
		return m_vecMile;
	}

	hnMile hnProject::getCloseMile(double targetMile)
	{
		QVector<hnMile> miles = getCurrentMileVector();
		//查找第一个大于或者等于目标桩号的位置
		 int line = getCurProSetInfo().nLineType;
		auto it = std::lower_bound(miles.begin(), miles.end(), targetMile, [line](const hnMile&hnmile, double mile) {

			if (line == 1)
			{
				return hnmile.dTrueMile < mile;
			}
			else
			{
				return hnmile.dTrueMile > mile;
			}

		});

		//处理边界清空
		if (it == miles.begin())
		{
			return (*it);
		}
		if (it == miles.end())
		{
			return (*(it - 1));
		}
		//比较it和it-1 找到最接近的点
		const hnMile& nextMile = *it;
		const hnMile &prevMile = *(it - 1);

		if (std::abs(nextMile.dTrueMile - targetMile) < std::abs(prevMile.dTrueMile - targetMile))
		{
			return nextMile;
		}
		else
		{
			return prevMile;
		}
	}

	hnMile hnProject::getCloseMileFromDmi(double dmi)
	{
		QVector<hnMile> miles = getCurrentMileVector();
		//查找第一个大于或者等于目标桩号的位置
		int line = getCurProSetInfo().nLineType;
		auto it = std::lower_bound(miles.begin(), miles.end(), dmi, [](const hnMile&hnmile, double dmi) {

				return hnmile.dEnclMile < dmi;
		});

		//处理边界清空
		if (it == miles.begin())
		{
			return (*it);
		}
		if (it == miles.end())
		{
			return (*(it - 1));
		}
		//比较it和it-1 找到最接近的点
		const hnMile& nextMile = *it;
		const hnMile &prevMile = *(it - 1);

		if (std::abs(nextMile.dEnclMile - dmi) < std::abs(prevMile.dEnclMile - dmi))
		{
			return nextMile;
		}
		else
		{
			return prevMile;
		}
	}

	hnMile hnProject::getCloseStreetMileFromDim(double dmi)
	{ 
		QVector<hnMile> miles = getLeftStreetMiles();
		//查找第一个大于或者等于目标桩号的位置
		int line = getCurProSetInfo().nLineType;
		auto it = std::lower_bound(miles.begin(), miles.end(), dmi, [](const hnMile&hnmile, double dmi) {

			return hnmile.dEnclMile < dmi;
		});

		//处理边界清空
		if (it == miles.begin())
		{
			return (*it);
		}
		if (it == miles.end())
		{
			return (*(it - 1));
		}
		//比较it和it-1 找到最接近的点
		const hnMile& nextMile = *it;
		const hnMile &prevMile = *(it - 1);

		if (std::abs(nextMile.dEnclMile - dmi) < std::abs(prevMile.dEnclMile - dmi))
		{
			return nextMile;
		}
		else
		{
			return prevMile;
		}
	}

	QVector<hnMile> hnProject::getLeftStreetMiles()
	{
	 if (currentLeftStreetMileVec.size()>0)
	 {
		 return currentLeftStreetMileVec;
	 }
		QVector <hnMile> tempMiles;
		int oldIdx = -1;
		for (int i = 0; i < m_vecMile.size(); ++i)
		{
			
			int curIdx = i / (m_leftStreetSpce / m_roadSpace);
			if (curIdx!=oldIdx)
			{
				hnMile curMile(m_vecMile[i]);
				tempMiles.push_back(curMile);
				oldIdx = curIdx;
			} 
		}


		/*for (int i = 0; i < m_vecMile.size(); ++i)
		{
		
			if (std::fmod(m_vecMile[i].dEnclMile, m_leftStreetSpce) == 0)
			{
				hnMile curMile(m_vecMile[i]);
				tempMiles.push_back(curMile);
			}
		}*/

		for (int i = 0; i <  tempMiles.size(); ++i)
		{ 
			int streetIndex = static_cast<int>(std::round(i* (m_leftStreetSpce / m_roadSpace)));
			int rightStreetIndex = static_cast<int>(std::round(i* (m_rightStreetSpce / m_roadSpace)));
			if (streetIndex >= m_vecMile.size())
			{
				streetIndex = m_vecMile.size()-1;
			}
			if (rightStreetIndex >= m_vecMile.size())
			{
				rightStreetIndex = m_vecMile.size() - 1;
			}
				int dirNum = i / 1000;
				QString dirName = QString("%1").arg(dirNum, 4, 10, QChar('0'));
				QString baseLeftPath = m_p2DProject->getBasePath() + "/StreetImg/Camera0/Image_" + dirName;
				QString baseRightPath = m_p2DProject->getBasePath() + "/StreetImg2/Camera0/Image_" + dirName;
				hnMile curMile(m_vecMile[streetIndex]);
				curMile.leftStreetPicPath = baseLeftPath + "/" + m_vecMile[streetIndex].leftStreetPicPath;
				 curMile.rightStreetPicPath = baseRightPath + "/" + m_vecMile[rightStreetIndex].rightStreetPicPath;
				currentLeftStreetMileVec.push_back(curMile);
		}
		return currentLeftStreetMileVec;
	}

	 QVector<hnMile> hnProject::getRightStreetMiles()
	{
		 if (currentRightStreetMileVec.size() > 0)
		 {
			 return currentRightStreetMileVec;
		 }
		 QVector <hnMile> tempMiles;
		 int oldIdx = -1;
		 for (int i = 0; i < m_vecMile.size(); ++i)
		 {

			 int curIdx = i / (m_rightStreetSpce / m_roadSpace);
			 if (curIdx != oldIdx)
			 {
				 hnMile curMile(m_vecMile[i]);
				 tempMiles.push_back(curMile);
				 oldIdx = curIdx;
			 }
		 }
		/* for (int i = 0; i < m_vecMile.size(); ++i)
		 {

			 if (std::fmod(m_vecMile[i].dEnclMile, m_rightStreetSpce/2) == 0)
			 {
				 hnMile curMile(m_vecMile[i]);
				 tempMiles.push_back(curMile);
			 }
		 }*/

		 for (int i = 0; i < tempMiles.size(); ++i)
		 {
			 int leftStreetIndex = static_cast<int>(std::round(i* (m_leftStreetSpce / m_roadSpace)));
			 int streetIndex = static_cast<int>(std::round( i* (m_rightStreetSpce / m_roadSpace)));
			 if (streetIndex >= m_vecMile.size())
			 {
				 streetIndex = m_vecMile.size()-1;
			 }
			 if (leftStreetIndex >= m_vecMile.size())
			 {
				 leftStreetIndex = m_vecMile.size() - 1;
			 }

			 int dirNum = i / 1000;
			 QString dirName = QString("%1").arg(dirNum, 4, 10, QChar('0'));
			 QString baseLeftPath = m_p2DProject->getBasePath() + "/StreetImg/Camera0/Image_" + dirName;
			 QString baseRightPath = m_p2DProject->getBasePath() + "/StreetImg2/Camera0/Image_" + dirName;
			 hnMile curMile(m_vecMile[streetIndex]);
			 curMile.leftStreetPicPath = baseLeftPath + "/" + m_vecMile[leftStreetIndex].leftStreetPicPath;
			 curMile.rightStreetPicPath = baseRightPath + "/" + m_vecMile[streetIndex].rightStreetPicPath;
			 currentRightStreetMileVec.push_back(curMile);
		 }
		 return currentRightStreetMileVec;
	}

	QVector<hnCommon::hnMarkInfo> hnProject::getCurrentMarkVector()
	{
	   
		QVector<hnCommon::hnMarkInfo> marks;
		marks = QVector<hnCommon::hnMarkInfo>::fromStdVector(m_vecMarkInfo);
		std::sort(marks.begin(), marks.end()); 
		if (m_projectInfo.nLineType==-1)
		{
			std::reverse(marks.begin(), marks.end());
		}
		return marks;
	}

	QVector<hnCommon::hnMilePile> hnProject::getCurrentMilePileVector()
	{ 
		return QVector<hnCommon::hnMilePile>::fromStdVector(m_vecMileagePile);
	}
	bool hnProject::isRoadAttributeMark(int nType) const
	{
		return nType == hnCommon::ROAD_MARK_TYPE::ROAD_SURFACE
			|| nType == hnCommon::ROAD_MARK_TYPE::ROAD_GRAD
			|| nType == hnCommon::ROAD_MARK_TYPE::ROAD_STANDARD;
	}

	bool hnProject::saveMarksToResultDb()
	{
		if (!m_pDbSqlite)
		{
			return false;
		}
		m_pDbSqlite->m_markerInfoTable.clearData();
		return m_pDbSqlite->m_markerInfoTable.writeData(m_vecMarkInfo);
	}

	bool hnProject::saveMileagePilesToResultDb()
	{
		if (!m_pDbSqlite)
		{
			return false;
		}
		m_pDbSqlite->m_milePileTable.clearData();
		return m_pDbSqlite->m_milePileTable.writeData(m_vecMileagePile);
	}

	bool hnProject::saveProjectSettingToResultDb()
	{
		if (!m_pDbSqlite)
		{
			return false;
		}
		m_pDbSqlite->m_projectSetTable.clearData();
		return m_pDbSqlite->m_projectSetTable.writeData(m_projectInfo);
	}

	void hnProject::import2DFieldDataToResultDb()
	{
		if ((!m_bNeedImportFieldMilePilesToResultDb && !m_bNeedImportFieldMarksToResultDb) || !m_p2DProject || !m_pDbSqlite)
		{
			return;
		}

		bool importSuccess = true;
		if (m_bNeedImportFieldMilePilesToResultDb)
		{
			QFileInfo milePileFileInfo(m_p2DProject->getMilePilePath());
			if (milePileFileInfo.exists())
			{
				m_p2DProject->add2dMilePile(m_vecMileagePile);
				sort(m_vecMileagePile.begin(), m_vecMileagePile.end(), compareMilepileByEnclMile);
				importSuccess = saveMileagePilesToResultDb() && importSuccess;
			}
			else
			{
				importSuccess = false;
			}
		}
		if (m_bNeedImportFieldMarksToResultDb)
		{
			QFileInfo markFileInfo(m_p2DProject->getMarkFilePath());
			if (markFileInfo.exists())
			{
				m_p2DProject->add2dMarkInfo(m_vecMarkInfo, this);
				importSuccess = saveMarksToResultDb() && importSuccess;
			}
			else
			{
				importSuccess = false;
			}
		}

		if (!importSuccess)
		{
			return;
		}

		QFile importFlag(m_strDbDirPath + "/FieldSourceImported.flag");
		if (importFlag.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			importFlag.close();
		}
		m_bNeedImportFieldMilePilesToResultDb = false;
		m_bNeedImportFieldMarksToResultDb = false;
	}


 bool hnProject::AddDisease()
 {
	 //病害写入数据库



	 //病害计入内存
	 return false;
 }

 void hnProject::setCurrentRoadMile(const hnMile & mile)
	{
		m_currentMile = mile;
	}

	void hnProject::setCurrent3dRoadDmi(const double& dmi)
	{
		this->m_current3dDmi = dmi;
	}

	bool hnProject::changeMark(QVector<hnCommon::hnMarkInfo>& marks, const QVector<int>&deleteMarkIndexs)
	{
		bool needUpdate = false;
		int lastIndex = m_pDbSqlite ? m_pDbSqlite->m_markerInfoTable.getMaxID() : 1;
		for (const auto& item : m_vecMarkInfo)
		{
			lastIndex = std::max(lastIndex, item.nID + 1);
		}

		for (auto& mark : marks)
		{
			if (isRoadAttributeMark(mark.nType))
			{
				needUpdate = true;
			}
			mark.dEnclMile = trueMileToEncl(mark.dTrueMile);
			mark.nID = lastIndex++;
			m_vecMarkInfo.push_back(mark);
		}

		QSet<int> deleteSet = QSet<int>::fromList(deleteMarkIndexs.toList());
		m_vecMarkInfo.erase(
			std::remove_if(
				m_vecMarkInfo.begin(),
				m_vecMarkInfo.end(),
				[&deleteSet, &needUpdate, this](const hnCommon::hnMarkInfo& obj) {
					const bool deleteItem = deleteSet.contains(obj.nID);
					if (deleteItem && isRoadAttributeMark(obj.nType))
					{
						needUpdate = true;
					}
					return deleteItem;
				}
			),
			m_vecMarkInfo.end()
		);

		saveMarksToResultDb();
		if (needUpdate)
		{
			initMileList(false);
		}
		return needUpdate;
	}

	bool hnProject::addMark(hnCommon::hnMarkInfo& mark)
	{
		bool needUpdate = isRoadAttributeMark(mark.nType);
		int lastIndex = m_pDbSqlite ? m_pDbSqlite->m_markerInfoTable.getMaxID() : 1;
		for (const auto& item : m_vecMarkInfo)
		{
			lastIndex = std::max(lastIndex, item.nID + 1);
		}

		mark.dEnclMile = trueMileToEncl(mark.dTrueMile);
		mark.nID = lastIndex;
		m_vecMarkInfo.push_back(mark);
		saveMarksToResultDb();
		if (needUpdate)
		{
			initMileList(false);
		}
		return needUpdate;
	}

	bool hnProject::deleteMark(const hnCommon::hnMarkInfo& Mark)
	{
		return deleteMark(Mark.nID);
	}

	bool hnProject::deleteMark(int id)
	{
		bool needUpdate = false;
		for (int i = static_cast<int>(m_vecMarkInfo.size()) - 1; i >= 0; --i)
		{
			hnCommon::hnMarkInfo curMark = m_vecMarkInfo[i];
			if (curMark.nID == id)
			{
				if (isRoadAttributeMark(curMark.nType))
				{
					needUpdate = true;
				}
				m_vecMarkInfo.erase(m_vecMarkInfo.begin() + i);
			}
		}

		saveMarksToResultDb();
		if (needUpdate)
		{
			initMileList(false);
		}
		return needUpdate;
	}

	void hnProject::changeMilePile(QVector < hnCommon::hnMilePile>& piles)
	{
		m_vecMileagePile.clear();
		for (auto mile : piles)
		{
			m_vecMileagePile.push_back(mile);
		}

		sort(piles.begin(), piles.end(), compareMilepileByEnclMile);
		sort(m_vecMileagePile.begin(), m_vecMileagePile.end(), compareMilepileByEnclMile);
		saveMileagePilesToResultDb();
		initMileList(false);
	}

	void hnProject::addMilePile(hnCommon::hnMilePile& pile)
	{
		int maxId = m_pDbSqlite ? m_pDbSqlite->m_milePileTable.getMaxID() : 1;
		for (const auto& item : m_vecMileagePile)
		{
			maxId = std::max(maxId, item.nID + 1);
		}
		pile.nID = maxId;
		m_vecMileagePile.push_back(pile);

		sort(m_vecMileagePile.begin(), m_vecMileagePile.end(), compareMilepileByEnclMile);
		saveMileagePilesToResultDb();
		initMileList(false);
	}

	void hnProject::deleteMilePile(int id)
	{
		for (int i = static_cast<int>(m_vecMileagePile.size()) - 1; i >= 0; --i)
		{
			hnCommon::hnMilePile curMile = m_vecMileagePile[i];
			if (curMile.nID == id)
			{
				m_vecMileagePile.erase(m_vecMileagePile.begin() + i);
			}
		}
		saveMileagePilesToResultDb();
		initMileList(false);
	}
	void hnProject::updataMarkDatabase()
	{
		saveMarksToResultDb();
	}

	// 获取是否存在成果db  根据道路等级+绘制模式 查找数据库地址    isOld 是否是数据库名称代码更新之前的工程，需要适配

	bool hnProject::getResultDB(const QString dbDirPath,QString& strDB, bool isOld)
	{
		 
		QString strpath = dbDirPath + "/dbSetting.txt";
		//获取软件所在电脑名称
		QString PCName = QHostInfo::localHostName();
		QDir dir1;
		//如果没有那么创建
		if (dir1.exists(strpath)&&!isOld)
		{
			 
			QSettings settings(strpath, QSettings::IniFormat);
			settings.setIniCodec(QTextCodec::codecForName("GBK"));

			//根据电脑名称找对应
			QString strValue = settings.value(QString::fromLocal8Bit("Project/DB_%1").arg(PCName)).toString();
			if (!strValue.isEmpty())//|| !dir1.exists(projectPath + "/" + strValue)
			{
				strDB= strValue;

				return true;
			}
		}
		// 查找所有db数据
		vector<QString> vecDBPath;
		hnFile fileManager;
		QString strFolder = dbDirPath /*+ "/" + m_strProjectName*/;
		fileManager.getFileType(strFolder, QString::fromLocal8Bit("db"), vecDBPath);
		if (vecDBPath.size() <= 0)
		{
			return false;
		}
		 if (isOld)
		 {
			 for (int i = 0; i < vecDBPath.size(); i++)
			 {
				 if (m_nProjectType == PROJECT_TYPE::PROJECT_23D_TYPE || m_nProjectType == PROJECT_TYPE::PROJECT_2D_TYPE)
				 {
					 if (vecDBPath[i] != m_str2DProName + ".db")
					 {
						 continue;
					 }
				 }
				 else
				 {
					 if (vecDBPath[i] !=m_str3DProName + ".db")
					 {
						 continue;
					 }
				 }


				 strDB = strFolder + "/" + vecDBPath[i];

				 return true;
			 }
		 }
		 else
		 {
			 for (int i = 0; i < vecDBPath.size(); i++)
			 {
				 if (vecDBPath[i] != QString::fromLocal8Bit("成果.db"))
				 {
					 continue;
				 }
				 strDB = strFolder + "/" + vecDBPath[i];

				 return true;
			 }
		 } 
		return false;
	}

	void hnProject::updatePorjectDb()
	{
		saveMarksToResultDb();
		saveMileagePilesToResultDb();
		saveProjectSettingToResultDb();
	}


	void hnProject::exportCorrectedFieldTextFiles()
	{
		if (this->get2DProject()==NULL)
		{
			return;
		}
		//导出内业打标修正文件，不覆盖外业原始 RoadStatuMarkInfo.txt。
		QFileInfo markFileInfo(this->get2DProject()->getMarkFilePath());
		QString markPath = markFileInfo.absolutePath() + QStringLiteral("/RoadStatuMarkInfo_内业修正.txt");
		QVector<hnCommon::hnMarkInfo> marks = 	this->getCurrentMarkVector();
		QStringList markTxts;
		for (int i = 0 ; i <marks.size();++i)
		{
			auto curMark = marks[i]; 
			// 打标类型:0-路面材质；1-路面单元；2-路面等级; 3-路面标准；4-路面情况
			QString curType="";
			switch (curMark.nType)
			{
			case 0:
				curType = QStringLiteral("路面材质");
				break;
			case  1:
				curType = QStringLiteral("路面单元");
				break;
			case  2:
				curType = QStringLiteral("路面等级");
				break;
			case  3:
				curType = QStringLiteral("路面标准");
				break;
			case 4:
				curType = QStringLiteral("路面情况");
			default:
				break;
			}
			QString markLine = QString::number(curMark.dTrueMile,'f',0) + " " + QString::number(curMark.dTrueMile,'f',0)+" ";
			if (this->get2DProject()->getIs23DEquipment()) //二三维外业采集的数据
			{
				markLine += QString::number(qRound((this->trueMileToEncl(curMark.dTrueMile) + m_projectInfo.dBegEnclMile)),'f',0) + " " + curType + ":" + QString::fromLocal8Bit(curMark.strMark);

			}
			else
			{
				markLine += QString::number(qRound(this->trueMileToEncl(curMark.dTrueMile)),'f',0) + " " + curType + ":" + QString::fromLocal8Bit(curMark.strMark);

			}
			markTxts.push_back(markLine);
		}
		 
		MyCommonMethods::writeAllLines(markPath, markTxts);
	

		//导出内业打标分段修正文件。
		 QFileInfo roadTypeFileInfo(this->get2DProject()->getFullRoadTypeMarkFilePath());
		 markPath = roadTypeFileInfo.absolutePath() + QStringLiteral("/RoadTypeInfo_内业修正.txt");
		  marks = this->getCurrentMarkVector();
		QStringList markRoadTypeTxts; 
		QVector<hnCommon::hnMarkInfo> curMarks;
		hnCommon::hnMarkInfo startMark;
		startMark.dEnclMile = 0;
		double mile = this->enclToTrueMile(0);
		startMark.dTrueMile = m_projectInfo.dBegMile;
		startMark.nType = 0;
		strcpy(startMark.strMark, m_projectInfo.getRSurfaceType().c_str()); 
		curMarks.push_back(startMark);

		hnCommon::hnMarkInfo lastMark;
		lastMark.dTrueMile = m_projectInfo.dEndMile;
		lastMark.dEnclMile = m_projectInfo.dLength;
		lastMark.nType = 0;
		if (marks.size()==0)
		{ 
			strcpy(lastMark.strMark, m_projectInfo.getRSurfaceType().c_str()); 
		}
		else
		{ 
			
			strcpy(lastMark.strMark, marks.last().strMark);
		}
		for (int  i = 0; i < marks.size(); ++i)
		{
			auto cur = marks.at(i);
			if (cur.nType == 0)
			{
				curMarks.push_back(cur);
			}
		}
		curMarks.push_back(lastMark);

		for (int i = 0  ; i <curMarks.size()-1; ++i)
		{
			auto mark0 = curMarks.at(i);
			auto mark1= curMarks.at(i+1);
			QString markStr = QString::number((int)mark0.dTrueMile) + "-" + QString::number((int)mark1.dTrueMile) + "Km" + QString::fromLocal8Bit( mark0.strMark);
			markRoadTypeTxts.push_back(markStr);
		}
		 
		MyCommonMethods::writeAllLines(markPath, markRoadTypeTxts);



		//导出内业较桩修正文件，不覆盖外业原始 Dmi2Mile.txt。
		QFileInfo milePileFileInfo(this->get2DProject()->getMilePilePath());
		QString path = milePileFileInfo.absolutePath() + QStringLiteral("/Dmi2Mile_内业修正.txt");
		QVector<hnCommon::hnMilePile> milePile = 	this->getCurrentMilePileVector();
		QStringList txts;
		QStringList For2DTxts;
		for (int i = 0; i < milePile.size();++i)
		{
			QString line = "";
			QString for2dTxt = "";
			hnCommon::hnMilePile curMile = milePile[i];

				if (this->get2DProject()->getIs23DEquipment()) //二三维外业采集的数据
				{ 
					line = QString::number( qRound((curMile.dEnclMile + m_projectInfo.dBegEnclMile))) + " " 
						+ QString::number(qRound( curMile.dTrueMile));
					
					 
						for2dTxt = QString::number(qRound(curMile.dEnclMile)) + " " + QString::number(qRound(curMile.dTrueMile));
						 
				}
				else
				{
					line = QString::number(qRound(curMile.dEnclMile)) + " " + QString::number(qRound(curMile.dTrueMile));
					 
					for2dTxt = QString::number(qRound(curMile.dEnclMile)) + " " + QString::number(qRound(curMile.dTrueMile));
				}
			txts.push_back(line);
			if (i != 0 && i != milePile.size() - 1)
			For2DTxts.push_back(for2dTxt);
		}
		MyCommonMethods::writeAllLines(path, txts);
		//For2DTxts.removeFirst();
		//For2DTxts.removeLast();
		QFileInfo caliFileInfo(this->get2DProject()->getMileStoneCaliInfoFilePath());
		MyCommonMethods::writeAllLines(caliFileInfo.absolutePath() + QStringLiteral("/MileStoneCaliInfo_内业修正.txt"), For2DTxts);

	}

	void hnProject::updatePorjectText()
	{
		//更新较桩文件 
		//更新打标文件
	}

	void hnProject::updatePorjectAllSettingSource()
	{
		// 只同步成果库。外业文本必须由用户手动导出，不能在刷新里程时自动覆盖。
		updatePorjectDb();
	}

	void hnProject::updataMilePileDatabase()
	{
		saveMileagePilesToResultDb();
	}

}