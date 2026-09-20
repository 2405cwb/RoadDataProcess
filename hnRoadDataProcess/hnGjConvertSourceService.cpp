#include "hnGjConvertSourceService.h"

#include <algorithm>
#include <cmath>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QRegExp>
#include <QSaveFile>
#include <QScopedValueRollback>
#include <QSet>
#include <QSettings>
#include <QStorageInfo>
#include <QTextCodec>
#include <QTextStream>
#include <QThread>
#include <QUuid>

#include "hnOutExcelMileManage.h"
#include "..\hnConfigService\HnXRSettings.h"
#include "..\hnProject\hn2DProject.h"
#include "..\hnProject\hnProject.h"
#include "..\hnQtCommon\EquipmentList.h"

namespace
{
	struct ExportContext
	{
		hnPro::hnProject* project;
		QString projectName;
		QString basePath;
		QString routeName;
		QString timeText;
		QString dateText;
		QString outputPath;
		HnProjectEnums::StandardParmTypeEnum standard;
		hnGjExportStandard exportStandard;
		hnGjOutputSelection outputSelection;
		int drawType;
		int direction;
		double startMile;
		double endMile;
		bool hasGeometryResult;
		int taskCount;
		int sourceIndex;
		QStringList preflightWarnings;
		QStringList skippedOutputs;
	};

	QString digitsOnly(const QString& value)
	{
		QString result;
		for (const QChar ch : value)
		{
			if (ch.isDigit())
			{
				result.append(ch);
			}
		}
		return result;
	}

	QString fixed(double value, int decimals)
	{
		return QString::number(value, 'f', decimals);
	}

	QString compact(double value)
	{
		return QString::number(value, 'g', 15);
	}

	QString startMileText(const ExportContext& context)
	{
		return fixed(context.startMile * 0.001, 3);
	}

	QString dataFileName(const ExportContext& context, const QString& type)
	{
		return QStringLiteral("%1-%2-%3-%4.txt")
			.arg(context.routeName, type, startMileText(context), context.timeText);
	}

