#pragma once

#include "../hnProject/hnProject.h"

#include <QByteArray>
#include <QString>

namespace hnReportProjectInfo
{
	struct Data
	{
		double roadWidth = 0.0;
		QString maintenanceUnit = QStringLiteral("\u4EA4\u901A\u5C40");
		QString laneType = QStringLiteral("\u53CC\u5411\u56DB\u8F66\u9053");
		QString taskYear = QStringLiteral("2024");
		QString inspectionCount = QStringLiteral("1-\u4E00\u6B21");
		QString regionCode = QStringLiteral("620122");
	};

	inline bool readDatabaseInfo(hnPro::hnProject* project, hnCommon::hnProjectSetInfo& info)
	{
		return project && project->getDB() && project->getDB()->isOpen()
			&& project->getDB()->m_projectSetTable.readData(info);
	}

	inline QString databaseText(const char* value, const QString& defaultValue)
	{
		const QString text = QString::fromLocal8Bit(value).trimmed();
		return text.isEmpty() ? defaultValue : text;
	}

	inline void copyDatabaseText(char* destination, int capacity, const QString& value)
	{
		const QByteArray bytes = value.toLocal8Bit();
		qstrncpy(destination, bytes.constData(), capacity);
	}

	inline Data load(hnPro::hnProject* project)
	{
		Data data;
		hnCommon::hnProjectSetInfo info;
		if (!readDatabaseInfo(project, info))
		{
			if (project && project->isLineCameraProject())
			{
				data.roadWidth = project->effectiveRoadWidth();
			}
			return data;
		}

		// Report fields are stored in SETTING_INFO.
		data.roadWidth = project->isLineCameraProject()
			? project->effectiveRoadWidth() : info.dRoadWidth;
		data.maintenanceUnit = databaseText(info.strAddFile2, data.maintenanceUnit);
		data.laneType = databaseText(info.strAddFile3, data.laneType);
		data.taskYear = databaseText(info.strAddFile4, data.taskYear);
		data.inspectionCount = databaseText(info.strAddFile5, data.inspectionCount);
		data.regionCode = databaseText(info.strRemark, data.regionCode);
		return data;
	}

	inline bool hasSavedRoadWidth(hnPro::hnProject* project)
	{
		hnCommon::hnProjectSetInfo info;
		return project && !project->isLineCameraProject()
			&& readDatabaseInfo(project, info) && info.dRoadWidth > 0.0;
	}

	inline bool save(hnPro::hnProject* project, const Data& data, QString* errorMessage = nullptr)
	{
		if (errorMessage)
		{
			errorMessage->clear();
		}
		if (!project || !project->getDB() || !project->getDB()->isOpen())
		{
			if (errorMessage)
			{
				*errorMessage = QStringLiteral("\u6210\u679C\u6570\u636E\u5E93\u4E0D\u53EF\u7528\u3002");
			}
			return false;
		}
		if (data.roadWidth <= 0.0)
		{
			if (errorMessage)
			{
				*errorMessage = QStringLiteral("\u68C0\u6D4B\u8DEF\u9762\u5BBD\u5EA6\u5FC5\u987B\u5927\u4E8E 0\u3002");
			}
			return false;
		}

		hnCommon::hnProjectSetInfo info;
		if (!readDatabaseInfo(project, info))
		{
			if (errorMessage)
			{
				*errorMessage = QStringLiteral("\u6210\u679C\u6570\u636E\u5E93 SETTING_INFO \u4E2D\u6CA1\u6709\u6709\u6548\u5DE5\u7A0B\u4FE1\u606F\u3002");
			}
			return false;
		}

		if (!project->isLineCameraProject())
		{
			info.dRoadWidth = data.roadWidth;
		}
		// AddFile2~5 store unit, lane type, year and inspection count; Remark stores region code.
		copyDatabaseText(info.strAddFile2, sizeof(info.strAddFile2), data.maintenanceUnit);
		copyDatabaseText(info.strAddFile3, sizeof(info.strAddFile3), data.laneType);
		copyDatabaseText(info.strAddFile4, sizeof(info.strAddFile4), data.taskYear);
		copyDatabaseText(info.strAddFile5, sizeof(info.strAddFile5), data.inspectionCount);
		copyDatabaseText(info.strRemark, sizeof(info.strRemark), data.regionCode);
		if (!project->getDB()->m_projectSetTable.writeData(info))
		{
			if (errorMessage)
			{
				*errorMessage = QStringLiteral("\u5199\u5165\u6210\u679C\u6570\u636E\u5E93 SETTING_INFO \u5931\u8D25\u3002");
			}
			return false;
		}
		return true;
	}

	inline double roadWidth(hnPro::hnProject* project)
	{
		return load(project).roadWidth;
	}
}
