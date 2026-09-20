#include "RutProfileDebugExporter.h"

#include "../hnAlgorithm/hnComputeCUT.h"
#include "../hnProject/hn2DProject.h"
#include "../hnProject/hnProject.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QRegExp>
#include <QTextStream>
#include <QtCore/QtMath>
#include <algorithm>
#include <cmath>

namespace
{
	const double kFrameDistance = 0.1;
	const double kHorizontalSpacingMm = 1.5;
	const float kInvalidHeight = 2200.0f;
}

RutProfileDebugExporter::RutProfileDebugExporter(hnPro::hnProject* project,
	const QString& projectBasePath, int horizontalPixels, int verticalPixels, int rutMode)
	: m_project(project),
	m_projectBasePath(projectBasePath),
	m_horizontalPixels(horizontalPixels),
	m_verticalPixels(verticalPixels),
	m_rutMode(rutMode),
	m_dmiScale(1.0),
	m_projectLength(0.0),
	m_enabled(false),
	m_finalized(false)
{
	m_configPath = QDir(projectBasePath).filePath(QStringLiteral("camera0/RutProfileDebugRanges.txt"));
}

RutProfileDebugExporter::~RutProfileDebugExporter()
{
	if (m_enabled && !m_finalized && !m_runDirectory.isEmpty())
	{
		QString ignoredError;
		finish(false, QStringLiteral("车辙计算提前结束。"), ignoredError);
	}
}

// 只有明确放置触发文件后才初始化导出，普通客户工程只执行一次文件存在性判断。
bool RutProfileDebugExporter::initialize(QString& errorMessage)
{
	if (!QFileInfo::exists(m_configPath))
	{
		return true;
	}

	m_enabled = true;
	if (m_project == nullptr || m_project->get2DProject() == nullptr)
	{
		errorMessage = QStringLiteral("车辙断面调试配置已启用，但当前二维工程无效：%1").arg(m_configPath);
		return false;
	}
	if (m_rutMode != 0 && m_rutMode != 2)
	{
		errorMessage = QStringLiteral("车辙断面调试仅支持双轮迹模式 0/2，当前模式为 %1。\r\n配置文件：%2")
			.arg(m_rutMode).arg(m_configPath);
		return false;
	}

	m_dmiScale = m_project->get2DProject()->_DMIScale;
	if (!qIsFinite(m_dmiScale) || m_dmiScale <= 0.0)
	{
		errorMessage = QStringLiteral("车辙断面调试无法使用无效的 DMI 比例：%1。\r\n配置文件：%2")
			.arg(m_dmiScale, 0, 'g', 15).arg(m_configPath);
		return false;
	}

	m_projectLength = m_project->getCurProSetInfo().dEndEnclMile;
	if (!qIsFinite(m_projectLength) || m_projectLength <= 0.0)
	{
		m_projectLength = qAbs(m_project->getCurProSetInfo().dEndMile
			- m_project->getCurProSetInfo().dBegMile);
	}
	if (!qIsFinite(m_projectLength) || m_projectLength <= 0.0)
	{
		errorMessage = QStringLiteral("车辙断面调试无法确定工程有效长度。\r\n配置文件：%1")
			.arg(m_configPath);
		return false;
	}

	return parseConfiguration(errorMessage) && createOutputFiles(errorMessage);
}

bool RutProfileDebugExporter::isEnabled() const
{
	return m_enabled;
}

bool RutProfileDebugExporter::shouldExport(qint64 frameIndex) const
{
	if (!m_enabled)
	{
		return false;
	}
	for (const FrameRange& range : m_ranges)
	{
		if (frameIndex >= range.startFrame && frameIndex <= range.endFrame
			&& ((frameIndex - range.startFrame) % range.step) == 0)
		{
			return true;
		}
	}
	return false;
}

// 配置文件固定按 UTF-8 读取，支持注释、空行以及多组 PILE/DMI 区间。
bool RutProfileDebugExporter::parseConfiguration(QString& errorMessage)
{
	QFile file(m_configPath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		errorMessage = QStringLiteral("无法读取车辙断面调试配置：%1\r\n%2")
			.arg(m_configPath, file.errorString());
		return false;
	}

	QTextStream stream(&file);
	stream.setCodec("UTF-8");
	int lineNumber = 0;
	while (!stream.atEnd())
	{
		++lineNumber;
		const QString line = stream.readLine().trimmed();
		if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
		{
			continue;
		}
		QStringList fields = line.split(QLatin1Char(','), QString::KeepEmptyParts);
		for (QString& field : fields)
		{
			field = field.trimmed();
		}
		if (!addRange(fields, lineNumber, errorMessage))
		{
			return false;
		}
	}
	file.close();

	if (m_ranges.isEmpty())
	{
		errorMessage = QStringLiteral("车辙断面调试配置没有有效区间：%1").arg(m_configPath);
		return false;
	}
	mergeRanges();
	return true;
}

