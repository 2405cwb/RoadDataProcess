#include "hn2dDiseaseExchangeService.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QPainter>
#include "xlsxdocument.h"
#include <QTextStream>
#include <QDateTime>
#include <QUuid>

#include "../hnApplication/hnDataManager.h"
#include "../hnApplication/hnDiseaseService.h"
#include "../hnProject/hn2DProject.h"
#include "../hnProject/hnProject.h"

namespace
{
	QString normalizedPath(const QString& path)
	{
		return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
	}

	void copyText(char* target, size_t targetSize, const QString& value)
	{
		const QByteArray bytes = value.toLocal8Bit();
		strncpy_s(target, targetSize, bytes.constData(), _TRUNCATE);
	}

	QRect rectFromDisease(const hnCommon::hnRoadDiseaseInfo& disease)
	{
		if (disease.vec2dRect.empty())
		{
			return QRect();
		}

		const hnCommon::hn2dRectI& source = disease.vec2dRect.front();
		return QRect(QPoint(source.p0.x, source.p0.y), QPoint(source.p2.x, source.p2.y)).normalized();
	}
}

hn2dDiseaseExchangeService::Result::Result()
	: success(false), roadCount(0), streetCount(0), skippedCount(0), stopped(false)
{
}

hn2dDiseaseExchangeService::ImageRef::ImageRef()
	: dmi(0.0), trueMile(0.0), side(0)
{
}

hn2dDiseaseExchangeService::hn2dDiseaseExchangeService(hnPro::hnProject* project)
	: m_project(project)
{
}

bool hn2dDiseaseExchangeService::parseRoadLine(const QString& line, const ImageRef& image,
	hnCommon::hnRoadDiseaseInfo& disease, QString& error) const
{
	const hnProjectSetInfo setting = m_project->getCurProSetInfo();
	const int mode = setting.nDrawType;
	const QStringList fields = line.trimmed().split(QRegularExpression(QStringLiteral("\\s+")), QString::SkipEmptyParts);
	if (fields.size() < (mode == 0 ? 7 : 4))
	{
		error = QStringLiteral("病害字段不完整");
		return false;
	}
	const QString name = fields.at(mode == 0 ? 4 : 0);
	const QString surface = fields.at(mode == 0 ? 6 : 2);
	const int surfaceIndex = surface == QStringLiteral("沥青") ? 0 : surface == QStringLiteral("水泥") ? 1 : -1;
	if (surfaceIndex < 0)
	{
		error = QStringLiteral("未知路面材质：%1").arg(surface);
		return false;
	}
	hnDiseaseSetInfo selected;
	int matches = 0;
	QVector<hnDiseaseSetInfo> settings;
	const auto byTable = hnApp::hnDataManager::getDataManager()->getTableNamesDiseaseInfoMap(m_project->getBaseStandard(), mode);
	for (auto table = byTable.constBegin(); table != byTable.constEnd(); ++table)
		for (const hnDiseaseSetInfo& candidate : table.value())
			if (candidate.nDrawType == mode && candidate.nRoadSurfaceType == surfaceIndex && candidate.nDiseaseType == 0)
				settings.append(candidate);
	for (const hnDiseaseSetInfo& candidate : settings)
	{
		if (name == QString::fromLocal8Bit(candidate.strDiseaseTypeName))
		{
			selected = candidate;
			++matches;
		}
	}
	if (matches != 1)
	{
		error = QStringLiteral("病害配置匹配数量为 %1：%2 / %3").arg(matches).arg(surface, name);
		return false;
	}
	QVector<QRect> rects;
	if (mode == 0)
	{
		int values[4];
		for (int i = 0; i < 4; ++i)
		{
			bool ok = false;
			values[i] = fields.at(i).toInt(&ok);
			if (!ok || values[i] < 0 || (i >= 2 && values[i] == 0))
			{
				error = QStringLiteral("病害框坐标或尺寸无效");
				return false;
			}
		}
		rects.append(QRect(values[0], values[1], values[2], values[3]));
		copyText(disease.strRemark, sizeof(disease.strRemark), fields.mid(7).join(QStringLiteral(" ")));
	}
	else
	{
		const int columns = qMax(1, int(setting.picPixelX * setting.dRadioX * 10.0));
		const int rows = qMax(1, int(setting.picPixelY * setting.dRadioY * 10.0));
		const int cellWidth = qMax(1, qRound(double(setting.picPixelX) / columns));
		const int cellHeight = qMax(1, qRound(double(setting.picPixelY) / rows));
		const QStringList cells = fields.last().split('-', QString::SkipEmptyParts);
		QSet<int> seen;
		for (const QString& cell : cells)
		{
			bool ok = false;
			const int index = cell.toInt(&ok);
			if (!ok || index < 0 || index >= columns * rows || seen.contains(index))
			{
				error = QStringLiteral("小框编号无效或重复：%1").arg(cell);
				return false;
			}
			seen.insert(index);
			const int x = index % columns * cellWidth;
			const int y = index / columns * cellHeight;
			rects.append(QRect(QPoint(x, y), QPoint(qMin(setting.picPixelX, x + cellWidth), qMin(setting.picPixelY, y + cellHeight))));
		}
		if (rects.isEmpty()) { error = QStringLiteral("病害小框列表为空"); return false; }
	}
	QRect bounds;
	for (const QRect& rect : rects)
	{
		bounds = bounds.isNull() ? rect : bounds.united(rect);
		hn2dRectI output;
		output.p0 = hn2dPointWithMileI(rect.left(), rect.top(), image.dmi, 0.0);
		output.p1 = hn2dPointWithMileI(rect.right(), rect.top(), image.dmi, 0.0);
		output.p2 = hn2dPointWithMileI(rect.right(), rect.bottom(), image.dmi, 0.0);
		output.p3 = hn2dPointWithMileI(rect.left(), rect.bottom(), image.dmi, 0.0);
		disease.vec2dRect.push_back(output);
	}
	disease.nRectCnt = int(disease.vec2dRect.size());
	disease.nDrawType = mode;
	disease.nRSurfaceType = surfaceIndex;
	disease.nLevel = selected.nLevel;
	disease.ndiseaseType = 0;
	disease.diseaseWeight = selected.fWidget;
	disease.dRoadWidth = m_project->effectiveRoadWidth();
	disease.dDmi = image.dmi;
	// 与二三维手绘病害一致，记录几何范围和中心 DMI。
	disease.dMileage = image.dmi + (setting.picPixelY - bounds.center().y()) * setting.dRadioY;
	disease.dDmiStart = image.dmi + (setting.picPixelY - bounds.bottom() - 1) * setting.dRadioY;
	disease.dDmiEnd = image.dmi + (setting.picPixelY - bounds.top()) * setting.dRadioY;
	disease.dLength = bounds.height() * setting.dRadioY;
	disease.dWidth = bounds.width() * setting.dRadioX;
	disease.nPixelLen = bounds.height();
	disease.nPixelWid = bounds.width();
	copyText(disease.strDisName, sizeof(disease.strDisName), name);
	copyText(disease.strDiseaseTableName, sizeof(disease.strDiseaseTableName), QString::fromLocal8Bit(selected.strDBTableName));
	copyText(disease.strRoadStandard, sizeof(disease.strRoadStandard), HnProjectEnums::roadTypeEnumToQString(m_project->getBaseStandard()));
	hnApp::hnDataManager::getDataManager()->calculateImportedDiseaseSize(selected, disease);
	if (!std::isfinite(disease.dArea) || disease.dArea < 0.0)
	{
		error = QStringLiteral("病害面积计算无效");
		return false;
	}
	return true;
}

