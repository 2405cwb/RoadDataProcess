#pragma once
#pragma once
#include <QString>
#include <QMap>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QJsonDocument>
#include <QDebug>
#include "../hnQtCommon/HnProjectEnums.h"
#include "../hnCommon/hnRoadTypeDef.h"
// 报表项配置
using namespace hnCommon;
class ReportItem {
public:
	//报表Index
	int index;
	//报表类型名称
	QString roadModelName;

	//分段区间 10,100,100
	QList<QString> segments;

	int selectSegmentIndex;

	//报表是否选中
	bool isChecked;

	//模板报表名称
	QString modelName;

	//报表展示名称
	QString displayName;

	//模板报表地址
	QString modelPath;

	bool isShow;
	QJsonObject toJson() const; 
	static ReportItem fromJson(const QJsonObject& json);
};
//TODO 报表配置类
// 道路类别配置（支持动态出表类型）
class RoadCategory {
public:
	QString categoryName;
	// 动态存储不同类型的出表配置（键：出表类型名，值：报表项列表）
	QMap<QString, QList<ReportItem>> reportGroups;

	QJsonObject toJson() const;
	static RoadCategory fromJson(const QString& name, const QJsonObject& json, HnProjectEnums::StandardParmTypeEnum& type, ROAD_WORK_TYPE& work);
};

// 全局配置
class AppConfig {
public:
	QMap<HnProjectEnums::StandardParmTypeEnum, RoadCategory> categories;

	static AppConfig loadFromFile(const QString& filePath, ROAD_WORK_TYPE type);
	bool saveToFile(const QString& filePath) const;
};
