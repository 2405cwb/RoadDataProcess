#include "hnImportAidcDiseases.h"
#include <QTime>
#include <QDebug>
#include <QVector>
#include "mergeAidcDiseases.h"

#include "../hnQtCommon/MyCommonMethods.h"
#include "../hnApplication/hnDiseaseService.h"
hnImportAidcDiseases::hnImportAidcDiseases(const int frameType, QWidget *parent)
	: QObject(parent)
{
	m_frameType = frameType;
	m_isDiseaseMap = false;
	m_parent = parent;
	m_isMerge = false;
}

hnImportAidcDiseases::hnImportAidcDiseases(const int frameType, bool isMerge, bool isMap, QWidget * parent)
{
	m_frameType = frameType;
	m_isDiseaseMap = false;
	m_parent = parent;
	m_isMerge = isMerge;
	m_isDiseaseMap = isMap;
}

hnImportAidcDiseases::~hnImportAidcDiseases()
{

}

void hnImportAidcDiseases::import()
{
#define DEBUG_DISEASE_MERGE 0		// 手动绘制病害进行合并的测试流程

#if !DEBUG_DISEASE_MERGE
	//加载数据库里面的病害
	if (!this->loadDb())
	{
		//提示用户完成
		QMessageBox::warning(m_parent, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("自动识别成果不存在，请检查是否已经进行自动识别"),
			QString::fromLocal8Bit("确定"));
		return;
	}
#endif

#if 0
	// 是否映射的选项放到进行选择，默认不映射
	auto rc = QMessageBox::question(m_parent, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("病害映射耗时较长，是否进行病害映射?"),
		QString::fromLocal8Bit("是"), QString::fromLocal8Bit("否"));
	if (0 == rc)
	{
		m_isDiseaseMap = true;
	}
	else
	{
		m_isDiseaseMap = false;
	}
#endif


#if DEBUG_DISEASE_MERGE
	// 从 DisZXLF 中读取病害，进行合并测试
	std::vector<hnRoadDiseaseInfo> diseasesVecData;
	hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurDB()->m_diseaseTable
		.readAllData("DisZXLF", diseasesVecData);

	// 根据桩号对病害进行从小到大排序
	std::sort(diseasesVecData.begin(), diseasesVecData.end(), [](hnRoadDiseaseInfo &a, hnRoadDiseaseInfo& b) {
		return a.dDmi < b.dDmi;
	});


	QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> diseases;
	diseases.insert(QString::fromLocal8Bit(diseasesVecData.at(0).strDiseaseTableName), diseasesVecData);

#else
	//	//将自动识别的病害转换成本程序的病害类型
	QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> diseases = this->transformDiseases();
	if (diseases.isEmpty())
	{
		return;
	}
#endif

	//TODO 考虑设计弹出窗口
#if 1		
	const double roadWidth = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRoadLength;
	const int pixHeight = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelY;
	bool isVMirror = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getIsVMirrored();
	// 是否合并病害
	if (this->m_isMerge)
	{
		//合并纵向裂缝、修补病害。
		mergeAidcDiseases merge(roadWidth, pixHeight, isVMirror, this->m_isDiseaseMap, m_parent);
		// 注意：自动识别的病害坐标是没有经过翻转的，所以要考虑翻转情况进行拼接
		// 如果是第二张图拼接到第一张顶部，则是第二张最底部和第一张顶部判断距离，进行拼接
		diseases = merge.mergeDiseases(diseases);
	}

	// 修改自动化模式数量，进行人工模式病害映射
	for (auto &iter = diseases.begin(); iter != diseases.end(); iter++)
	{
		auto &oneTypeDisease = iter.value();
		for (hnCommon::hnRoadDiseaseInfo &newDisease : oneTypeDisease)
		{
			newDisease.nRectCnt = newDisease.vec2dRect.size();
			if (this->m_isDiseaseMap&&hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
			{
				// 修改人工模式框数量
				if (newDisease.nDrawType == 0)
				{
					//// todo 人工模式病害2d映射到3d
					//// 2d病害映射到3D图像上
					//hn2dRectI rectI = newDisease.vec2dRect[0];
					//QRect rect2D = QRect(QPoint(rectI.p0.x, rectI.p0.y), QPoint(rectI.p2.x, rectI.p2.y));
					////QRect rect2D = QRect(QPoint(rectI.p0.x, pixHeight - rectI.p0.y), QPoint(rectI.p2.x, pixHeight - rectI.p2.y));
					//std::vector<hn3dRectI> rect3D = this->generateLargeFrameHn3dRectVector(rect2D, newDisease.dDmiStart);
					//newDisease.vec3dRect.insert(newDisease.vec3dRect.end(), rect3D.begin(), rect3D.end());
					newDisease.n3dCnt = newDisease.vec3dRect.size();
				}
				else
				{
					// 修改自动化模式数量
					QVector<QRect> rect2D;
					rect2D.clear();
					for (hn2dRectI rectI : newDisease.vec2dRect)
					{
						rect2D.push_back(QRect(QPoint(rectI.p0.x, rectI.p0.y), QPoint(rectI.p2.x, rectI.p2.y)));
					}
					// 对合并后的病害进行2D自动化模式映射为3D框
					//newDisease.vec3dRect = createLittleFrame3dRectIVector(newDisease.vec2dRect);
					newDisease.n3dCnt = newDisease.vec3dRect.size();
				}

			}
			else
			{
				newDisease.vec3dRect.clear();
				newDisease.n3dCnt = 0;
			}
		}
	}


#endif

#if DEBUG_DISEASE_MERGE   // 只用于测试
	// 替换一下，将数据写入其他表格
	QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> Dmove;
	Dmove.insert(QString::fromLocal8Bit("DisYB"), diseases.value("DisZXLF"));
	this->writeDb(Dmove);
#else
	// 添加自动识别的病害应先删除原本病害，这样才不会出现 ID冲突问题
	//批量写入数据库
	this->writeDb(diseases);
#endif

	//提示用户完成
	QMessageBox::information(m_parent, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("导入数据库完成"));

}

void hnImportAidcDiseases::import2dDisease()
{
#define DEBUG_DISEASE_MERGE 0		// 手动绘制病害进行合并的测试流程

#if !DEBUG_DISEASE_MERGE
	//加载数据库里面的病害
	if (!this->load2dDb())
	{
		//提示用户完成
		QMessageBox::warning(m_parent, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("文本结果不存在，请检查。"),
			QString::fromLocal8Bit("确定"));
		return;
	}
#endif

#if 0
	// 是否映射的选项放到进行选择，默认不映射
	auto rc = QMessageBox::question(m_parent, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("病害映射耗时较长，是否进行病害映射?"),
		QString::fromLocal8Bit("是"), QString::fromLocal8Bit("否"));
	if (0 == rc)
	{
		m_isDiseaseMap = true;
	}
	else
	{
		m_isDiseaseMap = false;
	}
#endif


#if DEBUG_DISEASE_MERGE
	// 从 DisZXLF 中读取病害，进行合并测试
	std::vector<hnRoadDiseaseInfo> diseasesVecData;
	hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurDB()->m_diseaseTable
		.readAllData("DisZXLF", diseasesVecData);

	// 根据桩号对病害进行从小到大排序
	std::sort(diseasesVecData.begin(), diseasesVecData.end(), [](hnRoadDiseaseInfo &a, hnRoadDiseaseInfo& b) {
		return a.dDmi < b.dDmi;
	});


	QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> diseases;
	diseases.insert(QString::fromLocal8Bit(diseasesVecData.at(0).strDiseaseTableName), diseasesVecData);

#else
	//	//将自动识别的病害转换成本程序的病害类型
	QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> diseases = this->transformDiseases();
	if (diseases.isEmpty())
	{
		return;
	}
#endif

	//TODO 考虑设计弹出窗口
#if 1		
	const double roadWidth = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRoadLength;
	const int pixHeight = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelY;
	bool isVMirror = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getIsVMirrored();
	// 是否合并病害
	if (this->m_isMerge)
	{
		//合并纵向裂缝、修补病害。
		mergeAidcDiseases merge(roadWidth, pixHeight, isVMirror, this->m_isDiseaseMap, m_parent);
		// 注意：自动识别的病害坐标是没有经过翻转的，所以要考虑翻转情况进行拼接
		// 如果是第二张图拼接到第一张顶部，则是第二张最底部和第一张顶部判断距离，进行拼接
		diseases = merge.mergeDiseases(diseases);
	}

	// 修改自动化模式数量，进行人工模式病害映射
	for (auto &iter = diseases.begin(); iter != diseases.end(); iter++)
	{
		auto &oneTypeDisease = iter.value();
		for (hnCommon::hnRoadDiseaseInfo &newDisease : oneTypeDisease)
		{
			newDisease.nRectCnt = newDisease.vec2dRect.size();
			if (this->m_isDiseaseMap&&hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
			{
				// 修改人工模式框数量
				if (newDisease.nDrawType == 0)
				{
					//// todo 人工模式病害2d映射到3d
					//// 2d病害映射到3D图像上
					//hn2dRectI rectI = newDisease.vec2dRect[0];
					//QRect rect2D = QRect(QPoint(rectI.p0.x, rectI.p0.y), QPoint(rectI.p2.x, rectI.p2.y));
					////QRect rect2D = QRect(QPoint(rectI.p0.x, pixHeight - rectI.p0.y), QPoint(rectI.p2.x, pixHeight - rectI.p2.y));
					//std::vector<hn3dRectI> rect3D = this->generateLargeFrameHn3dRectVector(rect2D, newDisease.dDmiStart);
					//newDisease.vec3dRect.insert(newDisease.vec3dRect.end(), rect3D.begin(), rect3D.end());
					newDisease.n3dCnt = newDisease.vec3dRect.size();
				}
				else
				{
					// 修改自动化模式数量
					QVector<QRect> rect2D;
					rect2D.clear();
					for (hn2dRectI rectI : newDisease.vec2dRect)
					{
						rect2D.push_back(QRect(QPoint(rectI.p0.x, rectI.p0.y), QPoint(rectI.p2.x, rectI.p2.y)));
					}
					// 对合并后的病害进行2D自动化模式映射为3D框
					//newDisease.vec3dRect = createLittleFrame3dRectIVector(newDisease.vec2dRect);
					newDisease.n3dCnt = newDisease.vec3dRect.size();
				}

			}
			else
			{
				newDisease.vec3dRect.clear();
				newDisease.n3dCnt = 0;
			}
		}
	}


#endif

#if DEBUG_DISEASE_MERGE   // 只用于测试
	// 替换一下，将数据写入其他表格
	QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> Dmove;
	Dmove.insert(QString::fromLocal8Bit("DisYB"), diseases.value("DisZXLF"));
	this->writeDb(Dmove);
#else
	// 添加自动识别的病害应先删除原本病害，这样才不会出现 ID冲突问题
	//批量写入数据库
	this->writeDb(diseases);
#endif

	//提示用户完成
	QMessageBox::information(m_parent, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("导入数据库完成"));
}

bool hnImportAidcDiseases::loadDb()
{
	//异常处理
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return false;
	}

	//创建数据库连接
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

	QString dbName;
	if (m_frameType == 0)
	{
		dbName = hnDataManager::getDataManager()->getCurrentProject()->get2DProPath() + QString::fromLocal8Bit("/RoadImg/Camera0/自动识别大框.db");
	}
	else
	{
		dbName = hnDataManager::getDataManager()->getCurrentProject()->get2DProPath() + QString::fromLocal8Bit("/RoadImg/Camera0/自动识别小框.db");
	}

	QFile file(dbName);
	if (!file.exists())
	{ 
		return false;
	}

	//设置数据库路径 如果数据库不存在，会自动创建一个
	db.setDatabaseName(dbName);

	//打开数据库
	if (db.open())
	{
		qDebug() << QString::fromLocal8Bit("打开数据库成功");
	}
	else
	{
		qWarning() << QString::fromLocal8Bit("打开数据库失败");
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("打开数据库失败，请检查是否存在自动识别数据库"));
		return false;
	}

	//TODO 修改获取配置文件方式  获取公路等级
	/*QString projectIniName = hnDataManager::getDataManager()->getCurrentProject()->get2DProPath() + "/Setting.ini";

	configService config;
	config.loadCfg(projectIniName);*/
	//QString level = QString::fromLocal8Bit(hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().strRoadLevel);
	QString standard = QString::fromLocal8Bit(hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().strRoadStandard);
	/*if (level.isEmpty())
	{
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),QString::fromLocal8Bit("加载二维工程配置文件失败"));
		return false;
	}*/

	//公路等级转枚举
	HnProjectEnums::StandardParmTypeEnum roadLevelEnum = HnProjectEnums::roadTypeQStringToEnum(standard);

	//获取病害表名与病害类型的对应关系
	m_diseaseNameMap = hnApp::hnDataManager::getDataManager()->getTableNamesDiseaseNamesMap(roadLevelEnum, m_frameType);
	m_diseaseInfoMap = hnApp::hnDataManager::getDataManager()->getTableNamesDiseaseInfoMap(roadLevelEnum, m_frameType);
	//根据公路等级获取数据库的所有表名
	QVector<QString> tableNames = hnDataManager::getDataManager()->getTableNames(roadLevelEnum);

	//清空缓存
	m_allAidcDiseases.clear();

	int testDiseaseCount = 0;

	//遍历数据库表名，把所有自动识别的病害加载到内存中	
	for (const QString tableName : qAsConst(tableNames))
	{
		//某个表的自动识别病害
		QVector<AidcDisease> singleTableDiseases;

		QString statement = QString("select * from %1").arg(tableName);
		QSqlQuery query;
		query.prepare(statement);
		query.exec();

		while (query.next())
		{
			AidcDisease disease;
			disease.roadStandard = query.value(2).toString();
			disease.roadSurfaceType = query.value(3).toInt();
			disease.drawType = query.value(4).toInt();
			disease.level = query.value(5).toInt();
			disease.lenth = query.value(10).toInt();
			disease.width = query.value(11).toInt();
			disease.coord = query.value(14).toString();				// 自动识别标记的是自动化模式的索引
			disease.tableName = query.value(21).toString();
			disease.dmi = query.value(27).toDouble();
			//自动识别数据库是从2开始的
			//路面高度 2米，每隔两米拍一张图片
			const double roadHeight = 2;
			disease.dmi = disease.dmi - roadHeight;
			disease.roadWidth = query.value(28).toDouble();

			singleTableDiseases.push_back(disease);

		}

		if (!singleTableDiseases.isEmpty())
		{
			//向总的病害map中插入单个表的自动识别病害
			m_allAidcDiseases.insert(tableName, singleTableDiseases);
		}
	}

	//关闭数据库
	db.close();

	return true;
}

