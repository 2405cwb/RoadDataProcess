#include "hn3DProject.h"
#include <QDir>
#include "hnFile.h"
#include <QSettings>

namespace hnPro
{
	hn3DProject::hn3DProject():m_pPcdReader(NULL), m_strProjectPath(""), m_strXDCloudPath(""), m_strGreyImage(""), m_strDepthImage("")
	{
		m_configFileName = "3dProjectConfig.ini";
	}


	hn3DProject::~hn3DProject()
	{
		// 清除工程
		closeProject();
	}

	// 初始化3D工程
	bool hn3DProject::init(const QString& strProjectPath, const QString& strProjectName)
	{
		// 相对点云路径
		QString now3dProjectRootPath = strProjectPath +"/"+ strProjectName;
		m_strXDCloudPath = now3dProjectRootPath + "/PointCloud/1/Mms-Cam-1.cam";
		QDir dir;
		if (!dir.exists(m_strXDCloudPath))
		{
			m_strXDCloudPath = now3dProjectRootPath + "/PointCloud/1/iScan-Cam-1.cam";
			if (!dir.exists(m_strXDCloudPath))
			{
				qDebug() << QStringLiteral("三维项目初始化失败:") << m_strXDCloudPath << QStringLiteral("缺失");
				return false;
			}
		}

		//相机参数
		m_strImageParam = now3dProjectRootPath + QString::fromLocal8Bit("/Image/Pavement-cam-1.idx");

		// 点云灰度图路径
		m_strGreyImage = now3dProjectRootPath + QString::fromLocal8Bit("/Image/灰度图");

		// 点云深度图路径
		m_strDepthImage = now3dProjectRootPath + QString::fromLocal8Bit("/Image/深度图");

		// 当前工程
		m_strProjectPath = now3dProjectRootPath;

		return true;
	}

	// 设置当前工程
	bool hn3DProject::setCurProject()
	{
		// 点云数据
		string strPcdPath = m_strXDCloudPath.toLocal8Bit();
		if (!m_pPcdReader)
		{
			m_pPcdReader = new hnPavementCamReader();
			if (!m_pPcdReader->Open(strPcdPath.c_str()))
			{
				delete m_pPcdReader;
				m_pPcdReader = NULL;
				return false;
			}
		}

		// 获取绝对点云路径
		hnFile pFile;
		m_vecJDCloudPath.clear();
		m_vecJDCloudName.clear();

		//QString strPcdFolder = m_strProjectPath + "/PointCloud/1";
		QString strPcdFolder = m_strProjectPath + "/PointCloud/1";
		pFile.getAllFolderFile(strPcdFolder, "hlz", m_vecJDCloudPath);
		
		for (int i = 0; i < m_vecJDCloudPath.size(); i++)
		{
			strPcdFolder = m_vecJDCloudPath[i];
			strPcdFolder = strPcdFolder.right(strPcdFolder.count() - strPcdFolder.lastIndexOf("/") - 1);
			strPcdFolder = strPcdFolder.left(strPcdFolder.lastIndexOf("."));
			m_vecJDCloudName.push_back(strPcdFolder);
		}

		if (m_vecImageInfo.size() > 0)
		{
			return true;
		}
		m_vecImageInfoMap.clear();
		// 查找所有影像
		string strImagePath = m_strImageParam.toLocal8Bit();
		FILE* pIndexFile = fopen(strImagePath.c_str(), "rt");
		if (!pIndexFile)
		{
			return false;
		}

		bool bSucc = false;
		char strLine[1024];
		int nIndexCount = 0;
		m_vecImageInfo.resize(5000);
		while (!feof(pIndexFile))
		{
			memset(strLine, 0, 1024);
			fgets(strLine, 1024, pIndexFile);

			PAVEMENT_IMAGE_INDEX info;
			bSucc = info.serialize(strLine);
			if (!bSucc)
			{
				continue;
			}

			m_vecImageInfo[nIndexCount] = info;
			nIndexCount++;
			QString picName = QString::fromLocal8Bit(info.strImgName);
			m_vecImageInfoMap[picName] = info.dStartDmi;
			if (nIndexCount >= m_vecImageInfo.size())
			{
				m_vecImageInfo.resize(m_vecImageInfo.size() + 5000);
			}
		}

		fclose(pIndexFile);
		m_vecImageInfo.resize(nIndexCount);
        
		return true;
	}