hn2dDiseaseExchangeService::Result hn2dDiseaseExchangeService::replaceDiseases(const std::function<bool()>& cancelled)
{
	Result result;
	if (!m_project || !m_project->get2DProject() || !m_project->getDB() || !m_project->getDB()->isOpen())
	{
		result.error = QStringLiteral("二维工程或成果数据库不可用。"); return result;
	}
	const hnProjectSetInfo setting = m_project->getCurProSetInfo();
	if ((setting.nDrawType != 0 && setting.nDrawType != 1) || setting.picPixelX <= 0 || setting.picPixelY <= 0 ||
		!std::isfinite(setting.dRadioX) || !std::isfinite(setting.dRadioY) || setting.dRadioX <= 0 || setting.dRadioY <= 0)
	{
		result.error = QStringLiteral("绘制模式或二维图像比例无效。"); return result;
	}
	const QString root = QDir(m_project->get2DProPath()).filePath(QStringLiteral("RoadImg/Camera0"));
	const QVector<ImageRef> images = roadImages();
	if (!QDir(root).exists() || images.isEmpty())
	{
		result.error = QStringLiteral("路面源目录或图片映射缺失，保留原病害。"); return result;
	}
	const QString suffix = setting.nDrawType == 0 ? QStringLiteral(".txt") : QStringLiteral("_PartClass.txt");
	QMap<QString, ImageRef> sourceImages;
	for (const ImageRef& image : images)
		sourceImages.insert(normalizedPath(image.imagePath + suffix).toLower(), image);
	QMap<QString, QVector<hnRoadDiseaseInfo>> diseases;
	QDirIterator files(root, QStringList() << (QStringLiteral("*.jpg") + suffix), QDir::Files, QDirIterator::Subdirectories);
	while (files.hasNext())
	{
		if (cancelled && cancelled()) { result.stopped = true; return result; }
		const QString path = files.next();
		const QString key = normalizedPath(path).toLower();
		if (!sourceImages.contains(key))
		{
			result.error = QStringLiteral("病害文件无法匹配图片：%1").arg(path); return result;
		}
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			result.error = QStringLiteral("无法读取病害文件：%1").arg(path); return result;
		}
		QTextStream stream(&file); stream.setCodec("UTF-8");
		int lineNumber = 0;
		while (!stream.atEnd())
		{
			const QString line = stream.readLine(); ++lineNumber;
			if (lineNumber % 64 == 0 && cancelled && cancelled()) { result.stopped = true; return result; }
			if (line.trimmed().isEmpty()) continue;
			hnRoadDiseaseInfo disease;
			if (!parseRoadLine(line, sourceImages.value(key), disease, result.error))
			{
				// 二维允许混合规范；无法识别或不匹配当前绘制配置的文本静默跳过。
				result.error.clear();
				continue;
			}
			diseases[QString::fromLocal8Bit(disease.strDiseaseTableName)].append(disease);
			++result.roadCount;
		}
		if (file.error() != QFile::NoError) { result.error = QStringLiteral("读取病害文件失败：%1").arg(path); return result; }
	}
	loadStreetChannel(streetImages(0), 0, diseases, result);
	loadStreetChannel(streetImages(1), 1, diseases, result);
	// 不匹配规范的景观记录仅跳过；文件读取失败仍阻止覆盖，避免丢失原病害。
	if (!result.error.isEmpty()) return result;
	if (cancelled && cancelled()) { result.stopped = true; return result; }
	hnDBSqlite* db = m_project->getDB();
	if (db->getDiseaseTable()->GetAllDiseaseTableNames().empty())
	{
		result.error = QStringLiteral("病害表配置未初始化，保留原病害。"); return result;
	}
	result.backupPath = QString::fromLocal8Bit(db->getDBPath()) + QStringLiteral(".before-2d-import-") +
		QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddHHmmsszzz")) + QUuid::createUuid().toString() + QStringLiteral(".db");
	if (!db->backupDatabase(result.backupPath))
	{
		result.error = QStringLiteral("成果库备份失败，未修改病害：%1").arg(result.backupPath); return result;
	}
	if (!db->executeDB("BEGIN IMMEDIATE"))
	{
		result.error = QStringLiteral("无法启动病害覆盖事务。"); return result;
	}
	bool ok = true;
	try
	{
		// 清理所有病害表，包括旧版本按错误材质写入的同名记录。
		const vector<string> tables = db->getDiseaseTable()->GetAllDiseaseTableNames();
		for (const string& table : tables)
		{
			const QString name = QString::fromLocal8Bit(table.c_str());
			if (!QRegularExpression(QStringLiteral("^[A-Za-z][A-Za-z0-9_]*$")).match(name).hasMatch() ||
				!db->executeDB(QStringLiteral("DELETE FROM [%1]").arg(name).toLocal8Bit().constData())) { ok = false; break; }
		}
		for (auto it = diseases.begin(); ok && it != diseases.end(); ++it)
		{
			int id = 1;
			for (hnRoadDiseaseInfo& disease : it.value())
			{
				if (cancelled && cancelled()) { result.stopped = true; ok = false; break; }
				disease.nID = id++;
				if (!db->getDiseaseTable()->writeSingleDatas_Service(setting, disease)) { ok = false; break; }
			}
		}
		if (ok && cancelled && cancelled()) { result.stopped = true; ok = false; }
		if (ok) ok = db->executeDB("COMMIT");
	}
	catch (...) { ok = false; }
	if (!ok)
	{
		bool rolledBack = false;
		try { rolledBack = db->executeDB("ROLLBACK"); } catch (...) { rolledBack = false; }
		result.error = rolledBack ? QStringLiteral("当前工程未完成，已回滚并保留原病害。") : QStringLiteral("回滚失败，请从备份恢复成果库：%1").arg(result.backupPath);
		return result;
	}
	result.success = true;
	return result;
}

