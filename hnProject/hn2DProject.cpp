#include "hn2DProject.h"
#include <QDir>
#include <QFileInfoList>
#include <QFileInfo>
#include <QDebug>
#include <QTextCodec>
#include <QSettings> 
#include "../hnQtCommon/MyCommonMethods.h" 
#include "../hnConfigService/HnXRSettings.h"
#include <QImage>
#include <QImageReader>
namespace hnPro
{
	hn2DProject::hn2DProject()
	{
		m_xrSetting = HnXRSettings::getInstance();
	}


	hn2DProject::~hn2DProject()
	{
	}

	// 初始化2D工程
	//配置必须文件   缺少必须文件返回false  二维工程初始化失败
	bool hn2DProject::init(QString strProjectPath, QString strProjectName, hnProjectSetInfo projectInfo,PROJECT_TYPE type)
	{
		m_projectInfo = projectInfo;
		QString proPath;
		switch (type)
		{
		case hnCommon::PROJECT_2D_TYPE:
			proPath = strProjectPath;
			break;
		case hnCommon::PROJECT_XD_3D_TYPE:
			proPath = strProjectPath + "/" + strProjectName;
			break;
		case hnCommon::PROJECT_JD_3D_TYPE:
			proPath = strProjectPath + "/" + strProjectName;
			break;
		case hnCommon::PROJECT_23D_TYPE:
			proPath = strProjectPath + "/" + strProjectName;
			break;
		default:
			proPath = strProjectPath + "/" + strProjectName;
			break;
		}
		this->m_strProjectPath = proPath;

		bool b1 = setPath(proPath + QStringLiteral("/IRIMTD"), m_strIRIPath, false);
		bool b4 = setPath(proPath + QStringLiteral("/Setting.ini"), m_strSettinginiPath, true);
		bool b2 = setPath(proPath + QStringLiteral("/camera0"), m_strLeftRutPath);
		bool b5 = setPath(proPath + QStringLiteral("/camera1"), m_strRightRutPath);
		setPath(proPath + QStringLiteral("/RUT"), m_strRutResultPath);

		bool b3 = setPath(proPath + QStringLiteral("/RoadStatuMarkInfo.txt"), m_strMarkInfoPath);

		setPath(proPath + QStringLiteral("/RoadTypeInfo.txt"), m_strFullRoadTypeMarkInfoPath);
		QString markPathStr = proPath + QStringLiteral("/RoadStatuMarkInfo.txt");
		QFile markFile(markPathStr);
		if (!markFile.exists())
		{
			if (markFile.open(QIODevice::WriteOnly))
			{
				markFile.close();
			}
		}
		 
		bool b6 = setPath(proPath + QStringLiteral("/Dmi2Mile.txt"), m_strMilePilePath);
	     setPath(proPath + QStringLiteral("/MileStoneCaliInfo.txt"), m_MileStoneCaliInfoFilePath);

		setPath(proPath + QStringLiteral("/23dConfig.txt"), m_strUserMilePath);

		//bool b5 = setPath(proPath + QStringLiteral("/Setting.ini"), m_strSettinginiPath);
		  setPath(proPath + QStringLiteral("/ProjectInfo.txt"), m_projectPath, true);
		setPath(proPath + QStringLiteral("/GPS2Mile.txt"), m_gpsResultFilePath);
		//添加道路图片路径
		m_vecRoadPicMilePath = addPicturePaths(proPath, QStringLiteral("/RoadImg/Camera0"));
		if (m_vecRoadPicMilePath.size()>0)
		{ 
			if (QFile::exists(m_vecRoadPicMilePath[0]))
			{ 
				QImage image(m_vecRoadPicMilePath[0]);
				if (image.isNull())
				{
				}
				else
				{
					m_RoadPictureWidth = image.width();
					m_RoadPictureHeight = image.height();
				}
			}
		}
	



		//添加景观图片路径
		m_vecRoadLeftStreetPicMilePath.clear();
		m_vecRoadRightStreetPicMilePath.clear();

		m_vecRoadLeftStreetPicMilePath = addPicturePaths(proPath, QStringLiteral("/StreetImg/Camera0"));
		m_vecRoadRightStreetPicMilePath = addPicturePaths(proPath, QStringLiteral("/StreetImg/Camera1"));
		if (m_vecRoadRightStreetPicMilePath.size()==0)
		{
			m_vecRoadRightStreetPicMilePath = addPicturePaths(proPath, QStringLiteral("/StreetImg2/Camera0"));
		}

		m_strMilesTextPath = proPath + QStringLiteral("/23D_milesText.txt");

		initEquipmentBasePath(proPath);
		initRoadInfo(proPath);
		initGpsInfos();
		////初始化桩号文件 
		//vector<hnMilePile>;
		//hnMilePile mile;
		//QString  gpsFilePath = proPath + QStringLiteral("/RoadImg/SYN/gps.txt");
		//QString  triggerFilePath = proPath + QStringLiteral("/RoadImg/SYN/trigger.txt");

		return b1&&b4;

		//return true;
	}