bool RutProfileDebugExporter::parsePileText(const QString& text, double& trueMile)
{
	QRegExp expression(QStringLiteral("^[Kk]?(\\d+)(?:\\+(\\d+(?:\\.\\d+)?))?$"));
	if (!expression.exactMatch(text.trimmed()))
	{
		return false;
	}
	bool kilometerOk = false;
	const double kilometerOrMile = expression.cap(1).toDouble(&kilometerOk);
	if (!kilometerOk)
	{
		return false;
	}
	if (expression.cap(2).isEmpty())
	{
		trueMile = kilometerOrMile;
		return qIsFinite(trueMile);
	}
	bool meterOk = false;
	const double meter = expression.cap(2).toDouble(&meterOk);
	if (!meterOk || meter < 0.0 || meter >= 1000.0)
	{
		return false;
	}
	trueMile = kilometerOrMile * 1000.0 + meter;
	return qIsFinite(trueMile);
}

qint64 RutProfileDebugExporter::firstFrameAtOrAfter(double dmi, double dmiScale)
{
	return qCeil(dmi * dmiScale / kFrameDistance - 1e-9);
}

qint64 RutProfileDebugExporter::lastFrameAtOrBefore(double dmi, double dmiScale)
{
	return qFloor(dmi * dmiScale / kFrameDistance + 1e-9);
}

bool RutProfileDebugExporter::addRange(const QStringList& fields, int lineNumber,
	QString& errorMessage)
{
	if (fields.size() < 3 || fields.size() > 4)
	{
		errorMessage = QStringLiteral("车辙断面调试配置第 %1 行字段数量错误，应为 类型,起点,终点[,抽帧步长]。\r\n%2")
			.arg(lineNumber).arg(m_configPath);
		return false;
	}

	int step = 1;
	if (fields.size() == 4)
	{
		bool stepOk = false;
		step = fields.at(3).toInt(&stepOk);
		if (!stepOk || step <= 0)
		{
			errorMessage = QStringLiteral("车辙断面调试配置第 %1 行抽帧步长必须为正整数：%2")
				.arg(lineNumber).arg(fields.at(3));
			return false;
		}
	}

	double startDmi = 0.0;
	double endDmi = 0.0;
	const QString type = fields.at(0).toUpper();
	if (type == QStringLiteral("PILE"))
	{
		double startPile = 0.0;
		double endPile = 0.0;
		if (!parsePileText(fields.at(1), startPile) || !parsePileText(fields.at(2), endPile))
		{
			errorMessage = QStringLiteral("车辙断面调试配置第 %1 行桩号格式错误：%2,%3")
				.arg(lineNumber).arg(fields.at(1), fields.at(2));
			return false;
		}
		startDmi = m_project->trueMileToEncl(startPile);
		endDmi = m_project->trueMileToEncl(endPile);
	}
	else if (type == QStringLiteral("DMI"))
	{
		bool startOk = false;
		bool endOk = false;
		startDmi = fields.at(1).toDouble(&startOk);
		endDmi = fields.at(2).toDouble(&endOk);
		if (!startOk || !endOk || !qIsFinite(startDmi) || !qIsFinite(endDmi))
		{
			errorMessage = QStringLiteral("车辙断面调试配置第 %1 行 DMI 格式错误：%2,%3")
				.arg(lineNumber).arg(fields.at(1), fields.at(2));
			return false;
		}
	}
	else
	{
		errorMessage = QStringLiteral("车辙断面调试配置第 %1 行类型必须为 PILE 或 DMI：%2")
			.arg(lineNumber).arg(fields.at(0));
		return false;
	}

	if (startDmi > endDmi)
	{
		qSwap(startDmi, endDmi);
	}
	const double tolerance = 0.001;
	if (startDmi < -tolerance || endDmi > m_projectLength + tolerance)
	{
		errorMessage = QStringLiteral("车辙断面调试配置第 %1 行超出工程 DMI 范围 [0,%2]：[%3,%4]")
			.arg(lineNumber).arg(m_projectLength, 0, 'f', 3)
			.arg(startDmi, 0, 'f', 3).arg(endDmi, 0, 'f', 3);
		return false;
	}
	startDmi = qBound(0.0, startDmi, m_projectLength);
	endDmi = qBound(0.0, endDmi, m_projectLength);

	FrameRange range;
	range.startFrame = firstFrameAtOrAfter(startDmi, m_dmiScale);
	range.endFrame = lastFrameAtOrBefore(endDmi, m_dmiScale);
	range.step = step;
	if (range.endFrame < range.startFrame)
	{
		errorMessage = QStringLiteral("车辙断面调试配置第 %1 行没有命中任何 0.1m 断面。").arg(lineNumber);
		return false;
	}
	m_ranges.append(range);
	return true;
}

