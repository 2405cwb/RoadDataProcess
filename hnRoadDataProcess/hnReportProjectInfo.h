#pragma once

#include "../hnProject/hnProject.h"

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QSaveFile>
#include <QString>
#include <QStringList>
#include <QTextCodec>
#include <QTextStream>

namespace hnReportProjectInfo
{
	inline QString sectionName()
	{
		return QStringLiteral("\u4E8C\u4E09\u7EF4\u8BBE\u7F6E\u4FE1\u606F");
	}

	inline QString legacySectionName()
	{
		return QStringLiteral("\u4E8C\u4E09\u4F4D\u8BBE\u7F6E\u4FE1\u606F");
	}

	inline QString taskYearKey()
	{
		return QStringLiteral("\u62A5\u8868\u6D4B\u8BD5\u4EFB\u52A1\u5E74\u4EFD");
	}

	inline QString legacyTaskYearKey()
	{
		return QStringLiteral("\u62A5\u8868\u68C0\u6D4B\u4EFB\u52A1\u5E74\u4EFD");
	}

	inline QString inspectionCountKey()
	{
		return QStringLiteral("\u62A5\u8868\u68C0\u6D4B\u6B21\u6570");
	}

	inline QString maintenanceUnitKey()
	{
		return QStringLiteral("\u62A5\u8868\u7BA1\u517B\u5355\u4F4D");
	}

	inline QString regionCodeKey()
	{
		return QStringLiteral("\u62A5\u8868\u884C\u653F\u533A\u57DF\u4EE3\u7801");
	}

	inline QString roadWidthKey()
	{
		return QStringLiteral("\u62A5\u8868\u8DEF\u9762\u5BBD\u5EA6");
	}

	inline QString laneTypeKey()
	{
		return QStringLiteral("\u62A5\u8868\u8F66\u9053\u7C7B\u578B");
	}

	struct Data
	{
		double roadWidth = 0.0;
		QString maintenanceUnit = QStringLiteral("\u4EA4\u901A\u5C40");
		QString laneType = QStringLiteral("\u53CC\u5411\u56DB\u8F66\u9053");
		QString taskYear = QStringLiteral("2024");
		QString inspectionCount = QStringLiteral("1-\u4E00\u6B21");
		QString regionCode = QStringLiteral("620122");
	};

	struct WidthCacheEntry
	{
		QDateTime lastModified;
		qint64 fileSize = -1;
		double roadWidth = 0.0;
	};

	inline QMap<QString, WidthCacheEntry>& widthCache()
	{
		static QMap<QString, WidthCacheEntry> cache;
		return cache;
	}

	inline QString configPath(hnPro::hnProject* project)
	{
		if (!project)
		{
			return QString();
		}

		QString projectPath = project->get2DProPath();
		if (projectPath.isEmpty())
		{
			projectPath = project->getAbsulotelyPath();
		}
		if (projectPath.isEmpty())
		{
			projectPath = project->get3DProPath();
		}
		return projectPath.isEmpty()
			? QString()
			: QDir(projectPath).filePath(QStringLiteral("ProjectInfo.txt"));
	}

	inline int separatorIndex(const QString& line)
	{
		const int halfWidth = line.indexOf(':');
		const int fullWidth = line.indexOf(QChar(0xFF1A));
		if (halfWidth < 0)
		{
			return fullWidth;
		}
		if (fullWidth < 0)
		{
			return halfWidth;
		}
		return qMin(halfWidth, fullWidth);
	}

	inline bool isReportSection(const QString& section)
	{
		return section == sectionName() || section == legacySectionName();
	}

	inline bool isKnownKey(const QString& key)
	{
		return key == taskYearKey() || key == legacyTaskYearKey() ||
			key == inspectionCountKey() || key == maintenanceUnitKey() ||
			key == regionCodeKey() || key == roadWidthKey() || key == laneTypeKey();
	}

	inline Data defaults(hnPro::hnProject* project)
	{
		Data data;
		if (project)
		{
			data.roadWidth = project->effectiveRoadWidth();
		}
		return data;
	}

	inline QMap<QString, QString> readValues(const QString& path)
	{
		QMap<QString, QString> values;
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			return values;
		}