	bool hn2DProject::getIsHMirrored()
	{
		QString fileName = m_strProjectPath + "/mirroredSetting.ini";
		this->checkMirroredFile(fileName);

		QSettings settings(fileName,QSettings::IniFormat);
		settings.beginGroup("CONFIG");
		QString value = settings.value("HMirrored").toString();
		settings.endGroup();
		if (value == "true")
		{
			return true;
		}
		else
		{
			return false;
		}	
		return false;
	}

	void hn2DProject::setIsHMirrored(bool mirrored)
	{
		QString fileName = m_strProjectPath + "/mirroredSetting.ini";

		this->checkMirroredFile(fileName);

		QString strMirrored = mirrored ? "true" : "false";

		QSettings settings(fileName, QSettings::IniFormat);
		settings.beginGroup("CONFIG");
		settings.setValue("HMirrored", strMirrored);
		settings.endGroup();
		settings.sync();

		return;
	}

	bool hn2DProject::getIsVMirrored()
	{
		QString fileName = m_strProjectPath + "/mirroredSetting.ini";
		this->checkMirroredFile(fileName);

		QSettings settings(fileName, QSettings::IniFormat);
		settings.beginGroup("CONFIG");
		QString value = settings.value("VMirrored").toString();
		settings.endGroup();
		if (value == "true")
		{
			return true;
		}
		else
		{
			return false;
		}
		return false;
	}

	void hn2DProject::setIsVMirrored(bool mirrored)
	{
		QString fileName = m_strProjectPath + "/mirroredSetting.ini";

		this->checkMirroredFile(fileName);

		QString strMirrored = mirrored ? "true" : "false";

		QSettings settings(fileName, QSettings::IniFormat);
		settings.beginGroup("CONFIG");
		settings.setValue("VMirrored", strMirrored);
		settings.endGroup();
		settings.sync();

		return;
	}

	void hn2DProject::getPixEncoderMile(const double encoderMile,const double yScale, double & pixEncoderMile, int & y)
	{
		if (encoderMile < 0)
		{
			pixEncoderMile = 0;
			y = 0;
		}
		if (encoderMile > _EndDmi)
		{
			pixEncoderMile = _EndDmi - _RoadImgDis;
			y = 0;
		}

		int frameIdx = encoderMile / _RoadImgDis;
		pixEncoderMile = frameIdx * _RoadImgDis;
		double singlePixMile = fmod(encoderMile, _RoadImgDis);
		singlePixMile = _RoadImgDis - singlePixMile;
		y = singlePixMile / yScale;
	}



