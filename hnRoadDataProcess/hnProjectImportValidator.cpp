#include "hnProjectImportValidator.h"

#include "..\hnCommon\hnRoadTypeDef.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QSettings>
#include <QSet>
#include <QTextCodec>
#include <QTextStream>
#include <algorithm>

namespace
{
	const qint64 kLargeProjectImageCount = 500000;
	const qint64 kLargeBatchImageCount = 1000000;
	const int kImageChunkFrameCount = 1000;
}

ProjectImportValidationResult::ProjectImportValidationResult()
	: imageFileCount(0)
{
}

bool ProjectImportValidationResult::isValid() const
{
	return errors.isEmpty();
}

ProjectImportValidationResult hnProjectImportValidator::validate(
	const std::vector<hnCommon::hnProjectDataInfo>& projects,
	const ProgressCallback& progressCallback) const
{
	ProjectImportValidationResult result;
	const int projectCount = static_cast<int>(projects.size());
	for (int projectIndex = 0; projectIndex < projectCount; ++projectIndex)
	{
		const hnCommon::hnProjectDataInfo& project = projects.at(projectIndex);
		const QString projectName = projectDisplayName(project, projectIndex);
		if (progressCallback)
		{
			progressCallback(projectIndex + 1, projectCount, projectName, result.imageFileCount);
		}

		const QString rootPath = QString::fromLocal8Bit(project.strProjectPath);
		validateRequiredDirectory(projectName, QStringLiteral("工程根目录"), rootPath, result);
		if (QFileInfo(rootPath).isDir() && !QFileInfo(rootPath).isWritable())
		{
			addError(result, projectName,
				QStringLiteral("工程根目录不可写，软件无法创建或更新成果数据库。"), rootPath);
		}

		const int workType = project.proSetInfo.nWorkType;
		if (workType == hnCommon::PROJECT_2D_TYPE || workType == hnCommon::PROJECT_23D_TYPE)
		{
			validateTwoDProject(project, projectIndex, projectCount, result, progressCallback);
		}
		if (workType != hnCommon::PROJECT_2D_TYPE)
		{
			validateThreeDProject(project, projectIndex, result);
		}
	}

	if (result.imageFileCount > kLargeBatchImageCount)
	{
		result.warnings.append(QStringLiteral("本次共检测到 %1 张二维图片。多工程会同时保留图片索引，初始化时间和内存占用可能较高，建议按批次导入。")
			.arg(result.imageFileCount));
	}
	return result;
}

QString hnProjectImportValidator::projectDisplayName(
	const hnCommon::hnProjectDataInfo& project,
	int projectIndex) const
{
	QString name = QString::fromLocal8Bit(project.str2DProName).trimmed();
	if (name.isEmpty())
	{
		name = QString::fromLocal8Bit(project.str3DProName).trimmed();
	}
	if (name.isEmpty())
	{
		name = QString::fromLocal8Bit(project.strProJectName).trimmed();
	}
	return name.isEmpty() ? QStringLiteral("未命名工程_%1").arg(projectIndex + 1) : name;
}

QString hnProjectImportValidator::twoDProjectPath(const hnCommon::hnProjectDataInfo& project) const
{
	const QString rootPath = QString::fromLocal8Bit(project.strProjectPath);
	if (project.proSetInfo.nWorkType == hnCommon::PROJECT_2D_TYPE)
	{
		return QDir::cleanPath(rootPath);
	}
	return QDir(rootPath).filePath(QString::fromLocal8Bit(project.str2DProName));
}

QString hnProjectImportValidator::threeDProjectPath(const hnCommon::hnProjectDataInfo& project) const
{
	const QString rootPath = QString::fromLocal8Bit(project.str3dProjectPath);
	return QDir(rootPath).filePath(QString::fromLocal8Bit(project.str3DProName));
}

void hnProjectImportValidator::validateRequiredDirectory(
	const QString& projectName,
	const QString& description,
	const QString& path,
	ProjectImportValidationResult& result) const
{
	const QFileInfo info(path);
	if (!info.exists() || !info.isDir() || !info.isReadable())
	{
		addError(result, projectName, QStringLiteral("%1不存在或无法读取。").arg(description), path);
	}
}