bool hnImportAidcDiseases::load2dDb()
{
	//异常处理
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return false;
	}
	//清空缓存
	m_allAidcDiseases.clear();

	int testDiseaseCount = 0;
	//获取所有病害文本
	QList<QString> allDiseaseTxtFilePaths;

	QString fileFix;
	if (m_frameType == 0)
	{
		fileFix = ".jpg.txt";
	}
	else
	{
		fileFix = ".jpg_PartClass.txt";
	}

	QString projectPicturePath = hnDataManager::getDataManager()->getCurrentProject()->get2DProPath() + "/RoadImg/Camera0";
	QDir dir(projectPicturePath);
	QVector<QString>filePaths;
	if (!dir.exists())
	{
		qDebug() << "path not exits";
		return false;
	}
	QRegularExpression regex("^Image_\\d+$");
	QFileInfoList dirList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QFileInfo& dirInfo : dirList)
	{
		if (dirInfo.isDir() && regex.match(dirInfo.fileName()).hasMatch())
		{
			QDir subDir(dirInfo.absoluteFilePath());

			QStringList nameFilters;
			nameFilters << "*" + fileFix;
			QFileInfoList fileList = subDir.entryInfoList(nameFilters, QDir::Files);

			for (const QFileInfo& fileInfo : fileList)
			{
				filePaths.append(fileInfo.absoluteFilePath());
			}

		}

	}

	QString standard = QString::fromLocal8Bit(hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().strRoadStandard);

	double roadWidth = hnDataManager::getDataManager()->getCurrentProject()->effectiveRoadWidth();
	//公路等级转枚举
	HnProjectEnums::StandardParmTypeEnum roadLevelEnum = HnProjectEnums::roadTypeQStringToEnum(standard);

	//获取病害表名与病害类型的对应关系
	m_diseaseNameMap = hnApp::hnDataManager::getDataManager()->getTableNamesDiseaseNamesMap(roadLevelEnum, m_frameType);
	m_diseaseInfoMap = hnApp::hnDataManager::getDataManager()->getTableNamesDiseaseInfoMap(roadLevelEnum, m_frameType);

	//根据公路等级获取数据库的所有表名
	QMap<QString, QString> tableNamesAndDiseaseNames = hnDataManager::getDataManager()->getTableNamesDiseaseNamesMap(roadLevelEnum, m_frameType);

	QMapIterator<QString, QString> it(tableNamesAndDiseaseNames);
	int disIndex = 0;

	QVector<QPair<int, QString>> allDisTxts;

	for (disIndex = 0; disIndex < filePaths.size(); ++disIndex)
	{
		QString filePath = filePaths[disIndex];

		QFileInfo curFile(filePath);
		
		QString curDirName = curFile.dir().dirName();
		int dirIndex = curDirName.split("_")[1].toInt();
		
		QString fileName = curFile.fileName();
		int pictureIndex = fileName.split("_").first().toInt();
		int curDmi = dirIndex*2000 + (pictureIndex)*2; 

		QStringList txts = MyCommonMethods::ReadAllLines(filePath, "utf-8");
		for (auto& txt : txts)
		{
			allDisTxts.append(QPair<int,QString>(curDmi, txt));
		}
	}

	while (it.hasNext())
	{
		it.next();
		QString tableName = it.key();
		QString disName = it.value();
		//某个表的自动识别病害
		QVector<AidcDisease> singleTableDiseases;

		//读取文本 分析病害
		for (auto& disSingleInfo : allDisTxts)
		{
			int curDmi = disSingleInfo.first;
			QStringList disSingleInfoSplits = disSingleInfo.second. trimmed().split(' ');

			if (m_frameType == 0)
			{

				if (disSingleInfoSplits.size() < 7)
				{
					continue;
				}
				QString mark = "";
				if (disSingleInfoSplits.size()>7)
				{
					mark = disSingleInfoSplits[7];
				}
				QString curDisName = disSingleInfoSplits[4];
				QStringList disNameSplit = curDisName.split(".");
				QString disNameWithoutWeight("");
				if (disNameSplit.size() > 1)
				{
					if (disNameSplit.last() != QStringLiteral("轻") && disNameSplit.last() != QStringLiteral("中") && disNameSplit.last() != QStringLiteral("重"))
					{
						disNameWithoutWeight = curDisName;
					}
					else
					{
						disNameWithoutWeight = disNameSplit.first();
					}
				}
				else
				{
					disNameWithoutWeight = disNameSplit.first();
				}
				if (disNameWithoutWeight != disName)
				{
					continue;
				}
				int level = 0;
				if (disNameSplit.size() > 1)
				{
					if (disNameSplit[1].contains(QStringLiteral("轻")))
					{
						level = 1;
					}
					else if (disNameSplit[1].contains(QStringLiteral("中")))
					{
						level = 2;
					}
					else if (disNameSplit[1].contains(QStringLiteral("重")))
					{
						level = 3;
					}
					else
					{
						level = 0;
					}

				}
				else
				{
					level = 0;
				}
				AidcDisease disease;
				disease.disMark = mark;
				disease.roadStandard = standard;
				disease.roadSurfaceType = hnDataManager::getDataManager()->getRoadSurfaceFromStr(disSingleInfoSplits.last());

				disease.drawType = m_frameType;

				disease.level = level;
				disease.lenth = disSingleInfoSplits.at(2).toInt();
				disease.width = disSingleInfoSplits.at(3).toInt();
				
				disease.coord = disSingleInfoSplits[0] + "," + disSingleInfoSplits[1];				// 自动识别标记的是自动化模式的索引
				disease.tableName = tableName;
				QString mileStr = disSingleInfoSplits.at(5);
				QStringList mileStrSplit = mileStr.split(":");
				/*double mile = MyCommonMethods::convertStakeToDouble(mileStrSplit.last());
				double dmi = hnDataManager::getDataManager()->getCurrentProject()->trueMileToEncl(mile);*/
			//	disease.mile = dmi;
				disease.dmi = curDmi;
				disease.roadWidth = roadWidth;
				singleTableDiseases.push_back(disease);
			}
			else
			{
				if (disSingleInfoSplits.size() < 4)
				{
					continue;
				}
				QString curDisName = disSingleInfoSplits[0];
				QStringList disNameSplit = curDisName.split(".");
				QString disNameWithoutWeight("");
				if (disNameSplit.size() > 1)
				{
					if (disNameSplit.last() != QStringLiteral("轻") && disNameSplit.last() != QStringLiteral("中") && disNameSplit.last() != QStringLiteral("重"))
					{
						disNameWithoutWeight = curDisName;
					}
					else
					{
						disNameWithoutWeight = disNameSplit.first();
					}
				}
				else
				{
					disNameWithoutWeight = disNameSplit.first();
				}
				if (disNameWithoutWeight != disName)
				{
					continue;
				}
				int level = 0;
				if (disNameSplit.size() > 1)
				{
					if (disNameSplit[1].contains(QStringLiteral("轻")))
					{
						level = 1;
					}
					else if (disNameSplit[1].contains(QStringLiteral("中")))
					{
						level = 2;
					}
					else if (disNameSplit[1].contains(QStringLiteral("重")))
					{
						level = 3;
					}
					else
					{
						level = 0;
					}

				}
				else
				{
					level = 0;
				}
				AidcDisease disease;
				disease.roadStandard = standard;
				disease.roadSurfaceType = hnDataManager::getDataManager()->getRoadSurfaceFromStr(disSingleInfoSplits[2]);

				disease.drawType = m_frameType;

				disease.level = level;
				disease.lenth = 0;
				disease.width = 0;

				disease.coord = disSingleInfoSplits.last();				// 自动识别标记的是自动化模式的索引
				disease.tableName = tableName;
				QString mileStr = disSingleInfoSplits.at(1);
				QStringList mileStrSplit = mileStr.split(":");
				/*double mile = MyCommonMethods::convertStakeToDouble(mileStrSplit.last());
				double dmi = hnDataManager::getDataManager()->getCurrentProject()->trueMileToEncl(mile);
				disease.mile = dmi;*/
				disease.dmi = curDmi; 
				disease.roadWidth = roadWidth;
				singleTableDiseases.push_back(disease);
			}
		}

		if (!singleTableDiseases.isEmpty())
		{
			//向总的病害map中插入单个表的自动识别病害
			m_allAidcDiseases.insert(tableName, singleTableDiseases);
		}
	}
	return true;
}