void RutProfileDebugExporter::mergeRanges()
{
	std::sort(m_ranges.begin(), m_ranges.end(), [](const FrameRange& left, const FrameRange& right)
	{
		return left.startFrame < right.startFrame;
	});
	QVector<FrameRange> merged;
	for (const FrameRange& range : m_ranges)
	{
		if (merged.isEmpty() || range.startFrame > merged.last().endFrame + 1)
		{
			merged.append(range);
		}
		else
		{
			merged.last().endFrame = qMax(merged.last().endFrame, range.endFrame);
			merged.last().step = qMin(merged.last().step, range.step);
		}
	}
	m_ranges = merged;
}

bool RutProfileDebugExporter::createOutputFiles(QString& errorMessage)
{
	const QString root = QDir(m_projectBasePath).filePath(QStringLiteral("RUT/camera0/profile_debug"));
	const QString runName = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));
	m_finalDirectory = QDir(root).filePath(runName);
	m_runDirectory = m_finalDirectory + QStringLiteral("_incomplete");
	if (!QDir().mkpath(m_runDirectory))
	{
		errorMessage = QStringLiteral("无法创建车辙断面调试输出目录：%1").arg(m_runDirectory);
		return false;
	}
	const QString configSnapshot = QDir(m_runDirectory).filePath(
		QStringLiteral("RutProfileDebugRanges.txt"));
	if (!QFile::copy(m_configPath, configSnapshot))
	{
		errorMessage = QStringLiteral("无法保存车辙断面调试配置快照：%1 -> %2")
			.arg(m_configPath, configSnapshot);
		return false;
	}

	const QByteArray ascHeader("// X_mm Y_dmi_mm Z frame_index pixel_index true_mile_m stage_applied\r\n");
	return openOutputFile(m_rawSensorFile, QStringLiteral("00_raw_sensor.asc"), ascHeader, errorMessage)
		&& openOutputFile(m_worldHeightFile, QStringLiteral("01_world_height.asc"), ascHeader, errorMessage)
		&& openOutputFile(m_firstDetrendedFile, QStringLiteral("02_detrended_first.asc"), ascHeader, errorMessage)
		&& openOutputFile(m_firstOutlierFile, QStringLiteral("03_outlier_replaced_first.asc"), ascHeader, errorMessage)
		&& openOutputFile(m_secondDetrendedFile, QStringLiteral("04_detrended_second.asc"), ascHeader, errorMessage)
		&& openOutputFile(m_secondOutlierFile, QStringLiteral("05_outlier_replaced_second.asc"), ascHeader, errorMessage)
		&& openOutputFile(m_filteredFile, QStringLiteral("06_filtered.asc"), ascHeader, errorMessage)
		&& openOutputFile(m_featuresFile, QStringLiteral("rut_features.csv"),
			QByteArray("frame_index,dmi_m,true_mile_m,source_dat,second_stage_applied,first_k,first_b,second_k,second_b,envelope_type,w0_index,w0_value,w1_index,w1_value,w2_index,w2_value,w3_index,w3_value,w4_index,w4_value,left_rut,right_rut,raw_rut\r\n"), errorMessage)
		&& openOutputFile(m_manifestFile, QStringLiteral("manifest.csv"),
			QByteArray("frame_index,dmi_m,true_mile_m,source_dat,raw_valid,world_valid,algorithm_applied,second_stage_applied,filtered_valid,x_spacing_mm,raw_z_unit,processed_z_unit\r\n"), errorMessage);
}

bool RutProfileDebugExporter::openOutputFile(QFile& file, const QString& fileName,
	const QByteArray& header, QString& errorMessage)
{
	file.setFileName(QDir(m_runDirectory).filePath(fileName));
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		errorMessage = QStringLiteral("无法创建车辙断面调试文件：%1\r\n%2")
			.arg(file.fileName(), file.errorString());
		return false;
	}
	if (file.write(header) != header.size())
	{
		errorMessage = QStringLiteral("无法写入车辙断面调试文件头：%1\r\n%2")
			.arg(file.fileName(), file.errorString());
		return false;
	}
	return true;
}