QVector<hn2dDiseaseExchangeService::ImageRef> hn2dDiseaseExchangeService::roadImages() const
{
	QVector<ImageRef> result;
	if (!m_project || !m_project->get2DProject())
	{
		return result;
	}

	const QVector<hnMile> miles = m_project->getCurrentMileVector();
	const QString basePath = m_project->get2DProPath();
	for (int i = 0; i < miles.size(); ++i)
	{
		if (miles.at(i).picturePath.isEmpty())
		{
			continue;
		}

		ImageRef image;
		image.imagePath = miles.at(i).picturePath;
		if (!QFileInfo(image.imagePath).isAbsolute())
		{
			const QString directory = QStringLiteral("Image_%1").arg(i / 1000, 4, 10, QChar('0'));
			image.imagePath = QDir(basePath).filePath(QStringLiteral("RoadImg/Camera0/%1/%2")
				.arg(directory, miles.at(i).picturePath));
		}
		image.dmi = miles.at(i).dEnclMile;
		image.trueMile = miles.at(i).dTrueMile;
		result.push_back(image);
	}
	return result;
}

QVector<hn2dDiseaseExchangeService::ImageRef> hn2dDiseaseExchangeService::streetImages(int side) const
{
	QVector<ImageRef> result;
	if (!m_project || !m_project->get2DProject())
	{
		return result;
	}

	const QVector<hnMile> miles = side == 0 ? m_project->getLeftStreetMiles() : m_project->getRightStreetMiles();
	for (int i = 0; i < miles.size(); ++i)
	{
		const QString path = side == 0 ? miles.at(i).leftStreetPicPath : miles.at(i).rightStreetPicPath;
		if (path.isEmpty())
		{
			continue;
		}

		ImageRef image;
		image.imagePath = path;
		image.dmi = miles.at(i).dEnclMile;
		image.trueMile = miles.at(i).dTrueMile;
		image.side = side;
		result.push_back(image);
	}
	return result;
}

hn2dDiseaseExchangeService::ImageRef hn2dDiseaseExchangeService::closestImage(
	const QVector<ImageRef>& images, double dmi) const
{
	ImageRef result;
	if (images.isEmpty())
	{
		return result;
	}

	int bestIndex = 0;
	double bestDistance = qAbs(images.first().dmi - dmi);
	for (int i = 1; i < images.size(); ++i)
	{
		const double distance = qAbs(images.at(i).dmi - dmi);
		if (distance < bestDistance)
		{
			bestDistance = distance;
			bestIndex = i;
		}
	}
	return images.at(bestIndex);
}

QString hn2dDiseaseExchangeService::formatStake(double trueMile) const
{
	const qint64 rounded = qRound64(trueMile);
	const qint64 absolute = qAbs(rounded);
	const qint64 kilometer = absolute / 1000;
	const qint64 meter = absolute % 1000;
	return QStringLiteral("%1K%2+%3")
		.arg(rounded < 0 ? QStringLiteral("-") : QString())
		.arg(kilometer)
		.arg(meter, 3, 10, QChar('0'));
}