void hnImportAidcDiseases::writeDb(QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> diseases)
{
	// 进度条
	QProgressDialog progressDialog(m_parent);
	progressDialog.setFixedSize(QSize(300, 50));
	progressDialog.setWindowTitle(QString::fromLocal8Bit("正在将病害写入数据库"));
	progressDialog.setAutoClose(true);
	progressDialog.setCancelButton(nullptr);
	int progressValue = 0;
	progressDialog.setValue(progressValue);
	progressDialog.setModal(true);
	progressDialog.setMaximum(diseases.size());
	progressDialog.show();


	 

	for (auto iter = diseases.begin(); iter != diseases.end(); iter++)
	{
		hnApp::hnDataManager::getDataManager()-> getDiseaseService()->addDataAffairs(iter.key(), true,QVector<hnCommon::hnRoadDiseaseInfo>::fromStdVector( iter.value()));
		
		//hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurretDiseaseVector()

		//更新进度条
		progressValue++;
		progressDialog.setValue(progressValue);
		QApplication::processEvents();
	}
}

QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> hnImportAidcDiseases::transformDiseases()
{
	QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> result;

	if (0 == m_frameType)
	{
		//人工模式转换
		result = this->transformBigFrameDiseases();
	}
	else if (1 == m_frameType)
	{
		//自动化模式转换
		result = this->transformLittleFrameDiseases();
	}
	else
	{
		return result;
	}

	return result;
}

QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> hnImportAidcDiseases::transformBigFrameDiseases()
{
	QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> result;

	//进度条
	QProgressDialog progressDialog(m_parent);
	progressDialog.setFixedSize(QSize(300, 50));
	progressDialog.setWindowTitle(QString::fromLocal8Bit("正在处理人工模式自动识别病害"));
	progressDialog.setAutoClose(true);
	progressDialog.setCancelButton(nullptr);
	int progressValue = 0;
	progressDialog.setValue(progressValue);
	progressDialog.setModal(true);
	progressDialog.setMaximum(m_allAidcDiseases.size());
	progressDialog.show();

	//遍历所有表格，对每个表的自动识别病害进行操作
	for (auto singleTableDiseases = m_allAidcDiseases.begin(); singleTableDiseases != m_allAidcDiseases.end(); singleTableDiseases++)
	{
		std::vector<hnCommon::hnRoadDiseaseInfo> singleTableResultDiseases;

		QString tableName = singleTableDiseases.key();
		int firstID = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurDB()->
			getDiseaseTable()->getMaxID(tableName.toStdString());

		int idCount = firstID;
		//转换每个表中的病害
		for (const AidcDisease &disease : qAsConst(singleTableDiseases.value()))
		{
			hnRoadDiseaseInfo newDisease;

			//计算人工模式病害的中心里程
			newDisease.dMileage = this->caculateBigFrameDiseaseCenterMile(disease);
			//计算人工模式病害的开始里程
			newDisease.dDmiStart = this->caculateBigFrameDiseaseBeginMile(disease);
			//计算人工模式病害的结束里程
			newDisease.dDmiEnd = this->caculateBigFrameDiseaseEndMile(disease);

			//道路等级
			strcpy(newDisease.strRoadStandard, disease.roadStandard.toLocal8Bit().data());

			//绘制方式
			newDisease.nDrawType = disease.drawType;

			//病害等级
			newDisease.nLevel = disease.level;
			//对带.的病害进行了额外处理，把病害等级改成 0-轻
			if (QString::fromLocal8Bit(newDisease.strDisName).contains("."))
			{
				newDisease.nLevel = 0;
			}
			//病害材质
			newDisease.nRSurfaceType = disease.roadSurfaceType;
			//二维坐标
			QRect diseaseRect = this->createBigFrameRect(disease.coord, disease.lenth, disease.width);
			if (diseaseRect.width() == 0)
			{
				continue;
			}
			newDisease.vec2dRect = this->generateLargeFrameHn2dRectVector(diseaseRect, disease.dmi);

			if (m_isDiseaseMap && hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
			{
				hn2dRectI rectI;
				rectI.p0 = hn2dPointWithMileI(diseaseRect.topLeft().x(), diseaseRect.topLeft().y(), disease.dmi, 0);
				rectI.p1 = hn2dPointWithMileI(diseaseRect.topRight().x(), diseaseRect.topRight().y(), disease.dmi, 0);
				rectI.p2 = hn2dPointWithMileI(diseaseRect.bottomRight().x(), diseaseRect.bottomRight().y(), disease.dmi, 0);
				rectI.p3 = hn2dPointWithMileI(diseaseRect.bottomLeft().x(), diseaseRect.bottomLeft().y(), disease.dmi, 0);
				//映射到三维
				newDisease.vec3dRect = this->generateLargeFrameHn3dRectVector(rectI);
			}


			//病害表名
			strcpy(newDisease.strDiseaseTableName, disease.tableName.toLocal8Bit().data());


			//病害名字
			auto iter = m_diseaseInfoMap.find(disease.tableName);
			if (iter != m_diseaseInfoMap.end())
			{
				for (auto& curDisInfo : iter.value())
				{
					if (curDisInfo.nLevel == newDisease.nLevel)
					{

						strcpy(newDisease.strDisName, curDisInfo.strDiseaseTypeName);
						newDisease.diseaseWeight = curDisInfo.fWidget;
						break;
					}
				}
			}
			else
			{
				continue;
			}

			//里程
			newDisease.dDmi = disease.dmi;

			//路面宽度
			newDisease.dRoadWidth = disease.roadWidth;

			//横向比例
			const double widthScale = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioX;
			const double heightScale = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioY;

			//病害宽度
			newDisease.dLength = diseaseRect.height() *   heightScale;

			newDisease.dWidth = diseaseRect.width() * widthScale;



			//面积相关属性计算
			hnApp::hnDataManager::getDataManager()->setDiseaseCalcuteSize(newDisease);

			//id
			newDisease.nID = idCount;
			if (idCount == 148)
			{

			}
			idCount++;

			//把转换后的病害添加到单个表的病害中
			singleTableResultDiseases.push_back(newDisease);

			//刷新界面
			QApplication::processEvents();
		}

		//把每个表转换的结果添加到结果中
		result.insert(tableName, singleTableResultDiseases);

		//更新进度条
		progressValue++;
		progressDialog.setValue(progressValue);
		QApplication::processEvents();
	}

	return result;
}

QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>>  hnImportAidcDiseases::transformLittleFrameDiseases()
{
	//声明转换结果
	QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> result;

	//创建单张图片的自动化模式rect数组
	QVector<QRect> rects = this->createSingleImageLittleFrameRect();

	//计算病害总数
	int diseasesCount = this->caculateDiseasesCount(m_allAidcDiseases);

	//进度条
	QProgressDialog progressDialog(m_parent);
	progressDialog.setFixedSize(QSize(300, 50));
	progressDialog.setWindowTitle(QString::fromLocal8Bit("正在处理自动化模式自动识别病害"));
	progressDialog.setAutoClose(true);
	progressDialog.setCancelButton(nullptr);
	int progressValue = 0;
	progressDialog.setValue(progressValue);
	progressDialog.setModal(true);
	progressDialog.setMaximum(diseasesCount);
	progressDialog.show();

	//遍历所有表格，对每个表的自动识别病害进行操作
	for (auto singleTableDiseases = m_allAidcDiseases.begin(); singleTableDiseases != m_allAidcDiseases.end(); singleTableDiseases++)
	{
		std::vector<hnCommon::hnRoadDiseaseInfo> singleTableResultDiseases;

		QString tableName = singleTableDiseases.key();
		int firstID = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurDB()->
			getDiseaseTable()->getMaxID(tableName.toStdString());

		int idCount = firstID;
		//转换每个表中的病害
		for (const AidcDisease &disease : qAsConst(singleTableDiseases.value()))
		{
			hnRoadDiseaseInfo newDisease;

			//计算自动化模式病害的中心里程
			newDisease.dMileage = this->caculateLittleFrameDiseaseCenterMile(disease, rects);

			//开始里程
			newDisease.dDmiStart = this->caculateLittleFrameDiseaseBeginMile(disease, rects);

			//结束里程
			newDisease.dDmiEnd = this->caculateLittleFrameDiseaseEndMile(disease, rects);

			//道路等级
			strcpy(newDisease.strRoadStandard, disease.roadStandard.toLocal8Bit().data());

			//病害等级
			newDisease.nLevel = disease.level;

			//绘制方式
			newDisease.nDrawType = disease.drawType;

			//病害材质
			newDisease.nRSurfaceType = disease.roadSurfaceType;

			//获取该病害的QRect自动化模式数组 
			QVector<QRect> littleRects = createLittleFrameRects(disease, rects);

			// todo 每个格子的面积固定
			newDisease.dArea = littleRects.size() * 0.1*0.1;
			//二维坐标
			newDisease.vec2dRect = this->createLittleFrame2dRectIVector(littleRects, disease.dmi);

			if (m_isDiseaseMap && hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
			{
				//映射到三维
			//	newDisease.vec3dRect = this->createLittleFrame3dRectIVector(littleRects, disease.dmi);
			}

			//病害表名
			strcpy(newDisease.strDiseaseTableName, disease.tableName.toLocal8Bit().data());



			//病害名字
			auto iter = m_diseaseInfoMap.find(disease.tableName);
			if (iter != m_diseaseInfoMap.end())
			{
				for (auto& curDisInfo : iter.value())
				{
					if (curDisInfo.nLevel == newDisease.nLevel)
					{

						strcpy(newDisease.strDisName, curDisInfo.strDiseaseTypeName);
						newDisease.diseaseWeight = curDisInfo.fWidget;
						break;
					}
				}
			}
			else
			{
				continue;
			}



			//里程
			newDisease.dDmi = disease.dmi;

			//路面宽度
			newDisease.dRoadWidth = disease.roadWidth;

			//id
			newDisease.nID = idCount;
			idCount++;

			//把转换后的病害添加到单个表的病害中
			singleTableResultDiseases.push_back(newDisease);

			//更新进度条
			progressValue++;
			progressDialog.setValue(progressValue);
			QApplication::processEvents();
		}

		//把每个表转换的结果添加到结果中
		result.insert(tableName, singleTableResultDiseases);
	}

	return result;
}
double hnImportAidcDiseases::caculateBigFrameDiseaseCenterMile(const AidcDisease disease)
{
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return 0.0;
	}

	double result;

	//病害左上角的y值
	int y = 0;
	QStringList  list = disease.coord.split(",");
	if (list.size() == 2)
	{
		y = list.at(1).toInt();
	}

	//获取二维的图片高度
	int pixHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelY;

	//获取纵向每像素代表多少米
	double scale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioY;

	//病害的高度
	int diseaseHeight = disease.width;

	//图片底部的里程
	int mile = disease.dmi;

	//计算病害的中心里程
	// todo 这里是否要考虑图像是否翻转的问题？
	//result = mile + (y + 0.5 * diseaseHeight) * scale;		// 图像没有翻转时最上方就是图像对应的里程
	result = mile + (pixHeight - (y + 0.5 * diseaseHeight)) * scale;

	return result;
}

double hnImportAidcDiseases::caculateBigFrameDiseaseBeginMile(const AidcDisease disease)
{
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return 0.0;
	}

	double result;

	//病害左上角的y值
	int y = 0;
	QStringList  list = disease.coord.split(",");
	if (list.size() == 2)
	{
		y = list.at(1).toInt();
	}

	//获取二维的图片高度
	int pixHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelY;

	//获取纵向每像素代表多少米
	double scale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioY;

	//病害的高度
	int diseaseHeight = disease.width;

	//图片底部的里程
	int mile = disease.dmi;

	//计算病害的开始里程
	result = mile + (pixHeight - (y + diseaseHeight)) * scale;

	return result;
}