	// 清空工程
	void hn3DProject::closeProject()
	{
		if (m_pPcdReader)
		{
			m_pPcdReader->Close();
			delete m_pPcdReader;
			m_pPcdReader = NULL;
		}

		m_vecImageInfo.clear();
		m_vecJDCloudPath.clear();
        m_vecJDCloudName.clear();
	}

	// 根据影像名称以及图片ROW坐标获取GPS时间
	double hn3DProject::getGpsTimer(QString strImageName, int nImageRow)
	{
		if (!m_pPcdReader)
		{
			return 0.0;
		}

		QString strOriImage = "";
		int nFrame = 0;
		double dGpsTimer = 0.0;

		for (int i = 0; i < m_vecImageInfo.size(); i++)
		{
			strOriImage = QString::fromLocal8Bit(m_vecImageInfo[i].strImgName);
			if (!strOriImage.contains(strImageName))
			{
				continue;
			}

			nFrame = i*m_vecImageInfo[i].nImgHeight + (m_vecImageInfo[i].nImgHeight - nImageRow);

			m_pPcdReader->getGpsTime(nFrame, dGpsTimer);
			break;
		}

		return dGpsTimer;
	}

	// 根据影像名称以及图片ROW坐标获取点云帧号
	int hn3DProject::getPcdFrame(QString strImageName, int nImageRow)
	{
		QString strOriImage = "";
		int nFrame = 0;
		double dGpsTimer = 0.0;

		for (int i = 0; i < m_vecImageInfo.size(); i++)
		{
			strOriImage = QString::fromLocal8Bit(m_vecImageInfo[i].strImgName);
			if (!strOriImage.contains(strImageName))
			{
				continue;
			}

			nFrame = i*m_vecImageInfo[i].nImgHeight+ (m_vecImageInfo[i].nImgHeight - nImageRow);
            
			return nFrame;
		}

		return 0;
	}

