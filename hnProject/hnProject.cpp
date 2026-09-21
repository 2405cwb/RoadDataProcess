#include "hnProject.h"
#include <QDir>
#include "hn2DProject.h"
#include "hn3DProject.h"
#include "hnFile.h"
#include "..\hnCommon\hnCompare.h"
#include <QSettings>
#include <QImage>
#include <algorithm>
#include <cmath>
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
#include <QInputDialog>
#include <QElapsedTimer>
#include <QDateTime>
namespace hnPro
{
	// 成果库是否需要修复由实际校桩状态判断，外业文本只用于恢复或完整重建。
	// 当前成果库的统一约定：工程起点桩号对应的 EnclMile 必须为 0，
	// 其余校桩和打标使用相对于工程起点的 DMI，病害数据不参与本流程。
	static double getProjectDmiLength(const hnProjectSetInfo& setting)
	{
		if (std::isfinite(setting.dEndEnclMile) && setting.dEndEnclMile > 0.0)
			return setting.dEndEnclMile;
		if (std::isfinite(setting.dLength) && setting.dLength > 0.0)
			return setting.dLength;
		return qAbs(setting.dEndMile - setting.dBegMile);
	}

	static bool isStandardStartPile(const hnMilePile& pile, double projectStartMile)
	{
		return qAbs(pile.dTrueMile - projectStartMile) <= 1.0 && qAbs(pile.dEnclMile) <= 0.001;
	}

	static bool isStandardEndPile(const hnMilePile& pile, double projectEndMile, double projectLength)
	{
		return qAbs(pile.dTrueMile - projectEndMile) <= 1.0 && qAbs(pile.dEnclMile - projectLength) <= 0.001;
	}

	static bool hasStandardAnchors(const vector<hnMilePile>& piles, double projectStartMile,
		double projectEndMile, double projectLength, bool& hasStart, bool& hasEnd)
	{
		hasStart = false;
		hasEnd = false;
		for (const auto& pile : piles)
		{
			if (!std::isfinite(pile.dTrueMile) || !std::isfinite(pile.dEnclMile))
				return false;
			if (isStandardStartPile(pile, projectStartMile)) hasStart = true;
			if (isStandardEndPile(pile, projectEndMile, projectLength)) hasEnd = true;
		}
		return true;
	}

	static bool containsSameMilePile(const vector<hnMilePile>& piles, double trueMile, double enclMile)
	{
		for (const auto& pile : piles)
		{
			if (qAbs(pile.dTrueMile - trueMile) <= 0.001 && qAbs(pile.dEnclMile - enclMile) <= 0.001)
				return true;
		}
		return false;
	}

	static void appendMilePile(vector<hnMilePile>& piles, double trueMile, double enclMile, int& nextId)
	{
		if (containsSameMilePile(piles, trueMile, enclMile)) return;
		hnMilePile pile;
		pile.nID = nextId++;
		pile.dTrueMile = trueMile;
		pile.dEnclMile = enclMile;
		piles.push_back(pile);
	}

	// 只有成果库缺少标准起点或终点时，才读取 MileStoneCaliInfo.txt 恢复外业校桩。
	// 文件只作为原始记录来源，不比较首尾跨度，也不参与偏移量猜测。
	// 偏移扣减完成后再次按“真实桩号 + 相对 DMI”去重，避免外业起点与标准起点同时变为起点零值。
	// 标准起终点固定排在前两项，其余校桩保留原有附加字段，并统一重新分配连续唯一 ID。
	static bool normalizeMilePilesAfterOffset(const vector<hnMilePile>& sourcePiles,
		double projectStartMile, double projectEndMile, double projectLength, double offset,
		vector<hnMilePile>& normalizedPiles)
	{
		normalizedPiles.clear();
		const double zeroTolerance = 0.001;
		const hnMilePile* standardStart = nullptr;
		const hnMilePile* standardEnd = nullptr;
		for (const auto& pile : sourcePiles)
		{
			if (!standardStart && isStandardStartPile(pile, projectStartMile)) standardStart = &pile;
			if (!standardEnd && isStandardEndPile(pile, projectEndMile, projectLength)) standardEnd = &pile;
		}
		if (!standardStart || !standardEnd) return false;

		hnMilePile startPile = *standardStart;
		startPile.nID = 0;
		startPile.dTrueMile = projectStartMile;
		startPile.dEnclMile = 0.0;
		normalizedPiles.push_back(startPile);

		hnMilePile endPile = *standardEnd;
		endPile.nID = 1;
		endPile.dTrueMile = projectEndMile;
		endPile.dEnclMile = projectLength;
		normalizedPiles.push_back(endPile);

		for (const auto& sourcePile : sourcePiles)
		{
			if (isStandardStartPile(sourcePile, projectStartMile) ||
				isStandardEndPile(sourcePile, projectEndMile, projectLength))
			{
				continue;
			}

			hnMilePile correctedPile = sourcePile;
			if (offset > zeroTolerance) correctedPile.dEnclMile -= offset;
			if (containsSameMilePile(normalizedPiles, correctedPile.dTrueMile, correctedPile.dEnclMile))
			{
				continue;
			}
			correctedPile.nID = static_cast<int>(normalizedPiles.size());
			normalizedPiles.push_back(correctedPile);
		}

		if (normalizedPiles.size() != sourcePiles.size()) return true;
		for (int i = 0; i < static_cast<int>(normalizedPiles.size()); ++i)
		{
			if (sourcePiles[i].nID != normalizedPiles[i].nID ||
				qAbs(sourcePiles[i].dTrueMile - normalizedPiles[i].dTrueMile) > zeroTolerance ||
				qAbs(sourcePiles[i].dEnclMile - normalizedPiles[i].dEnclMile) > zeroTolerance)
			{
				return true;
			}
		}
		return false;
	}

	// 无论外业文本是否存在，都先在内存中补齐标准起点和终点。
	static void ensureStandardAnchors(vector<hnMilePile>& piles, double projectStartMile,
		double projectEndMile, double projectLength)
	{
		int nextId = 0;
		for (const hnMilePile& pile : piles) nextId = qMax(nextId, pile.nID + 1);
		appendMilePile(piles, projectStartMile, 0.0, nextId);
		appendMilePile(piles, projectEndMile, projectLength, nextId);
	}

	// 偏移只由成果库中同一工程起点桩号的“标准 0 + 唯一正 DMI”共同确定。
	static bool findDatabaseStartOffset(const vector<hnMilePile>& piles, double projectStartMile,
		double& offset, QString* errorMessage)
	{
		bool hasZeroStart = false;
		vector<double> candidates;
		for (const hnMilePile& pile : piles)
		{
			if (!std::isfinite(pile.dTrueMile) || !std::isfinite(pile.dEnclMile))
			{
				if (errorMessage) *errorMessage = QStringLiteral("成果库校桩存在无效数值，无法判断 DMI 状态。");
				return false;
			}
			if (qAbs(pile.dTrueMile - projectStartMile) > 1.0) continue;
			if (qAbs(pile.dEnclMile) <= 0.001)
			{
				hasZeroStart = true;
				continue;
			}
			if (pile.dEnclMile < 0.0)
			{
				if (errorMessage)
					*errorMessage = QStringLiteral("工程起点校桩存在负 DMI=%1，数据状态异常，未自动修复。")
						.arg(pile.dEnclMile, 0, 'f', 6);
				return false;
			}
			bool duplicate = false;
			for (double value : candidates)
			{
				if (qAbs(value - pile.dEnclMile) <= 0.001)
				{
					duplicate = true;
					break;
				}
			}
			if (!duplicate) candidates.push_back(pile.dEnclMile);
		}

		if (!hasZeroStart)
		{
			if (errorMessage) *errorMessage = QStringLiteral("成果库没有工程起点桩号对应的 DMI=0 标准起点校桩。");
			return false;
		}
		if (candidates.size() > 1)
		{
			QStringList values;
			for (double value : candidates) values << QString::number(value, 'f', 3);
			if (errorMessage)
				*errorMessage = QStringLiteral("工程起点存在多个不同的非零 DMI 候选（%1），数据可能混合，未自动修复。")
					.arg(values.join(QStringLiteral(", ")));
			return false;
		}
		offset = candidates.empty() ? 0.0 : candidates.front();
		return true;
	}