double hnImportAidcDiseases::caculateBigFrameDiseaseEndMile(const AidcDisease disease)
{
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return 0.0;
	}

	double result;

	//病害左上角的y值
	int y = 0;
	QStringList  list = disease.coord.split(",");
	if (list.size() == 2)
	{
		y = list.at(1).toInt();
	}

	//获取二维的图片高度
	int pixHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelY;

	//获取纵向每像素代表多少米
	double scale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioY;

	//病害的高度
	int diseaseHeight = disease.width;

	//图片底部的里程
	int mile = disease.dmi;

	//计算病害的开始里程
	result = mile + (pixHeight - y) * scale;

	return result;
}

double hnImportAidcDiseases::caculateRectCenterMile(const QRect & rect, const double buttomMile)
{
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return 0.0;
	}

	double result;

	//病害左上角的y值
	int y = 0;
	y = rect.center().y();

	//获取二维的图片高度
	int pixHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelY;

	//获取纵向每像素代表多少米
	double scale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioY;

	//图片底部的里程
	int mile = buttomMile;

	//计算病害的中心里程
	result = mile + (pixHeight - y) * scale;

	return result;
}

double hnImportAidcDiseases::caculateLittleFrameDiseaseCenterMile(const AidcDisease disease, QVector<QRect> rects)
{
	//获取病害自动化模式的角标
	QString coord = disease.coord;

	//这个坐标吧，最后多了一个 - ,要把它去掉
	if (coord.endsWith("-"))
	{
		coord = coord.left(coord.size() - 1);
	}

	QStringList list = coord.split("-");

	if (list.isEmpty())
	{
		return 0.0f;
	}

	int firstIdx = list.first().toInt();
	int lastIdx = list.last().toInt();

	if (rects.size() < firstIdx + 1 || rects.size() < lastIdx + 1)
	{
		return 0.0f;
	}

	QRect firstRect = rects.at(firstIdx);
	QRect lastRect = rects.at(lastIdx);

	double firstMile = this->caculateRectCenterMile(firstRect, disease.dmi);
	double lastMile = this->caculateRectCenterMile(lastRect, disease.dmi);

	double centerMile = (firstMile + lastMile) / 2;

	return centerMile;
}