void hnProjectImportValidator::validateRequiredFile(
	const QString& projectName,
	const QString& description,
	const QString& path,
	ProjectImportValidationResult& result) const
{
	const QFileInfo info(path);
	if (!info.exists() || !info.isFile() || !info.isReadable())
	{
		addError(result, projectName, QStringLiteral("%1不存在或无法读取。").arg(description), path);
	}
}

void hnProjectImportValidator::validateTwoDProject(
	const hnCommon::hnProjectDataInfo& project,
	int projectIndex,
	int projectCount,
	ProjectImportValidationResult& result,
	const ProgressCallback& progressCallback) const
{
	const QString projectName = projectDisplayName(project, projectIndex);
	const QString projectPath = twoDProjectPath(project);
	validateRequiredDirectory(projectName, QStringLiteral("二维工程目录"), projectPath, result);
	if (!QFileInfo(projectPath).isDir())
	{
		return;
	}

	const QString settingPath = QDir(projectPath).filePath(QStringLiteral("Setting.ini"));
	validateRequiredFile(projectName, QStringLiteral("二维配置文件 Setting.ini"), settingPath, result);
	validateRequiredFile(projectName, QStringLiteral("二维工程信息文件 ProjectInfo.txt"),
		QDir(projectPath).filePath(QStringLiteral("ProjectInfo.txt")), result);

	if (QFileInfo(settingPath).isReadable())
	{
		QSettings settings(settingPath, QSettings::IniFormat);
		bool hasWorkMode = settings.childGroups().contains(QStringLiteral("WorkMode"));
		if (!hasWorkMode)
		{
			QFile settingFile(settingPath);
			if (settingFile.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				QTextCodec* gbCodec = QTextCodec::codecForName("GB2312");
				const QString content = gbCodec ? gbCodec->toUnicode(settingFile.readAll()) : QString();
				hasWorkMode = content.contains(QStringLiteral("工作模式"));
			}
		}
		if (!hasWorkMode)
		{
			addError(result, projectName,
				QStringLiteral("Setting.ini 缺少 WorkMode/工作模式 配置组，设备类型无法安全初始化。"),
				settingPath);
		}

		settings.beginGroup(QStringLiteral("Parm"));
		int roadDistance = settings.value(QStringLiteral("RoadDis"), 2).toInt();
		int leftStreetDistance = settings.value(QStringLiteral("StreetDis"), 20).toInt();
		int rightStreetDistance = settings.value(QStringLiteral("StreetDis2"), leftStreetDistance).toInt();
		settings.endGroup();
		if (roadDistance <= 0)
		{
			roadDistance = 2;
		}
		if (leftStreetDistance <= 0)
		{
			leftStreetDistance = 20;
		}
		if (rightStreetDistance <= 0)
		{
			rightStreetDistance = leftStreetDistance;
		}
		if (leftStreetDistance < roadDistance || rightStreetDistance < roadDistance)
		{
			addError(result, projectName,
				QStringLiteral("Setting.ini 的景观采样间距不能小于路面采样间距，否则加载里程索引时会发生整数除零。当前 RoadDis=%1，StreetDis=%2，StreetDis2=%3。")
				.arg(roadDistance).arg(leftStreetDistance).arg(rightStreetDistance), settingPath);
		}
	}

	const QString markPath = QDir(projectPath).filePath(QStringLiteral("RoadStatuMarkInfo.txt"));
	QFile markFile(markPath);
	if (markFile.exists() && markFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream markStream(&markFile);
		markStream.setCodec(QTextCodec::codecForName("UTF-8"));
		int lineNumber = 0;
		while (!markStream.atEnd())
		{
			++lineNumber;
			const QString line = markStream.readLine();
			const QStringList fields = line.simplified().split(QStringLiteral(" "), QString::SkipEmptyParts);
			if (fields.size() >= 4 && !fields.at(3).contains(QChar(':')) &&
				!fields.at(3).contains(QStringLiteral("：")))
			{
				addError(result, projectName,
					QStringLiteral("RoadStatuMarkInfo.txt 第 %1 行标记内容缺少冒号分隔符，旧解析逻辑会发生越界访问。")
					.arg(lineNumber), markPath);
				break;
			}
		}
	}

	qint64 projectImageCount = 0;
	const QStringList channels = QStringList()
		<< QStringLiteral("RoadImg/Camera0")
		<< QStringLiteral("StreetImg/Camera0")
		<< QStringLiteral("StreetImg/Camera1")
		<< QStringLiteral("StreetImg2/Camera0");
	for (const QString& relativeChannel : channels)
	{
		const QString channelPath = QDir(projectPath).filePath(relativeChannel);
		if (!QFileInfo(channelPath).isDir())
		{
			continue;
		}
		projectImageCount += validateImageChannel(projectName, channelPath, projectIndex,
			projectCount, projectImageCount, result, progressCallback);
	}
	result.imageFileCount += projectImageCount;
	if (projectImageCount > kLargeProjectImageCount)
	{
		result.warnings.append(QStringLiteral("工程【%1】包含 %2 张二维图片，初始化可能耗时较长并占用较多内存。")
			.arg(projectName).arg(projectImageCount));
	}
}