	// 从 MileStoneCaliInfo.txt 读取原始校桩；文件缺失、为空或没有工程起点记录时按零偏移处理。
	static bool readFieldMilePileSource(const QString& filePath, double projectStartMile,
		double projectEndMile, vector<hnMilePile>& sourcePiles, double& sourceOffset,
		bool* sourceHasStartPile, QString* errorMessage)
	{
		sourcePiles.clear();
		sourceOffset = 0.0;
		if (sourceHasStartPile) *sourceHasStartPile = false;
		QFile file(filePath);
		if (!file.exists())
		{
			// 没有外业校桩文件等同于空文件，后续仍生成标准起终点并读取打标。
			return true;
		}
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			if (errorMessage) *errorMessage = QStringLiteral("无法读取校桩文件：%1").arg(filePath);
			return false;
		}

		const double minTrueMile = qMin(projectStartMile, projectEndMile);
		const double maxTrueMile = qMax(projectStartMile, projectEndMile);
		vector<double> startCandidates;
		QTextStream in(&file);
		while (!in.atEnd())
		{
			const QString text = in.readLine().simplified();
			if (text.isEmpty()) continue;
			const QStringList fields = text.split(QStringLiteral(" "), QString::SkipEmptyParts);
			if (fields.size() != 2)
			{
				if (errorMessage) *errorMessage = QStringLiteral("MileStoneCaliInfo.txt 存在格式错误的记录：%1").arg(text);
				return false;
			}
			bool dmiOk = false;
			bool trueMileOk = false;
			const double dmi = fields.at(0).toDouble(&dmiOk);
			const double trueMile = fields.at(1).toDouble(&trueMileOk);
			// 仅忽略桩号明显越界的记录；格式错误及无效数值仍按原规则处理。
			if (trueMileOk && std::isfinite(trueMile) &&
				(trueMile < minTrueMile - 1.0 || trueMile > maxTrueMile + 1.0))
			{
				continue;
			}
			if (!dmiOk || !trueMileOk || !std::isfinite(dmi) || !std::isfinite(trueMile))
			{
				if (errorMessage) *errorMessage = QStringLiteral("MileStoneCaliInfo.txt 存在无效或越界记录：%1").arg(text);
				return false;
			}
			hnMilePile pile;
			pile.dTrueMile = trueMile;
			pile.dEnclMile = dmi;
			sourcePiles.push_back(pile);
			if (qAbs(trueMile - projectStartMile) <= 1.0)
			{
				bool duplicate = false;
				for (double value : startCandidates)
				{
					if (qAbs(value - dmi) <= 0.001)
					{
						duplicate = true;
						break;
					}
				}
				if (!duplicate) startCandidates.push_back(dmi);
			}
		}
		if (sourcePiles.empty())
		{
			// 空文件或仅包含空白行表示没有外业校桩，偏移保持为零。
			return true;
		}
		if (startCandidates.empty())
		{
			// 外业未记录起点校桩表示工程从相对 DMI 0 开始，不属于异常数据。
			sourceOffset = 0.0;
			return true;
		}
		if (sourceHasStartPile) *sourceHasStartPile = true;
		if (startCandidates.size() > 1)
		{
			if (errorMessage)
				*errorMessage = QStringLiteral("MileStoneCaliInfo.txt 包含多个不同的工程起点校桩，无法安全确定 DMI 偏移。");
			return false;
		}
		if (startCandidates.front() < -0.001)
		{
			if (errorMessage)
				*errorMessage = QStringLiteral("MileStoneCaliInfo.txt 的工程起点校桩 DMI 不能为负数。");
			return false;
		}
		sourceOffset = qAbs(startCandidates.front()) <= 0.001 ? 0.0 : startCandidates.front();
		return true;
	}

	// 根据外业校桩源构造只包含相对 DMI、唯一锚点和连续 ID 的最终集合。
	static bool buildRelativeMilePilesFromField(const vector<hnMilePile>& sourcePiles,
		double sourceOffset, double projectStartMile, double projectEndMile,
		double projectLength, vector<hnMilePile>& finalPiles, QString* errorMessage)
	{
		vector<hnMilePile> workingPiles;
		int nextId = 0;
		appendMilePile(workingPiles, projectStartMile, 0.0, nextId);
		appendMilePile(workingPiles, projectEndMile, projectLength, nextId);
		for (const hnMilePile& sourcePile : sourcePiles)
		{
			const double relativeDmi = sourcePile.dEnclMile - sourceOffset;
			if (!std::isfinite(relativeDmi) || relativeDmi < -0.001 ||
				relativeDmi > projectLength + 0.001)
			{
				if (errorMessage)
					*errorMessage = QStringLiteral("校桩转换为相对里程后越界：桩号=%1，原DMI=%2，偏移=%3。")
						.arg(sourcePile.dTrueMile, 0, 'f', 3)
						.arg(sourcePile.dEnclMile, 0, 'f', 3)
						.arg(sourceOffset, 0, 'f', 3);
				return false;
			}
			appendMilePile(workingPiles, sourcePile.dTrueMile,
				qAbs(relativeDmi) <= 0.001 ? 0.0 : relativeDmi, nextId);
		}

		if (!normalizeMilePilesAfterOffset(workingPiles, projectStartMile,
			projectEndMile, projectLength, 0.0, finalPiles))
		{
			finalPiles = workingPiles;
		}
		return true;
	}

	// 读取打标文本并使用校桩文件确定的相同偏移生成相对 DMI。
	static bool buildRelativeMarksFromField(hn2DProject* project2D, hnProject* project,
		double sourceOffset, double projectStartMile, double projectEndMile,
		double projectLength, vector<hnMarkInfo>& finalMarks, QString* errorMessage)
	{
		finalMarks.clear();
		if (!project2D || !project)
		{
			if (errorMessage) *errorMessage = QStringLiteral("二维工程尚未初始化，无法读取打标文件。");
			return false;
		}

		const QString filePath = project2D->getMarkFilePath();
		QFile file(filePath);
		if (!file.exists() || !file.open(QIODevice::ReadOnly))
		{
			if (errorMessage) *errorMessage = QStringLiteral("无法读取打标文件：%1").arg(filePath);
			return false;
		}
		const qint64 sourceSize = file.size();
		file.close();

		vector<hnMarkInfo> sourceMarks;
		project2D->add2dMarkInfo(sourceMarks, project);
		if (sourceMarks.empty() && sourceSize > 0)
		{
			if (errorMessage) *errorMessage = QStringLiteral("RoadStatuMarkInfo.txt 没有可导入的有效记录：%1").arg(filePath);
			return false;
		}

		const double minTrueMile = qMin(projectStartMile, projectEndMile);
		const double maxTrueMile = qMax(projectStartMile, projectEndMile);
		for (hnMarkInfo mark : sourceMarks)
		{
			double relativeDmi = mark.dEnclMile;
			if (qAbs(relativeDmi) > 0.001) relativeDmi -= sourceOffset;
			if (!std::isfinite(mark.dTrueMile) || !std::isfinite(relativeDmi) ||
				mark.dTrueMile < minTrueMile - 1.0 || mark.dTrueMile > maxTrueMile + 1.0 ||
				relativeDmi < -0.001 || relativeDmi > projectLength + 0.001)
			{
				if (errorMessage)
					*errorMessage = QStringLiteral("打标转换为相对里程后越界：桩号=%1，原DMI=%2，偏移=%3。")
						.arg(mark.dTrueMile, 0, 'f', 3)
						.arg(mark.dEnclMile, 0, 'f', 3)
						.arg(sourceOffset, 0, 'f', 3);
				return false;
			}
			mark.nID = static_cast<int>(finalMarks.size());
			mark.dEnclMile = qAbs(relativeDmi) <= 0.001 ? 0.0 : relativeDmi;
			finalMarks.push_back(mark);
		}
		return true;
	}

	// 数据发生变化前创建只读备份，调用方在事务提交后保留该路径供人工恢复。
	static bool createResultDbBackup(const QString& databasePath, QString& backupPath,
		QString* errorMessage)
	{
		backupPath = databasePath + QStringLiteral(".dmi-repair-backup-") +
			QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmsszzz"));
		if (!QFile::copy(databasePath, backupPath))
		{
			if (errorMessage) *errorMessage = QStringLiteral("创建成果库备份失败，数据库未修改：%1").arg(backupPath);
			backupPath.clear();
			return false;
		}
		QFile::setPermissions(backupPath,
			QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther);
		return true;
	}

	static bool hasValidMarkSource(const QString& filePath)
	{
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
		if (file.size() == 0) return true;
		QTextStream in(&file);
		in.setCodec(QTextCodec::codecForName("UTF-8"));
		while (!in.atEnd())
		{
			const QStringList fields = in.readLine().simplified().split(
				QStringLiteral(" "), QString::SkipEmptyParts);
			bool dmiOk = false;
			if (fields.size() >= 4) fields.at(2).toDouble(&dmiOk);
			if (dmiOk && (fields.at(3).contains(QStringLiteral(":")) ||
				fields.at(3).contains(QStringLiteral("："))))
			{
				return true;
			}
		}
		return false;
	}

	hnProject::hnProject() :m_pDbSqlite(NULL), m_p2DProject(NULL), m_p3DProject(NULL), m_current3dDmi(0)
	{
		m_xrSetting = HnXRSettings::getInstance();
	}

	hnProject::~hnProject()
	{

	}

	// 打开工程
	bool hnProject::openProject(hnProjectDataInfo& curProDataInfo)
	{
		m_roadSpace = 2;
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
			if (curProDataInfo.proSetInfo.nWorkType == PROJECT_TYPE::PROJECT_2D_TYPE)
			{
				m_str2DProPath = m_strProjectPath;
				dir2D.setPath(m_str2DProPath);
			}
			else if (!m_str2DProName.isEmpty())
			{
				QMessageBox::critical(QApplication::activeWindow(),
					QStringLiteral("数据错误"),
					QStringLiteral("工程数据不完整。\n\nProjectInfo.xml 中指定的二维工程目录不存在：\n%1\n\n请检查“二维工程名”是否与实际文件夹名称一致。")
					.arg(m_str2DProPath),
					QMessageBox::Ok);
				return false;
			}
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

		m_lineCameraInfo = bExist2D ? hnLineCameraConfig::load(m_str2DProPath) : hnLineCameraInfo();
		if (m_lineCameraInfo.isLineCamera)
		{
			if (!m_lineCameraInfo.cameraConfigValid)
			{
				THROW_RUNTIME(m_lineCameraInfo.errorMessage);
			}
			if (!m_lineCameraInfo.validAreaConfigured)
			{
				THROW_RUNTIME(QStringLiteral("线阵工程尚未设置有效区域，无法打开：%1").arg(m_str2DProPath));
			}
			curProDataInfo.proSetInfo.dRoadWidth = m_lineCameraInfo.roadWidthMeters();
			curProDataInfo.proSetInfo.dRoadLength = m_lineCameraInfo.imageLengthMeters();
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
					const int resolutionResult = getPixResoluion(path1);
					if (m_lineCameraInfo.isLineCamera && resolutionResult != 0)
					{
						qWarning().noquote() << "[HN_LINE_CAMERA] failed to initialize image resolution, error=" << resolutionResult;
						return false;
					}
				}
				else
				{
					//
				}
				if (m_p2DProject)
				{
					// 首次创建时先导入原始打标，再统一检查校桩完整性和 DMI 偏移。
					QString dmiRepairError;
					if (!ensureResultDbDmiNormalized(&dmiRepairError))
						QMessageBox::warning(QApplication::activeWindow(), QStringLiteral("校桩与打标自检"), dmiRepairError);
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
				// Existing result databases are the sole source of project settings.
				if (!m_pDbSqlite->m_markerInfoTable.readData(m_vecMarkInfo))
				{
					return false;

				}

				bool milePileLoadedFromDb = m_pDbSqlite->m_milePileTable.readData(m_vecMileagePile);
				if (!milePileLoadedFromDb)
				{
					// 已有成果库即使校桩表为空，也不能在备份前直接写入；统一恢复流程稍后处理。
					m_vecMileagePile.clear();
				}

				/*	if (!m_pDbSqlite->m_diseaseTable.readAllData(m_vecDisData))
				{
				return false;
				}*/

				//如果不存在控制点的表，则创建控制点的表
				HN_CREATE_RESULT_TABLE tableCmd;
				tableCmd.createCtrlPointTableCmd;
				m_pDbSqlite->executeDB(tableCmd.createCtrlPointTableCmd.c_str());


				// Existing result databases own their project settings at runtime.
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

		if (m_xrSetting->mile2dmiToInt)
		{
			return  MyCommonMethods::csharpRoundToInt( dTrueMile);
		}
		else
		{
			return dTrueMile;
		}
		 
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
		if (m_xrSetting->mile2dmiToInt)
		{
			return MyCommonMethods::csharpRoundToInt(dEnclMile);
		}
		else
		{
			return dEnclMile;
		}
		
		
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
		if (m_lineCameraInfo.isLineCamera && m_lineCameraInfo.cameraConfigValid && m_lineCameraInfo.validAreaConfigured)
		{
			if (m_lineCameraInfo.imageWidth != image.width())
			{
				return -4;
			}
			m_lineCameraInfo.imageHeight = image.height();
			m_projectInfo.dRadioX = m_lineCameraInfo.meterPerPixelWidth();
			m_projectInfo.dRadioY = m_lineCameraInfo.meterPerPixelHeight();
			m_projectInfo.dRoadWidth = m_lineCameraInfo.roadWidthMeters();
			const double configuredImageLength = m_lineCameraInfo.imageLengthMeters();
			if (!qFuzzyCompare(m_projectInfo.dRoadLength + 1.0, configuredImageLength + 1.0))
			{
				qWarning().noquote() << "[HN_LINE_CAMERA] RoadDis differs from MmPerPix H; using line camera value"
					<< "RoadDis=" << m_projectInfo.dRoadLength << "lineLength=" << configuredImageLength;
			}
			m_projectInfo.dRoadLength = configuredImageLength;
			m_roadSpace = configuredImageLength;
		}
		else
		{
			m_projectInfo.dRadioX = m_projectInfo.dRoadWidth / m_projectInfo.picPixelX;
			m_projectInfo.dRadioY = m_projectInfo.dRoadLength / m_projectInfo.picPixelY;
		}
		return 0;
	}

	



	bool hnProject::reloadLineCameraInfo(QString* errorMessage)
	{
		hnLineCameraInfo info = hnLineCameraConfig::load(m_str2DProPath);
		if (!info.isLineCamera || !info.cameraConfigValid || !info.validAreaConfigured)
		{
			if (errorMessage)
			{
				*errorMessage = info.errorMessage.isEmpty() ? QStringLiteral("线阵相机有效区域配置不完整。") : info.errorMessage;
			}
			return false;
		}
		m_lineCameraInfo = info;
		m_projectInfo.dRadioX = info.meterPerPixelWidth();
		m_projectInfo.dRadioY = info.meterPerPixelHeight();
		m_projectInfo.dRoadWidth = info.roadWidthMeters();
		m_projectInfo.dRoadLength = info.imageLengthMeters();
		m_roadSpace = m_projectInfo.dRoadLength;
		if (m_pDbSqlite)
		{
			m_pDbSqlite->m_projectSetTable.writeData(m_projectInfo);
		}
		return true;
	}

	bool hnProject::setLineCameraValidArea(int leftPixel, int rightPixel, QString* errorMessage)
	{
		if (!m_lineCameraInfo.isLineCamera)
		{
			if (errorMessage)
			{
				*errorMessage = QStringLiteral("当前工程不是线阵相机工程。");
			}
			return false;
		}
		hnLineCameraInfo updated = m_lineCameraInfo;
		updated.leftPixel = leftPixel;
		updated.rightPixel = rightPixel;
		updated.validAreaConfigured = leftPixel >= 0 && leftPixel < rightPixel && rightPixel <= updated.imageWidth;
		if (!hnLineCameraConfig::saveValidArea(updated, errorMessage))
		{
			return false;
		}
		return reloadLineCameraInfo(errorMessage);
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

	bool hnProject::saveMarksToResultDb(QString* errorMessage)
	{
		if (!m_pDbSqlite)
		{
			if (errorMessage) *errorMessage = QStringLiteral("成果数据库不可用。");
			return false;
		}
		QSet<int> markIds;
		for (const hnMarkInfo& mark : m_vecMarkInfo)
		{
			if (!validateUserMark(mark, errorMessage)) return false;
			if (markIds.contains(mark.nID))
			{
				if (errorMessage) *errorMessage = QStringLiteral("打标存在重复 ID，已拒绝写入数据库。");
				return false;
			}
			markIds.insert(mark.nID);
		}
		if (!m_pDbSqlite->executeDB("BEGIN IMMEDIATE;"))
		{
			if (errorMessage) *errorMessage = QStringLiteral("无法开始打标数据库事务。");
			return false;
		}
		if (!m_pDbSqlite->m_markerInfoTable.clearData() ||
			!m_pDbSqlite->m_markerInfoTable.writeData(m_vecMarkInfo) ||
			!m_pDbSqlite->executeDB("COMMIT;"))
		{
			m_pDbSqlite->executeDB("ROLLBACK;");
			if (errorMessage) *errorMessage = QStringLiteral("打标写入失败，数据库事务已经回滚。");
			return false;
		}
		return true;
	}

	bool hnProject::saveMileagePilesToResultDb(QString* errorMessage)
	{
		if (!m_pDbSqlite)
		{
			if (errorMessage) *errorMessage = QStringLiteral("成果数据库不可用。");
			return false;
		}
		return m_pDbSqlite->saveRelativeMileageData(nullptr, m_vecMileagePile,
			m_vecMarkInfo, false, m_projectInfo.dBegMile, m_projectInfo.dEndMile,
			getProjectDmiLength(m_projectInfo), errorMessage);
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

	bool hnProject::changeMark(QVector<hnCommon::hnMarkInfo>& marks, const QVector<int>& deleteMarkIndexs)
	{
		bool needUpdate = false;
		return changeMark(marks, deleteMarkIndexs, &needUpdate, nullptr) && needUpdate;
	}

	bool hnProject::changeMark(QVector<hnCommon::hnMarkInfo>& marks,
		const QVector<int>& deleteMarkIndexs, bool* needUpdate, QString* errorMessage)
	{
		const vector<hnCommon::hnMarkInfo> previousMarks = m_vecMarkInfo;
		bool shouldUpdate = false;
		if (needUpdate) *needUpdate = false;
		int lastIndex = m_pDbSqlite ? m_pDbSqlite->m_markerInfoTable.getMaxID() : 1;
		for (const auto& item : m_vecMarkInfo)
		{
			lastIndex = qMax(lastIndex, item.nID + 1);
		}

		for (auto& mark : marks)
		{
			mark.dEnclMile = trueMileToEncl(mark.dTrueMile);
			if (!validateUserMark(mark, errorMessage))
			{
				m_vecMarkInfo = previousMarks;
				return false;
			}
			if (isRoadAttributeMark(mark.nType))
			{
				shouldUpdate = true;
			}
			mark.nID = lastIndex++;
			m_vecMarkInfo.push_back(mark);
		}

		QSet<int> deleteSet = QSet<int>::fromList(deleteMarkIndexs.toList());
		m_vecMarkInfo.erase(
			std::remove_if(
				m_vecMarkInfo.begin(),
				m_vecMarkInfo.end(),
				[&deleteSet, &shouldUpdate, this](const hnCommon::hnMarkInfo& obj) {
					const bool deleteItem = deleteSet.contains(obj.nID);
					if (deleteItem && isRoadAttributeMark(obj.nType))
					{
						shouldUpdate = true;
					}
					return deleteItem;
				}
			),
			m_vecMarkInfo.end()
		);

		if (!saveMarksToResultDb(errorMessage)) { m_vecMarkInfo = previousMarks; return false; }
		if (shouldUpdate)
		{
			initMileList(false);
		}
		if (needUpdate) *needUpdate = shouldUpdate;
		return true;
	}

	bool hnProject::addMark(hnCommon::hnMarkInfo& mark)
	{
		bool needUpdate = false;
		return addMark(mark, &needUpdate, nullptr) && needUpdate;
	}

	bool hnProject::addMark(hnCommon::hnMarkInfo& mark, bool* needUpdate, QString* errorMessage)
	{
		const vector<hnCommon::hnMarkInfo> previousMarks = m_vecMarkInfo;
		const bool shouldUpdate = isRoadAttributeMark(mark.nType);
		if (needUpdate) *needUpdate = false;
		mark.dEnclMile = trueMileToEncl(mark.dTrueMile);
		if (!validateUserMark(mark, errorMessage) || !validateMarkConflict(mark, -1, errorMessage)) return false;
		int lastIndex = m_pDbSqlite ? m_pDbSqlite->m_markerInfoTable.getMaxID() : 1;
		for (const auto& item : m_vecMarkInfo)
		{
			lastIndex = qMax(lastIndex, item.nID + 1);
		}

		mark.nID = lastIndex;
		m_vecMarkInfo.push_back(mark);
		if (!saveMarksToResultDb(errorMessage)) { m_vecMarkInfo = previousMarks; return false; }
		if (shouldUpdate)
		{
			initMileList(false);
		}
		if (needUpdate) *needUpdate = shouldUpdate;
		return true;
	}

	// 用户新增打标时，真实桩号和计算后的相对 DMI 都必须位于当前工程范围内。
	bool hnProject::validateUserMark(const hnCommon::hnMarkInfo& mark, QString* errorMessage) const
	{
		const double projectLength = getProjectDmiLength(m_projectInfo);
		const double minTrueMile = qMin(m_projectInfo.dBegMile, m_projectInfo.dEndMile);
		const double maxTrueMile = qMax(m_projectInfo.dBegMile, m_projectInfo.dEndMile);
		if (!std::isfinite(mark.dTrueMile) || !std::isfinite(mark.dEnclMile))
		{
			if (errorMessage) *errorMessage = QStringLiteral("打标桩号或相对里程不是有效数字，请重新输入。");
			return false;
		}
		if (mark.dTrueMile < minTrueMile - 0.001 || mark.dTrueMile > maxTrueMile + 0.001)
		{
			if (errorMessage)
				*errorMessage = QStringLiteral("打标桩号必须在工程起点 %1 和终点 %2 之间，请重新输入。")
					.arg(m_projectInfo.dBegMile, 0, 'f', 3).arg(m_projectInfo.dEndMile, 0, 'f', 3);
			return false;
		}
		if (mark.dEnclMile < -0.001 || mark.dEnclMile > projectLength + 0.001)
		{
			if (errorMessage)
				*errorMessage = QStringLiteral("打标相对里程必须在 0 到工程总长度 %1 之间，请重新输入。")
					.arg(projectLength, 0, 'f', 3);
			return false;
		}
		return true;
	}


    bool hnProject::validateMarkConflict(const hnMarkInfo& value, int ignoredId, QString* errorMessage) const
    {
        if (value.nType < 0 || value.nType > 4 ||
            memchr(value.strMark, 0, sizeof(value.strMark)) == nullptr ||
            QString::fromLocal8Bit(value.strMark).trimmed().isEmpty())
        {
            if (errorMessage) *errorMessage = QStringLiteral("请选择有效打标类型并填写内容，内容不能超出字段长度。");
            return false;
        }
        for (const auto& mark : m_vecMarkInfo)
        {
            if (ignoredId >= 0 && mark.nID == ignoredId) continue;
            if (mark.nType != value.nType || qAbs(mark.dEnclMile - value.dEnclMile) > 0.001) continue;
            if (isRoadAttributeMark(value.nType) ||
                QString::fromLocal8Bit(mark.strMark).trimmed() == QString::fromLocal8Bit(value.strMark).trimmed())
            {
                if (errorMessage) *errorMessage = QStringLiteral("该位置已有同类型打标“%1”（桩号 %2），请修改位置或内容。")
                    .arg(QString::fromLocal8Bit(mark.strMark)).arg(mark.dTrueMile, 0, 'f', 3);
                return false;
            }
        }
        return true;
    }

    bool hnProject::updateMark(int id, const hnMarkInfo& value, QString* errorMessage)
    {
        if (!m_pDbSqlite || !m_pDbSqlite->isOpen() || !m_p2DProject)
        {
            if (errorMessage) *errorMessage = QStringLiteral("当前工程数据库不可用或不支持修改打标。");
            return false;
        }
        int index = -1;
        for (int i = 0; i < static_cast<int>(m_vecMarkInfo.size()); ++i)
            if (m_vecMarkInfo[i].nID == id) { index = i; break; }
        if (index < 0)
        {
            if (errorMessage) *errorMessage = QStringLiteral("原打标已不存在，请刷新列表后重试。");
            return false;
        }
        const hnMarkInfo previous = m_vecMarkInfo[index];
        hnMarkInfo candidate = value;
        candidate.nID = id;
        // 只改内容或类型时保留精确位置，避免整米换算设置使位置漂移。
        candidate.dEnclMile = candidate.dTrueMile == previous.dTrueMile ? previous.dEnclMile : trueMileToEncl(candidate.dTrueMile);
        if (!validateUserMark(candidate, errorMessage) || !validateMarkConflict(candidate, id, errorMessage)) return false;
        candidate.dGpsTimer = previous.dGpsTimer;
        m_vecMarkInfo[index] = candidate;
        if (!saveMarksToResultDb(errorMessage))
        {
            m_vecMarkInfo[index] = previous;
            return false;
        }
        const double currentDmi = m_currentMile.dEnclMile;
        initMileList(false);
        setCurrentRoadMile(getCloseMileFromDmi(currentDmi));
        return true;
    }

    bool hnProject::updateMilePile(int id, const hnMilePile& value, QString* errorMessage)
    {
        if (!m_pDbSqlite || !m_pDbSqlite->isOpen() || !m_p2DProject)
        {
            if (errorMessage) *errorMessage = QStringLiteral("当前工程数据库不可用或不支持修改校桩。");
            return false;
        }
        vector<hnMilePile> finalPiles = m_vecMileagePile;
        int index = -1;
        for (int i = 0; i < static_cast<int>(finalPiles.size()); ++i)
            if (finalPiles[i].nID == id) { index = i; break; }
        if (index < 0)
        {
            if (errorMessage) *errorMessage = QStringLiteral("原校桩已不存在，请刷新列表后重试。");
            return false;
        }
        if (isStandardAnchorPile(finalPiles[index]))
        {
            if (errorMessage) *errorMessage = QStringLiteral("工程起终点校桩由系统维护，请通过“编辑工程”修改起终点。");
            return false;
        }
        if (!validateUserMilePile(value, errorMessage, id)) return false;
        if (value.dTrueMile == finalPiles[index].dTrueMile && value.dEnclMile == finalPiles[index].dEnclMile) return true;
        finalPiles[index].dTrueMile = value.dTrueMile;
        finalPiles[index].dEnclMile = value.dEnclMile;
        sort(finalPiles.begin(), finalPiles.end(), compareMilepileByEnclMile);
        const double direction = m_projectInfo.dEndMile > m_projectInfo.dBegMile ? 1.0 : -1.0;
        for (size_t i = 1; i < finalPiles.size(); ++i)
        {
            if (finalPiles[i].dEnclMile - finalPiles[i - 1].dEnclMile <= 0.001 ||
                (finalPiles[i].dTrueMile - finalPiles[i - 1].dTrueMile) * direction <= 0.001)
            {
                if (errorMessage) *errorMessage = QStringLiteral("校桩列表存在重复或顺序交叉，未保存修改。");
                return false;
            }
        }
        // 草稿内重新换算打标桩号；与校桩一次提交，失败不发布到内存。
        vector<hnMarkInfo> finalMarks = m_vecMarkInfo;
        for (auto& mark : finalMarks)
        {
            bool found = false;
            for (size_t i = 1; i < finalPiles.size(); ++i)
            {
                const auto& first = finalPiles[i - 1];
                const auto& last = finalPiles[i];
                if (mark.dEnclMile < first.dEnclMile || mark.dEnclMile > last.dEnclMile) continue;
                mark.dTrueMile = first.dTrueMile + (last.dTrueMile - first.dTrueMile) *
                    (mark.dEnclMile - first.dEnclMile) / (last.dEnclMile - first.dEnclMile);
                if (m_xrSetting->mile2dmiToInt) mark.dTrueMile = MyCommonMethods::csharpRoundToInt(mark.dTrueMile);
                found = true;
                break;
            }
            if (!found || !validateUserMark(mark, errorMessage))
            {
                if (errorMessage && errorMessage->isEmpty()) *errorMessage = QStringLiteral("有打标超出校桩覆盖范围，未保存修改。");
                return false;
            }
        }
        if (!m_pDbSqlite->saveRelativeMileageData(nullptr, finalPiles, finalMarks, true,
            m_projectInfo.dBegMile, m_projectInfo.dEndMile, getProjectDmiLength(m_projectInfo), errorMessage)) return false;
        const double currentDmi = m_currentMile.dEnclMile;
        m_vecMileagePile = finalPiles;
        m_vecMarkInfo = finalMarks;
        initMileList(false);
        setCurrentRoadMile(getCloseMileFromDmi(currentDmi));
        return true;
    }

	bool hnProject::deleteMark(const hnCommon::hnMarkInfo& Mark)
	{
		return deleteMark(Mark.nID);
	}

	bool hnProject::deleteMark(int id)
	{
		const vector<hnCommon::hnMarkInfo> previousMarks = m_vecMarkInfo;
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

		if (!saveMarksToResultDb()) { m_vecMarkInfo = previousMarks; return false; }
		if (needUpdate)
		{
			initMileList(false);
		}
		return needUpdate;
	}

	bool hnProject::isStandardAnchorPile(const hnCommon::hnMilePile& pile) const
	{
		return isStandardStartPile(pile, m_projectInfo.dBegMile) ||
			isStandardEndPile(pile, m_projectInfo.dEndMile, getProjectDmiLength(m_projectInfo));
	}

	// 用户新增校桩时同时检查真实桩号、相对 DMI、端点保护和重复桩号。
	bool hnProject::validateUserMilePile(const hnCommon::hnMilePile& pile, QString* errorMessage) const
	{
		return validateUserMilePile(pile, errorMessage, -1);
	}

	bool hnProject::validateUserMilePile(const hnCommon::hnMilePile& pile, QString* errorMessage, int ignoredId) const
	{
		const double projectLength = getProjectDmiLength(m_projectInfo);
		const double minTrueMile = qMin(m_projectInfo.dBegMile, m_projectInfo.dEndMile);
		const double maxTrueMile = qMax(m_projectInfo.dBegMile, m_projectInfo.dEndMile);
		if (!std::isfinite(pile.dTrueMile) || !std::isfinite(pile.dEnclMile))
		{
			if (errorMessage) *errorMessage = QStringLiteral("校桩桩号或相对里程不是有效数字，请重新输入。");
			return false;
		}
		if (pile.dTrueMile < minTrueMile - 0.001 || pile.dTrueMile > maxTrueMile + 0.001)
		{
			if (errorMessage)
				*errorMessage = QStringLiteral("校桩桩号必须在工程起点 %1 和终点 %2 之间，请重新输入。")
					.arg(m_projectInfo.dBegMile, 0, 'f', 3).arg(m_projectInfo.dEndMile, 0, 'f', 3);
			return false;
		}
		if (pile.dEnclMile < -0.001 || pile.dEnclMile > projectLength + 0.001)
		{
			if (errorMessage)
				*errorMessage = QStringLiteral("校桩相对里程必须在 0 到工程总长度 %1 之间，请重新输入。")
					.arg(projectLength, 0, 'f', 3);
			return false;
		}
		if (qAbs(pile.dTrueMile - m_projectInfo.dBegMile) <= 1.0 ||
			qAbs(pile.dTrueMile - m_projectInfo.dEndMile) <= 1.0)
		{
			if (errorMessage) *errorMessage = QStringLiteral("工程起点和终点校桩由系统维护，不能重复添加或修改。");
			return false;
		}
		for (const hnMilePile& existingPile : m_vecMileagePile)
		{
			if (ignoredId >= 0 && existingPile.nID == ignoredId) continue;
			if (qAbs(existingPile.dTrueMile - pile.dTrueMile) <= 0.001)
			{
				if (errorMessage) *errorMessage = QStringLiteral("该真实桩号已经存在校桩，请重新输入。");
				return false;
			}
			if (qAbs(existingPile.dEnclMile - pile.dEnclMile) <= 0.001)
			{
				if (errorMessage) *errorMessage = QStringLiteral("该相对里程已经存在校桩，请重新输入。");
				return false;
			}
            // 相对里程向前增加时，桩号必须符合工程方向，不能与其他校桩交叉。
            const double direction = m_projectInfo.dEndMile > m_projectInfo.dBegMile ? 1.0 : -1.0;
            if ((pile.dEnclMile - existingPile.dEnclMile) *
                (pile.dTrueMile - existingPile.dTrueMile) * direction <= 0.0)
            {
                if (errorMessage) *errorMessage = QStringLiteral("修改后的校桩与桩号 %1、相对里程 %2 米的校桩顺序冲突，请调整桩号或位置。")
                    .arg(existingPile.dTrueMile, 0, 'f', 3).arg(existingPile.dEnclMile, 0, 'f', 3);
                return false;
            }

		}
		return true;
	}

	bool hnProject::changeMilePile(QVector < hnCommon::hnMilePile>& piles, QString* errorMessage)
	{
		const vector<hnCommon::hnMilePile> previousPiles = m_vecMileagePile;
		m_vecMileagePile.clear();
		for (auto mile : piles)
		{
			m_vecMileagePile.push_back(mile);
		}
		ensureStandardAnchors(m_vecMileagePile, m_projectInfo.dBegMile,
			m_projectInfo.dEndMile, getProjectDmiLength(m_projectInfo));
		vector<hnMilePile> normalizedPiles;
		normalizeMilePilesAfterOffset(m_vecMileagePile, m_projectInfo.dBegMile,
			m_projectInfo.dEndMile, getProjectDmiLength(m_projectInfo), 0.0, normalizedPiles);
		if (!normalizedPiles.empty()) m_vecMileagePile = normalizedPiles;
		sort(piles.begin(), piles.end(), compareMilepileByEnclMile);
		sort(m_vecMileagePile.begin(), m_vecMileagePile.end(), compareMilepileByEnclMile);
		if (!saveMileagePilesToResultDb(errorMessage))
		{
			m_vecMileagePile = previousPiles;
			return false;
		}
		initMileList(false);
		return true;
	}

	bool hnProject::addMilePile(hnCommon::hnMilePile& pile, QString* errorMessage)
	{
		if (!validateUserMilePile(pile, errorMessage)) return false;
		const vector<hnCommon::hnMilePile> previousPiles = m_vecMileagePile;
		int maxId = m_pDbSqlite ? m_pDbSqlite->m_milePileTable.getMaxID() : 1;
		for (const auto& item : m_vecMileagePile)
		{
			maxId = qMax(maxId, item.nID + 1);
		}
		pile.nID = maxId;
		m_vecMileagePile.push_back(pile);

		sort(m_vecMileagePile.begin(), m_vecMileagePile.end(), compareMilepileByEnclMile);
		if (!saveMileagePilesToResultDb(errorMessage))
		{
			m_vecMileagePile = previousPiles;
			return false;
		}
		initMileList(false);
		return true;
	}

	bool hnProject::deleteMilePile(int id, QString* errorMessage)
	{
		const vector<hnCommon::hnMilePile> previousPiles = m_vecMileagePile;
		bool found = false;
		for (int i = static_cast<int>(m_vecMileagePile.size()) - 1; i >= 0; --i)
		{
			hnCommon::hnMilePile curMile = m_vecMileagePile[i];
			if (curMile.nID == id)
			{
				found = true;
				if (isStandardAnchorPile(curMile))
				{
					if (errorMessage) *errorMessage = QStringLiteral("工程起点和终点校桩不能删除。");
					return false;
				}
				m_vecMileagePile.erase(m_vecMileagePile.begin() + i);
			}
		}
		if (!found)
		{
			if (errorMessage) *errorMessage = QStringLiteral("未找到需要删除的校桩。");
			return false;
		}
		if (!saveMileagePilesToResultDb(errorMessage))
		{
			m_vecMileagePile = previousPiles;
			return false;
		}
		initMileList(false);
		return true;
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

	// 打开工程时依据实际校桩状态补齐锚点，并在证据充分时重建相对里程数据。
	bool hnProject::ensureResultDbDmiNormalized(QString* errorMessage)
	{
		if (!m_pDbSqlite || !m_pDbSqlite->isOpen() || !m_p2DProject)
		{
			if (errorMessage) *errorMessage = QStringLiteral("成果库或二维工程尚未初始化，无法检查校桩和打标。");
			return false;
		}

		const double projectStartMile = m_projectInfo.dBegMile;
		const double projectEndMile = m_projectInfo.dEndMile;
		const double projectLength = getProjectDmiLength(m_projectInfo);
		if (!std::isfinite(projectStartMile) || !std::isfinite(projectEndMile) ||
			!std::isfinite(projectLength) || projectLength <= 0.0)
		{
			if (errorMessage) *errorMessage = QStringLiteral("SETTING_INFO 中的工程起点、终点或总长度无效，未修改校桩和打标。");
			return false;
		}

		bool hasStart = false;
		bool hasEnd = false;
		if (!hasStandardAnchors(m_vecMileagePile, projectStartMile, projectEndMile,
			projectLength, hasStart, hasEnd))
		{
			if (errorMessage) *errorMessage = QStringLiteral("成果库校桩存在无效数值，未修改校桩和打标。");
			return false;
		}
		const bool anchorsMissing = !hasStart || !hasEnd;

		// 兼容旧版首次入库留下的越界校桩，过滤后通过原备份和事务流程保存。
		// 不过滤打标，也不改变正常校桩的 DMI 偏移判断。
		vector<hnMilePile> finalPiles;
		const double minTrueMile = qMin(projectStartMile, projectEndMile);
		const double maxTrueMile = qMax(projectStartMile, projectEndMile);
		for (const hnMilePile& pile : m_vecMileagePile)
		{
			if (pile.dTrueMile < minTrueMile - 1.0 || pile.dTrueMile > maxTrueMile + 1.0)
			{
				continue;
			}
			finalPiles.push_back(pile);
		}
		const bool outOfRangePilesRemoved = finalPiles.size() != m_vecMileagePile.size();
		ensureStandardAnchors(finalPiles, projectStartMile, projectEndMile, projectLength);
		vector<hnMilePile> normalizedCurrentPiles;
		const bool normalizedCurrentChanged = normalizeMilePilesAfterOffset(finalPiles,
			projectStartMile, projectEndMile, projectLength, 0.0, normalizedCurrentPiles);
		if (!normalizedCurrentPiles.empty()) finalPiles = normalizedCurrentPiles;

		double databaseOffset = 0.0;
		QString offsetError;
		if (!findDatabaseStartOffset(finalPiles, projectStartMile, databaseOffset, &offsetError))
		{
			if (anchorsMissing)
			{
				QString backupPath;
				QString writeError;
				if (!createResultDbBackup(m_strDbFilePath, backupPath, &writeError) ||
					!m_pDbSqlite->saveRelativeMileageData(nullptr, finalPiles, m_vecMarkInfo,
						false, projectStartMile, projectEndMile, projectLength, &writeError))
				{
					if (errorMessage) *errorMessage = writeError;
					return false;
				}
				m_vecMileagePile = finalPiles;
				initMileList(false);
			}
			if (errorMessage) *errorMessage = offsetError;
			return false;
		}

		vector<hnMilePile> sourcePiles;
		double sourceOffset = 0.0;
		bool sourceHasStartPile = false;
		QString sourceError;
		const bool needSource = anchorsMissing || databaseOffset > 0.001 || m_vecMarkInfo.empty();
		const bool sourceValid = needSource && readFieldMilePileSource(
			m_p2DProject->getMileStoneCaliInfoFilePath(), projectStartMile,
			projectEndMile, sourcePiles, sourceOffset, &sourceHasStartPile, &sourceError);
		bool replaceMarks = false;
		vector<hnMarkInfo> finalMarks = m_vecMarkInfo;

		if (databaseOffset > 0.001)
		{
			if (!sourceValid || (sourceHasStartPile && qAbs(sourceOffset - databaseOffset) > 0.001))
			{
				QString anchorBackupPath;
				QString anchorWriteError;
				if (anchorsMissing)
				{
					if (!createResultDbBackup(m_strDbFilePath, anchorBackupPath, &anchorWriteError) ||
						!m_pDbSqlite->saveRelativeMileageData(nullptr, finalPiles, m_vecMarkInfo,
							false, projectStartMile, projectEndMile, projectLength, &anchorWriteError))
					{
						if (errorMessage) *errorMessage = anchorWriteError;
						return false;
					}
					m_vecMileagePile = finalPiles;
					initMileList(false);
				}
				if (errorMessage)
				{
					const QString repairError = sourceValid
						? QStringLiteral("成果库起点偏移与 MileStoneCaliInfo.txt 起点偏移不一致，未自动清空重建。")
						: sourceError;
					*errorMessage = anchorsMissing
						? QStringLiteral("已补齐标准起终点校桩，但未修复非零起点偏移。%1").arg(repairError)
						: repairError;
				}
				return false;
			}
			if (!buildRelativeMilePilesFromField(sourcePiles, sourceOffset,
				projectStartMile, projectEndMile, projectLength, finalPiles, errorMessage) ||
				!buildRelativeMarksFromField(m_p2DProject, this, sourceOffset,
					projectStartMile, projectEndMile, projectLength, finalMarks, errorMessage))
			{
				return false;
			}
			replaceMarks = true;
		}
		else if (anchorsMissing && sourceValid)
		{
			vector<hnMilePile> relativeSourcePiles;
			if (!buildRelativeMilePilesFromField(sourcePiles, sourceOffset,
				projectStartMile, projectEndMile, projectLength, relativeSourcePiles, errorMessage))
			{
				return false;
			}
			if (sourceOffset > 0.001)
			{
				finalPiles = relativeSourcePiles;
				if (!buildRelativeMarksFromField(m_p2DProject, this, sourceOffset,
					projectStartMile, projectEndMile, projectLength, finalMarks, errorMessage))
				{
					return false;
				}
				replaceMarks = true;
			}
			else
			{
				int nextId = static_cast<int>(finalPiles.size());
				for (const hnMilePile& pile : relativeSourcePiles)
					appendMilePile(finalPiles, pile.dTrueMile, pile.dEnclMile, nextId);
				normalizeMilePilesAfterOffset(finalPiles, projectStartMile,
					projectEndMile, projectLength, 0.0, normalizedCurrentPiles);
				if (!normalizedCurrentPiles.empty()) finalPiles = normalizedCurrentPiles;
			}
		}

		if (!replaceMarks && m_vecMarkInfo.empty() && sourceValid &&
			hasValidMarkSource(m_p2DProject->getMarkFilePath()))
		{
			if (!buildRelativeMarksFromField(m_p2DProject, this, sourceOffset,
				projectStartMile, projectEndMile, projectLength, finalMarks, errorMessage))
			{
				return false;
			}
			replaceMarks = true;
		}

		const bool pilesChanged = outOfRangePilesRemoved || anchorsMissing || normalizedCurrentChanged ||
			databaseOffset > 0.001 || (anchorsMissing && sourceValid);
		if (!pilesChanged && !replaceMarks) return true;

		QString backupPath;
		if (!createResultDbBackup(m_strDbFilePath, backupPath, errorMessage)) return false;
		QString writeError;
		if (!m_pDbSqlite->saveRelativeMileageData(nullptr, finalPiles, finalMarks,
			replaceMarks, projectStartMile, projectEndMile, projectLength, &writeError))
		{
			if (errorMessage)
				*errorMessage = QStringLiteral("校桩与打标修复失败，数据库事务已经回滚。备份：%1\n%2")
					.arg(backupPath).arg(writeError);
			return false;
		}

		m_vecMileagePile.clear();
		m_vecMarkInfo.clear();
		if (!m_pDbSqlite->m_milePileTable.readData(m_vecMileagePile) ||
			!m_pDbSqlite->m_markerInfoTable.readData(m_vecMarkInfo))
		{
			if (errorMessage)
				*errorMessage = QStringLiteral("数据库修复完成，但重新加载校桩或打标失败。请关闭工程后重新打开。备份：%1")
					.arg(backupPath);
			return false;
		}
		initMileList(false);
		return true;
	}

	// 用户主动从外业文本完整重建校桩和打标，重复执行仍由同一原始数据生成相同结果。
	bool hnProject::reimportMileagePilesAndMarks(QString* backupPath, int* pileCount,
		int* markCount, QString* errorMessage)
	{
		if (!m_pDbSqlite || !m_pDbSqlite->isOpen() || !m_p2DProject)
		{
			if (errorMessage) *errorMessage = QStringLiteral("当前工程不支持重新导入校桩打标。");
			return false;
		}

		const double projectStartMile = m_projectInfo.dBegMile;
		const double projectEndMile = m_projectInfo.dEndMile;
		const double projectLength = getProjectDmiLength(m_projectInfo);
		vector<hnMilePile> sourcePiles;
		double sourceOffset = 0.0;
		if (!readFieldMilePileSource(m_p2DProject->getMileStoneCaliInfoFilePath(),
			projectStartMile, projectEndMile, sourcePiles, sourceOffset,
			nullptr, errorMessage))
		{
			return false;
		}

		vector<hnMilePile> finalPiles;
		vector<hnMarkInfo> finalMarks;
		if (!buildRelativeMilePilesFromField(sourcePiles, sourceOffset,
			projectStartMile, projectEndMile, projectLength, finalPiles, errorMessage) ||
			!buildRelativeMarksFromField(m_p2DProject, this, sourceOffset,
				projectStartMile, projectEndMile, projectLength, finalMarks, errorMessage))
		{
			return false;
		}

		QString createdBackupPath;
		if (!createResultDbBackup(m_strDbFilePath, createdBackupPath, errorMessage)) return false;
		QString writeError;
		if (!m_pDbSqlite->saveRelativeMileageData(nullptr, finalPiles, finalMarks,
			true, projectStartMile, projectEndMile, projectLength, &writeError))
		{
			if (errorMessage)
				*errorMessage = QStringLiteral("重新导入失败，数据库事务已经回滚。备份：%1\n%2")
					.arg(createdBackupPath).arg(writeError);
			return false;
		}

		m_vecMileagePile = finalPiles;
		m_vecMarkInfo = finalMarks;
		initMileList(false);
		if (backupPath) *backupPath = createdBackupPath;
		if (pileCount) *pileCount = static_cast<int>(finalPiles.size());
		if (markCount) *markCount = static_cast<int>(finalMarks.size());
		return true;
	}

	// 预览工程范围变化将移除的普通校桩和打标数量，标准锚点不计入删除数。
	bool hnProject::previewProjectRangeChange(const hnCommon::hnProjectSetInfo& settings,
		int& removedPileCount, int& removedMarkCount, QString* errorMessage) const
	{
		removedPileCount = 0;
		removedMarkCount = 0;
		if (settings.dBegMile == settings.dEndMile ||
			(settings.nLineType == 1 && settings.dBegMile > settings.dEndMile) ||
			(settings.nLineType != 1 && settings.dBegMile < settings.dEndMile))
		{
			if (errorMessage) *errorMessage = QStringLiteral("工程起点、终点与行驶方向不一致。");
			return false;
		}

		const double minTrueMile = qMin(settings.dBegMile, settings.dEndMile);
		const double maxTrueMile = qMax(settings.dBegMile, settings.dEndMile);
		const double projectLength = getProjectDmiLength(m_projectInfo);
		for (const hnMilePile& pile : m_vecMileagePile)
		{
			if (isStandardAnchorPile(pile)) continue;
			if (pile.dTrueMile < minTrueMile - 0.001 || pile.dTrueMile > maxTrueMile + 0.001 ||
				pile.dEnclMile < -0.001 || pile.dEnclMile > projectLength + 0.001 ||
				qAbs(pile.dTrueMile - settings.dBegMile) <= 1.0 ||
				qAbs(pile.dTrueMile - settings.dEndMile) <= 1.0)
			{
				++removedPileCount;
			}
		}
		for (const hnMarkInfo& mark : m_vecMarkInfo)
		{
			if (mark.dTrueMile < minTrueMile - 0.001 || mark.dTrueMile > maxTrueMile + 0.001 ||
				mark.dEnclMile < -0.001 || mark.dEnclMile > projectLength + 0.001)
			{
				++removedMarkCount;
			}
		}
		return true;
	}

	// 工程范围修改与锚点更新、越界校桩及打标删除在同一个事务中完成。
	bool hnProject::updateProjectSettings(const hnCommon::hnProjectSetInfo& settings, QString* errorMessage)
	{
		if (!m_pDbSqlite || !m_pDbSqlite->isOpen())
		{
			if (errorMessage) *errorMessage = QStringLiteral("成果数据库不可用。");
			return false;
		}
		int removedPileCount = 0;
		int removedMarkCount = 0;
		if (!previewProjectRangeChange(settings, removedPileCount, removedMarkCount, errorMessage))
			return false;
		if (strlen(settings.strRoadStandard) == 0 || strlen(settings.strRoadLevel) == 0)
		{
			if (errorMessage) *errorMessage = QStringLiteral("道路标准和道路等级不能为空。");
			return false;
		}

		hnCommon::hnProjectSetInfo candidate = settings;
		const double projectLength = getProjectDmiLength(m_projectInfo);
		candidate.dBegEnclMile = 0.0;
		candidate.dEndEnclMile = projectLength;
		candidate.dLength = m_projectInfo.dLength > 0.0 ? m_projectInfo.dLength : projectLength;
		const double minTrueMile = qMin(candidate.dBegMile, candidate.dEndMile);
		const double maxTrueMile = qMax(candidate.dBegMile, candidate.dEndMile);

		vector<hnMilePile> finalPiles;
		int nextPileId = 0;
		appendMilePile(finalPiles, candidate.dBegMile, 0.0, nextPileId);
		appendMilePile(finalPiles, candidate.dEndMile, projectLength, nextPileId);
		for (const hnMilePile& pile : m_vecMileagePile)
		{
			if (isStandardAnchorPile(pile) ||
				pile.dTrueMile < minTrueMile - 0.001 || pile.dTrueMile > maxTrueMile + 0.001 ||
				pile.dEnclMile < -0.001 || pile.dEnclMile > projectLength + 0.001 ||
				qAbs(pile.dTrueMile - candidate.dBegMile) <= 1.0 ||
				qAbs(pile.dTrueMile - candidate.dEndMile) <= 1.0)
			{
				continue;
			}
			hnMilePile retainedPile = pile;
			retainedPile.nID = nextPileId;
			if (!containsSameMilePile(finalPiles, retainedPile.dTrueMile, retainedPile.dEnclMile))
			{
				finalPiles.push_back(retainedPile);
				++nextPileId;
			}
		}

		vector<hnMarkInfo> finalMarks;
		for (const hnMarkInfo& mark : m_vecMarkInfo)
		{
			if (mark.dTrueMile < minTrueMile - 0.001 || mark.dTrueMile > maxTrueMile + 0.001 ||
				mark.dEnclMile < -0.001 || mark.dEnclMile > projectLength + 0.001)
			{
				continue;
			}
			hnMarkInfo retainedMark = mark;
			retainedMark.nID = static_cast<int>(finalMarks.size());
			finalMarks.push_back(retainedMark);
		}

		QString backupPath;
		if ((qAbs(candidate.dBegMile - m_projectInfo.dBegMile) > 0.001 ||
			qAbs(candidate.dEndMile - m_projectInfo.dEndMile) > 0.001) &&
			!createResultDbBackup(m_strDbFilePath, backupPath, errorMessage))
		{
			return false;
		}

		QString writeError;
		if (!m_pDbSqlite->saveRelativeMileageData(&candidate, finalPiles, finalMarks,
			true, candidate.dBegMile, candidate.dEndMile, projectLength, &writeError))
		{
			if (errorMessage)
				*errorMessage = backupPath.isEmpty() ? writeError :
					QStringLiteral("工程范围更新失败，数据库事务已经回滚。备份：%1\n%2")
						.arg(backupPath).arg(writeError);
			return false;
		}

		m_projectInfo = candidate;
		m_vecMileagePile = finalPiles;
		m_vecMarkInfo = finalMarks;
		initMileList(false);
		return true;
	}

	bool hnProject::ensureInitialSurfaceMaterial(QWidget* parent, QString* errorMessage)
	{
		const double startMile = m_projectInfo.dBegMile;
		for (const hnCommon::hnMarkInfo& mark : m_vecMarkInfo) if (mark.nType == 0 && mark.dGpsTimer == -2 && qAbs(mark.dTrueMile - startMile) < 0.01 && strlen(mark.strMark) > 0) return true;
		const QStringList materials = QStringList() << QString::fromUtf8("\346\262\245\351\235\222") << QString::fromUtf8("\346\260\264\346\263\245") << QString::fromUtf8("\347\240\202\347\237\263");
		if (materials.isEmpty()) { if (errorMessage) *errorMessage = QStringLiteral("未配置允许的路面材质."); return false; }
		bool accepted = false; const QString material = QInputDialog::getItem(parent, QStringLiteral("初始化路面材质"), QStringLiteral("选择工程初始道路材质之后绘制病害..."), materials, 0, false, &accepted);
		if (!accepted || material.isEmpty()) { if (errorMessage) *errorMessage = QStringLiteral("在绘制病害之前必须确认初始路面材料"); return false; }
		hnCommon::hnMarkInfo mark; mark.nType = 0; mark.dTrueMile = startMile; mark.dEnclMile = trueMileToEncl(startMile); mark.dGpsTimer = -2; strncpy_s(mark.strMark, material.toLocal8Bit().constData(), _TRUNCATE);
		bool needUpdate = false;
		return addMark(mark, &needUpdate, errorMessage);
	}}