	// 根据影像名称以及图片像素坐标获取三维坐标
	bool hn3DProject::get3DCoord(QString strImageName, hn2dPointI pixelCoord, hn3dPointD& pt3D)
	{
		if (m_vecImageInfo.size() <= 0)
		{
			return false;
		}

		QString strOriImage = "";
		int nFrame = -1;

		for (int i = 0; i < m_vecImageInfo.size(); i++)
		{
			strOriImage = QString::fromLocal8Bit(m_vecImageInfo[i].strImgName);
			if (!strOriImage.contains(strImageName))
			{
				continue;
			}

			nFrame = i;
			break;
		}

		if (nFrame == -1)
		{
			return false;
		}

		// 获取当前帧索引信息;
		PAVEMENT_IMAGE_INDEX& indexInfo = m_vecImageInfo[nFrame];

#if 1  // 原代码
		// 图像像素坐标转换三维坐标尺度因素;
		double widthDist = sqrt((indexInfo.downRightPt.x - indexInfo.upRightPt.x) * (indexInfo.downRightPt.x - indexInfo.upRightPt.x) + (indexInfo.downRightPt.y - indexInfo.upRightPt.y) * (indexInfo.downRightPt.y - indexInfo.upRightPt.y));
		double heightDist = sqrt((indexInfo.upLeftPt.x - indexInfo.upRightPt.x) * (indexInfo.upLeftPt.x - indexInfo.upRightPt.x) + (indexInfo.upLeftPt.y - indexInfo.upRightPt.y) * (indexInfo.upLeftPt.y - indexInfo.upRightPt.y));
		double updateWidthDist = widthDist * pixelCoord.x / indexInfo.nImgWidth;
		double updateHeightDist = heightDist * pixelCoord.y / indexInfo.nImgHeight;


		//
		double dx = indexInfo.downRightPt.x - indexInfo.upRightPt.x;
		double dy = indexInfo.downRightPt.y - indexInfo.upRightPt.y;

		// 
		double angleK = atan2(indexInfo.upLeftPt.y - indexInfo.upRightPt.y, indexInfo.upLeftPt.x - indexInfo.upRightPt.x);

		// 为垂线弧度;
		double angleKInvert = atan2(dy, dx);

		pt3D.x = indexInfo.upRightPt.x + updateWidthDist * cos(angleKInvert) + updateHeightDist * cos(angleK);
		pt3D.y = indexInfo.upRightPt.y + updateWidthDist * sin(angleKInvert) + updateHeightDist * sin(angleK);


#endif
	
#if 0 // 代码修改
		// 图像像素坐标转换三维坐标尺度因素;
		double widthDist = sqrt((indexInfo.upLeftPt.x - indexInfo.upRightPt.x) * (indexInfo.upLeftPt.x - indexInfo.upRightPt.x) + (indexInfo.upLeftPt.y - indexInfo.upRightPt.y) * (indexInfo.upLeftPt.y - indexInfo.upRightPt.y));
		double heightDist = sqrt((indexInfo.upLeftPt.x - indexInfo.downLeftPt.x) * (indexInfo.upLeftPt.x - indexInfo.downLeftPt.x) + (indexInfo.upLeftPt.y - indexInfo.downLeftPt.y) * (indexInfo.upLeftPt.y - indexInfo.downLeftPt.y));
		double updateWidthDist = widthDist * pixelCoord.x / indexInfo.nImgWidth;
		double updateHeightDist = heightDist * pixelCoord.y / indexInfo.nImgHeight;

		// 根据垂线斜率计算角度;
		double dx = indexInfo.upRightPt.x - indexInfo.upLeftPt.x;
		double dy = indexInfo.upRightPt.y - indexInfo.upLeftPt.y;

		// 为列向斜率,其行向斜率应垂直于此;
		double angleK = atan2(indexInfo.downRightPt.y - indexInfo.upRightPt.y, indexInfo.downRightPt.x - indexInfo.upRightPt.x);

		// 为行向斜率;
		double angleKInvert = atan2(dy, dx);

		// 坐标原点在左上角处;
		//double tempX = updateWidthDist * cos(angleKInvert) + updateHeightDist * cos(angleK);
		//double tempY = updateWidthDist * sin(angleKInvert) + updateHeightDist * sin(angleK);
		 
		// 原始
		//pt3D.x = indexInfo.upLeftPt.x + updateWidthDist * cos(angleKInvert) + updateHeightDist * cos(angleK);
		//pt3D.y = indexInfo.upLeftPt.y + updateWidthDist * sin(angleKInvert) + updateHeightDist * sin(angleK);

		// 下面为测试部分
		pt3D.x = indexInfo.upLeftPt.x + updateWidthDist * cos(angleKInvert) + updateHeightDist * sin(angleK);
		pt3D.y = indexInfo.upLeftPt.y + updateWidthDist * sin(angleKInvert) - updateHeightDist * cos(angleK);

		//pt3D.y = indexInfo.upLeftPt.y + updateWidthDist * cos(angleKInvert) + updateHeightDist * sin(angleK);
		//pt3D.x = indexInfo.upLeftPt.x + updateWidthDist * sin(angleKInvert) - updateHeightDist * cos(angleK);

		/*	if (angleKInvert  >=  0  )
		{
		if (angleK >= 0)
		{
		pt3D.x = indexInfo.upRightPt.x + updateWidthDist * cos(angleKInvert) + updateHeightDist * cos(angleK);
		pt3D.y = indexInfo.upRightPt.y + updateWidthDist * sin(angleKInvert) + updateHeightDist * sin(angleK);
		}
		else
		{
		pt3D.x = indexInfo.upRightPt.x + updateWidthDist * cos(angleKInvert) - updateHeightDist * cos(angleK);
		pt3D.y = indexInfo.upRightPt.y + updateWidthDist * sin(angleKInvert) + updateHeightDist * sin(angleK);
		}

		}
		else
		{
		if (angleK >= 0)
		{
		pt3D.x = indexInfo.upRightPt.x - updateWidthDist * cos(angleKInvert) + updateHeightDist * cos(angleK);
		pt3D.y = indexInfo.upRightPt.y + updateWidthDist * sin(angleKInvert) + updateHeightDist * sin(angleK);
		}
		else
		{
		pt3D.x = indexInfo.upRightPt.x - updateWidthDist * cos(angleKInvert) - updateHeightDist * cos(angleK);
		pt3D.y = indexInfo.upRightPt.y + updateWidthDist * sin(angleKInvert) + updateHeightDist * sin(angleK);
		}
		}*/


#endif
		
		return true;
	}

	// 根据影像名称和像素坐标获取相对坐标
	bool hn3DProject::get2DCoord(QString strImageName, hn2dPointI pixelCoord, hn3dPointD& pt3D)
	{
		QString strOriImage = "";
		int nImageIndex = -1;

		for (int i = 0; i < m_vecImageInfo.size(); i++)
		{
			strOriImage = QString::fromLocal8Bit(m_vecImageInfo[i].strImgName);
			if (!strOriImage.contains(strImageName))
			{
				continue;
			}

			nImageIndex = i;
			break;
		}

		if (nImageIndex == -1)
		{
			return false;
		}

		// 获取点相对二维坐标
		if (!m_pPcdReader->getFramePoints(nImageIndex, pixelCoord.x, pixelCoord.y, pt3D))
		{
			return false;
		}

		return true;
	}