QString hn2dDiseaseExchangeService::roadSurfaceName(int surface) const
{
	if (surface == 1)
	{
		return QStringLiteral("水泥");
	}
	if (surface == 2)
	{
		return QStringLiteral("砂石");
	}
	return QStringLiteral("沥青");
}

QString hn2dDiseaseExchangeService::roadDiseaseName(const hnCommon::hnRoadDiseaseInfo& disease) const
{
	return QString::fromLocal8Bit(disease.strDisName).trimmed();
}

int hn2dDiseaseExchangeService::exchangeSide(const hnCommon::hnRoadDiseaseInfo& disease) const
{
	return disease.nDrawType == 1 || disease.nDrawType == 11 ? 1 : 0;
}

bool hn2dDiseaseExchangeService::isUserStreetDisease(const hnCommon::hnRoadDiseaseInfo& disease) const
{
	return disease.nDrawType >= 10;
}

bool hn2dDiseaseExchangeService::buildBigFrameFiles(
	const QVector<hnCommon::hnRoadDiseaseInfo>& diseases,
	const QVector<ImageRef>& images, TextFileMap& files, QStringList& warnings, int& skippedCount) const
{
	if (!m_project || images.isEmpty())
	{
		return false;
	}

	const hnCommon::hnProjectSetInfo setting = m_project->getCurProSetInfo();
	const int imageHeight = setting.picPixelY;
	const double scaleY = setting.dRadioY;
	if (imageHeight <= 0 || scaleY <= 0.0)
	{
		return false;
	}

	for (int diseaseIndex = 0; diseaseIndex < diseases.size(); ++diseaseIndex)
	{
		const hnCommon::hnRoadDiseaseInfo& disease = diseases.at(diseaseIndex);
		if (disease.vec2dRect.empty())
		{
			warnings.append(QStringLiteral("病害ID %1没有二维框，已跳过。").arg(disease.nID));
			skippedCount++;
			continue;
		}

		QMap<QString, QRect> pieces;
		for (size_t rectIndex = 0; rectIndex < disease.vec2dRect.size(); ++rectIndex)
		{
			const hnCommon::hn2dRectI& rect = disease.vec2dRect.at(rectIndex);
			const double firstMile = rect.p0.m_dmi + (imageHeight - rect.p0.y) * scaleY;
			const double secondMile = rect.p2.m_dmi + (imageHeight - rect.p2.y) * scaleY;
			const double begin = qMin(firstMile, secondMile);
			const double end = qMax(firstMile, secondMile);
			const int left = qMin(rect.p0.x, rect.p2.x);
			const int right = qMax(rect.p0.x, rect.p2.x);

			for (int imageIndex = 0; imageIndex < images.size(); ++imageIndex)
			{
				const ImageRef& image = images.at(imageIndex);
				const double frameBegin = image.dmi;
				const double frameEnd = image.dmi + setting.dRoadLength;
				const double overlapBegin = qMax(begin, qMin(frameBegin, frameEnd));
				const double overlapEnd = qMin(end, qMax(frameBegin, frameEnd));
				if (overlapEnd + 1e-6 < overlapBegin)
				{
					continue;
				}

				int top = qRound(imageHeight - (overlapEnd - image.dmi) / scaleY);
				int bottom = qRound(imageHeight - (overlapBegin - image.dmi) / scaleY);
				top = qBound(0, top, imageHeight);
				bottom = qBound(0, bottom, imageHeight);
				QRect piece(QPoint(left, qMin(top, bottom)), QPoint(right, qMax(top, bottom)));
				piece = piece.normalized();
				if (piece.width() <= 0 || piece.height() <= 0)
				{
					continue;
				}

				const QString key = normalizedPath(image.imagePath + QStringLiteral(".txt"));
				pieces[key] = pieces.contains(key) ? pieces.value(key).united(piece) : piece;
			}
		}

		if (pieces.isEmpty())
		{
			warnings.append(QStringLiteral("病害ID %1不在有效图片里程范围内，已跳过。").arg(disease.nID));
			skippedCount++;
			continue;
		}
		for (QMap<QString, QRect>::const_iterator it = pieces.constBegin(); it != pieces.constEnd(); ++it)
		{
			const QRect rect = it.value();
			QString line = QStringLiteral("%1 %2 %3 %4 %5 桩号:%6 %7")
				.arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height())
				.arg(roadDiseaseName(disease))
				.arg(formatStake(m_project->enclToTrueMile(disease.dMileage)))
				.arg(roadSurfaceName(disease.nRSurfaceType));
			const QString remark = QString::fromLocal8Bit(disease.strRemark).trimmed();
			if (!remark.isEmpty())
			{
				line += QStringLiteral(" ") + remark;
			}
			files[it.key()].append(line);
		}
	}
	return true;
}