double hnImportAidcDiseases::caculateLittleFrameDiseaseBeginMile(const AidcDisease disease, QVector<QRect> rects)
{
	//获取病害自动化模式的角标
	QString coord = disease.coord;

	//这个坐标吧，最后多了一个 - ,要把它去掉
	if (coord.endsWith("-"))
	{
		coord = coord.left(coord.size() - 1);
	}

	QStringList list = coord.split("-");

	if (list.isEmpty())
	{
		return 0.0f;
	}

	int firstIdx = list.first().toInt();
	int lastIdx = list.last().toInt();

	if (rects.size() < firstIdx + 1 || rects.size() < lastIdx + 1)
	{
		return 0.0f;
	}

	

	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return 0.0;
	}

	double result;

	//获取二维的图片高度
	int pixHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelY;

	//获取纵向每像素代表多少米
	double scale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioY;

	int width = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRoadWidth;



	 

	//算出每个像素代表多少米
	double widthScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioX;

	//矩形宽度（米） 每个小矩形宽度是10cm 也就是0.1米
	double rectWidth = 0.1;

	//算出自动化模式边长 单位：像素
	int sideLenth = rectWidth / widthScale;

	//图片像素宽度
	int imagePixelWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelX;

	//横向矩形的个数
	int widthRectCount = imagePixelWidth / sideLenth;


	//找到最上端的单元格
	int rowIndex = list[0].toInt()/ widthRectCount;
	 
	for (int i = 0; i < list.size(); ++i)
	{
		if (list[i].toInt()/ widthRectCount >= rowIndex)
		{
			//找到最上面的一行索引
			rowIndex = list[i].toInt()/ widthRectCount;
			firstIdx = list[i].toInt();
		}
	} 
	QRect lastRect = rects.at(firstIdx);
	 
	double lastMile = disease.dmi + scale * (pixHeight - lastRect.bottom());

	double beginMile = lastMile;
	return beginMile;
}

double hnImportAidcDiseases::caculateLittleFrameDiseaseEndMile(const AidcDisease disease, QVector<QRect> rects)
{
	//获取病害自动化模式的角标
	QString coord = disease.coord;

	//这个坐标吧，最后多了一个 - ,要把它去掉
	if (coord.endsWith("-"))
	{
		coord = coord.left(coord.size() - 1);
	}

	QStringList list = coord.split("-");

	if (list.isEmpty())
	{
		return 0.0f;
	}

	int firstIdx = list.first().toInt();
	int lastIdx = list.last().toInt();

	if (rects.size() < firstIdx + 1 || rects.size() < lastIdx + 1)
	{
		return 0.0f;
	}

	 

	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return 0.0;
	}

	//获取二维的图片高度
	int pixHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelY;

	//获取纵向每像素代表多少米
	double scale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioY;

	//算出每个像素代表多少米
	double widthScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioX;

	//矩形宽度（米） 每个小矩形宽度是10cm 也就是0.1米
	double rectWidth = 0.1;

	//算出自动化模式边长 单位：像素
	int sideLenth = rectWidth / widthScale;

	//图片像素宽度
	int imagePixelWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelX;

	//横向矩形的个数
	int widthRectCount = imagePixelWidth / sideLenth;


	//找到最上端的单元格
	int rowIndex = list[0].toInt() / widthRectCount;
	 
	for (int i = 0; i < list.size(); ++i)
	{
		if (list[i].toInt() / widthRectCount <= rowIndex)
		{
			//找到最上面的一行索引
			rowIndex = list[i].toInt() / widthRectCount;
			firstIdx = list[i].toInt();
		}
	}
	QRect firstRect = rects.at(firstIdx);

	double lastMile = disease.dmi + scale * (pixHeight - firstRect.top());

	double beginMile = lastMile;
	return beginMile;





	
}


QRect hnImportAidcDiseases::createBigFrameRect(const QString & coord, int width, int height)
{
	QStringList coordList = coord.split(",");
	if (coordList.size() != 2)
	{
		return QRect(0, 0, 0, 0);
	}
	int x = coordList.at(0).toInt();
	int y = coordList.at(1).toInt();
	QRect result(x, y, width, height);

	return result;
}


vector<hn2dRectI> hnImportAidcDiseases::generateLargeFrameHn2dRectVector(const QRect & rect, const double mile)
{
	hn2dRectI hnRect;

	hnRect.p0 = getHnPoint2dWithMileI(rect.topLeft(), mile);
	hnRect.p1 = getHnPoint2dWithMileI(rect.topRight(), mile);
	hnRect.p2 = getHnPoint2dWithMileI(rect.bottomRight(), mile);
	hnRect.p3 = getHnPoint2dWithMileI(rect.bottomLeft(), mile);

	//人工模式只有一个框
	std::vector<hn2dRectI> vec;
	vec.push_back(hnRect);
	return vec;
}

hnCommon::hn2dPointWithMileI hnImportAidcDiseases::getHnPoint2dWithMileI(const QPoint & point, const double mile)
{
	// 这里使用的是不考虑翻转情况的转换方法，因为自动识别的图像是没有翻转的，而 hn2dPixWidget 的同名函数就是考虑翻转
	hnCommon::hn2dPointWithMileI point2dWithMileI;
	point2dWithMileI.x = point.x();
	point2dWithMileI.y = point.y();

	point2dWithMileI.m_dmi = mile;

	return point2dWithMileI;
}