	// 根据GPS获取图像
	QString hn3DProject::getImageByGps(double dGpsTimer)
	{
		QString strName = "";

		if (m_vecImageInfo.size() <= 0)
		{
			return strName;
		}

		for (int i = 0; i < m_vecImageInfo.size(); i++)
		{
			if (dGpsTimer < m_vecImageInfo[i].dStartTime || dGpsTimer > m_vecImageInfo[i].dEndTime)
			{
				continue;
			}

			strName = QString::fromLocal8Bit(m_vecImageInfo[i].strImgName);
			break;
		}

		return strName;
	}

	// 根据里程获取图像
	QString hn3DProject::getImageByMile(double dMile)
	{
		QString strName = "";

		for (int i = 0; i < m_vecImageInfo.size(); i++)
		{
			if (dMile < m_vecImageInfo[i].dStartDmi || dMile > m_vecImageInfo[i].dEndDmi)
			{
				continue;
			}

			strName = QString::fromLocal8Bit(m_vecImageInfo[i].strImgName);
			break;
		}

		return strName;
	}

	// 根据帧号获取图像
	QString hn3DProject::getImageByIndex(int nIndex)
	{
		QString strName = "";
		if (nIndex < 0 || nIndex >= m_vecImageInfo.size())
		{
			return strName;
		}

		strName = QString::fromLocal8Bit(m_vecImageInfo[nIndex].strImgName);

		return strName;
	}

	// 获取所有图像路径
	QVector<QString> hn3DProject::getAllImage()
	{
		QVector<QString> vecAllImage;

		for (int i = 0; i < m_vecImageInfo.size(); i++)
		{
			vecAllImage.push_back(QString::fromLocal8Bit(m_vecImageInfo[i].strImgName));
		}

		return vecAllImage;
	}

	// 根据影像名称获取里程
	double hn3DProject::getMileByImage(const QString& strImageName)
	{
		QString imageName = strImageName;
		imageName.replace("GREY", "");
		imageName.replace("RGB", "");

		if (m_vecImageInfo.size() <= 0)
		{
			return -1.0;
		}

	 
		return m_vecImageInfoMap[imageName];
		/*for (int i = 0; i < m_vecImageInfo.size(); i++)
		{
			strValue = QString::fromLocal8Bit(m_vecImageInfo[i].strImgName);
			if (strValue != imageName)
			{
				continue;
			}

			return m_vecImageInfo[i].dStartDmi;
		}*/

	
	}
	int hn3DProject::getImagePixelWidth()
	{
		return 2560;
	}
	int hn3DProject::getImagePixelHeight()
	{
		return 4000;
	}
	double hn3DProject::getImageWidthScale()
	{
		//return 0.0015;
		//return 0.00150390625;
		return getRoadWidth() / getImagePixelWidth();
	}
	double hn3DProject::getImageHeightScale()
	{
		return 0.002;
	}
	double hn3DProject::getRoadWidth()
	{
		QString configFileName = m_strProjectPath + "/" + m_configFileName;
		QSettings settings(configFileName, QSettings::IniFormat);
		settings.beginGroup("CONFIG");
		//三维默认道路宽度3.85米
		double roadWidth =  settings.value("roadWidth", 3.85).toDouble();
		settings.endGroup();

		return roadWidth;
	}

	void hn3DProject::setRoadWidth(double width)
	{
		QString configFileName = m_strProjectPath + "/" + m_configFileName;
		QSettings settings(configFileName, QSettings::IniFormat);
		settings.beginGroup("CONFIG");
		//三维默认道路宽度3.85米
		settings.setValue("roadWidth", width);
		settings.endGroup();

		return;
	}

	bool hn3DProject::getIsHMirrored()
	{
		QString fileName = m_strProjectPath + "/mirroredSetting.ini";
		this->checkMirroredFile(fileName);

		QSettings settings(fileName, QSettings::IniFormat);
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

	void hn3DProject::setIsHMirrored(bool mirrored)
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

	bool hn3DProject::getIsVMirrored()
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

	void hn3DProject::setIsVMirrored(bool mirrored)
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

	void hn3DProject::checkMirroredFile(const QString & fileName)
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
}