bool hn2dDiseaseExchangeService::buildLittleFrameFiles(
	const QVector<hnCommon::hnRoadDiseaseInfo>& diseases,
	const QVector<ImageRef>& images, TextFileMap& files, QStringList& warnings, int& skippedCount) const
{
	if (!m_project || images.isEmpty())
	{
		return false;
	}

	const hnCommon::hnProjectSetInfo setting = m_project->getCurProSetInfo();
	const double realWidth = setting.picPixelX * setting.dRadioX;
	const double realHeight = setting.picPixelY * setting.dRadioY;
	const int columnCount = qMax(1, static_cast<int>(realWidth * 10.0));
	const int rowCount = qMax(1, static_cast<int>(realHeight * 10.0));
	const int cellWidth = qMax(1, qRound(setting.picPixelX * 1.0 / columnCount));
	const int cellHeight = qMax(1, qRound(setting.picPixelY * 1.0 / rowCount));

	for (int diseaseIndex = 0; diseaseIndex < diseases.size(); ++diseaseIndex)
	{
		const hnCommon::hnRoadDiseaseInfo& disease = diseases.at(diseaseIndex);
		QMap<QString, QSet<int>> imageCells;
		for (size_t rectIndex = 0; rectIndex < disease.vec2dRect.size(); ++rectIndex)
		{
			const hnCommon::hn2dRectI& rect = disease.vec2dRect.at(rectIndex);
			const ImageRef image = closestImage(images, rect.p0.m_dmi);
			if (image.imagePath.isEmpty())
			{
				continue;
			}
			const int centerX = (rect.p0.x + rect.p2.x) / 2;
			const int centerY = (rect.p0.y + rect.p2.y) / 2;
			const int column = qBound(0, centerX / cellWidth, columnCount - 1);
			const int row = qBound(0, centerY / cellHeight, rowCount - 1);
			imageCells[normalizedPath(image.imagePath + QStringLiteral("_PartClass.txt"))].insert(row * columnCount + column);
		}

		if (imageCells.isEmpty())
		{
			warnings.append(QStringLiteral("病害ID %1没有有效的小框，已跳过。").arg(disease.nID));
			skippedCount++;
			continue;
		}
		for (QMap<QString, QSet<int>>::const_iterator it = imageCells.constBegin(); it != imageCells.constEnd(); ++it)
		{
			QList<int> cells = it.value().values();
			std::sort(cells.begin(), cells.end());
			QStringList cellTexts;
			for (int i = 0; i < cells.size(); ++i)
			{
				cellTexts.append(QString::number(cells.at(i)));
			}
			const QString line = QStringLiteral("%1 桩号:%2 %3 %4-")
				.arg(roadDiseaseName(disease))
				.arg(formatStake(m_project->enclToTrueMile(disease.dMileage)))
				.arg(roadSurfaceName(disease.nRSurfaceType))
				.arg(cellTexts.join(QStringLiteral("-")));
			files[it.key()].append(line);
		}
	}
	return true;
}

bool hn2dDiseaseExchangeService::buildStreetFiles(
	const QVector<hnCommon::hnRoadDiseaseInfo>& diseases,
	TextFileMap& normalFiles, TextFileMap& userFiles, QStringList& warnings, int& skippedCount) const
{
	if (!m_project)
	{
		return false;
	}

	const QVector<ImageRef> leftImages = streetImages(0);
	const QVector<ImageRef> rightImages = streetImages(1);
	for (int i = 0; i < diseases.size(); ++i)
	{
		const hnCommon::hnRoadDiseaseInfo& disease = diseases.at(i);
		const int side = exchangeSide(disease);
		const QVector<ImageRef>& images = side == 1 && !rightImages.isEmpty() ? rightImages : leftImages;
		const ImageRef image = closestImage(images, disease.dDmi);
		if (image.imagePath.isEmpty())
		{
			warnings.append(QStringLiteral("景观病害ID %1找不到对应图片，已跳过。").arg(disease.nID));
			skippedCount++;
			continue;
		}

		hnCommon::hnDiseaseSetInfo setInfo;
		const bool customDisease = disease.ndiseaseType == 3;
		if (customDisease)
		{
			setInfo.dEffectMeasure = 1;
			setInfo.nDWKF = 0;
		}
		else if (!hnApp::hnDataManager::getDataManager()->getStreetDiseaseSetInfo(
			QString::fromLocal8Bit(disease.strDisName), m_project->getBaseStandard(), setInfo))
		{
			warnings.append(QStringLiteral("景观病害“%1”不在当前规范中，已跳过。")
				.arg(QString::fromLocal8Bit(disease.strDisName)));
			skippedCount++;
			continue;
		}

		const double amount = disease.dArea;
		const double length = setInfo.dEffectMeasure == 0 ? amount : 0.0;
		const double count = setInfo.dEffectMeasure == 1 ? amount : 0.0;
		const double score = amount * setInfo.nDWKF;
		const QRect rect = rectFromDisease(disease);
		const QString stake = formatStake(m_project->enclToTrueMile(disease.dDmi));
		const QString name = QString::fromLocal8Bit(disease.strDisName);
		if (isUserStreetDisease(disease))
		{
			QString line;
			const QString info = QString::fromLocal8Bit(disease.strRemark);
			if (rect.isValid())
			{
				line = QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8,%9")
					.arg(side).arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height())
					.arg(stake).arg(name).arg(qRound(amount)).arg(info);
			}
			else
			{
				line = QStringLiteral("%1,%2,%3,%4").arg(stake).arg(name).arg(qRound(amount)).arg(info);
			}
			userFiles[normalizedPath(image.imagePath + QStringLiteral("_UserSign.txt"))].append(line);
		}
		else
		{
			QString line = QStringLiteral("%1,%2,%3,%4,%5")
				.arg(name).arg(stake).arg(QString::number(score, 'g', 12))
				.arg(qRound(count)).arg(qRound(length));
			if (rect.isValid())
			{
				line += QStringLiteral(",%1,%2,%3,%4,%5")
					.arg(side).arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
			}
			normalFiles[normalizedPath(image.imagePath + QStringLiteral(".txt"))].append(line);
		}
	}
	return true;
}