vector<hn3dRectI> hnImportAidcDiseases::generateLargeFrameHn3dRectVector(const hn2dRectI &rect)
{
	hn3dRectI hnRect;

	hnRect.p0 = this->getHnPoint3dWithMileI(QPoint(rect.p0.x, rect.p0.y), rect.p0.m_dmi);
	hnRect.p1 = this->getHnPoint3dWithMileI(QPoint(rect.p1.x, rect.p1.y), rect.p1.m_dmi);
	hnRect.p2 = this->getHnPoint3dWithMileI(QPoint(rect.p2.x, rect.p2.y), rect.p2.m_dmi);
	hnRect.p3 = this->getHnPoint3dWithMileI(QPoint(rect.p3.x, rect.p3.y), rect.p3.m_dmi);

	std::vector<hn3dRectI> vec;
	//要是某个点里程小于0，就不映射
	if (hnRect.p0.bottomEncoderMile < 0 ||
		hnRect.p1.bottomEncoderMile < 0 ||
		hnRect.p2.bottomEncoderMile < 0 ||
		hnRect.p3.bottomEncoderMile < 0)
	{
		return vec;
	}
	vec.push_back(hnRect);
	return vec;
}


//二维视图点转三维视图点,使用里程进行计算，简化计算步骤
hnCommon::hn3dPointWithMileI hnImportAidcDiseases::getHnPoint3dWithMileI(const QPoint & point, const double mile)
{
	//目标点
	hnCommon::hn3dPointWithMileI dstPoint;
	//异常处理
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return dstPoint;
	}
	//获取二维的图片高度
	int pixHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelY;

	//获取二维的图片高度
	int pixWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelX;

	double yScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioY;

	int mirrorX = point.x();
	int mirrorY = point.y();
	//2d镜像处理 必须先进行镜像处理，再进行里程差的计算
	// 函数传入的 2d 点和里程都是在界面上显示的坐标和里程，所以2d 点不需要翻转

	//算出当前点的编码器里程
	double encoderMile = mile + yScale*(pixHeight - mirrorY);

	//二三维有差值，要对其进行修正
	//获取二三维的编码器里程差值  
	double encoderMileDiff = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2d3dMileDiff();

	//计算出映射到三维的里程，一般都是 2d 开始采集一段之后开始3d 数据采集，所以2d点的里程比它的 3d 对应点里程大
	double newMile = encoderMile - encoderMileDiff;

	double buttomMile;
	int yPixel;
	// 计算2d 点经过里程校正后的点的x、y坐标，主要思路就是根据新的里程，结合图像高度（2米）和y分辨率，取整取余，确定新的里程值和坐标
	hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getPixEncoderMile(newMile, yScale, buttomMile, yPixel);

	//新的2d点
	QPoint new2dBigImagePoint;
	new2dBigImagePoint.setY(yPixel);
	new2dBigImagePoint.setX(mirrorX);

	//单张2d图片内的坐标
	QPoint single2dPoint = new2dBigImagePoint;

	//四分之一3d图片内的坐标
	QPoint quarter3dPoint;
	//四分之一3d图片的宽度
	int quarter3dImageWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelWidth();
	//四分之一3d图片的高度
	int quarter3dImageHeight = 0.25 * hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelHeight();

#if 1
	// 因为 2d 图像和 3d 图像宽度不一致，所以这里将图像进行缩放，计算出对应点
	//赋值四分之一3d图片的x
	hn2d3dCoordinates coordinates;
	quarter3dPoint.setX(coordinates.single2dXToSingle3dX(single2dPoint.x()));
	//quarter3dPoint.setX((single2dPoint.x() * quarter3dImageWidth) * 1.0 / pixWidth);
	dstPoint.x = quarter3dPoint.x();
#else
	int colDiffSize = 0.1;			// 2d 图像最左侧对应到3d 图像上的距离值，其余值坐标通过 每个分辨率进行折算

#endif

	//赋值四分之一3d图片的y
	quarter3dPoint.setY((single2dPoint.y() * quarter3dImageHeight) * 1.0 / pixHeight);

	// todo 这些数字都应该从配置中获取？
	//二维图片底部的里程
	double bottomEncoder2dMile = buttomMile;
	int encoderMileSingle3d = (int)((int)bottomEncoder2dMile) % 8;			// 2d 图像每张 2m ，3d 图像每张 8m，取模8 是算在当前3d图像的位置
	int quarter3dIdx = 3 - (encoderMileSingle3d / 2);						// 将8m 分成4张图，最上面一张编号0，最下面一张编号3
	dstPoint.y = quarter3dIdx * quarter3dImageHeight + quarter3dPoint.y();

	//赋值里程
	double heightScale3d = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageHeightScale();
	dstPoint.bottomEncoderMile = (int)(bottomEncoder2dMile / 8) * 8;

	//3d镜像处理
	int image3dWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelWidth();
	int image3dHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelHeight();
	if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getIsHMirrored())
	{
		dstPoint.x = image3dWidth - dstPoint.x;
	}
	if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getIsVMirrored())
	{
		dstPoint.y = image3dHeight - dstPoint.y;
	}

	return dstPoint;
}


//二维视图点转三维视图点
hnCommon::hn3dPointWithMileI hnImportAidcDiseases::getHnPoint3dWithMileI(const QPoint & point, const double mile, bool hMirror, bool vMirror)
{
	//目标点
	hnCommon::hn3dPointWithMileI dstPoint;
	//异常处理
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return dstPoint;
	}
	//获取二维的图片高度
	int pixHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelY;

	//获取二维的图片高度
	int pixWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelX;

	double yScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioY;

	int mirrorX = point.x();
	int mirrorY = point.y();
	// todo 由于在拼接病害之前已经对病害进行过翻转，所以这个地方病害坐标就是不包含翻转的病害坐标
	// 在进行映射是就无需进行翻转
	//2d镜像处理 必须先进行镜像处理，再进行里程差的计算
	if (hMirror)
	{
		mirrorX = pixWidth - point.x();
	}
	if (vMirror)
	{
		mirrorY = pixHeight - point.y();
	}

	//算出当前点的编码器里程
	double encoderMile = mile + yScale*(pixHeight - mirrorY);

	//二三维有差值，要对其进行修正
	//获取二三维的编码器里程差值  
	double encoderMileDiff = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2d3dMileDiff();

	//计算出映射到三维的里程
	double newMile = encoderMile - encoderMileDiff;

	double buttomMile;
	int yPixel;
	hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getPixEncoderMile(newMile, yScale, buttomMile, yPixel);

	//新的2d点
	QPoint new2dBigImagePoint;
	new2dBigImagePoint.setY(yPixel);
	new2dBigImagePoint.setX(mirrorX);


	//单张2d图片内的坐标
	QPoint single2dPoint = new2dBigImagePoint;

	//四分之一3d图片内的坐标
	QPoint quarter3dPoint;
	//四分之一3d图片的宽度
	int quarter3dImageWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelWidth();
	//四分之一3d图片的高度
	int quarter3dImageHeight = 0.25 * hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelHeight();

	//赋值四分之一3d图片的x
	hn2d3dCoordinates coordinates;
	quarter3dPoint.setX(coordinates.single2dXToSingle3dX(single2dPoint.x()));
	//quarter3dPoint.setX((single2dPoint.x() * quarter3dImageWidth) * 1.0 / pixWidth);
	dstPoint.x = quarter3dPoint.x();

	//赋值四分之一3d图片的y
	quarter3dPoint.setY((single2dPoint.y() * quarter3dImageHeight) * 1.0 / pixHeight);

	//二维图片底部的里程
	double bottomEncoder2dMile = buttomMile;
	int encoderMileSingle3d = (int)((int)bottomEncoder2dMile) % 8;
	int quarter3dIdx = 3 - (encoderMileSingle3d / 2);
	dstPoint.y = quarter3dIdx * quarter3dImageHeight + quarter3dPoint.y();

	//赋值里程
	double heightScale3d = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageHeightScale();
	dstPoint.bottomEncoderMile = (int)(bottomEncoder2dMile / 8) * 8;

	//3d镜像处理
	int image3dWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelWidth();
	int image3dHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelHeight();
	if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getIsHMirrored())
	{
		dstPoint.x = image3dWidth - dstPoint.x;
	}
	if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getIsVMirrored())
	{
		dstPoint.y = image3dHeight - dstPoint.y;
	}

	return dstPoint;
}