void hnProjectImportValidator::validateThreeDProject(
	const hnCommon::hnProjectDataInfo& project,
	int projectIndex,
	ProjectImportValidationResult& result) const
{
	const QString projectName = projectDisplayName(project, projectIndex);
	const QString projectPath = threeDProjectPath(project);
	validateRequiredDirectory(projectName, QStringLiteral("三维工程目录"), projectPath, result);
	if (!QFileInfo(projectPath).isDir())
	{
		return;
	}

	const QString mmsCameraPath = QDir(projectPath).filePath(QStringLiteral("PointCloud/1/Mms-Cam-1.cam"));
	const QString iScanCameraPath = QDir(projectPath).filePath(QStringLiteral("PointCloud/1/iScan-Cam-1.cam"));
	if (!QFileInfo(mmsCameraPath).isReadable() && !QFileInfo(iScanCameraPath).isReadable())
	{
		addError(result, projectName,
			QStringLiteral("三维点云相机文件缺失，Mms-Cam-1.cam 与 iScan-Cam-1.cam 至少需要一个。"),
			mmsCameraPath + QStringLiteral("\n") + iScanCameraPath);
	}
	const QString pavementIndexPath =
		QDir(projectPath).filePath(QStringLiteral("Image/Pavement-cam-1.idx"));
	const QFileInfo pavementIndexInfo(pavementIndexPath);
	if (!pavementIndexInfo.exists() || !pavementIndexInfo.isFile() || !pavementIndexInfo.isReadable())
	{
		if (project.proSetInfo.nWorkType == hnCommon::PROJECT_23D_TYPE)
		{
			// 二三维工程缺少三维影像索引时保留二维能力，不阻断整个批次导入。
			addNotice(result, projectName,
				QStringLiteral("三维影像索引 Pavement-cam-1.idx 不存在或无法读取，三维影像功能可能无法使用，但工程仍可导入并使用二维功能。"),
				pavementIndexPath);
		}
		else
		{
			addError(result, projectName,
				QStringLiteral("三维影像索引 Pavement-cam-1.idx 不存在或无法读取。"),
				pavementIndexPath);
		}
	}
}