	bool openUtf8TextFile(QFile& file, const QString& path, QString& errorMessage)
	{
		file.setFileName(path);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			errorMessage = QStringLiteral("无法写入文件：%1\n%2").arg(path, file.errorString());
			return false;
		}
		return true;
	}

	bool writeUtf8Line(QFile& file, const QString& line, QString& errorMessage)
	{
		const QByteArray data = line.toUtf8() + "\r\n";
		if (file.write(data) != data.size())
		{
			errorMessage = QStringLiteral("写入文件失败：%1\n%2").arg(file.fileName(), file.errorString());
			return false;
		}
		return true;
	}

	bool createDirectory(const QString& path, QString& errorMessage)
	{
		if (!QDir().mkpath(path))
		{
			errorMessage = QStringLiteral("无法创建目录：%1").arg(path);
			return false;
		}
		return true;
	}

	QString decodeProjectInfo(const QByteArray& data)
	{
		QString text = QString::fromUtf8(data);
		if (text.contains(QChar::ReplacementCharacter))
		{
			QTextCodec* codec = QTextCodec::codecForLocale();
			text = codec ? codec->toUnicode(data) : text;
		}
		return text;
	}

	QString projectInfoValue(const QString& text, const QStringList& keys)
	{
		const QStringList lines = text.split(QRegExp(QStringLiteral("[\\r\\n]+")), QString::SkipEmptyParts);
		for (const QString& rawLine : lines)
		{
			const QString line = rawLine.trimmed();
			for (const QString& key : keys)
			{
				if (!line.startsWith(key))
				{
					continue;
				}
				int separator = line.indexOf(QChar(0xFF1A));
				if (separator < 0)
				{
					separator = line.indexOf(QLatin1Char(':'));
				}
				if (separator >= 0)
				{
					return line.mid(separator + 1).trimmed();
				}
			}
		}
		return QString();
	}

	QString readProjectInfoText(hnPro::hnProject* project)
	{
		if (!project || !project->get2DProject())
		{
			return QString();
		}
		QFile file(QDir(project->get2DProject()->getBasePath()).filePath(QStringLiteral("ProjectInfo.txt")));
		if (!file.open(QIODevice::ReadOnly))
		{
			return QString();
		}
		return decodeProjectInfo(file.readAll());
	}

	QString routeCodeFromProject(hnPro::hnProject* project, const QString& projectInfo)
	{
		QString routeCode = projectInfoValue(projectInfo,
			QStringList() << QStringLiteral("检测公路路线编号") << QStringLiteral("检测公路编号")
				<< QStringLiteral("公路路线编号") << QStringLiteral("路线编号")
				<< QStringLiteral("公路编号") << QStringLiteral("路线代码"));
		if (routeCode.isEmpty() && project && project->get2DProject())
		{
			routeCode = project->get2DProject()->_RoadCode.trimmed();
		}
		return routeCode.trimmed();
	}

	QString countyCodeFromText(const QString& projectInfo)
	{
		QString code = projectInfoValue(projectInfo,
			QStringList() << QStringLiteral("县级行政区划代码") << QStringLiteral("县级行政代码")
				<< QStringLiteral("行政区划代码") << QStringLiteral("县级代码"));
		return digitsOnly(code);
	}

	bool validateReadableFile(const QString& projectName, const QString& description,
		const QString& path, QStringList& errors)
	{
		const QFileInfo info(path);
		if (info.isFile() && info.isReadable() && info.size() > 0)
		{
			return true;
		}
		errors.append(QStringLiteral("工程【%1】：%2缺失、为空或不可读。\n%3")
			.arg(projectName, description, path));
		return false;
	}

	bool isReadableDataFile(const QString& path)
	{
		const QFileInfo info(path);
		return info.isFile() && info.isReadable() && info.size() > 0;
	}

	bool hasUsableGeometryResult(const QString& basePath)
	{
		QFile file(QDir(basePath).filePath(QStringLiteral("Geoalig_10m.txt")));
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			return false;
		}
		QTextStream stream(&file);
		while (!stream.atEnd())
		{
			const QStringList fields = stream.readLine().trimmed().split(
				QLatin1Char(','), QString::KeepEmptyParts);
			if (fields.size() < 4)
			{
				continue;
			}
			bool dmiValid = false;
			bool curvatureValid = false;
			bool longitudinalValid = false;
			bool crossValid = false;
			const double dmi = fields.at(0).toDouble(&dmiValid);
			const double curvature = fields.at(1).toDouble(&curvatureValid);
			const double longitudinal = fields.at(2).toDouble(&longitudinalValid);
			const double cross = fields.at(3).toDouble(&crossValid);
			if (dmiValid && std::isfinite(dmi)
				&& ((curvatureValid && std::isfinite(curvature))
					|| (longitudinalValid && std::isfinite(longitudinal))
					|| (crossValid && std::isfinite(cross))))
			{
				return true;
			}
		}
		return false;
	}

	bool writeDatabaseImageIndex(const ExportContext& context, const QStringList& imagePaths,
		int imageInterval, const QString& cameraPath, const QString& indexFileName,
		QString& errorMessage)
	{
		if (imageInterval <= 0)
		{
			errorMessage = QStringLiteral("工程【%1】：图片采集间隔无效，无法生成 %2：%3 米。")
				.arg(context.projectName, indexFileName).arg(imageInterval);
			return false;
		}
		if (imagePaths.isEmpty())
		{
			errorMessage = QStringLiteral("工程【%1】：图片目录中没有可用于生成 %2 的图片。\n%3")
				.arg(context.projectName, indexFileName, cameraPath);
			return false;
		}

		const QString indexPath = QDir(cameraPath).filePath(indexFileName);
		QSaveFile output(indexPath);
		if (!output.open(QIODevice::WriteOnly))
		{
			errorMessage = QStringLiteral("工程【%1】：无法覆盖图片索引文件：%2\n%3")
				.arg(context.projectName, indexPath, output.errorString());
			return false;
		}

		int writtenCount = 0;
		for (int imageIndex = 0; imageIndex < imagePaths.size(); ++imageIndex)
		{
			const double dmi = static_cast<double>(imageIndex) * imageInterval;
			const int mile = static_cast<int>(std::nearbyint(context.project->enclToTrueMile(dmi)));
			// 保持二维 Image2Mile 的停止规则，避免改变国检转换软件既有图片范围。
			if ((context.direction < 0 && mile <= 0)
				|| (context.direction > 0 && mile >= context.endMile))
			{
				break;
			}

			QString relativePath = QDir(cameraPath).relativeFilePath(imagePaths.at(imageIndex));
			relativePath.replace(QLatin1Char('/'), QLatin1Char('\\'));
			if (!relativePath.startsWith(QLatin1Char('\\')))
			{
				relativePath.prepend(QLatin1Char('\\'));
			}
			const QByteArray line = QStringLiteral("%1 %2\r\n").arg(mile).arg(relativePath).toUtf8();
			if (output.write(line) != line.size())
			{
				errorMessage = QStringLiteral("工程【%1】：写入图片索引失败：%2\n%3")
					.arg(context.projectName, indexPath, output.errorString());
				output.cancelWriting();
				return false;
			}
			++writtenCount;
		}

		if (writtenCount <= 0)
		{
			errorMessage = QStringLiteral("工程【%1】：数据库校桩换算后没有生成任何 %2 记录。\n%3")
				.arg(context.projectName, indexFileName, indexPath);
			output.cancelWriting();
			return false;
		}
		if (!output.commit())
		{
			errorMessage = QStringLiteral("工程【%1】：无法用新索引覆盖 %2。\n%3")
				.arg(context.projectName, indexPath, output.errorString());
			return false;
		}
		return true;
	}

	bool writeDatabaseImageIndexes(const ExportContext& context, QString& errorMessage)
	{
		hnPro::hn2DProject* project2D = context.project->get2DProject();
		if (project2D->_IsRoad
			&& !writeDatabaseImageIndex(context, project2D->getRoadPicturePath(), project2D->_RoadImgDis,
				QDir(context.basePath).filePath(QStringLiteral("RoadImg/Camera0")),
				QStringLiteral("Road2Mile.txt"), errorMessage))
		{
			return false;
		}
		if (project2D->_IsStreet
			&& !writeDatabaseImageIndex(context, project2D->getLeftStreetPicturePath(), project2D->_StreetImgDis,
				QDir(context.basePath).filePath(QStringLiteral("StreetImg/Camera0")),
				QStringLiteral("Street2Mile.txt"), errorMessage))
		{
			return false;
		}
		return true;
	}

	QStringList requiredDirectories(const ExportContext& context)
	{
		if (context.exportStandard == hnGjExportStandard::NationalRoad2026)
		{
			const QString routeWithoutDirection = context.routeName.left(context.routeName.size() - 1);
			QStringList result;
			if (context.outputSelection.pb) result << QDir(QStringLiteral("BUMP")).filePath(context.dateText + QLatin1Char('/') + context.routeName);
			if (context.outputSelection.iri) result << QStringLiteral("GPS");
			result << QDir(QStringLiteral("Images")).filePath(context.dateText + QLatin1Char('/') + context.routeName + QStringLiteral("/0"));
			if (context.outputSelection.iri) result << QDir(QStringLiteral("IRI")).filePath(context.dateText + QLatin1Char('/') + routeWithoutDirection);
			if (context.outputSelection.rd) result << QDir(QStringLiteral("RDFile")).filePath(context.dateText + QLatin1Char('/') + context.routeName);
			result << QDir(QStringLiteral("前方图像")).filePath(context.dateText + QLatin1Char('/') + context.routeName);
			return result;
		}
		QStringList result;
		if (context.outputSelection.dr) result << QStringLiteral("DR");
		if (context.outputSelection.iri) result << QStringLiteral("IRI");
		if (context.outputSelection.lbiFile) result << QStringLiteral("LBIFile");
		if (context.outputSelection.riFile) result << QStringLiteral("RIFile");
		result << QStringLiteral("Images") << QStringLiteral("ViewImages");
		if (context.standard == HnProjectEnums::DegreeRoad2018)
		{
			if (context.outputSelection.rd) result << QStringLiteral("RD");
			if (context.outputSelection.pb) result << QStringLiteral("PB");
			if (context.outputSelection.mpd) result << QStringLiteral("MPD");
			if (context.outputSelection.smtd) result << QStringLiteral("SMTD");
			if (context.outputSelection.ttFile) result << QStringLiteral("TTFile");
			if (context.outputSelection.lFile) result << QStringLiteral("LFile");
			if (context.outputSelection.rdFile) result << QStringLiteral("RDFile");
			if (context.outputSelection.haFile && context.hasGeometryResult)
			{
				result << QStringLiteral("HAFile");
			}
		}
		return result;
	}

	QString rutProfileDirectory(const ExportContext& context)
	{
		const int cameraIndex = context.project->get2DProject()->_RutMode == 1 ? 1 : 0;
		return QDir(context.basePath).filePath(QStringLiteral("RUT/camera%1/data").arg(cameraIndex));
	}

	qint64 rutProfileTaskCount(const ExportContext& context)
	{
		const qint64 frames = qMax<qint64>(1,
			static_cast<qint64>(std::floor(qAbs(context.endMile - context.startMile) / 0.1)));
		return qMax<qint64>(1, (frames + 999) / 1000);
	}

	qint64 textureTaskCount(const ExportContext& context)
	{
		const qint64 points = qMax<qint64>(1,
			qRound64(qAbs(context.endMile - context.startMile) * 1000.0));
		return qMax<qint64>(1, (points + 499999) / 500000);
	}

	bool hasMpdData(const ExportContext& context)
	{
		return QFileInfo(QDir(context.project->get2DProject()->getIRIPath())
			.filePath(QStringLiteral("Laser0/MPD_10m.txt"))).isFile();
	}

	QStringList missingLaserFiles(const ExportContext& context, const QString& name)
	{
		QStringList missing;
		for (int side = 0; side < 3; ++side)
		{
			if (side == 1 && !context.project->get2DProject()->_IsDIRIMTD) continue;
			if (side == 2 && !context.project->get2DProject()->_IsMMTD) continue;
			const QString path = QDir(context.project->get2DProject()->getIRIPath())
				.filePath(QStringLiteral("Laser%1/%2").arg(side).arg(name));
			const QFileInfo info(path);
			if (!info.isFile() || info.size() == 0) missing.append(path);
		}
		return missing;
	}
	qint64 estimatedOutputBytes(const ExportContext& context)
	{
		const double distance = qAbs(context.endMile - context.startMile);
		qint64 estimate = 200LL * 1024LL * 1024LL;
		if (context.exportStandard == hnGjExportStandard::NationalRoad2026)
		{
			return estimate;
		}
		estimate += static_cast<qint64>(distance / 0.1) * 80LL; // LP
		const QString laserPath = QDir(context.basePath).filePath(QStringLiteral("IRIMTD/Laser0/lasval.txt"));
		if (context.standard == HnProjectEnums::DegreeRoad2018 && QFileInfo(laserPath).isFile())
		{
			estimate += static_cast<qint64>(distance * 1000.0) * 70LL; // TT
		}
		const QFileInfoList profiles = QDir(rutProfileDirectory(context)).entryInfoList(
			QStringList() << QStringLiteral("*.dtw"), QDir::Files);
		qint64 profileBytes = 0;
		for (const QFileInfo& profile : profiles) profileBytes += profile.size();
		estimate += profileBytes;
		return estimate;
	}

	bool preflight(const std::vector<hnPro::hnProject*>& projects,
		const QString& batchCountyCode,
		hnGjExportStandard exportStandard,
		const hnGjOutputSelection& requestedSelection,
		QVector<ExportContext>& contexts, QVector<hnGjProjectResult>& rejectedProjects,
		QStringList& errors, const hnGjConvertSourceService::ProgressCallback& callback,
		bool& canceled)
	{
		QSet<QString> routeNames;
		const QString normalizedBatchCountyCode = digitsOnly(batchCountyCode);
		if (normalizedBatchCountyCode.size() != 6)
		{
			errors.append(QStringLiteral("本批次县级行政区划代码无效，必须为 6 位数字：%1")
				.arg(batchCountyCode));
			return false;
		}
		for (size_t index = 0; index < projects.size(); ++index)
		{
			hnPro::hnProject* project = projects[index];
			const QString fallbackName = QStringLiteral("第 %1 个工程").arg(index + 1);
			const QString projectName = project ? project->get2DProName().trimmed() : fallbackName;
			if (callback && !callback(static_cast<int>(index), static_cast<int>(projects.size()),
				QStringLiteral("正在预检工程【%1】（%2/%3）")
					.arg(projectName.isEmpty() ? fallbackName : projectName)
					.arg(index + 1).arg(projects.size())))
			{
				canceled = true;
				return false;
			}
			if (!project || !project->get2DProject())
			{
				hnGjProjectResult rejected;
				rejected.projectName = projectName;
				rejected.sourceIndex = static_cast<int>(index);
				rejected.skipped = true;
				rejected.errorMessage = QStringLiteral("二维子工程对象无效。");
				rejectedProjects.append(rejected);
				continue;
			}
			QStringList projectErrors;

			ExportContext context;
			context.project = project;
			context.sourceIndex = static_cast<int>(index);
			context.projectName = projectName.isEmpty() ? fallbackName : projectName;
			context.basePath = QDir::cleanPath(project->get2DProject()->getBasePath());
			context.outputPath = QDir(context.basePath).filePath(QStringLiteral("ConverSource"));
			context.standard = project->getBaseStandard();
			context.exportStandard = exportStandard;
			hnGjOutputSelection outputSelection = requestedSelection;
            if (context.standard == HnProjectEnums::RuralRoadlowLevel)
            {
                outputSelection.rd = outputSelection.pb = outputSelection.mpd = outputSelection.smtd = false;
                outputSelection.haFile = outputSelection.rdFile = outputSelection.ttFile = outputSelection.lFile = false;
            }
            context.outputSelection = outputSelection;
			context.drawType = project->getCurProSetInfo().nDrawType;
			context.direction = project->getCurProSetInfo().nLineType;
			context.startMile = project->getCurProSetInfo().dBegMile;
			context.endMile = project->getCurProSetInfo().dEndMile;
			context.hasGeometryResult = hasUsableGeometryResult(context.basePath);

			if (!QFileInfo(context.basePath).isDir())
			{
				projectErrors.append(QStringLiteral("工程【%1】：二维工程目录不存在。\n%2")
					.arg(context.projectName, context.basePath));
			}
			const QStorageInfo storage(context.basePath);
			const qint64 requiredBytes = estimatedOutputBytes(context);
			if (!storage.isValid() || !storage.isReady())
			{
				projectErrors.append(QStringLiteral("工程【%1】：无法读取二维工程所在磁盘的可用空间。\n%2")
					.arg(context.projectName, context.basePath));
			}
			else if (storage.bytesAvailable() < requiredBytes)
			{
				projectErrors.append(QStringLiteral("工程【%1】：磁盘空间不足，预计至少需要 %2 MB，当前可用 %3 MB。\n%4")
					.arg(context.projectName)
					.arg(requiredBytes / 1024 / 1024)
					.arg(storage.bytesAvailable() / 1024 / 1024)
					.arg(context.basePath));
			}
			const QString infoPath = QDir(context.basePath).filePath(QStringLiteral("ProjectInfo.txt"));
			validateReadableFile(context.projectName, QStringLiteral("ProjectInfo.txt"), infoPath, projectErrors);
			const QString projectInfo = readProjectInfoText(project);
			const QString routeCode = routeCodeFromProject(project, projectInfo);
			QString projectCountyCode = countyCodeFromText(projectInfo);
			if (projectCountyCode.isEmpty()) projectCountyCode = digitsOnly(project->get2DProject()->_CityCode);
			const QString countyCode = normalizedBatchCountyCode;
			if (routeCode.size() < 4)
			{
				projectErrors.append(QStringLiteral("工程【%1】：路线编号无效，至少需要 4 位。\n%2")
					.arg(context.projectName, infoPath));
			}
			if (!projectCountyCode.isEmpty() && projectCountyCode != countyCode)
			{
				projectErrors.append(QStringLiteral("工程【%1】：工程内县级行政区划代码为 %2，与本批次输入的 %3 不一致。"
					"国检转换一次只能处理同一个县区，请重新选择工程。\n%4")
					.arg(context.projectName).arg(projectCountyCode).arg(countyCode).arg(infoPath));
			}
			if (context.direction != 1 && context.direction != -1)
			{
				projectErrors.append(QStringLiteral("工程【%1】：方向 nLineType=%2 无效，必须为 1 或 -1。")
					.arg(context.projectName).arg(context.direction));
			}
			if (context.standard != HnProjectEnums::DegreeRoad2018
				&& context.standard != HnProjectEnums::RuralRoadlowLevel)
			{
				projectErrors.append(QStringLiteral("工程【%1】：第一期仅支持等级公路 JTG H20-2018 和低等级农村公路。当前规范：%2")
					.arg(context.projectName, HnProjectEnums::roadTypeEnumToQString(context.standard)));
			}
			if (exportStandard == hnGjExportStandard::NationalRoad2026
				&& context.standard != HnProjectEnums::DegreeRoad2018)
			{
				projectErrors.append(QStringLiteral("工程【%1】：农养国省道路况检测数据提交格式_2026年仅支持等级公路工程。当前规范：%2")
					.arg(context.projectName, HnProjectEnums::roadTypeEnumToQString(context.standard)));
			}
			if (context.drawType != hnCommon::ROAD_WORK_LARGE_RECT
				&& context.drawType != hnCommon::ROAD_WORK_SMALL_RECT)
			{
				projectErrors.append(QStringLiteral("工程【%1】：第一期仅支持大框和智能小框，当前绘制模式值为 %2。")
					.arg(context.projectName).arg(context.drawType));
			}
			if (!std::isfinite(context.startMile) || !std::isfinite(context.endMile)
				|| qFuzzyCompare(context.startMile + 1.0, context.endMile + 1.0))
			{
				projectErrors.append(QStringLiteral("工程【%1】：起终点无效，起点=%2，终点=%3。")
					.arg(context.projectName).arg(context.startMile).arg(context.endMile));
			}
			validateReadableFile(context.projectName, QStringLiteral("成果数据库"),
				project->getDbResultFilePath(), projectErrors);
			if (project->getCurrentMileVector().isEmpty())
			{
				projectErrors.append(QStringLiteral("工程【%1】：成果数据库中没有可用于分段的里程数据。\n%2")
					.arg(context.projectName, project->getDbResultFilePath()));
			}
			if (project->getCurrentMilePileVector().isEmpty())
			{
				projectErrors.append(QStringLiteral("工程【%1】：成果数据库中没有校桩信息，无法生成图片 2Mile.txt。\n%2")
					.arg(context.projectName, project->getDbResultFilePath()));
			}
			const bool needIriInput = outputSelection.iri;
			const bool needLpInput = outputSelection.riFile;
			const bool needPbInput = outputSelection.pb;
			const bool needRutProfileSpeed = outputSelection.rdFile;
			const bool needSmtdSpeed = outputSelection.smtd;
			if (!project->get2DProject()->_IsIRIMTD
				&& (needIriInput || needLpInput || needPbInput || needRutProfileSpeed || needSmtdSpeed))
			{
				QStringList affectedOutputs;
				if (outputSelection.iri) affectedOutputs << QStringLiteral("IRI");
				if (outputSelection.riFile) affectedOutputs << QStringLiteral("RIFile");
				if (outputSelection.pb) affectedOutputs << QStringLiteral("PB");
				if (outputSelection.rdFile) affectedOutputs << QStringLiteral("RDFile");
				if (outputSelection.smtd) affectedOutputs << QStringLiteral("SMTD");
				context.preflightWarnings.append(QStringLiteral("未配置平整度设备，已跳过：%1。")
					.arg(affectedOutputs.join(QStringLiteral("、"))));
				context.skippedOutputs.append(affectedOutputs);
				outputSelection.iri = false;
				outputSelection.riFile = false;
				outputSelection.pb = false;
				outputSelection.rdFile = false;
				outputSelection.smtd = false;
				context.outputSelection = outputSelection;
			}
			else
			{
				const QString iriPath = project->get2DProject()->getIRIPath();
				QStringList missingIriFiles;
				QStringList missingSpeedFiles;
				QStringList missingResampleFiles;
				if (needIriInput && !isReadableDataFile(QDir(iriPath).filePath(QStringLiteral("DAQ0/IRI_10m.txt"))))
				{
					missingIriFiles << QDir(iriPath).filePath(QStringLiteral("DAQ0/IRI_10m.txt"));
				}
				if ((needIriInput || needLpInput || needPbInput || needRutProfileSpeed || needSmtdSpeed)
					&& !isReadableDataFile(QDir(iriPath).filePath(QStringLiteral("DAQ0/Speed_10m.txt"))))
				{
					missingSpeedFiles << QDir(iriPath).filePath(QStringLiteral("DAQ0/Speed_10m.txt"));
				}
				if ((needLpInput || needPbInput)
					&& !isReadableDataFile(QDir(iriPath).filePath(QStringLiteral("DAQ0/resample.txt"))))
				{
					missingResampleFiles << QDir(iriPath).filePath(QStringLiteral("DAQ0/resample.txt"));
				}
				if (project->get2DProject()->_IsDIRIMTD)
				{
					if (needIriInput && !isReadableDataFile(QDir(iriPath).filePath(QStringLiteral("DAQ1/IRI_10m.txt"))))
					{
						missingIriFiles << QDir(iriPath).filePath(QStringLiteral("DAQ1/IRI_10m.txt"));
					}
					if ((needIriInput || needLpInput || needPbInput || needRutProfileSpeed || needSmtdSpeed)
						&& !isReadableDataFile(QDir(iriPath).filePath(QStringLiteral("DAQ1/Speed_10m.txt"))))
					{
						missingSpeedFiles << QDir(iriPath).filePath(QStringLiteral("DAQ1/Speed_10m.txt"));
					}
					if ((needLpInput || needPbInput)
						&& !isReadableDataFile(QDir(iriPath).filePath(QStringLiteral("DAQ1/resample.txt"))))
					{
						missingResampleFiles << QDir(iriPath).filePath(QStringLiteral("DAQ1/resample.txt"));
					}
				}
				if (!missingIriFiles.isEmpty() && outputSelection.iri)
				{
					outputSelection.iri = false;
					context.skippedOutputs << QStringLiteral("IRI");
					context.preflightWarnings << QStringLiteral("IRI 结果缺失、为空或不可读，已跳过 IRI。\n%1")
						.arg(missingIriFiles.join(QStringLiteral("\n")));
				}
				if (!missingSpeedFiles.isEmpty())
				{
					QStringList affectedOutputs;
					if (outputSelection.iri) affectedOutputs << QStringLiteral("IRI");
					if (outputSelection.riFile) affectedOutputs << QStringLiteral("RIFile");
					if (outputSelection.pb) affectedOutputs << QStringLiteral("PB");
					if (outputSelection.rdFile) affectedOutputs << QStringLiteral("RDFile/TP");
					if (outputSelection.smtd) affectedOutputs << QStringLiteral("SMTD");
					outputSelection.iri = false;
					outputSelection.riFile = false;
					outputSelection.pb = false;
					outputSelection.rdFile = false;
					outputSelection.smtd = false;
					if (!affectedOutputs.isEmpty())
					{
						context.skippedOutputs.append(affectedOutputs);
						context.preflightWarnings << QStringLiteral("速度结果缺失、为空或不可读，已跳过：%1。\n%2")
							.arg(affectedOutputs.join(QStringLiteral("、")), missingSpeedFiles.join(QStringLiteral("\n")));
					}
				}
				if (!missingResampleFiles.isEmpty())
				{
					QStringList affectedOutputs;
					if (outputSelection.riFile) affectedOutputs << QStringLiteral("RIFile");
					if (outputSelection.pb) affectedOutputs << QStringLiteral("PB");
					outputSelection.riFile = false;
					outputSelection.pb = false;
					if (!affectedOutputs.isEmpty())
					{
						context.skippedOutputs.append(affectedOutputs);
						context.preflightWarnings << QStringLiteral("原始高程 resample.txt 缺失、为空或不可读，已跳过：%1。\n%2")
							.arg(affectedOutputs.join(QStringLiteral("、")), missingResampleFiles.join(QStringLiteral("\n")));
					}
				}
				context.outputSelection = outputSelection;
			}
			const bool needGpsInput = outputSelection.lbiFile
				|| (exportStandard == hnGjExportStandard::NationalRoad2026 && outputSelection.iri);
			const QString gpsResultPath = project->get2DProject()->getGpsResultFilePath();
			if (needGpsInput && !isReadableDataFile(gpsResultPath))
			{
				QStringList affectedOutputs;
				if (outputSelection.lbiFile)
				{
					outputSelection.lbiFile = false;
					affectedOutputs << QStringLiteral("LBIFile");
				}
				if (exportStandard == hnGjExportStandard::NationalRoad2026 && outputSelection.iri)
				{
					outputSelection.iri = false;
					affectedOutputs << QStringLiteral("IRI/GPS");
				}
				context.outputSelection = outputSelection;
				context.skippedOutputs.append(affectedOutputs);
				context.preflightWarnings << QStringLiteral("GPS 结果缺失、为空或不可读，已跳过：%1。\n%2")
					.arg(affectedOutputs.join(QStringLiteral("、")), gpsResultPath);
			}
			if (project->get2DProject()->_IsRoad)
			{
				if (project->get2DProject()->_RoadImgDis <= 0
					|| project->get2DProject()->getRoadPicturePath().isEmpty())
				{
					projectErrors.append(QStringLiteral("工程【%1】：路面图片或采集间隔无效，无法从成果库生成 Road2Mile.txt。\n%2")
						.arg(context.projectName,
							QDir(context.basePath).filePath(QStringLiteral("RoadImg/Camera0"))));
				}
			}
			if (project->get2DProject()->_IsStreet)
			{
				if (project->get2DProject()->_StreetImgDis <= 0
					|| project->get2DProject()->getLeftStreetPicturePath().isEmpty())
				{
					projectErrors.append(QStringLiteral("工程【%1】：景观图片或采集间隔无效，无法从成果库生成 Street2Mile.txt。\n%2")
						.arg(context.projectName,
							QDir(context.basePath).filePath(QStringLiteral("StreetImg/Camera0"))));
				}
			}

		if (routeCode.size() >= 4 && countyCode.size() == 6
				&& (context.direction == 1 || context.direction == -1))
			{
				context.routeName = routeCode.left(4) + countyCode
					+ (context.direction == 1 ? QStringLiteral("A") : QStringLiteral("B"));
				const QString duplicateKey = context.routeName.toUpper();
				if (routeNames.contains(duplicateKey))
				{
					projectErrors.append(QStringLiteral("工程【%1】：批次内转换路线名重复：%2。")
						.arg(context.projectName, context.routeName));
				}
				routeNames.insert(duplicateKey);
			}

			context.timeText = digitsOnly(project->get2DProject()->_DataDate + project->get2DProject()->_DataTime);
			context.dateText = digitsOnly(project->get2DProject()->_DataDate);
			if (context.timeText.size() != 14)
			{
				projectErrors.append(QStringLiteral("工程【%1】：采集日期与开始时刻必须组成 14 位 yyyyMMddHHmmss，当前为“%2”。\n%3")
					.arg(context.projectName).arg(context.timeText).arg(infoPath));
			}
			if (context.dateText.size() != 8)
			{
				projectErrors.append(QStringLiteral("工程【%1】：采集日期必须为 8 位 yyyyMMdd，当前为“%2”。\n%3")
					.arg(context.projectName).arg(context.dateText).arg(infoPath));
			}
			if (exportStandard == hnGjExportStandard::Standard2025 && outputSelection.mpd)
			{
				const QStringList missingMpdFiles = missingLaserFiles(context, QStringLiteral("MPD_10m.txt"));
				if (!missingMpdFiles.isEmpty())
				{
					outputSelection.mpd = false;
					context.outputSelection = outputSelection;
					context.skippedOutputs << QStringLiteral("MPD");
					context.preflightWarnings << QStringLiteral("MPD 结果缺失或为空，已跳过 MPD；请先执行 IRM 中的 MPD 计算。\n%1")
						.arg(missingMpdFiles.join(QStringLiteral("\n")));
				}
			}
			if (exportStandard == hnGjExportStandard::Standard2025
				&& context.standard == HnProjectEnums::DegreeRoad2018 && outputSelection.smtd)
			{
				const QStringList missingSmtdFiles = missingLaserFiles(context, QStringLiteral("MTD_10m.txt"));
				if (!missingSmtdFiles.isEmpty())
				{
					outputSelection.smtd = false;
					context.outputSelection = outputSelection;
					context.skippedOutputs << QStringLiteral("SMTD");
					context.preflightWarnings << QStringLiteral("SMTD 结果缺失或为空，已跳过 SMTD；请先执行 IRM 中的 SMTD 计算。\n%1")
						.arg(missingSmtdFiles.join(QStringLiteral("\n")));
				}
			}
			if (exportStandard == hnGjExportStandard::Standard2025
				&& context.standard == HnProjectEnums::DegreeRoad2018 && outputSelection.ttFile)
			{
				const QStringList missingLasvalFiles = missingLaserFiles(context, QStringLiteral("lasval.txt"));
				if (!missingLasvalFiles.isEmpty())
				{
					outputSelection.ttFile = false;
					context.outputSelection = outputSelection;
					context.skippedOutputs << QStringLiteral("TTFile");
					context.preflightWarnings << QStringLiteral("纹理原始数据 lasval.txt 缺失或为空，已跳过 TTFile。\n%1")
						.arg(missingLasvalFiles.join(QStringLiteral("\n")));
				}
			}
			if (exportStandard == hnGjExportStandard::Standard2025
				&& context.standard == HnProjectEnums::DegreeRoad2018
				&& outputSelection.haFile && !context.hasGeometryResult)
			{
				outputSelection.haFile = false;
				context.outputSelection = outputSelection;
				context.skippedOutputs << QStringLiteral("HAFile");
				context.preflightWarnings << QStringLiteral("未找到有效的路面几何结果，已跳过 HAFile。\n%1")
					.arg(QDir(context.basePath).filePath(QStringLiteral("Geoalig_10m.txt")));
			}
			if (context.standard == HnProjectEnums::DegreeRoad2018 && !project->get2DProject()->_IsRut
				&& (outputSelection.rd || outputSelection.rdFile))
			{
				QStringList affectedOutputs;
				if (outputSelection.rd) affectedOutputs << (exportStandard == hnGjExportStandard::NationalRoad2026
					? QStringLiteral("RDFile") : QStringLiteral("RD"));
				if (outputSelection.rdFile) affectedOutputs << QStringLiteral("RDFile/TP");
				context.preflightWarnings.append(QStringLiteral("未配置车辙设备，已跳过：%1。")
					.arg(affectedOutputs.join(QStringLiteral("、"))));
				context.skippedOutputs.append(affectedOutputs);
				outputSelection.rd = false;
				outputSelection.rdFile = false;
				context.outputSelection = outputSelection;
			}
			if (context.standard == HnProjectEnums::DegreeRoad2018 && project->get2DProject()->_IsRut
				&& (outputSelection.rd || outputSelection.rdFile))
			{
				if (outputSelection.rd)
				{
					QStringList missingRutFiles;
					const QString leftRutPath = QDir(context.basePath).filePath(QStringLiteral("RUT/camera0/orirut.txt"));
					if (!isReadableDataFile(leftRutPath)) missingRutFiles << leftRutPath;
					if (project->get2DProject()->_RutMode == 1)
					{
						const QString rightRutPath = QDir(context.basePath).filePath(QStringLiteral("RUT/camera1/orirut.txt"));
						if (!isReadableDataFile(rightRutPath)) missingRutFiles << rightRutPath;
					}
					if (!missingRutFiles.isEmpty())
					{
						outputSelection.rd = false;
						const QString rutOutputName = exportStandard == hnGjExportStandard::NationalRoad2026
							? QStringLiteral("RDFile") : QStringLiteral("RD");
						context.skippedOutputs << rutOutputName;
						context.preflightWarnings << QStringLiteral("车辙结果 orirut.txt 缺失、为空或不可读，已跳过 %1。\n%2")
							.arg(rutOutputName, missingRutFiles.join(QStringLiteral("\n")));
					}
				}
				if (outputSelection.rdFile)
				{
					const QString profilePath = rutProfileDirectory(context);
					QStringList missingProfileInputs;
					if (QDir(profilePath).entryList(QStringList() << QStringLiteral("*.dtw"), QDir::Files).isEmpty())
					{
						missingProfileInputs << QStringLiteral("原始断面 .dtw：%1").arg(profilePath);
					}
					const QString rutConfigPath = QDir(context.basePath).filePath(QStringLiteral("camera%1/rutcfg.ini")
						.arg(project->get2DProject()->_RutMode == 1 ? 1 : 0));
					if (!isReadableDataFile(rutConfigPath))
					{
						missingProfileInputs << QStringLiteral("车辙参数：%1").arg(rutConfigPath);
					}
					if (!missingProfileInputs.isEmpty())
					{
						outputSelection.rdFile = false;
						context.skippedOutputs << QStringLiteral("RDFile/TP");
						context.preflightWarnings << QStringLiteral("车辙原始断面依赖不完整，已跳过 RDFile/TP。\n%1")
							.arg(missingProfileInputs.join(QStringLiteral("\n")));
					}
				}
				context.outputSelection = outputSelection;
			}
			if (!projectErrors.isEmpty())
			{
				hnGjProjectResult rejected;
				rejected.projectName = context.projectName;
				rejected.sourceIndex = context.sourceIndex;
				rejected.outputPath = context.outputPath;
				rejected.errorMessage = projectErrors.join(QStringLiteral("\n\n"));
				rejectedProjects.append(rejected);
				continue;
			}

			context.taskCount = requiredDirectories(context).size();
			if (exportStandard == hnGjExportStandard::NationalRoad2026)
			{
				if (outputSelection.iri) ++context.taskCount;
				if (outputSelection.pb) ++context.taskCount;
				if (outputSelection.rd) ++context.taskCount;
			}
			if (project->get2DProject()->_IsRoad || project->get2DProject()->_IsStreet)
			{
				++context.taskCount;
			}
			if (exportStandard == hnGjExportStandard::NationalRoad2026
				&& outputSelection.rd && project->get2DProject()->_IsRut)
			{
				++context.taskCount;
			}
			if (exportStandard == hnGjExportStandard::Standard2025
				&& context.standard == HnProjectEnums::DegreeRoad2018)
			{
				if (outputSelection.pb) ++context.taskCount;
				if (outputSelection.haFile && context.hasGeometryResult) ++context.taskCount;
				if (outputSelection.rd) ++context.taskCount;
				if (outputSelection.rdFile && project->get2DProject()->_IsRut
					&& !QDir(rutProfileDirectory(context)).entryList(QStringList() << QStringLiteral("*.dtw"), QDir::Files).isEmpty())
				{
					context.taskCount += static_cast<int>(rutProfileTaskCount(context));
				}
				if (outputSelection.ttFile)
				{
					context.taskCount += static_cast<int>(textureTaskCount(context));
				}
				if (outputSelection.smtd) ++context.taskCount;
				if (outputSelection.mpd && hasMpdData(context)) ++context.taskCount;
			}
			contexts.append(context);
		}
		if (callback)
		{
			if (!callback(static_cast<int>(projects.size()), static_cast<int>(projects.size()),
				QStringLiteral("工程预检完成，可导出 %1 个，跳过 %2 个。")
					.arg(contexts.size()).arg(rejectedProjects.size())))
			{
				canceled = true;
				return false;
			}
		}
		return true;
	}

	QVector<hnOutExcelMile> createMetricRows(const ExportContext& context, double interval,
		bool rut, bool mpd, bool jump, bool iri, bool speed, bool gps,
		QString& errorMessage, bool includeMarks = false, bool smtd = false)
	{
		MyQtCommon::MyEquipment equipment;
		equipment.ROAD = true;
		equipment.IRI = iri;
		equipment.SPEED = speed;
		equipment.GPS = gps;
		equipment.RUT = rut;
		equipment.MPD = mpd;
		equipment.SMTD = smtd;
		equipment.JUMP = jump;
		QScopedValueRollback<bool> restoreMileWithMark(
			HnXRSettings::getInstance()->outMileWithMark, includeMarks);
		hnOutExcelMileManage manager(context.standard, context.project,
			context.startMile, context.endMile, interval, equipment, !includeMarks);
		if (!manager.getDataComplete())
		{
			errorMessage = QStringLiteral("%1 米成果分段失败，请检查 IRI、速度、GPS、病害及可选设备结果是否覆盖完整起终点。")
				.arg(QString::number(interval, 'f', 0));
			return QVector<hnOutExcelMile>();
		}
		if (jump && !manager.getPbState())
		{
			errorMessage = QStringLiteral("跳车/PB 分段失败，请检查 Resample.txt 是否覆盖完整起终点。");
			return QVector<hnOutExcelMile>();
		}
		QVector<hnOutExcelMile> rows = manager.getRoadMessageVec();
		std::sort(rows.begin(), rows.end(), [&context](const hnOutExcelMile& left, const hnOutExcelMile& right)
		{
			return context.direction == 1
				? left.getStartMile() < right.getStartMile()
				: left.getStartMile() > right.getStartMile();
		});
		return rows;
	}

	QVector<hnOutExcelMile> createTenMeterRows(const ExportContext& context,
		bool rut, bool mpd, bool jump, bool iri, bool speed, bool gps,
		QString& errorMessage, bool includeMarks = false)
	{
		return createMetricRows(context, 10.0, rut, mpd, jump, iri, speed, gps,
			errorMessage, includeMarks);
	}

	QString drHeader(HnProjectEnums::StandardParmTypeEnum standard, int surface)
	{
		if (standard == HnProjectEnums::DegreeRoad2018 && surface == 0)
			return QStringLiteral("起点桩号(km),识别宽度(m),破损率DR(%),龟裂(m^),块状裂缝(m^),纵向裂缝(m),横向裂缝(m),沉陷(m^),车辙(m),波浪拥包(m^),坑槽(m^),松散(m^),泛油(m^),修补(m^)").replace(QLatin1Char('^'), QChar(0x00B2));
		if (standard == HnProjectEnums::DegreeRoad2018 && surface == 1)
			return QStringLiteral("起点桩号(km),识别宽度(m),破损率DR(%),破碎板(m^),裂缝(m),板角断裂(m^),错台(m),拱起(m^),边角剥落(m),接缝料损坏(m),坑洞(m^),唧泥(m),露骨(m^),修补(m^)").replace(QLatin1Char('^'), QChar(0x00B2));
		if (standard == HnProjectEnums::RuralRoadlowLevel && surface == 0)
			return QStringLiteral("起点桩号(km),识别宽度(m),破损率DR(%),纵向裂缝(m^),横向裂缝(m^),网裂(m^),坑槽(m^),松散(m^)").replace(QLatin1Char('^'), QChar(0x00B2));
		if (standard == HnProjectEnums::RuralRoadlowLevel && surface == 1)
			return QStringLiteral("起点桩号(km),识别宽度(m),破损率DR(%),破碎板(m^),裂缝(m^),坑洞(m^),露骨(m^),错台(m^),拱起(m^)").replace(QLatin1Char('^'), QChar(0x00B2));
		return QStringLiteral("起点桩号(km),识别宽度(m),破损率DR(%),坑槽(m^),沉陷(m^),车辙(m^),波浪搓板(m^)").replace(QLatin1Char('^'), QChar(0x00B2));
	}

	QStringList diseaseNames(HnProjectEnums::StandardParmTypeEnum standard, int surface)
	{
		if (standard == HnProjectEnums::DegreeRoad2018 && surface == 0)
			return QStringList() << QStringLiteral("龟裂") << QStringLiteral("块状裂缝") << QStringLiteral("纵向裂缝")
				<< QStringLiteral("横向裂缝") << QStringLiteral("沉陷") << QStringLiteral("车辙") << QStringLiteral("波浪拥包")
				<< QStringLiteral("坑槽") << QStringLiteral("松散") << QStringLiteral("泛油") << QStringLiteral("修补");
		if (standard == HnProjectEnums::DegreeRoad2018 && surface == 1)
			return QStringList() << QStringLiteral("破碎板") << QStringLiteral("裂缝") << QStringLiteral("板角断裂")
				<< QStringLiteral("错台") << QStringLiteral("拱起") << QStringLiteral("边角剥落") << QStringLiteral("接缝料损坏")
				<< QStringLiteral("坑洞") << QStringLiteral("唧泥") << QStringLiteral("露骨") << QStringLiteral("修补");
		if (standard == HnProjectEnums::RuralRoadlowLevel && surface == 0)
			return QStringList() << QStringLiteral("纵向裂缝") << QStringLiteral("横向裂缝") << QStringLiteral("网裂")
				<< QStringLiteral("坑槽") << QStringLiteral("松散");
		if (standard == HnProjectEnums::RuralRoadlowLevel && surface == 1)
			return QStringList() << QStringLiteral("破碎板") << QStringLiteral("裂缝") << QStringLiteral("坑洞")
				<< QStringLiteral("露骨") << QStringLiteral("错台") << QStringLiteral("拱起");
		return QStringList() << QStringLiteral("坑槽") << QStringLiteral("沉陷") << QStringLiteral("车辙") << QStringLiteral("波浪搓板");
	}

	double diseaseAmount(const QVector<hnCommon::hnRoadDiseaseInfo>& diseases, const QString& name, int surface)
	{
		double total = 0.0;
		for (const hnCommon::hnRoadDiseaseInfo& disease : diseases)
		{
			const QString diseaseName = QString::fromLocal8Bit(disease.strDisName).trimmed();
			if (disease.nRSurfaceType != surface || diseaseName != name)
			{
				continue;
			}
			if (disease.diseaseWeight > 0.0)
			{
				total += disease.dArea / disease.diseaseWeight;
			}
			else if (disease.dRealLen > 0.0 && disease.dReaWidth > 0.0)
			{
				total += disease.dRealLen * disease.dReaWidth;
			}
			else
			{
				total += disease.dArea;
			}
		}
		return total;
	}

	bool writeDrFiles(const ExportContext& context, const QString& routeRoot,
		const QVector<hnOutExcelMile>& rows, QString& errorMessage)
	{
		int begin = 0;
		while (begin < rows.size())
		{
			const int surface = static_cast<int>(rows.at(begin).RoadSurface);
			int end = begin + 1;
			while (end < rows.size() && static_cast<int>(rows.at(end).RoadSurface) == surface)
			{
				++end;
			}
			const double firstMile = rows.at(begin).getStartMile() * 0.001;
			const double lastMile = rows.at(end - 1).getEndMile() * 0.001;
			const QString fileName = QStringLiteral("%1-DR-%2-%3-%4.txt")
				.arg(context.routeName, fixed(firstMile, 3), fixed(lastMile, 3), context.timeText);
			QFile file;
			if (!openUtf8TextFile(file, QDir(routeRoot).filePath(QStringLiteral("DR/")) + fileName, errorMessage)
				|| !writeUtf8Line(file, drHeader(context.standard, surface), errorMessage))
			{
				return false;
			}
			const QStringList names = diseaseNames(context.standard, surface);
			for (int rowIndex = begin; rowIndex < end; ++rowIndex)
			{
				hnOutExcelMile row = rows.at(rowIndex);
				QStringList fields;
				fields << fixed(row.getStartMile() * 0.001, 3)
					<< compact(row.getSurveyWidth()) << fixed(row.getDRScore(), 2);
				const QVector<hnCommon::hnRoadDiseaseInfo> diseases = row.getRoadDisVec();
				for (const QString& name : names)
				{
					const double amount = diseaseAmount(diseases, name, surface);
					fields << (amount == 0.0 ? QStringLiteral("0") : fixed(amount, 2));
				}
				if (!writeUtf8Line(file, fields.join(QLatin1Char(',')), errorMessage))
				{
					return false;
				}
			}
			QStringList terminalFields;
			terminalFields << fixed(lastMile, 3) << compact(rows.at(end - 1).getSurveyWidth())
				<< QStringLiteral("0.00");
			for (int nameIndex = 0; nameIndex < names.size(); ++nameIndex)
			{
				terminalFields << QStringLiteral("0");
			}
			if (!writeUtf8Line(file, terminalFields.join(QLatin1Char(',')), errorMessage))
			{
				return false;
			}
			begin = end;
		}
		return true;
	}

	bool readSecondColumnValues(const QString& path, const QString& description,
		QVector<double>& values, QString& errorMessage);

	bool writeIriFile(const ExportContext& context, const QString& routeRoot,
		const QVector<hnOutExcelMile>& rows, QString& errorMessage)
	{
		const QString iriRoot = context.project->get2DProject()->getIRIPath();
		QVector<double> leftSpeeds;
		QVector<double> rightSpeeds;
		if (!readSecondColumnValues(QDir(iriRoot).filePath(QStringLiteral("DAQ0/Speed_10m.txt")),
			QStringLiteral("左侧 Speed_10m.txt"), leftSpeeds, errorMessage)) return false;
		if (context.project->get2DProject()->_IsDIRIMTD
			&& !readSecondColumnValues(QDir(iriRoot).filePath(QStringLiteral("DAQ1/Speed_10m.txt")),
				QStringLiteral("右侧 Speed_10m.txt"), rightSpeeds, errorMessage)) return false;
		QFile file;
		if (!openUtf8TextFile(file, QDir(routeRoot).filePath(QStringLiteral("IRI/"))
			+ dataFileName(context, QStringLiteral("IRI")), errorMessage)
			|| !writeUtf8Line(file, QStringLiteral("起点桩号(km),IRI_左(m/km),IRI_右(m/km),IRI(m/km),速度(m/s)"), errorMessage))
		{
			return false;
		}
		const int mileDecimals = context.standard == HnProjectEnums::DegreeRoad2018 ? 2 : 3;
		for (int rowIndex = 0; rowIndex < rows.size(); ++rowIndex)
		{
			hnOutExcelMile row = rows.at(rowIndex);
			const double left = row.getLeftIriValue();
			const double right = context.project->get2DProject()->_IsDIRIMTD ? row.getRightIriValue() : 0.0;
			const double combined = context.project->get2DProject()->_IsDIRIMTD ? qMax(left, right) : left;
			const double leftSpeed = leftSpeeds.at(qMin(rowIndex, leftSpeeds.size() - 1));
			const double speed = rightSpeeds.isEmpty() ? leftSpeed
				: (leftSpeed + rightSpeeds.at(qMin(rowIndex, rightSpeeds.size() - 1))) / 2.0;
			const QString line = QStringLiteral("%1,%2,%3,%4,%5")
				.arg(fixed(row.getStartMile() * 0.001, mileDecimals), fixed(left, 2), context.project->get2DProject()->_IsDIRIMTD ? fixed(right, 2) : QString(),
					fixed(combined, 2), fixed(speed / 3.6, 2));
			if (!writeUtf8Line(file, line, errorMessage)) return false;
		}
		return true;
	}

	bool writeLbiFile(const ExportContext& context, const QString& routeRoot,
		const QVector<hnOutExcelMile>& rows, QString& errorMessage)
	{
		QFile file;
		if (!openUtf8TextFile(file, QDir(routeRoot).filePath(QStringLiteral("LBIFile/"))
			+ dataFileName(context, QStringLiteral("LBI")), errorMessage)
			|| !writeUtf8Line(file, QStringLiteral("起点桩号(km),经度,纬度"), errorMessage))
		{
			return false;
		}
		for (hnOutExcelMile row : rows)
		{
			const _EXCELGPS_ gps = row.getStartGpsStr();
			const QString line = QStringLiteral("%1,%2,%3")
				.arg(fixed(row.getStartMile() * 0.001, 3), fixed(gps._longitude, 6), fixed(gps._latitude, 6));
			if (!writeUtf8Line(file, line, errorMessage)) return false;
		}
		if (!rows.isEmpty())
		{
			hnOutExcelMile lastRow = rows.last();
			const _EXCELGPS_ gps = lastRow.getEndGpsStr();
			const QString line = QStringLiteral("%1,%2,%3")
				.arg(fixed(lastRow.getEndMile() * 0.001, 3), fixed(gps._longitude, 6), fixed(gps._latitude, 6));
			if (!writeUtf8Line(file, line, errorMessage)) return false;
		}
		return true;
	}

	QString national2026Directory(const ExportContext& context, const QString& rootName,
		bool removeDirection)
	{
		const QString route = removeDirection
			? context.routeName.left(context.routeName.size() - 1) : context.routeName;
		return QDir(rootName).filePath(context.dateText + QLatin1Char('/') + route);
	}

	bool writeNational2026IriFile(const ExportContext& context, const QString& routeRoot,
		const QVector<hnOutExcelMile>& rows, QString& errorMessage)
	{
		const QString routeWithoutDirection = context.routeName.left(context.routeName.size() - 1);
		const QString direction = context.routeName.right(1);
		const QString fileName = QStringLiteral("%1-%2-IRI-%3-%4.csv")
			.arg(routeWithoutDirection, direction, startMileText(context), context.timeText);
		QFile file;
		if (!openUtf8TextFile(file, QDir(routeRoot).filePath(
			national2026Directory(context, QStringLiteral("IRI"), true) + QLatin1Char('/') + fileName), errorMessage)
			|| !writeUtf8Line(file, QStringLiteral("桩号(km),左IRI(m/km),右IRI(m/km),IRI(m/km)"), errorMessage))
		{
			return false;
		}
		for (hnOutExcelMile row : rows)
		{
			const double left = row.getLeftIriValue();
			const double right = context.project->get2DProject()->_IsDIRIMTD ? row.getRightIriValue() : 0.0;
			const double combined = context.project->get2DProject()->_IsDIRIMTD ? qMax(left, right) : left;
			if (!writeUtf8Line(file, QStringLiteral("%1,%2,%3,%4")
				.arg(fixed(row.getStartMile() * 0.001, 3), fixed(left, 2), context.project->get2DProject()->_IsDIRIMTD ? fixed(right, 2) : QString(),
					fixed(combined, 2)), errorMessage))
			{
				return false;
			}
		}
		return true;
	}

	bool writeNational2026GpsFile(const ExportContext& context, const QString& routeRoot,
		const QVector<hnOutExcelMile>& rows, QString& errorMessage)
	{
		const QString fileName = QStringLiteral("%1-GPS-%2-标准格式-%3.txt")
			.arg(context.routeName, startMileText(context), context.timeText);
		QFile file;
		if (!openUtf8TextFile(file, QDir(routeRoot).filePath(QStringLiteral("GPS/") + fileName), errorMessage)
			|| !writeUtf8Line(file, QStringLiteral("桩号,X,Y,Z,有效性,桩号核对"), errorMessage))
		{
			return false;
		}
		for (hnOutExcelMile row : rows)
		{
			const _EXCELGPS_ gps = row.getStartGpsStr();
			if (!writeUtf8Line(file, QStringLiteral("%1,%2,%3,%4,A,0,0")
				.arg(fixed(row.getStartMile(), 3), fixed(gps._longitude, 6), fixed(gps._latitude, 6),
					fixed(gps._elevation, 6)), errorMessage))
			{
				return false;
			}
		}
		return true;
	}

	bool writeNational2026RutFile(const ExportContext& context, const QString& routeRoot,
		const QVector<hnOutExcelMile>& rows, QString& errorMessage)
	{
		const QString routeWithoutDirection = context.routeName.left(context.routeName.size() - 1);
		const QString direction = context.routeName.right(1);
		const QString fileName = QStringLiteral("%1-%2-RD-%3-%4.csv")
			.arg(routeWithoutDirection, direction, startMileText(context), context.timeText);
		QFile file;
		if (!openUtf8TextFile(file, QDir(routeRoot).filePath(
			national2026Directory(context, QStringLiteral("RDFile"), false) + QLatin1Char('/') + fileName), errorMessage)
			|| !writeUtf8Line(file, QStringLiteral("桩号(km),左车辙RD1(mm),右车辙RD2(mm),路面车辙RD(mm)"), errorMessage))
		{
			return false;
		}
		for (hnOutExcelMile row : rows)
		{
			if (!writeUtf8Line(file, QStringLiteral("%1,%2,%3,%4")
				.arg(fixed(row.getStartMile() * 0.001, 3), fixed(row.getLeftRutValue(), 3),
					fixed(row.getRightRutValue(), 3), fixed(row.getjudgeRutValue(), 3)), errorMessage))
			{
				return false;
			}
		}
		return true;
	}

	bool writeNational2026BumpFile(const ExportContext& context, const QString& routeRoot,
		const QVector<hnOutExcelMile>& rows, QString& errorMessage)
	{
		const QString fileName = QStringLiteral("%1-PB-%2-%3.csv")
			.arg(context.routeName, startMileText(context), context.timeText);
		QFile file;
		if (!openUtf8TextFile(file, QDir(routeRoot).filePath(
			national2026Directory(context, QStringLiteral("BUMP"), false) + QLatin1Char('/') + fileName), errorMessage)
			|| !writeUtf8Line(file, QStringLiteral("桩号,PB_L,PB_M,PB_H,△h"), errorMessage))
		{
			return false;
		}
		for (hnOutExcelMile row : rows)
		{
			const double deltaH = context.project->get2DProject()->_IsDIRIMTD
				? qMax(row.getLeftPbValue(10), row.getRightPbValue(10)) : row.getLeftPbValue(10);
			if (!writeUtf8Line(file, QStringLiteral("%1,%2,%3,%4,%5")
				.arg(fixed(row.getStartMile() * 0.001, 3))
				.arg(row.getPbiNumber(1)).arg(row.getPbiNumber(2)).arg(row.getPbiNumber(3))
				.arg(fixed(deltaH, 2)), errorMessage))
			{
				return false;
			}
		}
		return true;
	}

	class ResampleReader
	{
	public:
		explicit ResampleReader(const QString& path) : m_file(path), m_stream(&m_file), m_last(0.0), m_previous(0.0), m_count(0) {}
		bool open() { return m_file.open(QIODevice::ReadOnly | QIODevice::Text); }
		bool next(double& value)
		{
			while (!m_stream.atEnd())
			{
				const QStringList fields = m_stream.readLine().trimmed().split(
					QRegExp(QStringLiteral("\\s+")), QString::SkipEmptyParts);
				bool ok = false;
				value = fields.size() > 2 ? fields.at(2).toDouble(&ok) : 0.0;
				if (!ok)
				{
					value = m_count >= 2 ? 2.0 * m_last - m_previous : m_last;
				}
				m_previous = m_last;
				m_last = value;
				++m_count;
				return true;
			}
			return false;
		}
	private:
		QFile m_file;
		QTextStream m_stream;
		double m_last;
		double m_previous;
		int m_count;
	};

	struct LpKbParameters
	{
		QVector<double> speedLimits;
		QVector<double> kValues;
		QVector<double> bValues;
	};

	bool readSecondColumnValues(const QString& path, const QString& description,
		QVector<double>& values, QString& errorMessage)
	{
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			errorMessage = QStringLiteral("无法读取%1：%2\n%3").arg(description, path, file.errorString());
			return false;
		}
		QTextStream stream(&file);
		int lineNumber = 0;
		while (!stream.atEnd())
		{
			++lineNumber;
			const QStringList fields = stream.readLine().trimmed().split(
				QRegExp(QStringLiteral("\\s+")), QString::SkipEmptyParts);
			bool ok = false;
			const double value = fields.size() >= 2 ? fields.at(1).toDouble(&ok) : 0.0;
			if (!ok || !std::isfinite(value))
			{
				errorMessage = QStringLiteral("%1第 %2 行格式无效，应为“段号 数值”：\n%3")
					.arg(description).arg(lineNumber).arg(path);
				return false;
			}
			values.append(value);
		}
		if (values.isEmpty())
		{
			errorMessage = QStringLiteral("%1为空：%2").arg(description, path);
			return false;
		}
		return true;
	}

	bool readLpKbParameters(const QString& path, LpKbParameters& parameters, QString& errorMessage)
	{
		if (!QFileInfo(path).isFile())
		{
			return true;
		}
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			errorMessage = QStringLiteral("无法读取 LP 修正参数：%1\n%2").arg(path, file.errorString());
			return false;
		}
		QStringList lines;
		QTextStream stream(&file);
		while (!stream.atEnd()) lines.append(stream.readLine().trimmed());
		bool countOk = false;
		const int count = lines.isEmpty() ? 0 : lines.first().toInt(&countOk);
		if (!countOk || count <= 0 || lines.size() < 1 + count * 3)
		{
			errorMessage = QStringLiteral("LP 修正参数数量或行数无效：%1").arg(path);
			return false;
		}
		for (int group = 0; group < 3; ++group)
		{
			for (int index = 0; index < count; ++index)
			{
				bool ok = false;
				const double value = lines.at(1 + group * count + index).toDouble(&ok);
				if (!ok || !std::isfinite(value))
				{
					errorMessage = QStringLiteral("LP 修正参数第 %1 行不是有效数字：\n%2")
						.arg(2 + group * count + index).arg(path);
					return false;
				}
				if (group == 0) parameters.speedLimits.append(value);
				else if (group == 1) parameters.kValues.append(value);
				else parameters.bValues.append(value);
			}
		}
		return true;
	}

	double roundLpValue(double value, int decimals)
	{
		const double factor = std::pow(10.0, decimals);
		return std::nearbyint(value * factor) / factor;
	}

	int lpMileDecimals(const ExportContext& context)
	{
		return context.standard == HnProjectEnums::RuralRoadlowLevel ? 6 : 4;
	}

	QString lpHeightText(const ExportContext& context, double value)
	{
		if (context.standard == HnProjectEnums::RuralRoadlowLevel
			&& context.drawType == hnCommon::ROAD_WORK_SMALL_RECT)
		{
			return fixed(value, 2);
		}
		return QString::number(value, 'g', 15);
	}

	void roundLpProfile(QVector<double>& profile, int decimals)
	{
		for (int index = 0; index < profile.size(); ++index)
		{
			profile[index] = roundLpValue(profile.at(index), decimals);
		}
	}

	QVector<double> calculateLpIri10M(const QVector<double>& profile)
	{
		QVector<double> results;
		if (profile.size() < 4) return results;
		const double szu[16] = {
			0.9994014, 0.004442351, 0.0002188854, 5.72179E-05,
			-0.2570548, 0.975036, 0.007966216, 0.02458427,
			0.003960378, 0.0003814527, 0.9548048, 0.004055587,
			1.687312, 0.1638951, -19.34264, 0.7948701
		};
		const double pzu[4] = { 0.0003793992, 0.2490886, 0.04123478, 17.65532 };
		const int pointsPerSection = 100;
		const int initializeIndex = qMin(110, profile.size() - 1);
		const double initialSlope = (profile.at(initializeIndex) - profile.first()) / 11.0;
		double oldZsu[4] = { initialSlope, 0.0, initialSlope, 0.0 };
		double zsu[4] = { 0.0, 0.0, 0.0, 0.0 };
		double iriSum = 0.0;
		int count = 0;
		for (int index = 3; index < profile.size(); ++index)
		{
			if ((index % pointsPerSection) == 0)
			{
				results.append(count > 0 ? iriSum / count : 0.0);
				iriSum = 0.0;
				count = 0;
			}
			const double ysu = (profile.at(index) - profile.at(index - 3)) / 0.3;
			for (int row = 0; row < 4; ++row)
			{
				double value = pzu[row] * ysu;
				for (int column = 0; column < 4; ++column)
				{
					value += szu[row * 4 + column] * oldZsu[column];
				}
				zsu[row] = value;
			}
			iriSum += qAbs(zsu[0] - zsu[2]);
			++count;
			for (int state = 0; state < 4; ++state) oldZsu[state] = zsu[state];
		}
		if (count > 0) results.append(iriSum / count);
		return results;
	}

	QVector<double> buildCorrectedLpProfile(const QVector<double>& original,
		const QVector<double>& sectionScale, int pointsPerSection)
	{
		QVector<double> corrected = original;
		for (int section = 0; section < sectionScale.size(); ++section)
		{
			const int startIndex = section * pointsPerSection;
			const int endExclusive = qMin(startIndex + pointsPerSection, corrected.size());
			const int lastIndex = endExclusive - 1;
			if (startIndex >= lastIndex) continue;
			const double startHeight = original.at(startIndex);
			const double endHeight = original.at(lastIndex);
			const double indexLength = lastIndex - startIndex;
			for (int index = startIndex; index < endExclusive; ++index)
			{
				const double ratio = (index - startIndex) / indexLength;
				const double baseline = startHeight + (endHeight - startHeight) * ratio;
				corrected[index] = baseline + sectionScale.at(section) * (original.at(index) - baseline);
			}
		}
		return corrected;
	}

	double lpIriError(double target, double actual)
	{
		return target > 0.000001 ? qAbs(actual - target) / target : qAbs(actual - target);
	}

	double maximumLpIriError(const QVector<double>& targets, const QVector<double>& actual)
	{
		const int count = qMin(targets.size(), actual.size());
		double maximum = 0.0;
		for (int index = 0; index < count; ++index)
		{
			maximum = qMax(maximum, lpIriError(targets.at(index), actual.at(index)));
		}
		return maximum;
	}

	bool isLpIriObjectiveBetter(const QVector<double>& targets, const QVector<double>& candidate,
		const QVector<double>& current, double targetError)
	{
		const int count = qMin(targets.size(), qMin(candidate.size(), current.size()));
		int candidateOverLimit = 0;
		int currentOverLimit = 0;
		double candidateMaximum = 0.0;
		double currentMaximum = 0.0;
		double candidateSum = 0.0;
		double currentSum = 0.0;
		for (int index = 0; index < count; ++index)
		{
			const double candidateError = lpIriError(targets.at(index), candidate.at(index));
			const double currentError = lpIriError(targets.at(index), current.at(index));
			candidateMaximum = qMax(candidateMaximum, candidateError);
			currentMaximum = qMax(currentMaximum, currentError);
			candidateSum += candidateError;
			currentSum += currentError;
			if (candidateError > targetError) ++candidateOverLimit;
			if (currentError > targetError) ++currentOverLimit;
		}
		const double epsilon = 0.000000001;
		if (candidateMaximum < currentMaximum - epsilon) return true;
		if (candidateMaximum > currentMaximum + epsilon) return false;
		if (candidateOverLimit != currentOverLimit) return candidateOverLimit < currentOverLimit;
		return candidateSum < currentSum - epsilon;
	}

	void selectLpKb(double speed, const LpKbParameters& parameters, double& k, double& b)
	{
		int selected = parameters.speedLimits.size() - 1;
		for (int index = 0; index < parameters.speedLimits.size(); ++index)
		{
			if (speed <= parameters.speedLimits.at(index))
			{
				selected = index;
				break;
			}
		}
		k = parameters.kValues.at(selected);
		b = parameters.bValues.at(selected);
	}

	void applyLpKbEquivalentCorrection(QVector<double>& profile, const QVector<double>& speeds,
		const LpKbParameters& parameters, int decimals)
	{
		if (profile.size() < 4 || parameters.speedLimits.isEmpty()) return;
		const double minimumRawIri = 0.000001;
		const double minimumScale = 0.10;
		const double maximumScale = 5.00;
		const double targetError = 0.01;
		const int maximumGlobalIterations = 5;
		const int pointsPerSection = 100;
		const QVector<double> original = profile;
		const QVector<double> rawIri = calculateLpIri10M(original);
		const int sectionCount = qMin(rawIri.size(), (profile.size() + pointsPerSection - 1) / pointsPerSection);
		if (sectionCount <= 0) return;

		QVector<double> targets(sectionCount);
		QVector<double> scales(sectionCount);
		double lastSpeed = speeds.isEmpty() ? 0.0 : speeds.first();
		for (int section = 0; section < sectionCount; ++section)
		{
			if (section < speeds.size()) lastSpeed = speeds.at(section);
			double k = 1.0;
			double b = 0.0;
			selectLpKb(lastSpeed, parameters, k, b);
			const double target = qMax(0.0, rawIri.at(section) * k + b);
			targets[section] = target;
			const double requested = rawIri.at(section) > minimumRawIri ? target / rawIri.at(section) : k;
			const double limited = qBound(minimumScale, requested, maximumScale);
			scales[section] = limited;
		}

		QVector<double> corrected = buildCorrectedLpProfile(original, scales, pointsPerSection);
		roundLpProfile(corrected, decimals);
		QVector<double> correctedIri = calculateLpIri10M(corrected);
		double currentMaximumError = maximumLpIriError(targets, correctedIri);
		int acceptedGlobalIterations = 0;
		while (acceptedGlobalIterations < maximumGlobalIterations && currentMaximumError > targetError)
		{
			bool accepted = false;
			for (double step = 1.0; step >= 0.03125; step *= 0.5)
			{
				QVector<double> candidateScale = scales;
				const int count = qMin(sectionCount, qMin(targets.size(), correctedIri.size()));
				for (int section = 0; section < count; ++section)
				{
					if (correctedIri.at(section) <= minimumRawIri || targets.at(section) <= minimumRawIri) continue;
					const double proposed = scales.at(section)
						* std::pow(targets.at(section) / correctedIri.at(section), step);
					candidateScale[section] = qBound(minimumScale, proposed, maximumScale);
				}
				QVector<double> candidate = buildCorrectedLpProfile(original, candidateScale, pointsPerSection);
				roundLpProfile(candidate, decimals);
				const QVector<double> candidateIri = calculateLpIri10M(candidate);
				const double candidateError = maximumLpIriError(targets, candidateIri);
				if (candidateError + 0.000000001 < currentMaximumError)
				{
					scales = candidateScale;
					corrected = candidate;
					correctedIri = candidateIri;
					currentMaximumError = candidateError;
					++acceptedGlobalIterations;
					accepted = true;
					break;
				}
			}
			if (!accepted) break;
		}

		const int maximumLocalCorrections = 24;
		int localCorrections = 0;
		for (int pass = 0; pass < 3 && localCorrections < maximumLocalCorrections; ++pass)
		{
			bool improvedInPass = false;
			while (localCorrections < maximumLocalCorrections)
			{
				const int compareCount = qMin(targets.size(), correctedIri.size());
				QVector<int> targetSections;
				for (int section = 0; section < compareCount; ++section)
				{
					const double error = lpIriError(targets.at(section), correctedIri.at(section));
					if (error > targetError) targetSections.append(section);
				}
				std::sort(targetSections.begin(), targetSections.end(),
					[&targets, &correctedIri](int left, int right)
				{
					return lpIriError(targets.at(left), correctedIri.at(left))
						> lpIriError(targets.at(right), correctedIri.at(right));
				});
				if (targetSections.isEmpty()) break;
				bool accepted = false;
				for (int targetIndex = 0; targetIndex < targetSections.size() && !accepted; ++targetIndex)
				{
					const int targetSection = targetSections.at(targetIndex);
					if (correctedIri.at(targetSection) <= minimumRawIri
						|| targets.at(targetSection) <= minimumRawIri) continue;
					for (int offset = 0; offset <= 1 && !accepted; ++offset)
					{
						const int controlSection = targetSection - offset;
						if (controlSection < 0 || controlSection >= scales.size()) continue;
						for (double step = 1.0; step >= 0.03125; step *= 0.5)
						{
							QVector<double> candidateScale = scales;
							const double proposed = scales.at(controlSection)
								* std::pow(targets.at(targetSection) / correctedIri.at(targetSection), step);
							candidateScale[controlSection] = qBound(minimumScale, proposed, maximumScale);
							QVector<double> candidate = buildCorrectedLpProfile(original, candidateScale, pointsPerSection);
							roundLpProfile(candidate, decimals);
							const QVector<double> candidateIri = calculateLpIri10M(candidate);
							if (isLpIriObjectiveBetter(targets, candidateIri, correctedIri, targetError))
							{
								scales = candidateScale;
								corrected = candidate;
								correctedIri = candidateIri;
								++localCorrections;
								improvedInPass = true;
								accepted = true;
								break;
							}
						}
					}
				}
				if (!accepted) break;
			}
			if (!improvedInPass) break;
		}
		profile = corrected;
	}

	bool loadLpProfile(const QString& path, qint64 maximumRawSamples, int outputPointCount,
		QVector<double>& profile, QString& errorMessage)
	{
		ResampleReader reader(path);
		if (!reader.open())
		{
			errorMessage = QStringLiteral("无法打开 LP 原始高程文件：%1").arg(path);
			return false;
		}
		QVector<double> window;
		QMap<qint64, double> tail;
		qint64 lastSampleIndex = -1;
		qint64 samplesRead = 0;
		double lastRawValue = 0.0;
		for (qint64 index = 0; index < maximumRawSamples; ++index)
		{
			double value = 0.0;
			if (!reader.next(value)) break;
			++samplesRead;
			lastRawValue = value;
			window.append(value);
			if (window.size() > 5) window.removeFirst();
			tail.insert(index, value);
			while (tail.size() > 2) tail.erase(tail.begin());
			if (index == 0)
			{
				profile.append(value);
				lastSampleIndex = 0;
			}
			if (window.size() == 5)
			{
				const qint64 centerIndex = index - 2;
				if ((centerIndex % 2) == 0)
				{
					double mean = 0.0;
					for (int item = 0; item < window.size(); ++item) mean += window.at(item);
					profile.append(mean / 5.0);
					lastSampleIndex = centerIndex;
				}
			}
		}
		if (samplesRead <= 0)
		{
			errorMessage = QStringLiteral("LP 原始高程文件没有有效数据：%1").arg(path);
			return false;
		}
		for (QMap<qint64, double>::const_iterator it = tail.constBegin(); it != tail.constEnd(); ++it)
		{
			if ((it.key() % 2) == 0 && it.key() > lastSampleIndex)
			{
				profile.append(it.value());
				lastSampleIndex = it.key();
			}
		}
		if (profile.isEmpty())
		{
			errorMessage = QStringLiteral("LP 原始高程无法形成 0.1 米采样：%1").arg(path);
			return false;
		}
		if (profile.size() > outputPointCount) profile.resize(outputPointCount);
		// 与二维项目 GetIRIHValF 一致：0.1 米目标点超出原始数组后，
		// 重复有效范围内最后一个 0.05 米原始高程，而不是最后一个偶数抽样点。
		while (profile.size() < outputPointCount) profile.append(lastRawValue);
		return true;
	}

	double meanSpeedForDmiInterval(const QVector<double>& values, double startDmi, double endDmi,
		double previousValue)
	{
		if (values.isEmpty()) return previousValue;
		// 对齐二维 C# Math.Round 的中点取偶规则：先消除二进制浮点尾差，
		// 避免精确的 x.5 边界提前进入下一个 10 米速度段。
		const double startRatio = std::round(((startDmi - 0.5) / 10.0) * 1000000000.0) / 1000000000.0;
		const double endRatio = std::round((endDmi / 10.0) * 1000000000.0) / 1000000000.0;
		const int startIndex = static_cast<int>(std::nearbyint(startRatio));
		const int endIndex = static_cast<int>(std::nearbyint(endRatio));
		double sum = 0.0;
		int count = 0;
		if (startIndex >= endIndex)
		{
			if (startIndex >= 0 && startIndex < values.size())
			{
				sum = values.at(startIndex);
				count = 1;
			}
		}
		else
		{
			for (int index = qMax(0, startIndex); index < endIndex && index < values.size(); ++index)
			{
				sum += values.at(index);
				++count;
			}
		}
		return count > 0 ? sum / count : previousValue;
	}

	// RDFile 仍按现有 10 米成果分段取速度，保持车辙断面文件的原有契约。
	double speedAtDmi(const QVector<hnOutExcelMile>& rows, double dmi)
	{
		for (hnOutExcelMile row : rows)
		{
			const double low = qMin(row.getStartDmi(), row.getEndDmi());
			const double high = qMax(row.getStartDmi(), row.getEndDmi());
			if (dmi >= low && dmi <= high) return row.getSpeed() / 3.6;
		}
		if (rows.isEmpty()) return 0.0;
		hnOutExcelMile lastRow = rows.last();
		return lastRow.getSpeed() / 3.6;
	}

	bool writeLpFile(const ExportContext& context, const QString& routeRoot,
		const QVector<hnOutExcelMile>&, const hnGjConvertSourceService::ProgressCallback& callback,
		int& currentTask, int totalTasks, QString& errorMessage, bool& canceled)
	{
		const QString iriRoot = context.project->get2DProject()->getIRIPath();
		const QString leftRoot = QDir(iriRoot).filePath(QStringLiteral("DAQ0"));
		const QString rightRoot = QDir(iriRoot).filePath(QStringLiteral("DAQ1"));
		QVector<double> leftSpeeds;
		QVector<double> rightSpeeds;
		if (!readSecondColumnValues(QDir(leftRoot).filePath(QStringLiteral("Speed_10m.txt")),
			QStringLiteral("左侧 Speed_10m.txt"), leftSpeeds, errorMessage)) return false;
		if (context.project->get2DProject()->_IsDIRIMTD
			&& !readSecondColumnValues(QDir(rightRoot).filePath(QStringLiteral("Speed_10m.txt")),
				QStringLiteral("右侧 Speed_10m.txt"), rightSpeeds, errorMessage)) return false;

		const double roadDistance = qAbs(context.endMile - context.startMile);
		double effectiveDistance = roadDistance;
		if (context.project->get2DProject()->_EndDmi > 0)
			effectiveDistance = qMin(effectiveDistance,
				static_cast<double>(context.project->get2DProject()->_EndDmi));
		const qint64 maximumRawSamples = qMax<qint64>(1, qRound64(effectiveDistance / 0.05));
		QVector<double> outputOffsets;
		const int regularPointCount = qMax(1, static_cast<int>(std::floor(roadDistance / 0.1 + 0.000000001)) + 1);
		outputOffsets.reserve(regularPointCount + 1);
		for (int index = 0; index < regularPointCount; ++index)
		{
			outputOffsets.append(index * 0.1);
		}
		if (outputOffsets.last() < roadDistance - 0.000000001)
		{
			outputOffsets.append(roadDistance);
		}
		const int outputPointCount = outputOffsets.size();
		QVector<double> leftProfile;
		QVector<double> rightProfile;
		if (!loadLpProfile(QDir(leftRoot).filePath(QStringLiteral("resample.txt")),
			maximumRawSamples, outputPointCount, leftProfile, errorMessage)) return false;
		if (context.project->get2DProject()->_IsDIRIMTD
			&& !loadLpProfile(QDir(rightRoot).filePath(QStringLiteral("resample.txt")),
				maximumRawSamples, outputPointCount, rightProfile, errorMessage)) return false;

		const int decimals = qBound(0, HnXRSettings::getInstance()->sheetRoundingOffNum, 10);
		LpKbParameters leftParameters;
		LpKbParameters rightParameters;
		if (!readLpKbParameters(QDir(leftRoot).filePath(QStringLiteral("Coeff.dat")),
			leftParameters, errorMessage)) return false;
		if (context.project->get2DProject()->_IsDIRIMTD
			&& !readLpKbParameters(QDir(rightRoot).filePath(QStringLiteral("Coeff.dat")),
				rightParameters, errorMessage)) return false;
		if (!leftParameters.speedLimits.isEmpty())
			applyLpKbEquivalentCorrection(leftProfile, leftSpeeds, leftParameters, decimals);
		else roundLpProfile(leftProfile, decimals);
		if (context.project->get2DProject()->_IsDIRIMTD)
		{
			if (!rightParameters.speedLimits.isEmpty())
				applyLpKbEquivalentCorrection(rightProfile, rightSpeeds, rightParameters, decimals);
			else roundLpProfile(rightProfile, decimals);
		}
		QFile file;
		if (!openUtf8TextFile(file, QDir(routeRoot).filePath(QStringLiteral("RIFile/"))
			+ dataFileName(context, QStringLiteral("LP")), errorMessage)
			|| !writeUtf8Line(file, QStringLiteral("起点桩号(km),左高程(mm),右高程(mm),速度(m/s)"), errorMessage))
		{
			return false;
		}

		double previousLeftSpeed = leftSpeeds.first();
		double previousRightSpeed = rightSpeeds.isEmpty() ? previousLeftSpeed : rightSpeeds.first();
		for (int index = 0; index < outputPointCount; ++index)
		{
			const double mile = context.startMile + context.direction * outputOffsets.at(index);
			const double nextMile = index + 1 < outputOffsets.size()
				? context.startMile + context.direction * outputOffsets.at(index + 1) : mile;
			const double startDmi = context.project->trueMileToEncl(mile);
			const double endDmi = context.project->trueMileToEncl(nextMile);
			previousLeftSpeed = meanSpeedForDmiInterval(leftSpeeds, startDmi, endDmi, previousLeftSpeed);
			if (!rightSpeeds.isEmpty())
				previousRightSpeed = meanSpeedForDmiInterval(rightSpeeds, startDmi, endDmi, previousRightSpeed);
			const double speedKmh = rightSpeeds.isEmpty()
				? previousLeftSpeed : (previousLeftSpeed + previousRightSpeed) / 2.0;
			const double roundedSpeedKmh = roundLpValue(speedKmh, decimals);
			const double rightValue = rightProfile.isEmpty() ? 0.0 : rightProfile.at(index);
			const QString rightText = rightProfile.isEmpty()
				? QString() : lpHeightText(context, rightValue);

	/*		const QString rightText = rightProfile.isEmpty()
				? QStringLiteral("0") : lpHeightText(context, rightValue);*/
			const QString line = QStringLiteral("%1,%2,%3,%4")
				.arg(fixed(mile * 0.001, lpMileDecimals(context)),
					lpHeightText(context, leftProfile.at(index)), rightText,
					fixed(roundedSpeedKmh / 3.6, 2));
			if (!writeUtf8Line(file, line, errorMessage)) return false;
			if ((index % 100000) == 0 && callback
				&& !callback(currentTask, totalTasks, QStringLiteral("工程【%1】：正在流式写入 LP 高程 %2 行")
					.arg(context.projectName).arg(index)))
			{
				canceled = true;
				return false;
			}
		}
		return true;
	}

	bool writeRdFile(const ExportContext& context, const QString& routeRoot,
		const QVector<hnOutExcelMile>& rows, QString& errorMessage)
	{
		QFile file;
		if (!openUtf8TextFile(file, QDir(routeRoot).filePath(QStringLiteral("RD/"))
			+ dataFileName(context, QStringLiteral("RD")), errorMessage)
			|| !writeUtf8Line(file, QStringLiteral("桩号(km),左车辙RD(mm),右车辙RD(mm),路面车辙RD(mm)"), errorMessage))
			return false;
		for (hnOutExcelMile row : rows)
		{
			const QString line = QStringLiteral("%1,%2,%3,%4")
				.arg(fixed(row.getStartMile() * 0.001, 3), fixed(row.getLeftRutValue(), 1),
					fixed(row.getRightRutValue(), 1), fixed(row.getjudgeRutValue(), 1));
			if (!writeUtf8Line(file, line, errorMessage)) return false;
		}
		return true;
	}

	bool writeMpdFile(const ExportContext& context, const QString& routeRoot,
		const QVector<hnOutExcelMile>& rows, QString& errorMessage)
	{
		QFile file;
		if (!openUtf8TextFile(file, QDir(routeRoot).filePath(QStringLiteral("MPD/"))
			+ dataFileName(context, QStringLiteral("MPD")), errorMessage)
			|| !writeUtf8Line(file, QStringLiteral("起点桩号(km),MPD_L(mm),MPD_C(mm),MPD_R(mm)"), errorMessage))
			return false;
		for (hnOutExcelMile row : rows)
		{
			const QString line = QStringLiteral("%1,%2,%3,%4")
				.arg(fixed(row.getStartMile() * 0.001, 3), fixed(row.getLeftMpdValue(), 2),
					fixed(row.getCenterMpdValue(), 2), fixed(row.getRightMpdValue(), 2));
			if (!writeUtf8Line(file, line, errorMessage)) return false;
		}
		return true;
	}

	bool writeSmtdFile(const ExportContext& context, const QString& routeRoot,
		const QVector<hnOutExcelMile>& rows, QString& errorMessage)
	{
		QFile file;
		if (!openUtf8TextFile(file, QDir(routeRoot).filePath(QStringLiteral("SMTD/"))
			+ dataFileName(context, QStringLiteral("SMTD")), errorMessage)
			|| !writeUtf8Line(file, QStringLiteral("起点桩号(km),SMTD(mm),速度(m/s)"), errorMessage))
			return false;
		for (hnOutExcelMile row : rows)
		{
			// 复用软件的 SMTD 代表值和分段速度，速度由 km/h 换算为 m/s。
			if (!writeUtf8Line(file, QStringLiteral("%1,%2,%3")
				.arg(fixed(row.getStartMile() * 0.001, 3), fixed(row.getRepresentSMtdValue(), 2),
					fixed(row.getSpeed() / 3.6, 2)), errorMessage)) return false;
		}
		return true;
	}
	struct HaSourceRow
	{
		HaSourceRow()
			: dmi(0.0), curvature(0.0), longitudinalSlope(0.0), crossSlope(0.0),
			curvatureValid(false), longitudinalValid(false), crossValid(false)
		{
		}

		double dmi;
		double curvature;
		double longitudinalSlope;
		double crossSlope;
		bool curvatureValid;
		bool longitudinalValid;
		bool crossValid;
	};

	QVector<HaSourceRow> loadHaSourceRows(const ExportContext& context)
	{
		const QString geometryPath = QDir(context.basePath).filePath(QStringLiteral("Geoalig_10m.txt"));
		QFile geometryFile(geometryPath);
		if (!geometryFile.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			return QVector<HaSourceRow>();
		}

		QMap<qint64, QVector<bool>> qualityByDmi;
		const QString qualityPath = QDir(context.basePath).filePath(QStringLiteral("Geoalig_10m.quality.csv"));
		QFile qualityFile(qualityPath);
		const bool hasQualityFile = qualityFile.open(QIODevice::ReadOnly | QIODevice::Text);
		if (hasQualityFile)
		{
			QTextStream qualityStream(&qualityFile);
			bool firstLine = true;
			while (!qualityStream.atEnd())
			{
				const QString line = qualityStream.readLine().trimmed();
				if (firstLine)
				{
					firstLine = false;
					continue;
				}
				const QStringList fields = line.split(QLatin1Char(','), QString::KeepEmptyParts);
				bool dmiValid = false;
				const double dmi = fields.value(0).toDouble(&dmiValid);
				if (!dmiValid || fields.size() < 4)
				{
					continue;
				}
				QVector<bool> flags;
				flags << (fields.at(1).toInt() != 0) << (fields.at(2).toInt() != 0)
					<< (fields.at(3).toInt() != 0);
				qualityByDmi.insert(qRound64(dmi * 1000.0), flags);
			}
		}

		QVector<HaSourceRow> result;
		QTextStream geometryStream(&geometryFile);
		while (!geometryStream.atEnd())
		{
			const QStringList fields = geometryStream.readLine().trimmed().split(
				QLatin1Char(','), QString::KeepEmptyParts);
			if (fields.size() < 4)
			{
				continue;
			}
			HaSourceRow row;
			bool dmiValid = false;
			row.dmi = fields.at(0).toDouble(&dmiValid);
			row.curvature = fields.at(1).toDouble(&row.curvatureValid);
			row.longitudinalSlope = fields.at(2).toDouble(&row.longitudinalValid);
			row.crossSlope = fields.at(3).toDouble(&row.crossValid);
			if (!dmiValid)
			{
				continue;
			}
			if (hasQualityFile)
			{
				const QVector<bool> flags = qualityByDmi.value(qRound64(row.dmi * 1000.0));
				row.curvatureValid = row.curvatureValid && flags.size() == 3 && flags.at(0);
				row.longitudinalValid = row.longitudinalValid && flags.size() == 3 && flags.at(1);
				row.crossValid = row.crossValid && flags.size() == 3 && flags.at(2);
			}
			result.append(row);
		}
		return result;
	}

	bool writeHaFile(const ExportContext& context, const QString& routeRoot,
		const QVector<hnOutExcelMile>& rows, QString& errorMessage)
	{
		QFile file;
		if (!openUtf8TextFile(file, QDir(routeRoot).filePath(QStringLiteral("HAFile/"))
			+ dataFileName(context, QStringLiteral("HA")), errorMessage)
			|| !writeUtf8Line(file, QStringLiteral("起点桩号(km),平曲线半径(m),纵坡(%),横坡(%)"), errorMessage))
		{
			return false;
		}

		const QVector<HaSourceRow> geometryRows = loadHaSourceRows(context);
		double lastRadiusCoverageEnd = -1.0;
		double lastLongitudinalCoverageEnd = -1.0;
		double lastCrossCoverageEnd = -1.0;
		for (const HaSourceRow& geometry : geometryRows)
		{
			if (geometry.curvatureValid && qAbs(geometry.curvature) > 0.000000000001)
			{
				lastRadiusCoverageEnd = qMax(lastRadiusCoverageEnd, geometry.dmi + 10.0);
			}
			if (geometry.longitudinalValid)
			{
				lastLongitudinalCoverageEnd = qMax(lastLongitudinalCoverageEnd, geometry.dmi + 10.0);
			}
			if (geometry.crossValid)
			{
				lastCrossCoverageEnd = qMax(lastCrossCoverageEnd, geometry.dmi + 10.0);
			}
		}
		QString previousRadiusText;
		QString previousLongitudinalText;
		QString previousCrossText;
		for (hnOutExcelMile row : rows)
		{
			const double beginDmi = qMin(row.getStartDmi(), row.getEndDmi());
			const double endDmi = qMax(row.getStartDmi(), row.getEndDmi());
			double curvatureSum = 0.0;
			double longitudinalSum = 0.0;
			double crossSum = 0.0;
			int curvatureCount = 0;
			int longitudinalCount = 0;
			int crossCount = 0;
			for (const HaSourceRow& geometry : geometryRows)
			{
				if (geometry.dmi >= endDmi || geometry.dmi + 10.0 <= beginDmi)
				{
					continue;
				}
				if (geometry.curvatureValid)
				{
					curvatureSum += qAbs(geometry.curvature);
					++curvatureCount;
				}
				if (geometry.longitudinalValid)
				{
					longitudinalSum += qAbs(geometry.longitudinalSlope);
					++longitudinalCount;
				}
				if (geometry.crossValid)
				{
					crossSum += qAbs(geometry.crossSlope);
					++crossCount;
				}
			}

			QString radiusText;
			if (curvatureCount > 0)
			{
				const double averageCurvature = curvatureSum / curvatureCount;
				if (averageCurvature > 0.000000000001)
				{
					radiusText = fixed(1.0 / averageCurvature, 2);
				}
			}
			QString longitudinalText = longitudinalCount > 0
				? fixed(longitudinalSum * 100.0 / longitudinalCount, 2) : QString();
			QString crossText = crossCount > 0
				? fixed(crossSum * 100.0 / crossCount, 2) : QString();

			// 仅对点云末端之后的缺失值续用最后有效结果，
			// 中间缺测仍留空，避免掩盖数据质量问题。
			if (radiusText.isEmpty() && beginDmi >= lastRadiusCoverageEnd
				&& !previousRadiusText.isEmpty())
			{
				radiusText = previousRadiusText;
			}
			if (longitudinalText.isEmpty() && beginDmi >= lastLongitudinalCoverageEnd
				&& !previousLongitudinalText.isEmpty())
			{
				longitudinalText = previousLongitudinalText;
			}
			if (crossText.isEmpty() && beginDmi >= lastCrossCoverageEnd
				&& !previousCrossText.isEmpty())
			{
				crossText = previousCrossText;
			}
			if (!radiusText.isEmpty())
			{
				previousRadiusText = radiusText;
			}
			if (!longitudinalText.isEmpty())
			{
				previousLongitudinalText = longitudinalText;
			}
			if (!crossText.isEmpty())
			{
				previousCrossText = crossText;
			}
			const QString line = QStringLiteral("%1,%2,%3,%4")
				.arg(fixed(row.getStartMile() * 0.001, 3), radiusText, longitudinalText, crossText);
			if (!writeUtf8Line(file, line, errorMessage))
			{
				return false;
			}
		}
		return true;
	}

	bool writePbFile(const ExportContext& context, const QString& routeRoot,
		const QVector<hnOutExcelMile>& rows, QString& errorMessage)
	{
		QFile file;
		if (!openUtf8TextFile(file, QDir(routeRoot).filePath(QStringLiteral("PB/"))
			+ dataFileName(context, QStringLiteral("PB")), errorMessage)
			|| !writeUtf8Line(file, QStringLiteral("桩号(km),PB_L,PB_M,PB_H,Δh(cm)"), errorMessage))
			return false;
		for (hnOutExcelMile row : rows)
		{
			const double deltaH = context.project->get2DProject()->_IsDIRIMTD
				? qMax(row.getLeftPbValue(10), row.getRightPbValue(10)) : row.getLeftPbValue(10);
			const QString line = QStringLiteral("%1,%2,%3,%4,%5")
				.arg(fixed(row.getStartMile() * 0.001, 3))
				.arg(row.getPbiNumber(1)).arg(row.getPbiNumber(2)).arg(row.getPbiNumber(3))
				.arg(fixed(deltaH, 2));
			if (!writeUtf8Line(file, line, errorMessage)) return false;
		}
		return true;
	}

	// 将一个车辙断面按国检 TP 格式写入文件。
	bool writeRutProfileLine(QFile& output, const ExportContext& context,
		const QVector<hnOutExcelMile>& speedRows, const QByteArray& frame,
		double scale, int astart, int cend, qint64 frameIndex, QString& errorMessage)
	{
		QStringList fields;
		const double dmi = frameIndex * 0.1;
		const double mile = context.startMile + context.direction * dmi;
		fields << fixed(mile * 0.001, 4);
		for (int pixel = astart; pixel <= cend; ++pixel)
		{
			const int byteIndex = pixel * 2;
			const quint16 raw = static_cast<quint8>(frame.at(byteIndex))
				| (static_cast<quint16>(static_cast<quint8>(frame.at(byteIndex + 1))) << 8);
			const qint16 signedValue = static_cast<qint16>(raw);
			double height = signedValue / scale;
			if (context.project->get2DProject()->_RutMode == 2) height = -height;
			fields << fixed(height, 2);
		}
		for (int pixel = astart; pixel <= cend; ++pixel)
		{
			fields << fixed((pixel - astart) * 1.5, 1);
		}
		fields << fixed(speedAtDmi(speedRows, dmi), 1);
		return writeUtf8Line(output, fields.join(QLatin1Char(',')), errorMessage);
	}

	bool writeRutProfileFile(const ExportContext& context, const QString& routeRoot,
		const QVector<hnOutExcelMile>& speedRows,
		const hnGjConvertSourceService::ProgressCallback& callback, int& currentTask,
		int totalTasks, QString& errorMessage, bool& canceled)
	{
		const QString profileDirectory = rutProfileDirectory(context);
		const QFileInfoList files = QDir(profileDirectory).entryInfoList(
			QStringList() << QStringLiteral("*.dtw"), QDir::Files, QDir::Name);
		if (files.isEmpty()) return true;

		const int cameraIndex = context.project->get2DProject()->_RutMode == 1 ? 1 : 0;
		QSettings settings(QDir(context.basePath).filePath(
			QStringLiteral("camera%1/rutcfg.ini").arg(cameraIndex)), QSettings::IniFormat);
		const int horizontalPixels = settings.value(QStringLiteral("camera/hpixel"), 2048).toInt();
		const double scale = settings.value(QStringLiteral("rut/scaleval"), 10.0).toDouble();
		int astart = settings.value(QStringLiteral("camera/rutastart"), 0).toInt();
		int cend = settings.value(QStringLiteral("camera/rutcend"), horizontalPixels - 1).toInt();
		astart = qBound(0, astart, horizontalPixels - 1);
		cend = qBound(astart, cend, horizontalPixels - 1);
		if (horizontalPixels <= 0 || scale <= 0.0 || cend < astart)
		{
			errorMessage = QStringLiteral("车辙原始断面配置无效：hpixel=%1，scaleval=%2。\n%3")
				.arg(horizontalPixels).arg(scale)
				.arg(settings.fileName());
			return false;
		}

		QFile output;
		if (!openUtf8TextFile(output, QDir(routeRoot).filePath(QStringLiteral("RDFile/"))
			+ dataFileName(context, QStringLiteral("TP")), errorMessage))
			return false;
		QStringList header;
		header << QStringLiteral("起点桩号(km)");
		for (int pixel = astart; pixel <= cend; ++pixel)
			header << QStringLiteral("高程值%1(mm)").arg(pixel - astart + 1);
		for (int pixel = astart; pixel <= cend; ++pixel)
			header << QStringLiteral("位置%1(mm)").arg(pixel - astart + 1);
		header << QStringLiteral("速度(m/s)");
		if (!writeUtf8Line(output, header.join(QLatin1Char(',')), errorMessage))
			return false;

		const qint64 frameBytes = static_cast<qint64>(horizontalPixels) * 2;
		QByteArray frame(static_cast<int>(frameBytes), 0);
		QByteArray lastFrame;
		qint64 frameIndex = 0;
		const qint64 maximumFrames = qMax<qint64>(1,
			static_cast<qint64>(std::floor(qAbs(context.endMile - context.startMile) / 0.1)));
		const qint64 progressChunk = 1000;
		for (const QFileInfo& fileInfo : files)
		{
			QFile input(fileInfo.absoluteFilePath());
			if (!input.open(QIODevice::ReadOnly))
			{
				errorMessage = QStringLiteral("无法读取车辙原始断面：%1\n%2")
					.arg(fileInfo.absoluteFilePath(), input.errorString());
				return false;
			}
			while (frameIndex < maximumFrames)
			{
				const qint64 readBytes = input.read(frame.data(), frameBytes);
				if (readBytes == 0) break;
				if (readBytes != frameBytes)
				{
					errorMessage = QStringLiteral("车辙原始断面文件尾部不完整：%1")
						.arg(fileInfo.absoluteFilePath());
					return false;
				}
				lastFrame = frame;
				if (!writeRutProfileLine(output, context, speedRows, lastFrame,
					scale, astart, cend, frameIndex, errorMessage)) return false;
				++frameIndex;
				if ((frameIndex % progressChunk) == 0 && callback)
				{
					++currentTask;
					if (!callback(currentTask, totalTasks,
						QStringLiteral("工程【%1】：正在流式写入 RDFile 断面 %2 行")
						.arg(context.projectName).arg(frameIndex)))
					{
						canceled = true;
						return false;
					}
				}
			}
			if (frameIndex >= maximumFrames) break;
		}
		if (lastFrame.isEmpty())
		{
			errorMessage = QStringLiteral("车辙原始断面没有可用完整帧。\n%1")
				.arg(profileDirectory);
			return false;
		}

		// 与 LP 的兼容规则一致：原始数据不足时使用最后一个有效断面补齐工程尾部。
		const qint64 actualFrameCount = frameIndex;
		if (frameIndex < maximumFrames && callback
			&& !callback(currentTask, totalTasks, QStringLiteral("工程【%1】：车辙断面实际 %2 帧，正在使用最后一帧补齐 %3 帧")
				.arg(context.projectName).arg(actualFrameCount).arg(maximumFrames - actualFrameCount)))
		{
			canceled = true;
			return false;
		}
		while (frameIndex < maximumFrames)
		{
			if (!writeRutProfileLine(output, context, speedRows, lastFrame,
				scale, astart, cend, frameIndex, errorMessage)) return false;
			++frameIndex;
			if ((frameIndex % progressChunk) == 0 && callback)
			{
				++currentTask;
				if (!callback(currentTask, totalTasks, QStringLiteral("工程【%1】：正在补齐 RDFile 断面 %2 行")
					.arg(context.projectName).arg(frameIndex)))
				{
					canceled = true;
					return false;
				}
			}
		}
		if (frameIndex % progressChunk != 0 && callback)
		{
			++currentTask;
			if (!callback(currentTask, totalTasks, QStringLiteral("工程【%1】：RDFile 已写入 %2 个断面")
				.arg(context.projectName).arg(frameIndex)))
			{
				canceled = true;
				return false;
			}
		}
		return true;
	}

	class LaserReader
	{
	public:
		explicit LaserReader(const QString& path)
			: m_file(path), m_stream(&m_file), m_step(0), m_index(0),
			m_previous(0.0), m_next(0.0), m_available(false), m_hasNext(false) {}
		bool open(QString& errorMessage)
		{
			if (!QFileInfo(m_file.fileName()).exists()) return true;
			if (!m_file.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				errorMessage = QStringLiteral("无法读取 TT 断面高程数据：\n%1\n%2")
					.arg(m_file.fileName(), m_file.errorString());
				return false;
			}
			QSettings settings(QFileInfo(m_file.fileName()).dir().filePath(QStringLiteral("Setting.ini")), QSettings::IniFormat);
			m_step = settings.value(QStringLiteral("Parm/PMode")).toInt();
			if (m_step <= 0)
			{
				errorMessage = QStringLiteral("TT 原始采样间距 PMode 无效：\n%1").arg(settings.fileName());
				return false;
			}
			m_available = true;
			if (!readValue(m_previous, errorMessage))
			{
				if (errorMessage.isEmpty()) errorMessage = QStringLiteral("TT 断面高程数据为空：\n%1").arg(m_file.fileName());
				return false;
			}
			m_hasNext = readValue(m_next, errorMessage);
			return errorMessage.isEmpty();
		}
		bool available() const { return m_available; }
		bool valueAt(qint64 offsetMm, double& value, QString& errorMessage)
		{
			value = 0.0;
			if (!m_available) return true;
			const qint64 index = offsetMm / m_step;
			while (m_index < index)
			{
				if (!m_hasNext)
				{
					errorMessage = QStringLiteral("TT 断面高程数据长度不足，无法覆盖工程范围：\n%1\n距起点 %2 mm")
						.arg(m_file.fileName()).arg(offsetMm);
					return false;
				}
				m_previous = m_next;
				++m_index;
				m_hasNext = readValue(m_next, errorMessage);
				if (!errorMessage.isEmpty()) return false;
			}
			// 第二列为去噪后高程，按实际采样间距线性插值到 1 mm。
			// 末点仅延续到最后一个采样间隔末尾，超出后报告数据不足。
			const double fraction = double(offsetMm % m_step) / m_step;
			value = m_hasNext ? m_previous + (m_next - m_previous) * fraction : m_previous;
			return true;
		}
	private:
		bool readValue(double& value, QString& errorMessage)
		{
			if (m_stream.atEnd()) return false;
			const QStringList fields = m_stream.readLine().split(QLatin1Char('\t'));
			bool ok = false;
			value = fields.size() > 1 ? fields.at(1).toDouble(&ok) : 0.0;
			if (!ok || !qIsFinite(value) || m_stream.status() != QTextStream::Ok)
			{
				errorMessage = QStringLiteral("TT 数据第二列不是有效高程值：\n%1").arg(m_file.fileName());
				return false;
			}
			return true;
		}
		QFile m_file;
		QTextStream m_stream;
		int m_step;
		qint64 m_index;
		double m_previous;
		double m_next;
		bool m_available;
		bool m_hasNext;
	};

	bool writeTtFile(const ExportContext& context, const QString& routeRoot,
		const hnGjConvertSourceService::ProgressCallback& callback, int& currentTask,
		int totalTasks, QString& errorMessage, QStringList& warnings, bool& canceled)
	{
		const QString base = context.basePath;
		LaserReader left(QDir(base).filePath(QStringLiteral("IRIMTD/Laser0/lasval.txt")));
		LaserReader right(QDir(base).filePath(QStringLiteral("IRIMTD/Laser1/lasval.txt")));
		LaserReader middle(QDir(base).filePath(QStringLiteral("IRIMTD/Laser2/lasval.txt")));
		if (!left.open(errorMessage) || !middle.open(errorMessage) || !right.open(errorMessage)) return false;
		if (!left.available() && !right.available() && !middle.available()) return true;
		QFile file;
		if (!openUtf8TextFile(file, QDir(routeRoot).filePath(QStringLiteral("TTFile/"))
			+ dataFileName(context, QStringLiteral("TT")), errorMessage)
			|| !writeUtf8Line(file, QStringLiteral("起点桩号(km),左断面高程(mm),中断面高程(mm),右断面高程(mm)"), errorMessage))
			return false;
		const qint64 targetPoints = qRound64(qAbs(context.endMile - context.startMile) * 1000.0);
		for (qint64 point = 0; point < targetPoints; ++point)
		{
			double values[3] = { 0.0, 0.0, 0.0 };
			if (!left.valueAt(point, values[0], errorMessage)
				|| !middle.valueAt(point, values[1], errorMessage)
				|| !right.valueAt(point, values[2], errorMessage))
			{
				if (errorMessage.startsWith(QStringLiteral("TT 断面高程数据长度不足")))
				{
					warnings.append(QStringLiteral("TT 断面高程数据长度不足，已按 lasval.txt 有效长度输出，后续断面未生成：\n%1")
						.arg(errorMessage));
					errorMessage.clear();
					return true;
				}
				return false;
			}
			const double mile = context.startMile + context.direction * point * 0.001;
			if (!writeUtf8Line(file, QStringLiteral("%1,%2,%3,%4")
				.arg(fixed(mile * 0.001, 6), fixed(values[0], 2), fixed(values[1], 2), fixed(values[2], 2)), errorMessage))
				return false;
			if (point > 0 && (point % 500000) == 0 && callback)
			{
				++currentTask;
				if (!callback(currentTask, totalTasks, QStringLiteral("工程【%1】：正在流式写入 TT 断面高程 %2 行")
					.arg(context.projectName).arg(point)))
				{
					canceled = true;
					return false;
				}
			}
		}
		return true;
	}

	bool validateGeneratedTree(const ExportContext& context, const QString& tempRoot, QString& errorMessage)
	{
		const QString routeRoot = QDir(tempRoot).filePath(context.routeName);
		for (const QString& directory : requiredDirectories(context))
		{
			if (!QFileInfo(QDir(routeRoot).filePath(directory)).isDir())
			{
				errorMessage = QStringLiteral("生成结果校验失败，缺少目录：%1")
					.arg(QDir(routeRoot).filePath(directory));
				return false;
			}
		}
		QStringList requiredDataDirs;
		if (context.exportStandard == hnGjExportStandard::NationalRoad2026)
		{
			if (context.outputSelection.iri)
			{
				requiredDataDirs << QStringLiteral("GPS")
					<< national2026Directory(context, QStringLiteral("IRI"), true);
			}
		}
		else
		{
			if (context.outputSelection.dr) requiredDataDirs << QStringLiteral("DR");
			if (context.outputSelection.iri) requiredDataDirs << QStringLiteral("IRI");
			if (context.outputSelection.lbiFile) requiredDataDirs << QStringLiteral("LBIFile");
			if (context.outputSelection.riFile) requiredDataDirs << QStringLiteral("RIFile");
			if (context.standard == HnProjectEnums::DegreeRoad2018)
			{
				if (context.outputSelection.haFile && context.hasGeometryResult)
				{
					requiredDataDirs << QStringLiteral("HAFile");
				}
			}
		}
		for (const QString& directory : requiredDataDirs)
		{
			const QFileInfoList files = QDir(QDir(routeRoot).filePath(directory)).entryInfoList(
				QStringList() << QStringLiteral("*.txt") << QStringLiteral("*.csv"), QDir::Files);
			if (files.isEmpty())
			{
				errorMessage = QStringLiteral("生成结果校验失败，目录中没有数据文件：%1")
					.arg(QDir(routeRoot).filePath(directory));
				return false;
			}
			for (const QFileInfo& file : files)
			{
				QFile input(file.absoluteFilePath());
				if (!input.open(QIODevice::ReadOnly))
				{
					errorMessage = QStringLiteral("生成结果校验失败，文件为空或不可读：%1")
						.arg(file.absoluteFilePath());
					return false;
				}
				const QByteArray headerBytes = input.readLine();
				const QString header = QString::fromUtf8(headerBytes).trimmed();
				if (header.isEmpty() || header.contains(QChar::ReplacementCharacter)
					|| !headerBytes.endsWith("\r\n"))
				{
					errorMessage = QStringLiteral("生成结果校验失败，表头编码或换行符不符合 UTF-8/CRLF 契约：%1")
						.arg(file.absoluteFilePath());
					return false;
				}
				const int fieldCount = header.split(QLatin1Char(',')).size();
				for (int sample = 0; sample < 5 && !input.atEnd(); ++sample)
				{
					const QByteArray rowBytes = input.readLine();
					const QString row = QString::fromUtf8(rowBytes).trimmed();
					const int rowFieldCount = row.split(QLatin1Char(',')).size();
					// 二维软件的 2026 GPS 模板表头把末尾两个核对值合并命名为“桩号核对”，数据仍为两列。
					const bool compatibleNationalGps = context.exportStandard == hnGjExportStandard::NationalRoad2026
						&& directory == QStringLiteral("GPS") && rowFieldCount == fieldCount + 1;
					if (row.isEmpty() || row.contains(QChar::ReplacementCharacter)
						|| !rowBytes.endsWith("\r\n")
						|| (rowFieldCount != fieldCount && !compatibleNationalGps))
					{
						errorMessage = QStringLiteral("生成结果校验失败，字段数、编码或换行符不符合契约：%1")
							.arg(file.absoluteFilePath());
						return false;
					}
				}
			}
		}
		return true;
	}

	bool copyDirectoryRecursively(const QString& sourcePath, const QString& destinationPath,
		QString& errorMessage)
	{
		if (!QDir().mkpath(destinationPath))
		{
			errorMessage = QStringLiteral("无法创建备份目录：%1").arg(destinationPath);
			return false;
		}

		const QDir source(sourcePath);
		const QFileInfoList entries = source.entryInfoList(
			QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System);
		for (const QFileInfo& entry : entries)
		{
			const QString destination = QDir(destinationPath).filePath(entry.fileName());
			if (entry.isDir() && !entry.isSymLink())
			{
				if (!copyDirectoryRecursively(entry.absoluteFilePath(), destination, errorMessage))
				{
					return false;
				}
			}
			else if (!QFile::copy(entry.absoluteFilePath(), destination))
			{
				errorMessage = QStringLiteral("无法备份文件：%1").arg(entry.absoluteFilePath());
				return false;
			}
		}
		return true;
	}

	bool renameWithinDirectory(const QString& parentPath, const QString& oldName, const QString& newName)
	{
		QDir parent(parentPath);
		for (int attempt = 0; attempt < 3; ++attempt)
		{
			if (parent.rename(oldName, newName))
			{
				return true;
			}
			if (attempt < 2)
			{
				QThread::msleep(100);
			}
		}
		return false;
	}

	bool replaceConverSource(const ExportContext& context, const QString& tempRoot, QString& errorMessage)
	{
		const QString parentPath = QFileInfo(context.outputPath).absolutePath();
		const QString officialName = QFileInfo(context.outputPath).fileName();
		const QString tempName = QFileInfo(tempRoot).fileName();
		const QString backupName = QStringLiteral(".ConverSource.old-%1")
			.arg(QUuid::createUuid().toString().remove(QLatin1Char('{')).remove(QLatin1Char('}')));
		const QString backupPath = QDir(parentPath).filePath(backupName);
		bool oldMoved = false;
		bool oldCopied = false;
		if (QFileInfo(context.outputPath).exists())
		{
			if (!renameWithinDirectory(parentPath, officialName, backupName))
			{
				// Windows 下旧目录含有历史文件时整体重命名可能失败；先做完整备份，再安全切换新结果。
				QString backupError;
				if (!copyDirectoryRecursively(context.outputPath, backupPath, backupError))
				{
					QDir(backupPath).removeRecursively();
					errorMessage = QStringLiteral("无法临时移开旧 ConverSource：%1\n%2")
						.arg(context.outputPath, backupError);
					return false;
				}
				if (!QDir(context.outputPath).removeRecursively())
				{
					errorMessage = QStringLiteral("无法清理旧 ConverSource，完整备份已保留：%1\n请关闭占用旧目录的程序后重试。")
						.arg(backupPath);
					return false;
				}
				oldCopied = true;
			}
			else
			{
				oldMoved = true;
			}
		}
		if (!renameWithinDirectory(parentPath, tempName, officialName))
		{
			bool restored = false;
			if (oldMoved)
			{
				restored = renameWithinDirectory(parentPath, backupName, officialName);
			}
			else if (oldCopied)
			{
				QString restoreError;
				if (QFileInfo(context.outputPath).exists())
				{
					QDir(context.outputPath).removeRecursively();
				}
				restored = copyDirectoryRecursively(backupPath, context.outputPath, restoreError);
				if (!restored && !restoreError.isEmpty())
				{
					errorMessage = QStringLiteral("无法将新结果切换为正式 ConverSource：%1\n恢复旧 ConverSource 失败：%2")
						.arg(context.outputPath, restoreError);
				}
			}
			if (restored && oldCopied)
			{
				QDir(backupPath).removeRecursively();
			}
			if (!restored)
			{
				if (errorMessage.isEmpty())
				{
					errorMessage = QStringLiteral("无法将新结果切换为正式 ConverSource，旧结果已尝试恢复：%1")
						.arg(context.outputPath);
				}
			}
			return false;
		}
		if (oldMoved)
		{
			QDir backup(backupPath);
			if (!backup.removeRecursively())
			{
				// 删除旧目录失败时回滚，保证“失败则旧结果不丢失”。
				const bool newMovedBack = renameWithinDirectory(parentPath, officialName, tempName);
				const bool oldRestored = newMovedBack && renameWithinDirectory(parentPath, backupName, officialName);
				if (oldRestored)
				{
					QDir(QDir(parentPath).filePath(tempName)).removeRecursively();
				}
				errorMessage = oldRestored
					? QStringLiteral("无法删除临时旧目录，已恢复原 ConverSource：%1").arg(backup.absolutePath())
					: QStringLiteral("删除临时旧目录失败且自动恢复未完整完成，请人工检查：%1").arg(parentPath);
				return false;
			}
		}
		else if (oldCopied)
		{
			QDir(backupPath).removeRecursively();
		}
		return true;
	}

	bool generateProject(const ExportContext& context, const QString& tempRoot,
		const hnGjConvertSourceService::ProgressCallback& callback, int& currentTask,
		int totalTasks, hnGjProjectResult& result, bool& canceled)
	{
		const QString routeRoot = QDir(tempRoot).filePath(context.routeName);
		for (const QString& directory : requiredDirectories(context))
		{
			QString path = QDir(routeRoot).filePath(directory);
			if (context.exportStandard == hnGjExportStandard::Standard2025
				&& (directory == QStringLiteral("Images") || directory == QStringLiteral("ViewImages")))
			{
				path = QDir(path).filePath(context.timeText);
			}
			if (!createDirectory(path, result.errorMessage)) return false;
			++currentTask;
			if (callback && !callback(currentTask, totalTasks,
				QStringLiteral("工程【%1】：创建目录 %2").arg(context.projectName, directory)))
			{
				canceled = true;
				return false;
			}
		}

		auto completed = [&](const QString& message) -> bool
		{
			++currentTask;
			if (callback && !callback(currentTask, totalTasks,
				QStringLiteral("工程【%1】：%2").arg(context.projectName, message)))
			{
				canceled = true;
				return false;
			}
			return true;
		};

		if (context.exportStandard == hnGjExportStandard::NationalRoad2026)
		{
			QString segmentError;
			const bool hasRut = context.project->get2DProject()->_IsRut;
			QVector<hnOutExcelMile> tenMeterRows;
			QVector<hnOutExcelMile> fiveMeterRows;
			if (context.outputSelection.iri || context.outputSelection.rd || context.outputSelection.pb)
			{
				tenMeterRows = createMetricRows(context, 10.0, hasRut && context.outputSelection.rd,
					false, context.outputSelection.pb, context.outputSelection.iri,
					context.outputSelection.iri || context.outputSelection.pb,
					false, segmentError, false);
				if (tenMeterRows.isEmpty())
				{
					result.errorMessage = segmentError.isEmpty()
						? QStringLiteral("没有生成任何 10 米分段结果。") : segmentError;
					return false;
				}
			}
			if (context.outputSelection.iri)
			{
				fiveMeterRows = createMetricRows(context, 5.0, false, false, false,
					false, false, true, segmentError, false);
				if (fiveMeterRows.isEmpty())
				{
					result.errorMessage = segmentError.isEmpty()
						? QStringLiteral("没有生成任何 5 米 GPS 分段结果。") : segmentError;
					return false;
				}
			}

			if (context.project->get2DProject()->_IsRoad || context.project->get2DProject()->_IsStreet)
			{
				if (!writeDatabaseImageIndexes(context, result.errorMessage)
					|| !completed(QStringLiteral("图片 2Mile.txt 已按成果库校桩覆盖生成")))
				{
					return false;
				}
			}
			if (context.outputSelection.iri && (!writeNational2026IriFile(context, routeRoot, tenMeterRows, result.errorMessage)
				|| !completed(QStringLiteral("2026 IRI 已生成")))) return false;
			if (context.outputSelection.iri && (!writeNational2026GpsFile(context, routeRoot, fiveMeterRows, result.errorMessage)
				|| !completed(QStringLiteral("2026 GPS 已生成")))) return false;
			if (context.outputSelection.rd && hasRut && (!writeNational2026RutFile(context, routeRoot, tenMeterRows, result.errorMessage)
				|| !completed(QStringLiteral("2026 RDFile 已生成")))) return false;
			if (context.outputSelection.pb && (!writeNational2026BumpFile(context, routeRoot, tenMeterRows, result.errorMessage)
				|| !completed(QStringLiteral("2026 BUMP 已生成")))) return false;
			if (!validateGeneratedTree(context, tempRoot, result.errorMessage)
				|| !completed(QStringLiteral("2026 格式校验通过"))) return false;
			return true;
		}

		const bool hasRut = context.standard == HnProjectEnums::DegreeRoad2018
			&& context.project->get2DProject()->_IsRut;
		const bool hasMpd = context.standard == HnProjectEnums::DegreeRoad2018 && hasMpdData(context);
		if (context.outputSelection.mpd && context.standard == HnProjectEnums::DegreeRoad2018 && !hasMpd)
		{
			result.skippedOutputs.append(QStringLiteral("MPD"));
			QStringList mpdPaths;
			const QString iriPath = context.project->get2DProject()->getIRIPath();
			mpdPaths << QDir(iriPath).filePath(QStringLiteral("Laser0/MPD_10m.txt"));
			if (context.project->get2DProject()->_IsDIRIMTD)
			{
				mpdPaths << QDir(iriPath).filePath(QStringLiteral("Laser1/MPD_10m.txt"));
			}
			result.warnings.append(QStringLiteral("未找到MPD_10m计算结果，MPD目录保持为空；请先执行IRM中的MPD计算。\n%1")
				.arg(mpdPaths.join(QStringLiteral("\n"))));
		}
		QString segmentError;
		const bool needPb = context.standard == HnProjectEnums::DegreeRoad2018 && context.outputSelection.pb;
		QVector<hnOutExcelMile> drRows;
		if (context.outputSelection.dr)
		{
			drRows = createTenMeterRows(context, false, false, false,
				false, false, false, segmentError, true);
			if (drRows.isEmpty())
			{
				result.errorMessage = segmentError.isEmpty() ? QStringLiteral("没有生成任何 10 米分段结果。") : segmentError;
				return false;
			}
		}
		QString continuousSegmentError;
		const bool needContinuousRows = context.outputSelection.iri || context.outputSelection.lbiFile
			|| context.outputSelection.haFile || context.outputSelection.rd || context.outputSelection.mpd
			|| context.outputSelection.smtd || needPb || context.outputSelection.rdFile;
		QVector<hnOutExcelMile> continuousRows;
		if (needContinuousRows)
		{
			continuousRows = createTenMeterRows(context,
				hasRut && (context.outputSelection.rd || context.outputSelection.rdFile),
				hasMpd && context.outputSelection.mpd, needPb, context.outputSelection.iri,
				context.outputSelection.iri || context.outputSelection.riFile
					|| context.outputSelection.smtd || needPb
					|| context.outputSelection.rdFile,
				context.outputSelection.lbiFile, continuousSegmentError, false);
			if (continuousRows.isEmpty())
			{
				result.errorMessage = continuousSegmentError.isEmpty()
					? QStringLiteral("没有生成不按材质标记拆分的 10 米分段结果。") : continuousSegmentError;
				return false;
			}
		}

		if (context.project->get2DProject()->_IsRoad || context.project->get2DProject()->_IsStreet)
		{
			if (!writeDatabaseImageIndexes(context, result.errorMessage)
				|| !completed(QStringLiteral("图片 2Mile.txt 已按成果库校桩覆盖生成")))
			{
				return false;
			}
		}

		if (context.outputSelection.dr && (!writeDrFiles(context, routeRoot, drRows, result.errorMessage) || !completed(QStringLiteral("DR 已生成")))) return false;
		if (context.outputSelection.iri && (!writeIriFile(context, routeRoot, continuousRows, result.errorMessage) || !completed(QStringLiteral("IRI 已生成")))) return false;
		if (context.outputSelection.lbiFile && (!writeLbiFile(context, routeRoot, continuousRows, result.errorMessage) || !completed(QStringLiteral("LBI 已生成")))) return false;
		if (context.standard == HnProjectEnums::DegreeRoad2018
			&& context.hasGeometryResult)
		{
			if (context.outputSelection.haFile && (!writeHaFile(context, routeRoot, continuousRows, result.errorMessage)
				|| !completed(QStringLiteral("HA 已生成")))) return false;
		}
		else if (context.outputSelection.haFile && context.standard == HnProjectEnums::DegreeRoad2018
			&& context.project->getProjectType() == hnCommon::PROJECT_23D_TYPE)
		{
			result.skippedOutputs.append(QStringLiteral("HAFile"));
			result.warnings.append(QStringLiteral(
				"未找到有效的路面几何结果，本次未生成HAFile，其他国检结果已正常生成。\n%1")
				.arg(QDir(context.basePath).filePath(QStringLiteral("Geoalig_10m.txt"))));
		}
		if (context.outputSelection.riFile && (!writeLpFile(context, routeRoot, continuousRows, callback, currentTask, totalTasks, result.errorMessage, canceled)
			|| !completed(QStringLiteral("LP 已生成")))) return false;

		if (context.outputSelection.rd && hasRut && (!writeRdFile(context, routeRoot, continuousRows, result.errorMessage)
			|| !completed(QStringLiteral("RD 已生成")))) return false;
		if (context.outputSelection.mpd && hasMpd && (!writeMpdFile(context, routeRoot, continuousRows, result.errorMessage)
			|| !completed(QStringLiteral("MPD 已生成")))) return false;
		if (context.outputSelection.smtd && context.standard == HnProjectEnums::DegreeRoad2018)
		{
			const QStringList missingSmtd = missingLaserFiles(context, QStringLiteral("MTD_10m.txt"));
			if (!missingSmtd.isEmpty())
			{
				result.skippedOutputs.append(QStringLiteral("SMTD"));
				result.warnings.append(QStringLiteral("缺少 SMTD 计算结果，SMTD 目录保持为空；请先执行 IRM 中的 SMTD 计算。其他文件继续生成。\n%1")
					.arg(missingSmtd.join(QStringLiteral("\n"))));
				if (!completed(QStringLiteral("SMTD 已跳过：缺少计算结果"))) return false;
			}
			else
			{
				QString smtdError;
				// 不按材质标记拆分，按现有分段器生成连续 10 m SMTD 成果。
				const QVector<hnOutExcelMile> smtdRows = createMetricRows(
					context, 10.0, false, false, false, false, true, false,
					smtdError, false, true);
				if (smtdRows.isEmpty())
				{
					result.errorMessage = QStringLiteral("SMTD 10 米分段失败：%1").arg(smtdError);
					return false;
				}
				if (!writeSmtdFile(context, routeRoot, smtdRows, result.errorMessage)
					|| !completed(QStringLiteral("SMTD 已生成"))) return false;
			}
		}
		if (context.outputSelection.pb && needPb && (!writePbFile(context, routeRoot, continuousRows, result.errorMessage)
			|| !completed(QStringLiteral("PB 已生成")))) return false;
		if (context.outputSelection.rdFile && hasRut && !QDir(rutProfileDirectory(context)).entryList(
			QStringList() << QStringLiteral("*.dtw"), QDir::Files).isEmpty())
		{
			if (!writeRutProfileFile(context, routeRoot, continuousRows, callback, currentTask, totalTasks,
				result.errorMessage, canceled)) return false;
		}
		if (context.outputSelection.ttFile && context.standard == HnProjectEnums::DegreeRoad2018)
		{
			const QStringList missingLasval = missingLaserFiles(context, QStringLiteral("lasval.txt"));
			if (!missingLasval.isEmpty())
			{
				result.skippedOutputs.append(QStringLiteral("TTFile"));
				result.warnings.append(QStringLiteral("缺少纹理原始数据 lasval.txt，本次未生成 TT 文件；请先执行 IRM 中的 SMTD 计算以补齐。其他文件继续生成。\n%1")
					.arg(missingLasval.join(QStringLiteral("\n"))));
				if (!completed(QStringLiteral("TT 已跳过：缺少 lasval.txt"))) return false;
			}
			else if (!writeTtFile(context, routeRoot, callback, currentTask, totalTasks,
				result.errorMessage, result.warnings, canceled) || !completed(QStringLiteral("TT 已生成"))) return false;
		}
		if (!validateGeneratedTree(context, tempRoot, result.errorMessage)
			|| !completed(QStringLiteral("格式校验通过"))) return false;
		return true;
	}
}