QVector<QRect> hnImportAidcDiseases::createSingleImageLittleFrameRect()
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return QVector<QRect>();
	}

	QVector<QRect> dstRects;

	//算出每个像素代表多少米
	double widthScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioX;

	//矩形宽度（米） 每个小矩形宽度是10cm 也就是0.1米
	double rectWidth = 0.1;

	//算出自动化模式边长 单位：像素
	int sideLenth = rectWidth / widthScale;

	//图片像素宽度
	int imagePixelWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelX;

	//横向矩形的个数
	int widthRectCount = imagePixelWidth / sideLenth;

	//图片像素高度
	int imagePixelHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelY;

	

	//纵向矩形的个数
	int heightRectCount = imagePixelHeight / sideLenth;
//	int heightRectCount = round(imagePixelHeight*1.0 / sideLenth);

	//循环向目标的数据里面添加矩形
	QRect rect;					//单个自动化模式的矩形
	QPoint topLeftPoint;		//左上角的点
	QPoint bottomRightPoint;	//右下角的点
	//int tmpLenth = sideLenth - 1;
	int tmpLenth = sideLenth;
	for (int i = 0; i < heightRectCount; i++)
	{
		for (int j = 0; j < widthRectCount; j++)
		{
			//计算左上角的点
			topLeftPoint = QPoint(j * tmpLenth, i * tmpLenth);
			//计算右下角的点
			bottomRightPoint = QPoint(j * tmpLenth + tmpLenth, i * tmpLenth + tmpLenth);
			//得到矩形
			rect = QRect(topLeftPoint, bottomRightPoint);
			//插入数组
			dstRects.push_back(rect);
		}
	}

	return dstRects;
}

QVector<QRect> hnImportAidcDiseases::createLittleFrameRects(const AidcDisease & disease, const QVector<QRect> rects)
{
	QVector<QRect> result;

	//获取病害自动化模式的角标
	QString coord = disease.coord;

	//这个坐标吧，最后多了一个 - ,要把它去掉
	if (coord.endsWith("-"))
	{
		coord = coord.left(coord.size() - 1);
	}

	QStringList list = disease.coord.split("-");

	if (list.isEmpty())
	{
		return QVector<QRect>();
	}

	for (const QString &str : qAsConst(list))
	{
		//有可能是空的，过滤掉，防止异常情况发生
		if (str.isEmpty())
		{
			continue;
		}

		const int idx = str.toInt();
		if (rects.size() > idx)
		{
			result.push_back(rects.at(idx));
		}
	}

	return result;
}



vector<hn2dRectI> hnImportAidcDiseases::createLittleFrame2dRectIVector(const QVector<QRect> rects, const double mile)
{
	std::vector<hn2dRectI> dstVec;
	hn2dRectI hnRect;

	for (const QRect &rect : qAsConst(rects))
	{
		// getHnPoint2dWithMileI 这个函数不考虑对数据进行翻转，所以在这个基础上合并的病害就是没有考虑翻转情况的病害合并
		hnRect.p0 = getHnPoint2dWithMileI(rect.topLeft(), mile);
		hnRect.p1 = getHnPoint2dWithMileI(rect.topRight(), mile);
		hnRect.p2 = getHnPoint2dWithMileI(rect.bottomRight(), mile);
		hnRect.p3 = getHnPoint2dWithMileI(rect.bottomLeft(), mile);
		dstVec.push_back(hnRect);
	}

	return dstVec;
}

vector<hn3dRectI> hnImportAidcDiseases::createLittleFrame3dRectIVector(const QVector<QRect> rects, const double mile)
{
	std::vector<hn3dRectI> dstVec;
	hn3dRectI hnRect;
	hn2d3dCoordinates tool;

	for (const QRect &rect : qAsConst(rects))
	{
#if 0
		//用每个自动化模式的中心点来映射
		hn3dPointWithMileI p;
		std::vector<hn3dRectI> tmpVec;

		// 由于外部进行了翻转，所以这里传入参数：不进行翻转
		//p = getHnPoint3dWithMileI(rect.center(), mile);
		p = getHnPoint3dWithMileI(rect.center(), mile, false, false);
		//p = getHnPoint3dWithMileI(rect.center(), mile, false, true);

		tmpVec = tool.get3dLittleRects(p);
		dstVec.insert(dstVec.end(), tmpVec.begin(), tmpVec.end());
#else
		//用每个自动化模式的四个顶点来映射
		hn3dRectI hnRect;

		hnRect.p0 = this->getHnPoint3dWithMileI(rect.topLeft(), mile);
		hnRect.p1 = this->getHnPoint3dWithMileI(rect.topRight(), mile);
		hnRect.p2 = this->getHnPoint3dWithMileI(rect.bottomRight(), mile);
		hnRect.p3 = this->getHnPoint3dWithMileI(rect.bottomLeft(), mile);

		std::vector<hn3dRectI> vec;
		//要是某个点里程小于0，就不映射
		if (hnRect.p0.bottomEncoderMile < 0 ||
			hnRect.p1.bottomEncoderMile < 0 ||
			hnRect.p2.bottomEncoderMile < 0 ||
			hnRect.p3.bottomEncoderMile < 0)
		{
			return vec;
		}

		dstVec.push_back(hnRect);
#endif
	}

	return dstVec;
}


vector<hn3dRectI> hnImportAidcDiseases::createLittleFrame3dRectIVector(hnCommon::hnRoadDiseaseInfo &newDisease)
{

	std::vector<hn3dRectI> dstVec;
#if 0	
	hn3dRectI hnRect;
	hn2d3dCoordinates tool;

	for (const hn2dRectI &rectI : qAsConst(rectI))
	{
		//用每个自动化模式的中心点来映射
		hn3dPointWithMileI p;
		std::vector<hn3dRectI> tmpVec;
		QRect rect2D(QRect(QPoint(rectI.p0.x, rectI.p0.y), QPoint(rectI.p2.x, rectI.p2.y)));

		p = getHnPoint3dWithMileI(rect2D.center(), rectI.);

		tmpVec = tool.get3dLittleRects(p);
		dstVec.insert(dstVec.end(), tmpVec.begin(), tmpVec.end());
	}
#endif
	return dstVec;
}


int hnImportAidcDiseases::caculateDiseasesCount(QMap<QString, QVector<AidcDisease>> diseases)
{
	int count = 0;
	for (auto iter = diseases.begin(); iter != diseases.end(); iter++)
	{
		count += iter.value().size();
	}
	return count;
}