qint64 hnProjectImportValidator::validateImageChannel(
	const QString& projectName,
	const QString& channelPath,
	int projectIndex,
	int projectCount,
	qint64 currentProjectImageCount,
	ProjectImportValidationResult& result,
	const ProgressCallback& progressCallback) const
{
	const QRegExp chunkPattern(QStringLiteral("^Image_(\\d+)$"));
	const QRegExp serialPattern(QStringLiteral("^(\\d+)_"));
	const QFileInfoList directories = QDir(channelPath).entryInfoList(
		QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::Name);
	QSet<int> chunkIndexes;
	qint64 imageCount = 0;
	for (const QFileInfo& directoryInfo : directories)
	{
		const QString directoryName = directoryInfo.fileName();
		if (!directoryName.startsWith(QStringLiteral("Image_")))
		{
			continue;
		}
		if (!chunkPattern.exactMatch(directoryName))
		{
			addError(result, projectName, QStringLiteral("图片分包目录名称格式错误，应为 Image_数字。"),
				directoryInfo.absoluteFilePath());
			continue;
		}
		bool chunkOk = false;
		const int chunkIndex = chunkPattern.cap(1).toInt(&chunkOk);
		if (!chunkOk || chunkIndexes.contains(chunkIndex))
		{
			addError(result, projectName, QStringLiteral("图片分包序号无效或重复。"), directoryInfo.absoluteFilePath());
			continue;
		}
		chunkIndexes.insert(chunkIndex);

		const QFileInfoList imageFiles = QDir(directoryInfo.absoluteFilePath()).entryInfoList(
			QStringList() << QStringLiteral("*.jpg") << QStringLiteral("*.jpeg"),
			QDir::Files, QDir::Name);
		for (const QFileInfo& imageInfo : imageFiles)
		{
			++imageCount;
			const QString fileName = imageInfo.fileName();
			if (serialPattern.indexIn(fileName) != 0)
			{
				addError(result, projectName, QStringLiteral("图片文件名缺少数字序号前缀和下划线。"),
					imageInfo.absoluteFilePath());
				continue;
			}
			bool serialOk = false;
			const QString serialText = serialPattern.cap(1);
			const int localSerial = serialText.toInt(&serialOk);
			if (!serialOk || localSerial < 0 || localSerial >= kImageChunkFrameCount)
			{
				addError(result, projectName, QStringLiteral("图片局部序号必须在 0 到 999 之间。"),
					imageInfo.absoluteFilePath());
				continue;
			}
			bool combinedOk = false;
			const qint64 combinedIndex = chunkPattern.cap(1).append(serialText).toLongLong(&combinedOk);
			const qint64 expectedIndex = static_cast<qint64>(chunkIndex) * kImageChunkFrameCount + localSerial;
			if (!combinedOk || combinedIndex != expectedIndex)
			{
				addError(result, projectName,
					QStringLiteral("图片序号位数与分包不匹配，旧加载逻辑会产生异常大的补位索引。"),
					imageInfo.absoluteFilePath());
				continue;
			}
			if (progressCallback && ((currentProjectImageCount + imageCount) % 1000 == 0))
			{
				progressCallback(projectIndex + 1, projectCount, projectName,
					result.imageFileCount + currentProjectImageCount + imageCount);
			}
		}
	}

	if (!chunkIndexes.isEmpty())
	{
		const int maximumChunk = *std::max_element(chunkIndexes.constBegin(), chunkIndexes.constEnd());
		for (int expectedChunk = 0; expectedChunk <= maximumChunk; ++expectedChunk)
		{
			if (!chunkIndexes.contains(expectedChunk))
			{
				addError(result, projectName,
					QStringLiteral("图片分包不连续，缺少 Image_%1。")
					.arg(expectedChunk, 4, 10, QLatin1Char('0')), channelPath);
				break;
			}
		}
	}
	return imageCount;
}

void hnProjectImportValidator::addError(
	ProjectImportValidationResult& result,
	const QString& projectName,
	const QString& message,
	const QString& path) const
{
	QString text = QStringLiteral("工程【%1】：%2").arg(projectName, message);
	if (!path.isEmpty())
	{
		text += QStringLiteral("\n路径：%1").arg(QDir::toNativeSeparators(path));
	}
	if (!result.errors.contains(text))
	{
		result.errors.append(text);
	}
}

void hnProjectImportValidator::addNotice(
	ProjectImportValidationResult& result,
	const QString& projectName,
	const QString& message,
	const QString& path) const
{
	QString text = QStringLiteral("工程【%1】：%2").arg(projectName, message);
	if (!path.isEmpty())
	{
		text += QStringLiteral("\n路径：%1").arg(QDir::toNativeSeparators(path));
	}
	if (!result.notices.contains(text))
	{
		result.notices.append(text);
	}
}