hnGjOutputSelection::hnGjOutputSelection()
	: dr(true), iri(true), rd(true), pb(true), mpd(true), smtd(true),
		lbiFile(true), haFile(true), riFile(true), rdFile(true), ttFile(true), lFile(true)
{
}

hnGjOutputSelection hnGjOutputSelection::allFor(hnGjExportStandard standard)
{
	hnGjOutputSelection selection;
	if (standard == hnGjExportStandard::NationalRoad2026)
	{
		selection.dr = false;
		selection.mpd = false;
		selection.smtd = false;
		selection.lbiFile = false;
		selection.haFile = false;
		selection.riFile = false;
		selection.rdFile = false;
		selection.ttFile = false;
		selection.lFile = false;
	}
	return selection;
}

hnGjProjectResult::hnGjProjectResult()
	: sourceIndex(-1), success(false), skipped(false)
{
}

hnGjBatchResult::hnGjBatchResult()
	: canceled(false)
{
}

int hnGjBatchResult::successCount() const
{
	int count = 0;
	for (const hnGjProjectResult& result : projects) if (result.success) ++count;
	return count;
}

int hnGjBatchResult::completeSuccessCount() const
{
	int count = 0;
	for (const hnGjProjectResult& result : projects)
	{
		if (result.success && result.skippedOutputs.isEmpty()) ++count;
	}
	return count;
}