// 每一帧直接写入各步骤文件，内存中只保留当前断面快照。
bool RutProfileDebugExporter::writeFrame(qint64 frameIndex, const QString& sourceDat,
	const std::vector<short>& rawProfile, const std::vector<float>& worldHeight,
	const hnRutProfileTrace* trace, float rawRut, QString& errorMessage)
{
	if (!m_enabled || !shouldExport(frameIndex))
	{
		return true;
	}
	const double dmi = frameIndex * kFrameDistance / m_dmiScale;
	const double trueMile = m_project->enclToTrueMile(dmi);
	if (!writeRawStage(frameIndex, dmi, trueMile, rawProfile, errorMessage)
		|| !writeFloatStage(m_worldHeightFile, frameIndex, dmi, trueMile,
			trace != nullptr ? trace->worldHeight : worldHeight, true, errorMessage))
	{
		return false;
	}

	if (trace != nullptr)
	{
		if (!writeFloatStage(m_firstDetrendedFile, frameIndex, dmi, trueMile,
			trace->firstDetrended, true, errorMessage)
			|| !writeFloatStage(m_firstOutlierFile, frameIndex, dmi, trueMile,
				trace->firstOutlierReplaced, true, errorMessage)
			|| !writeFloatStage(m_secondDetrendedFile, frameIndex, dmi, trueMile,
				trace->secondDetrended, trace->secondStageApplied, errorMessage)
			|| !writeFloatStage(m_secondOutlierFile, frameIndex, dmi, trueMile,
				trace->secondOutlierReplaced, trace->secondStageApplied, errorMessage)
			|| !writeFloatStage(m_filteredFile, frameIndex, dmi, trueMile,
				trace->filteredHeight, true, errorMessage))
		{
			return false;
		}

		QTextStream featureStream(&m_featuresFile);
		featureStream.setCodec("UTF-8");
		featureStream << frameIndex << ',' << QString::number(dmi, 'f', 6) << ','
			<< QString::number(trueMile, 'f', 6) << ',' << csvField(sourceDat) << ','
			<< (trace->secondStageApplied ? 1 : 0) << ','
			<< QString::number(trace->firstFitK, 'g', 9) << ','
			<< QString::number(trace->firstFitB, 'g', 9) << ','
			<< QString::number(trace->secondFitK, 'g', 9) << ','
			<< QString::number(trace->secondFitB, 'g', 9) << ',' << trace->envelopeType;
		for (int i = 0; i < 5; ++i)
		{
			featureStream << ',' << trace->featureIndexes[i] << ','
				<< QString::number(trace->featureValues[i], 'g', 9);
		}
		featureStream << ',' << QString::number(trace->leftRut, 'g', 9)
			<< ',' << QString::number(trace->rightRut, 'g', 9)
			<< ',' << QString::number(rawRut, 'g', 9) << "\r\n";
		featureStream.flush();
		if (m_featuresFile.error() != QFile::NoError)
		{
			errorMessage = QStringLiteral("写入车辙特征调试文件失败：%1\r\n%2")
				.arg(m_featuresFile.fileName(), m_featuresFile.errorString());
			return false;
		}
	}

	QTextStream manifestStream(&m_manifestFile);
	manifestStream.setCodec("UTF-8");
	manifestStream << frameIndex << ',' << QString::number(dmi, 'f', 6) << ','
		<< QString::number(trueMile, 'f', 6) << ',' << csvField(sourceDat) << ','
		<< countValidRaw(rawProfile) << ',' << countValidFloat(worldHeight) << ','
		<< (trace != nullptr ? 1 : 0) << ','
		<< (trace != nullptr && trace->secondStageApplied ? 1 : 0) << ','
		<< (trace != nullptr ? countValidFloat(trace->filteredHeight) : 0) << ','
		<< QString::number(kHorizontalSpacingMm, 'f', 1) << ",pixel,algorithm_unit\r\n";
	manifestStream.flush();
	if (m_manifestFile.error() != QFile::NoError)
	{
		errorMessage = QStringLiteral("写入车辙断面调试清单失败：%1\r\n%2")
			.arg(m_manifestFile.fileName(), m_manifestFile.errorString());
		return false;
	}
	return true;
}