bool hn2dDiseaseExchangeService::writeUtf8File(
	const QString& filePath, const QStringList& lines, QString& error) const
{
	QDir directory = QFileInfo(filePath).dir();
	if (!directory.exists() && !directory.mkpath(QStringLiteral(".")))
	{
		error = QStringLiteral("无法创建目录：%1").arg(directory.absolutePath());
		return false;
	}

	QSaveFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		error = QStringLiteral("无法写入文件：%1\n%2").arg(filePath, file.errorString());
		return false;
	}
	QTextStream stream(&file);
	stream.setCodec("UTF-8");
	stream.setGenerateByteOrderMark(false);
	for (int i = 0; i < lines.size(); ++i)
	{
		stream << lines.at(i) << "\r\n";
	}
	stream.flush();
	if (!file.commit())
	{
		error = QStringLiteral("覆盖文件失败：%1\n%2").arg(filePath, file.errorString());
		return false;
	}
	return true;
}

bool hn2dDiseaseExchangeService::writeTextFiles(
	const QString& rootPath, const QStringList& suffixes,
	const TextFileMap& files, QString& error) const
{
	QDir root(rootPath);
	if (!root.exists())
	{
		error = QStringLiteral("图片目录不存在：%1").arg(rootPath);
		return false;
	}

	QDirIterator iterator(rootPath, QDir::Files, QDirIterator::Subdirectories);
	while (iterator.hasNext())
	{
		const QString existing = iterator.next();
		bool matches = false;
		for (int i = 0; i < suffixes.size(); ++i)
		{
			if (existing.endsWith(suffixes.at(i), Qt::CaseInsensitive))
			{
				matches = true;
				break;
			}
		}
		if (matches && !files.contains(normalizedPath(existing)) && !QFile::remove(existing))
		{
			error = QStringLiteral("无法清理旧病害文本：%1").arg(existing);
			return false;
		}
	}

	for (TextFileMap::const_iterator it = files.constBegin(); it != files.constEnd(); ++it)
	{
		if (!writeUtf8File(it.key(), it.value(), error))
		{
			return false;
		}
	}
	return true;
}

hn2dDiseaseExchangeService::Result hn2dDiseaseExchangeService::exportDiseases(int frameType)
{
	Result result;
	if (!m_project || !m_project->get2DProject())
	{
		result.error = QStringLiteral("当前工程没有可用的二维图片工程。");
		return result;
	}

	const QVector<ImageRef> images = roadImages();
	if (images.isEmpty())
	{
		result.error = QStringLiteral("未找到路面图片与DMI映射，无法导出二维病害。");
		return result;
	}

	const QVector<hnCommon::hnRoadDiseaseInfo> roadDiseases =
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllRoadDiseases();
	TextFileMap roadFiles;
	const bool roadBuilt = frameType == 0
		? buildBigFrameFiles(roadDiseases, images, roadFiles, result.warnings, result.skippedCount)
		: buildLittleFrameFiles(roadDiseases, images, roadFiles, result.warnings, result.skippedCount);
	if (!roadBuilt)
	{
		result.error = QStringLiteral("路面图片参数无效，无法生成二维病害文本。");
		return result;
	}

	const QString roadRoot = QDir(m_project->get2DProPath()).filePath(QStringLiteral("RoadImg/Camera0"));
	const QString roadSuffix = frameType == 0 ? QStringLiteral(".jpg.txt") : QStringLiteral(".jpg_PartClass.txt");
	if (!writeTextFiles(roadRoot, QStringList() << roadSuffix, roadFiles, result.error))
	{
		return result;
	}

	const QVector<hnCommon::hnRoadDiseaseInfo> streetDiseases =
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllStreetDiseases();
	const int roadSkippedCount = result.skippedCount;
	TextFileMap normalStreetFiles;
	TextFileMap userStreetFiles;
	buildStreetFiles(streetDiseases, normalStreetFiles, userStreetFiles, result.warnings, result.skippedCount);
	const QStringList streetRoots = QStringList()
		<< QDir(m_project->get2DProPath()).filePath(QStringLiteral("StreetImg/Camera0"))
		<< QDir(m_project->get2DProPath()).filePath(QStringLiteral("StreetImg/Camera1"))
		<< QDir(m_project->get2DProPath()).filePath(QStringLiteral("StreetImg2/Camera0"));
	for (int i = 0; i < streetRoots.size(); ++i)
	{
		if (!QDir(streetRoots.at(i)).exists())
		{
			continue;
		}
		TextFileMap channelFiles;
		TextFileMap channelUserFiles;
		for (TextFileMap::const_iterator it = normalStreetFiles.constBegin(); it != normalStreetFiles.constEnd(); ++it)
		{
			if (it.key().startsWith(normalizedPath(streetRoots.at(i)) + QStringLiteral("/"), Qt::CaseInsensitive))
			{
				channelFiles.insert(it.key(), it.value());
			}
		}
		for (TextFileMap::const_iterator it = userStreetFiles.constBegin(); it != userStreetFiles.constEnd(); ++it)
		{
			if (it.key().startsWith(normalizedPath(streetRoots.at(i)) + QStringLiteral("/"), Qt::CaseInsensitive))
			{
				channelUserFiles.insert(it.key(), it.value());
			}
		}
		TextFileMap allChannelFiles = channelFiles;
		for (TextFileMap::const_iterator it = channelUserFiles.constBegin(); it != channelUserFiles.constEnd(); ++it)
		{
			allChannelFiles.insert(it.key(), it.value());
		}
		if (!writeTextFiles(streetRoots.at(i),
			QStringList() << QStringLiteral(".jpg.txt") << QStringLiteral(".jpg_UserSign.txt"),
			allChannelFiles, result.error))
		{
			return result;
		}
	}

	result.roadCount = qMax(0, roadDiseases.size() - roadSkippedCount);
	result.streetCount = qMax(0, streetDiseases.size() - (result.skippedCount - roadSkippedCount));
	result.success = true;
	return result;
}