		QTextStream stream(&file);
		stream.setCodec(QTextCodec::codecForName("UTF-8"));
		bool inReportSection = false;
		while (!stream.atEnd())
		{
			const QString line = stream.readLine().trimmed();
			if (line.startsWith('[') && line.endsWith(']'))
			{
				inReportSection = isReportSection(line.mid(1, line.size() - 2).trimmed());
				continue;
			}
			if (!inReportSection)
			{
				continue;
			}
			const int separator = separatorIndex(line);
			if (separator > 0)
			{
				values.insert(line.left(separator).trimmed(), line.mid(separator + 1).trimmed());
			}
		}
		return values;
	}

	inline Data load(hnPro::hnProject* project)
	{
		Data data = defaults(project);
		const QMap<QString, QString> values = readValues(configPath(project));
		bool widthOk = false;
		const double savedWidth = values.value(roadWidthKey()).toDouble(&widthOk);
		if ((!project || !project->isLineCameraProject()) && widthOk && savedWidth > 0.0)
		{
			data.roadWidth = savedWidth;
		}
		data.maintenanceUnit = values.value(maintenanceUnitKey(), data.maintenanceUnit);
		data.laneType = values.value(laneTypeKey(), data.laneType);
		data.taskYear = values.value(taskYearKey(), values.value(legacyTaskYearKey(), data.taskYear));
		data.inspectionCount = values.value(inspectionCountKey(), data.inspectionCount);
		data.regionCode = values.value(regionCodeKey(), data.regionCode);
		return data;
	}

	inline bool hasSavedRoadWidth(hnPro::hnProject* project)
	{
		if (project && project->isLineCameraProject())
		{
			return false;
		}
		bool widthOk = false;
		const double savedWidth = readValues(configPath(project)).value(roadWidthKey()).toDouble(&widthOk);
		return widthOk && savedWidth > 0.0;
	}

	inline QString valueLine(const QString& key, const QString& value)
	{
		return key + QChar(0xFF1A) + value;
	}

	inline bool save(hnPro::hnProject* project, const Data& data, QString* errorMessage = nullptr)
	{
		if (errorMessage)
		{
			errorMessage->clear();
		}
		const auto fail = [errorMessage](const QString& message)
		{
			if (errorMessage)
			{
				*errorMessage = message;
			}
			return false;
		};

		const QString path = configPath(project);
		if (path.isEmpty())
		{
			return fail(QStringLiteral("\u5de5\u7a0b\u914d\u7f6e\u6587\u4ef6\u8def\u5f84\u4e3a\u7a7a\u3002"));
		}
		if (data.roadWidth <= 0.0)
		{
			return fail(QStringLiteral("\u68c0\u6d4b\u8def\u9762\u5bbd\u5ea6\u5fc5\u987b\u5927\u4e8e 0\u3002"));
		}

		QStringList lines;
		QFile input(path);
		if (input.exists())
		{
			if (!input.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				return fail(QStringLiteral("\u65e0\u6cd5\u8bfb\u53d6\u73b0\u6709\u5de5\u7a0b\u914d\u7f6e\u6587\u4ef6\uff1a%1")
					.arg(input.errorString()));
			}
			QTextStream stream(&input);
			stream.setCodec(QTextCodec::codecForName("UTF-8"));
			while (!stream.atEnd())
			{
				lines.append(stream.readLine());
			}
		}
		input.close();

		int sectionStart = -1;
		int sectionEnd = lines.size();
		for (int i = 0; i < lines.size(); ++i)
		{
			const QString trimmed = lines.at(i).trimmed();
			if (!trimmed.startsWith('[') || !trimmed.endsWith(']'))
			{
				continue;
			}
			const QString section = trimmed.mid(1, trimmed.size() - 2).trimmed();
			if (sectionStart < 0 && isReportSection(section))
			{
				sectionStart = i;
				continue;
			}
			if (sectionStart >= 0)
			{
				sectionEnd = i;
				break;
			}
		}

		if (sectionStart < 0)
		{
			if (!lines.isEmpty() && !lines.last().trimmed().isEmpty())
			{
				lines.append(QString());
			}
			sectionStart = lines.size();
			lines.append(QStringLiteral("[") + sectionName() + QStringLiteral("]"));
			sectionEnd = lines.size();
		}
		else
		{
			lines[sectionStart] = QStringLiteral("[") + sectionName() + QStringLiteral("]");
			for (int i = sectionEnd - 1; i > sectionStart; --i)
			{
				const int separator = separatorIndex(lines.at(i));
				if (separator > 0 && isKnownKey(lines.at(i).left(separator).trimmed()))
				{
					lines.removeAt(i);
					--sectionEnd;
				}
			}
		}

		QStringList reportLines;
		reportLines << valueLine(taskYearKey(), data.taskYear)
			<< valueLine(inspectionCountKey(), data.inspectionCount)
			<< valueLine(maintenanceUnitKey(), data.maintenanceUnit)
			<< valueLine(regionCodeKey(), data.regionCode)
			<< valueLine(roadWidthKey(), QString::number(data.roadWidth, 'g', 12))
			<< valueLine(laneTypeKey(), data.laneType);
		for (int i = reportLines.size() - 1; i >= 0; --i)
		{
			lines.insert(sectionStart + 1, reportLines.at(i));
		}

		QSaveFile output(path);
		if (!output.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			return fail(QStringLiteral("\u65e0\u6cd5\u521b\u5efa\u5de5\u7a0b\u914d\u7f6e\u4e34\u65f6\u6587\u4ef6\uff1a%1")
				.arg(output.errorString()));
		}
		QTextStream stream(&output);
		stream.setCodec(QTextCodec::codecForName("UTF-8"));
		for (const QString& line : qAsConst(lines))
		{
			stream << line << '\n';
		}
		stream.flush();
		const bool committed = output.commit();
		if (committed)
		{
			widthCache().remove(path);
			return true;
		}
		return fail(QStringLiteral("\u65e0\u6cd5\u63d0\u4ea4\u5de5\u7a0b\u914d\u7f6e\u6587\u4ef6\uff1a%1")
			.arg(output.errorString()));
	}

	inline double roadWidth(hnPro::hnProject* project)
	{
		if (project && project->isLineCameraProject())
		{
			return project->effectiveRoadWidth();
		}
		const QString path = configPath(project);
		const QFileInfo fileInfo(path);
		const auto cached = widthCache().constFind(path);
		if (cached != widthCache().constEnd() && cached->lastModified == fileInfo.lastModified() &&
			cached->fileSize == fileInfo.size())
		{
			return cached->roadWidth;
		}

		const double value = load(project).roadWidth;
		WidthCacheEntry entry;
		entry.lastModified = fileInfo.lastModified();
		entry.fileSize = fileInfo.size();
		entry.roadWidth = value;
		widthCache().insert(path, entry);
		return value;
	}
}