bool RutProfileDebugExporter::writeRawStage(qint64 frameIndex, double dmi, double trueMile,
	const std::vector<short>& values, QString& errorMessage)
{
	QTextStream stream(&m_rawSensorFile);
	stream.setCodec("UTF-8");
	for (int pixel = 0; pixel < static_cast<int>(values.size()); ++pixel)
	{
		if (values[pixel] <= 0 || values[pixel] >= m_verticalPixels)
		{
			continue;
		}
		stream << QString::number(pixel * kHorizontalSpacingMm, 'f', 3) << ' '
			<< QString::number(dmi * 1000.0, 'f', 3) << ' ' << values[pixel] << ' '
			<< frameIndex << ' ' << pixel << ' ' << QString::number(trueMile, 'f', 6)
			<< " 1\r\n";
	}
	stream.flush();
	if (m_rawSensorFile.error() != QFile::NoError)
	{
		errorMessage = QStringLiteral("写入原始车辙断面失败：%1\r\n%2")
			.arg(m_rawSensorFile.fileName(), m_rawSensorFile.errorString());
		return false;
	}
	return true;
}

bool RutProfileDebugExporter::writeFloatStage(QFile& file, qint64 frameIndex, double dmi,
	double trueMile, const std::vector<float>& values, bool stageApplied, QString& errorMessage)
{
	QTextStream stream(&file);
	stream.setCodec("UTF-8");
	for (int pixel = 0; pixel < static_cast<int>(values.size()); ++pixel)
	{
		if (!std::isfinite(values[pixel]) || std::abs(values[pixel]) >= kInvalidHeight)
		{
			continue;
		}
		stream << QString::number(pixel * kHorizontalSpacingMm, 'f', 3) << ' '
			<< QString::number(dmi * 1000.0, 'f', 3) << ' '
			<< QString::number(values[pixel], 'g', 9) << ' ' << frameIndex << ' '
			<< pixel << ' ' << QString::number(trueMile, 'f', 6) << ' '
			<< (stageApplied ? 1 : 0) << "\r\n";
	}
	stream.flush();
	if (file.error() != QFile::NoError)
	{
		errorMessage = QStringLiteral("写入车辙断面调试文件失败：%1\r\n%2")
			.arg(file.fileName(), file.errorString());
		return false;
	}
	return true;
}

int RutProfileDebugExporter::countValidRaw(const std::vector<short>& values) const
{
	int count = 0;
	for (short value : values)
	{
		if (value > 0 && value < m_verticalPixels)
		{
			++count;
		}
	}
	return count;
}

int RutProfileDebugExporter::countValidFloat(const std::vector<float>& values) const
{
	int count = 0;
	for (float value : values)
	{
		if (std::isfinite(value) && std::abs(value) < kInvalidHeight)
		{
			++count;
		}
	}
	return count;
}

QString RutProfileDebugExporter::csvField(const QString& value) const
{
	QString escaped = value;
	escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
	return QStringLiteral("\"") + escaped + QStringLiteral("\"");
}

void RutProfileDebugExporter::closeFiles()
{
	QFile* files[] = { &m_rawSensorFile, &m_worldHeightFile, &m_firstDetrendedFile,
		&m_firstOutlierFile, &m_secondDetrendedFile, &m_secondOutlierFile,
		&m_filteredFile, &m_featuresFile, &m_manifestFile };
	for (QFile* file : files)
	{
		if (file->isOpen())
		{
			file->flush();
			file->close();
		}
	}
}

bool RutProfileDebugExporter::renameRunDirectory(const QString& suffix, QString& errorMessage)
{
	const QString target = suffix.isEmpty() ? m_finalDirectory : m_finalDirectory + suffix;
	QDir parent = QFileInfo(m_runDirectory).absoluteDir();
	if (!parent.rename(QFileInfo(m_runDirectory).fileName(), QFileInfo(target).fileName()))
	{
		errorMessage = QStringLiteral("无法发布车辙断面调试目录：%1 -> %2")
			.arg(m_runDirectory, target);
		return false;
	}
	m_runDirectory = target;
	return true;
}

bool RutProfileDebugExporter::finish(bool success, const QString& failureMessage,
	QString& errorMessage)
{
	if (!m_enabled || m_finalized)
	{
		return true;
	}
	if (m_runDirectory.isEmpty())
	{
		m_finalized = true;
		return true;
	}
	closeFiles();
	if (!success)
	{
		QFile failureFile(QDir(m_runDirectory).filePath(QStringLiteral("FAILED.txt")));
		if (failureFile.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			failureFile.write(failureMessage.toUtf8());
			failureFile.write("\r\n");
			failureFile.close();
		}
	}
	const bool renamed = renameRunDirectory(success ? QString() : QStringLiteral("_failed"), errorMessage);
	m_finalized = true;
	return renamed;
}