bool hn2dDiseaseExchangeService::parseStreetLine(
	const QString& line, const ImageRef& image, int defaultSide,
	bool userSign, hnCommon::hnRoadDiseaseInfo& disease, QString& tableName) const
{
	const QStringList fields = line.trimmed().split(QChar(','), QString::KeepEmptyParts);
	QString name;
	double amount = 0.0;
	int side = defaultSide;
	QRect rect;
	QString remark;
	if (userSign)
	{
		if (fields.size() >= 9)
		{
			side = fields.at(0).toInt();
			rect = QRect(fields.at(1).toInt(), fields.at(2).toInt(), fields.at(3).toInt(), fields.at(4).toInt());
			name = fields.at(6).trimmed();
			amount = fields.at(7).toDouble();
			remark = fields.mid(8).join(QStringLiteral(","));
		}
		else if (fields.size() >= 4)
		{
			name = fields.at(1).trimmed();
			amount = fields.at(2).toDouble();
			remark = fields.mid(3).join(QStringLiteral(","));
		}
		else
		{
			return false;
		}
	}
	else
	{
		if (fields.size() < 5)
		{
			return false;
		}
		name = fields.at(0).trimmed();
		if (fields.size() >= 10)
		{
			side = fields.at(5).toInt();
			rect = QRect(fields.at(6).toInt(), fields.at(7).toInt(), fields.at(8).toInt(), fields.at(9).toInt());
		}
	}

	hnCommon::hnDiseaseSetInfo setInfo;
	if (userSign)
	{
		setInfo.nDiseaseType = 3;
		setInfo.nLevel = 0;
		setInfo.fWidget = 0;
		copyText(setInfo.strDBTableName, sizeof(setInfo.strDBTableName), QStringLiteral("UserStreetDisease"));
		if (name.isEmpty() || name.toLocal8Bit().size() >= sizeof(disease.strDisName) ||
			remark.toLocal8Bit().size() >= sizeof(disease.strRemark) || !std::isfinite(amount) || amount <= 0 || amount > 1000000 || amount != std::floor(amount)) return false;
	}
	else if (!hnApp::hnDataManager::getDataManager()->getStreetDiseaseSetInfo(
		name, m_project->getBaseStandard(), setInfo))
	{
		return false;
	}
	if (!userSign)
	{
		amount = setInfo.dEffectMeasure == 0 ? fields.at(4).toDouble() : fields.at(3).toDouble();
	}

	disease.dMileage = image.dmi;
	disease.dDmi = image.dmi;
	disease.dDmiStart = image.dmi;
	disease.dDmiEnd = image.dmi;
	disease.dRoadWidth = m_project->getCurProSetInfo().dRoadWidth;
	disease.dArea = amount;
	disease.nLevel = setInfo.nLevel;
	disease.nDrawType = userSign ? 10 + (side == 1 ? 1 : 0) : (side == 1 ? 1 : 0);
	disease.nRSurfaceType = m_project->getCurProSetInfo().nRSurfaceType;
	disease.ndiseaseType = setInfo.nDiseaseType;
	disease.diseaseWeight = setInfo.fWidget;
	copyText(disease.strRoadStandard, sizeof(disease.strRoadStandard),
		HnProjectEnums::roadTypeEnumToQString(m_project->getBaseStandard()));
	copyText(disease.strDiseaseTableName, sizeof(disease.strDiseaseTableName),
		QString::fromLocal8Bit(setInfo.strDBTableName));
	copyText(disease.strDisName, sizeof(disease.strDisName), name);
	copyText(disease.strRemark, sizeof(disease.strRemark), remark);
	if (rect.isValid())
	{
		hnCommon::hn2dRectI outputRect;
		outputRect.p0 = hnCommon::hn2dPointWithMileI(rect.left(), rect.top(), image.dmi, 0.0);
		outputRect.p1 = hnCommon::hn2dPointWithMileI(rect.right(), rect.top(), image.dmi, 0.0);
		outputRect.p2 = hnCommon::hn2dPointWithMileI(rect.right(), rect.bottom(), image.dmi, 0.0);
		outputRect.p3 = hnCommon::hn2dPointWithMileI(rect.left(), rect.bottom(), image.dmi, 0.0);
		disease.vec2dRect.push_back(outputRect);
		disease.nRectCnt = 1;
	}
	tableName = QString::fromLocal8Bit(setInfo.strDBTableName);
	return !tableName.isEmpty();
}

void hn2dDiseaseExchangeService::loadStreetFile(
	const QString& filePath, const ImageRef& image, int defaultSide,
	bool userSign, QMap<QString, QVector<hnCommon::hnRoadDiseaseInfo>>& diseases,
	Result& result) const
{
	QFile file(filePath);
	if (!file.exists())
	{
		return;
	}
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		result.error = QStringLiteral("无法读取景观病害文本：%1").arg(filePath);
		return;
	}
	QTextStream stream(&file);
	stream.setCodec("UTF-8");
	while (!stream.atEnd())
	{
		const QString line = stream.readLine();
		if (line.trimmed().isEmpty())
		{
			continue;
		}
		hnCommon::hnRoadDiseaseInfo disease;
		QString tableName;
		if (parseStreetLine(line, image, defaultSide, userSign, disease, tableName))
		{
			diseases[tableName].append(disease);
			result.streetCount++;
		}
		else
		{
			result.skippedCount++;
			result.warnings.append(QStringLiteral("景观病害无法匹配当前规范，已跳过：%1\n%2")
				.arg(filePath, line));
		}
	}
	if (file.error() != QFile::NoError)
	{
		result.error = QStringLiteral("读取景观病害文本失败：%1").arg(filePath);
	}
}