int hnGjBatchResult::partialSuccessCount() const
{
	int count = 0;
	for (const hnGjProjectResult& result : projects)
	{
		if (result.success && !result.skippedOutputs.isEmpty()) ++count;
	}
	return count;
}

int hnGjBatchResult::failedCount() const
{
	int count = 0;
	for (const hnGjProjectResult& result : projects) if (!result.success && !result.skipped) ++count;
	return count;
}

int hnGjBatchResult::skippedCount() const
{
	int count = 0;
	for (const hnGjProjectResult& result : projects) if (result.skipped) ++count;
	return count;
}

QString hnGjConvertSourceService::readCountyCode(hnPro::hnProject* project)
{
	if (!project || !project->get2DProject()) return QString();
	QString code = countyCodeFromText(readProjectInfoText(project));
	if (code.isEmpty()) code = digitsOnly(project->get2DProject()->_CityCode);
	return code;
}

QString hnGjConvertSourceService::standardDisplayName(hnGjExportStandard standard)
{
	if (standard == hnGjExportStandard::NationalRoad2026)
	{
		return QStringLiteral("农养国省道路况检测数据提交格式_2026年");
	}
	return QStringLiteral("公路路面技术状况自动化检测规程");
}

hnGjBatchResult hnGjConvertSourceService::exportBatch(
	const std::vector<hnPro::hnProject*>& projects,
	const QString& batchCountyCode,
	hnGjExportStandard exportStandard,
	const hnGjOutputSelection& outputSelection,
	const ProgressCallback& progressCallback) const
{
	hnGjBatchResult batchResult;
	QVector<ExportContext> contexts;
	QVector<hnGjProjectResult> rejectedProjects;
	bool preflightCanceled = false;
	if (!preflight(projects, batchCountyCode, exportStandard, outputSelection, contexts,
		rejectedProjects, batchResult.preflightErrors, progressCallback, preflightCanceled))
	{
		batchResult.canceled = preflightCanceled;
		if (preflightCanceled)
		{
			for (size_t index = 0; index < projects.size(); ++index)
			{
				hnGjProjectResult result;
				result.sourceIndex = static_cast<int>(index);
				result.projectName = projects[index] ? projects[index]->get2DProName()
					: QStringLiteral("第 %1 个工程").arg(index + 1);
				result.skipped = true;
				result.errorMessage = QStringLiteral("用户在预检阶段取消，未开始生成。");
				batchResult.projects.append(result);
			}
		}
		return batchResult;
	}
	batchResult.projects.append(rejectedProjects);

	int totalTasks = 0;
	for (const ExportContext& context : contexts) totalTasks += context.taskCount;
	int currentTask = 0;
	for (int index = 0; index < contexts.size(); ++index)
	{
		const ExportContext& context = contexts.at(index);
		hnGjProjectResult result;
		result.projectName = context.projectName;
		result.sourceIndex = context.sourceIndex;
		result.outputPath = context.outputPath;
		result.warnings = context.preflightWarnings;
		result.skippedOutputs = context.skippedOutputs;
		if (batchResult.canceled)
		{
			result.skipped = true;
			result.errorMessage = QStringLiteral("用户取消，尚未开始处理。");
			batchResult.projects.append(result);
			continue;
		}

		const QString tempName = QStringLiteral(".ConverSource.tmp-%1")
			.arg(QUuid::createUuid().toString().remove(QLatin1Char('{')).remove(QLatin1Char('}')));
		const QString tempRoot = QDir(context.basePath).filePath(tempName);
		QDir(tempRoot).removeRecursively();
		if (!createDirectory(tempRoot, result.errorMessage))
		{
			batchResult.projects.append(result);
			continue;
		}

		bool canceled = false;
		if (generateProject(context, tempRoot, progressCallback, currentTask,
			totalTasks, result, canceled))
		{
			if (replaceConverSource(context, tempRoot, result.errorMessage))
			{
				result.success = true;
			}
		}
		if (!result.success)
		{
			QDir(tempRoot).removeRecursively();
		}
		if (canceled)
		{
			batchResult.canceled = true;
			result.skipped = true;
			if (result.errorMessage.isEmpty()) result.errorMessage = QStringLiteral("用户取消，当前临时目录已清理。");
		}
		batchResult.projects.append(result);
	}
	std::sort(batchResult.projects.begin(), batchResult.projects.end(),
		[](const hnGjProjectResult& left, const hnGjProjectResult& right)
		{
			return left.sourceIndex < right.sourceIndex;
		});
	return batchResult;
}