	// 获取里程桩数据
	void hn2DProject::add2dMilePile(vector<hnMilePile>& vecMilePile)
	{
		 
		//判断是上行还是下行
		int line = 1;
		if (vecMilePile.size() >= 2)
		{
			if (vecMilePile.at(0).dEnclMile > vecMilePile.at(1).dEnclMile)
			{
				line = -1;
			}
		}

		int i = vecMilePile.size() - 1;
		QString markPath = this->m_strMilePilePath;
		QFile file(markPath);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			return;
		}
		QTextStream in(&file);
		while (!in.atEnd())
		{
			hnMilePile info;
			//文本格式
			//2115 2115    mile dmi
			QString line = in.readLine();
			QStringList lits = line.split(QStringLiteral(" "));
			if (lits.size() < 2)
			{
				return;
			}
			double mile = lits.at(1).toDouble();
			double dmi = lits.at(0).toDouble();
			if ( is23DEquipment) //二三维外业采集的数据
			{
				dmi = dmi - round(m_projectInfo.dBegEnclMile);
			} 
			bool isExits = false;
		 
			for each (hnMilePile nowMile in vecMilePile)
			{
				//已存在则不加入
				if (qAbs(nowMile.dTrueMile- mile)<=2)
				{
					isExits = true;
				}
			}
			if (isExits)
			{
				continue;
			}
			i++;
			info.nID = i;
			info.dTrueMile = mile;
			info.dEnclMile = dmi;
		 
			vecMilePile.push_back(info);
		}
		std::sort(vecMilePile.begin(), vecMilePile.end(), [=](const hnMilePile&a, const hnMilePile& b)
		{
			if (line > 1)
			{
				return a.dEnclMile > b.dEnclMile;
			}
			else
			{
				return a.dEnclMile < b.dEnclMile;
			}
		}
		);

	}

	// 分析二维工程中的打标信息  添加到vecMarkInfo
	QVector<hnMarkInfo>&  hn2DProject::add2dMarkInfo(vector<hnMarkInfo>& vecMarkInfo, hnProject * pro)
	{
		QVector<hnMarkInfo> newMarks;
		int lastMarkId = 0;
		if (vecMarkInfo.size()>=1)
		{
			lastMarkId = vecMarkInfo[vecMarkInfo.size() - 1].nID + 1;
		}
		 
		vector<hnMarkInfo> marksFromTxt; 
		QString markPath = this->m_strMarkInfoPath;
		QFile markFile(markPath);
		if (!markFile.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			return newMarks;
			 
		}
		QTextStream in(&markFile);
		in.setCodec(QTextCodec::codecForName("UTF-8"));
      
		while (!in.atEnd())
		{
			hnMarkInfo markInfo;
			//文本格式
			//{0} {0} {1} 路面单元:{2}", mile, dmi, info
			QString line = in.readLine();
			QStringList lits = line.split(QStringLiteral(" "));
			if (lits.size() < 4)
			{
				continue;
			}
			QString mileStr = lits[0];
			QString dmi = lits[2];
			double realMile = 0;
			if (is23DEquipment)
			{ 
				double realDmi = dmi.toDouble() - round(m_projectInfo.dBegEnclMile);  
				 realMile = pro->enclToTrueMile(realDmi);
			}
			else
			{
			 realMile = pro->enclToTrueMile(dmi.toDouble());
			}
			int realMilei = qRound(realMile);
			QString mile = QString::number(realMilei);
			QString context = lits[3];

			QStringList lists1;
			QString typeStr;
			QString message;
			lists1 = context.split(QStringLiteral("："));
			if (lists1.size() > 1)
			{
				typeStr = lists1[0];
				message = lists1[1];
			}
			else
			{
				lists1 = context.split(QStringLiteral(":"));
				typeStr = lists1[0];
				message = lists1[1];
			}

			hnCommon::ROAD_MARK_TYPE  type = getMarkType(typeStr);

			markInfo.nType = type;
			markInfo.dTrueMile = mile.toDouble();
			if (is23DEquipment) //二三维外业采集的数据
			{
				 
				markInfo.dEnclMile = dmi.toDouble() - round(m_projectInfo.dBegEnclMile);

			}
			else
			{
				markInfo.dEnclMile = dmi.toDouble();
			}
			//markInfo.strRemark = "";
			//markInfo.strMark =  message.toLocal8Bit().data();
			strcpy(markInfo.strMark, message.toLocal8Bit().data());
			
			marksFromTxt.push_back(markInfo);
		}

		if (vecMarkInfo.size() == 0 )
		{
			vecMarkInfo.insert(vecMarkInfo.end(), marksFromTxt.begin(), marksFromTxt.end());
		}
		else
		{
			 for ( auto& item: marksFromTxt)
			 {
				 if (std::find(vecMarkInfo.begin(),vecMarkInfo.end(),item) == vecMarkInfo.end()) 
				 {
					 item.nID = lastMarkId++;
					 vecMarkInfo.push_back(item);
					 newMarks.push_back(item);
				 }
			 }
		}

		return newMarks;
	}



	bool hn2DProject::setPath(const QString& path, QString& setPath, bool mustSet /*=false*/)
	{
		setPath = "";
		QDir dir;
		if (!dir.exists(path))
		{
			if (mustSet)
			{
				qDebug() << QStringLiteral("二维项目初始化失败:") << path << QStringLiteral("缺失");;
				return false;
			}
			else
			{

				setPath = path;
				return true;

			}
		}
		else
		{

			setPath = path;
			return true;
		}
	}

	QStringList hn2DProject::addPicturePaths(const QString& basePath0, const QString& basePath1)
	{
		QString basePath = basePath0 + basePath1;

		QStringList pathList;
		QDir dir(basePath);
		int imgcnt = 0;//数的图像张数，会丢帧
		int imgtrigcnt = 0;//触发计数，从开始
		QString tstr;
		QStringList tstrs;
		QString  fileName;
		if (dir.exists())
		{
			QStringList folders = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
			QStringList filteredFolders;
			for each (QString folderName in folders)
			{
				if (folderName.contains("Image_"))
				{
					filteredFolders.append(basePath + "/" + folderName);
				}
			}
			for each (QString folderPath in filteredFolders)
			{

				QDir dir1(folderPath);

				QStringList filters;
				filters << "*.jpg";
				dir1.setNameFilters(filters);

				QFileInfoList fileList = dir1.entryInfoList();
				for each (QFileInfo file in fileList)
				{
					fileName = basePath + "/" + dir1.dirName() + "/" + file.fileName();
					tstr = dir1.dirName() + file.fileName();
					tstrs = tstr.split(QString::fromLocal8Bit("_"));
					imgtrigcnt = tstrs.at(1).toInt();
					if (imgtrigcnt == imgcnt)
					{
						pathList.append(fileName);
						imgcnt++;
					}
					while (imgtrigcnt > imgcnt)
					{
						pathList.append(fileName);
						imgcnt++;
					}

				}

			}

		}
		else
		{

		}
		return pathList;
	}



	bool hn2DProject::initRoadInfo(const QString& basePath)
	{ 
		// 设置参数
		QFile setFile;
		setFile.setFileName( basePath+"\\ProjectInfo.txt");
		if (!setFile.open(QIODevice::ReadOnly))
		{
			return false;
		} 
		QString strData = "";
		QStringList listData;
		QString strValue = "";  
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
				if (listData.size() <= 1)
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
			strValue = listData[1];
			std::size_t found = strValue.indexOf("\r\n",0);
			if (found != std::string::npos)
			{
				strValue = strValue.mid(0, found);
			}

			std::size_t found2 = strValue.indexOf("\n",0);
			if (found2 != std::string::npos)
			{
				strValue = strValue.mid(0, found2);
			}
			if (listData[0].contains(QString::fromLocal8Bit("省")))
			{
				this->_Province = strValue;
			}
			else if (listData[0].contains(QString::fromLocal8Bit("市")))
			{
				this->_City = strValue;
				 
			}
			else if (listData[0].contains(QString::fromLocal8Bit("县")))
			{
				this->_District = strValue; 
			}
			else if (listData[0].contains(QString::fromLocal8Bit("工程起点道路编号")))
			{
				this->_RoadCode = strValue;
			}
			else if (listData[0].contains(QString::fromLocal8Bit("工程起点道路名称")))
			{
				this->_RoadName = strValue;
			}
			else if (listData[0].contains(QString::fromLocal8Bit("工程起点桩号")))
			{
				listData[1].replace("K", "");
				listData[1].replace("+", "");
				this->_StartMile = listData[1].toInt();
			}

			else if (listData[0].contains(QString::fromLocal8Bit("行车方向")))
			{
				if (strValue.compare(QStringLiteral("上行"))==0 )
				{
					this->_Direction = 1;
				}
				else
				{
					this->_Direction  = -1;
				}
			}
			else if (listData[0].contains(QString::fromLocal8Bit("公路等级")))
			{
				this->_RoadGrade = strValue;
			}
			else if (listData[0].contains(QString::fromLocal8Bit("车道")))
			{
				this->_RoadNum = strValue;
			}
			else if (listData[0].contains(QString::fromLocal8Bit("采集日期")))
			{
				this->_DataDate = strValue;
			}
			else if (listData[0].contains(QString::fromLocal8Bit("工程开始时刻")))
			{
				this->_DataTime = strValue;
			}
			else if (listData[0].contains(QString::fromLocal8Bit("检测员")))
			{
				this->_DataPerson = strValue;
			}
			else if (listData[0].contains(QString::fromLocal8Bit("检测天气")))
			{
				this->_DataWeather = strValue;
			}
			else if (listData[0].contains(QString::fromLocal8Bit("路面材质")))
			{

				if (strValue.compare(QStringLiteral("沥青")) == 0)
				{
					this->_RoadType = 0;
				}
				if (strValue.compare(QStringLiteral("水泥")) == 0)
				{
					this->_RoadType = 1;
				}
				if (strValue.compare(QStringLiteral("砂石")) == 0)
				{
					this->_RoadType = 2;
				}

				 
			}
			else if (listData[0].contains(QString::fromLocal8Bit("工程终点道路标识桩号")))
			{
				listData[1].replace("K", "");
				listData[1].replace("+", "");
				this->_EndMile = listData[1].toInt();
			}
			else if (listData[0].contains(QString::fromLocal8Bit("工程总里程数")))
			{

				listData[1].replace("K", "");
				listData[1].replace("+", "");
				this->_RoadGrade = listData[1].toInt();
			}
		}
	}

	void hn2DProject::initEquipmentBasePath(const QString& basePath)
	{
		QString iriPath;
		QString rutPath;
		QString roadPath;
		QString streetPath;
		iriPath = basePath + QStringLiteral("//IRIMTD");
		rutPath = basePath + QStringLiteral("//RUT");
		roadPath = basePath + QStringLiteral("//RoadImg");
		streetPath = basePath + QStringLiteral("//StreetImg");
		QDir  dir;
		if (dir.exists(iriPath))
		{
			m_EquipmentBasePath.insert(HnProjectEnums::EquipMentEnum::IRI, iriPath);
		}
		if (dir.exists(rutPath))
		{
			m_EquipmentBasePath.insert(HnProjectEnums::EquipMentEnum::RUT, rutPath);
		}
		if (dir.exists(roadPath))
		{
			m_EquipmentBasePath.insert(HnProjectEnums::EquipMentEnum::RUT, roadPath);
		}
		if (dir.exists(streetPath))
		{
			m_EquipmentBasePath.insert(HnProjectEnums::EquipMentEnum::RUT, streetPath);
		}

		QString str1;
		QFile file(m_strSettinginiPath);
		if (
			file.open(QIODevice::ReadWrite|QIODevice::Text))
		{
			
			QTextStream str(&file);
			 str1 = str.readAll();
			file.close();
		}
		if (str1.contains("3dRoad"))
		{
			is23DEquipment = true;
		}
		else
		{
			is23DEquipment = false;
		}
		//解析setting.ini文件 
		QSettings *settings = new QSettings(m_strSettinginiPath, QSettings::IniFormat);
		
		settings->beginGroup(QString("WorkMode")); 
		//检查组下是否存在子键
		bool exists = !settings->childKeys().isEmpty(); 
		if (!exists)
		{ 
			settings->endGroup();
			delete settings; 
			QFile* file = new QFile(m_strSettinginiPath);
			if (file->open(QIODevice::ReadWrite)) 
			{
				QTextCodec* gb = QTextCodec::codecForName("GB2312");
				QString content = gb->toUnicode(file->readAll()); 
				content.replace(QStringLiteral("工作模式"), "WorkMode"); 
				//写入
				MyCommonMethods::writeAllLines(m_strSettinginiPath, content);

				 settings = new QSettings(m_strSettinginiPath, QSettings::IniFormat);
				settings->beginGroup(QString("WorkMode"));
				exists = !settings->childKeys().isEmpty();
			} 
		}

		_IsStreet = settings->value("Street").toBool();
		_IsRoad = settings->value("Road").toBool();
		_IsIRIMTD = settings->value("IRIMTD").toBool();
		_IsDIRIMTD = settings->value("DIRIMTD").toBool();
		_IsMMTD = settings->value("MMTD").toBool();
		_GeoAlig = settings->value("GeoAlig").toInt();
		_IsRut = settings->value("Rut").toBool();
		if (!_IsRut)

		{
			_IsRut = settings->value("OnlyShowRut3d").toBool();
		}

		bool IsSRut = settings->value("SRut").toBool();
		bool IsDRut = settings->value("DRut").toBool();
		bool Is3DRut = settings->value("3DRut").toBool();
		if (IsSRut)
			_RutMode = 0;
		if (IsDRut)
			_RutMode = 1;
		if (Is3DRut)
			_RutMode = 2;

		_IsDStreet = settings->value("DStreet").toBool();

		_IsPano = settings->value("Pano").toBool();
		_DMIScale = settings->value("DMIScale").toDouble()*0.001;
		if (_DMIScale == 0)
		{
			_DMIScale = 1;
		}

		settings->endGroup();

		//设置要读取的组名 
		settings->beginGroup(QString("Parm"));

		_RutDis = settings->value("RUT_Dis").toInt();
		_RoadImgDis = settings->value("RoadDis").toInt();
		_StreetImgDis = settings->value("StreetDis").toInt();
		_StreeRightImgDis = settings->value("StreetDis2").toInt();
		if (_StreeRightImgDis==0)
		{
			_StreeRightImgDis = _StreetImgDis;
		}
		_PanoImgDis = settings->value("PanoDis").toInt();
		settings->endGroup();
		delete settings;
	}

	void hn2DProject::initGpsInfos()
	{ 
		QStringList datas;
		QString gps2MileFilePath = m_strProjectPath + "\\HighGps2Mile.txt";
		bool hasHighGpsInfo =false;
		if (QFile::exists(gps2MileFilePath))
		{
			datas = MyCommonMethods::ReadAllLines(gps2MileFilePath);
			if (datas.size()>0)
			{
				hasHighGpsInfo = true;
			}
			else
			{
			     
			}
		}
		if (!hasHighGpsInfo)
		{
			gps2MileFilePath = m_strProjectPath + "\\GPS2Mile.txt";
			datas = MyCommonMethods::ReadAllLines(gps2MileFilePath);
		}


	
		QVector<_EXCELGPS_> gpsInfos;
		for (int i = 0; i < datas.size(); ++i)
		{
			gpsInfos.push_back(_EXCELGPS_(datas[i]));
		}

		_GpsInfos = gpsInfos;

		
	}

	_EXCELGPS_ hn2DProject::findCloseGpsInfo(double targetMile,int x  ,int y )
	{
		 
		_EXCELGPS_ gps; 
		int line = _Direction; 
		if (_GpsInfos.isEmpty())
		{
			initGpsInfos();
			if (_GpsInfos.isEmpty())
			{
				return gps; 
			} 
		}
		//查找第一个大于或者等于目标桩号的位置
		auto it = std::lower_bound(_GpsInfos.begin(), _GpsInfos.end(), targetMile, [line](const _EXCELGPS_&gps, double mile) {

			if (line == 1)
			{
				return gps._mile < mile;
			}
			else
			{
				return gps._mile > mile;
			}

		});

		//处理边界清空
		if (it == _GpsInfos.begin())
		{
			return (*it);
		}
		if (it == _GpsInfos.end())
		{
			return (*(it - 1));
		}

		//比较it和it-1 找到最接近的点
		const _EXCELGPS_& nextGps = *it;
		const _EXCELGPS_ &prevGps = *(it - 1);

		if (std::abs(nextGps._mile - targetMile) < std::abs(prevGps._mile - targetMile))
		{
			return nextGps;
		}
		else
		{
			return prevGps;
		} 
	}

  

	_EXCELGPS_ hn2DProject::findCloseGpsInfoFromDmi(double targetDmi, int x, int y)
	{
		_EXCELGPS_ gps;
	 
		if (_GpsInfos.isEmpty())
		{
			initGpsInfos();
			if (_GpsInfos.isEmpty())
			{
				return gps;
			}
		}
		//查找第一个大于或者等于目标桩号的位置
		auto it = std::lower_bound(_GpsInfos.begin(), _GpsInfos.end(), targetDmi, [ ](const _EXCELGPS_&gps, double dmi) {
			return gps._dmi < dmi;
	 
		});

		//处理边界清空
		if (it == _GpsInfos.begin())
		{
			return (*it);
		}
		if (it == _GpsInfos.end())
		{
			return (*(it - 1));
		}

		//比较it和it-1 找到最接近的点
		const _EXCELGPS_& nextGps = *it;
		const _EXCELGPS_ &prevGps = *(it - 1);

		if (std::abs(nextGps._dmi - targetDmi) < std::abs(prevGps._dmi - targetDmi))
		{
			return nextGps;
		}
		else
		{
			return prevGps;
		}
	}

	hnCommon::ROAD_MARK_TYPE hn2DProject::getMarkType(const QString& strType)
	{
		if (strType.contains(QStringLiteral("材质")))
		{
			return ROAD_MARK_TYPE::ROAD_SURFACE;
		}
		else	if (strType.contains(QStringLiteral("单元")))
		{
			return ROAD_MARK_TYPE::ROAD_UNIT;
		}
		else if (strType.contains(QStringLiteral("等级")))
		{
			return ROAD_MARK_TYPE::ROAD_GRAD;
		}
		else if (strType.contains(QStringLiteral("标准")))
		{
			return ROAD_MARK_TYPE::ROAD_STANDARD;
		}
		else if (strType.contains(QStringLiteral("情况")))
		{
			return ROAD_MARK_TYPE::ROAD_SOMETHING;
		}
		else
		{
			return ROAD_MARK_TYPE::None;
		}
	}

	 QVector<_EXCELGPS_> hn2DProject::getGpsInfoList()
	{
		 return _GpsInfos;

	}

	 double hn2DProject::caculateStreetLength(QPoint p1, QPoint p2)
	 {
		 if (u_jgDatas.size()<=0 && v_jgDatas.size()<=0)
		 {
			 //读取文件 
			 QString u_jgPath = m_strProjectPath + "\\u_jg.bin";
			 QString v_jgPath = m_strProjectPath + "\\v_jg.bin";

			 //读取第一张景观图像 宽度，高度
			   if (m_vecRoadLeftStreetPicMilePath.size()<=0)
			   {
				   return 0.0;
			   }
			   else
			   {
				   QImageReader reader(m_vecRoadLeftStreetPicMilePath.first());
				   QSize size = reader.size();

				   int binWidth = size.width() + 120;
				   int binHeight = size.height() +120;
				   bool ok1 =    readBinToFloatArray(u_jgPath, binWidth, binHeight, u_jgDatas);
				   bool ok2= readBinToFloatArray(v_jgPath, binWidth, binHeight, v_jgDatas); 
			   }
		 }
		 else
		 {
			 float x1 = u_jgDatas[p1.y() + 60][p1.x() + 60];
			 float y1 = v_jgDatas[p1.y() + 60][p1.x() + 60];

			 float x2 = u_jgDatas[p2.y() + 60][p2.x() + 60];
			 float y2 = v_jgDatas[p2.y() + 60][p2.x() + 60];

			 double dis_x = std::abs(x1 - x2) * 0.005313751668892;
			 double dis_y = std::abs(y1 - y2)*0.0176470588235294;
			 if (x1==0 || y1==0 || x2==0 ||y2 == 0)
			 {
				 return -1;
			 }
			 double length = std::sqrt(dis_x * dis_x + dis_y * dis_y);
			 return length;
		 }
		 return 0.0;
	 }

	 void hn2DProject::checkMirroredFile(const QString & fileName)
	{
		QFile file(fileName);
		if (!file.exists())
		{
			file.open(QIODevice::WriteOnly | QIODevice::Text);
			QTextStream stream(&file);
			stream << "[CONFIG]" << endl;
			stream << "VMirrored=false" << endl;
			stream << "HMirrored=false" << endl;
			file.close();
		}	
	}

	 bool hn2DProject::readBinToFloatArray(const QString& filePath, int width, int height,
		 QVector<QVector<float>>&data)
	 {
		 QFile file(filePath);

		 if (!file.open(QIODevice::ReadOnly))
		 {
			 return false;
		 }
		 qint64 size =   file.size();
		 qint64 expectedSize = static_cast<qint64>(width) * height * sizeof(double);
		 if (file.size() <expectedSize)
		 {
			 return false;
		 } 
		 data = QVector<QVector<float>>(height, QVector<float>(width, 0.0f)); 
		 QVector<double> rowBuffer(width);
		 for (int y = 0 ; y<height ; ++y)
		 {
			 qint64 rowBytes = static_cast<qint64>(width) * sizeof(double); 
			 qint64 readBytes = file.read(reinterpret_cast<char*>(rowBuffer.data()), rowBytes);
			 if (readBytes != rowBytes)
			 {
				 return false;
			 }
			
			 for (int x = 0 ; x<width; ++x)
			 {
				 data[y][x] = static_cast<float>(rowBuffer[x]);
				 if (data[y][x]!=0)
				 {
					 int ddd = 0; 
				 }
				  
			 }
		 }
		 return true;
	 }

}