void hn2dDiseaseExchangeService::loadStreetChannel(
	const QVector<ImageRef>& images, int defaultSide,
	QMap<QString, QVector<hnCommon::hnRoadDiseaseInfo>>& diseases,
	Result& result) const
{
	for (int i = 0; i < images.size(); ++i)
	{
		loadStreetFile(images.at(i).imagePath + QStringLiteral(".txt"),
			images.at(i), defaultSide, false, diseases, result);
		loadStreetFile(images.at(i).imagePath + QStringLiteral("_UserSign.txt"),
			images.at(i), defaultSide, true, diseases, result);
	}
}

hn2dDiseaseExchangeService::Result hn2dDiseaseExchangeService::importStreetDiseases()
{
	Result result;
	if (!m_project || !m_project->get2DProject())
	{
		result.error = QStringLiteral("当前工程没有可用的二维景观图片工程。");
		return result;
	}

	QMap<QString, QVector<hnCommon::hnRoadDiseaseInfo>> diseases;
	loadStreetChannel(streetImages(0), 0, diseases, result);
	loadStreetChannel(streetImages(1), 1, diseases, result);
	if (!result.error.isEmpty()) return result;
	for (QMap<QString, QVector<hnCommon::hnRoadDiseaseInfo>>::iterator it = diseases.begin();
		it != diseases.end(); ++it)
	{
		int nextId = m_project->getDB()->getDiseaseTable()->getMaxID(it.key().toLocal8Bit().constData());
		for (int i = 0; i < it.value().size(); ++i)
		{
			it.value()[i].nID = nextId++;
		}
		if (!hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDataAffairs(
			it.key(), true, it.value()))
		{
			result.error = QStringLiteral("景观病害写入成果库失败，数据表：%1").arg(it.key());
			return result;
		}
	}
	result.success = true;
	return result;
}

// 自定义类别与标准病害分开输出，图片缺失保留记录并在结果中说明。
hn2dDiseaseExchangeService::Result hn2dDiseaseExchangeService::exportCustomStreetReport(const QString& filePath)
{
    Result result;
    if (!m_project || !m_project->get2DProject()) { result.error = QStringLiteral("当前工程没有景观数据。"); return result; }
    QVector<hnRoadDiseaseInfo> diseases;
    if (!m_project->getDB()->getDiseaseTable()->readStreetData_Service(
        HnProjectEnums::roadTypeEnumToQString(m_project->getBaseStandard()), m_project->getCurrentMileVector(),
        diseases, m_project->getCurProSetInfo().nLineType, m_project->getRoadSpace()))
    {
        result.error = QStringLiteral("读取景观病害数据库失败，未生成报表。");
        return result;
    }
    QXlsx::Document book;
    book.renameSheet(QStringLiteral("Sheet1"), QStringLiteral("自定义景观"));
    QStringList headers = QStringList() << QStringLiteral("序号") << QStringLiteral("桩号") << QStringLiteral("相对里程（米）")
        << QStringLiteral("相机") << QStringLiteral("名称") << QStringLiteral("数量") << QStringLiteral("备注")
        << QStringLiteral("图片路径") << QStringLiteral("现场图片");
    for (int i = 0; i < headers.size(); i++) book.write(1, i + 1, headers.at(i));
    book.setColumnWidth(1, 1, 8); book.setColumnWidth(2, 6, 18);
    book.setColumnWidth(7, 8, 36); book.setColumnWidth(9, 9, 48);
    const QVector<ImageRef> left = streetImages(0), right = streetImages(1);
    int row = 2;
    for (const auto& disease : diseases)
    {
        if (disease.ndiseaseType != 3) continue;
        int side = exchangeSide(disease);
        ImageRef image = closestImage(side == 1 ? right : left, disease.dDmi);
        book.write(row, 1, row - 1); book.write(row, 2, formatStake(m_project->enclToTrueMile(disease.dDmi)));
        book.write(row, 3, disease.dDmi); book.write(row, 4, side == 1 ? QStringLiteral("右侧") : QStringLiteral("左侧"));
        book.write(row, 5, QString::fromLocal8Bit(disease.strDisName)); book.write(row, 6, disease.dArea);
        book.write(row, 7, QString::fromLocal8Bit(disease.strRemark)); book.write(row, 8, image.imagePath);
        QImage picture(image.imagePath);
        if (!picture.isNull())
        {
            QPainter painter(&picture); painter.setPen(QPen(Qt::red, 5));
            QRect rect = rectFromDisease(disease); if (rect.isValid()) painter.drawRect(rect); painter.end();
            book.insertImage(row - 1, 8, picture.scaled(320, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            book.setRowHeight(row, row, 140);
        }
        else result.warnings.append(QStringLiteral("第 %1 条记录的图片不可用，已保留文字信息。").arg(row - 1));
        row++; result.streetCount++;
    }
    if (!book.saveAs(filePath)) { result.error = QStringLiteral("无法保存 Excel，请检查文件占用和目录权限。"); return result; }
    result.success = true; return result;
}
