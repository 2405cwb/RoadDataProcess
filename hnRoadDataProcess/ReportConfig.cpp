//实现文件 ReportConfig.cpp
#include "ReportConfig.h"

// ReportItem 实现
QJsonObject ReportItem::toJson() const {
	QJsonObject obj;
	obj["RoadModelName"] = roadModelName;
	QJsonArray segArray;
	for (QString val : segments) segArray.append(val);
	obj["Segments"] = segArray;
	obj["IsChecked"] = isChecked;
	obj["ModelName"] = modelName;
	obj["SelectSegment"] = selectSegmentIndex;
	obj["DisplayName"] = displayName;


	obj["ModelPath"] = modelPath/*.replace("/", "\\")*/;
	obj["IsShow"] = isShow;
	return obj;
}

ReportItem ReportItem::fromJson(const QJsonObject& json) {
	ReportItem item;
	item.roadModelName = json["RoadModelName"].toString();
	item.isShow = json["IsShow"].toBool();
	for (const QJsonValue& val : json["Segments"].toArray())
		item.segments.append(val.toString());
	item.isChecked = json["IsChecked"].toBool();
	item.modelName = json["ModelName"].toString();
	item.displayName = json["DisplayName"].toString();
	item.modelPath = json["ModelPath"].toString();
	item.selectSegmentIndex = json["SelectSegment"].toInt();
	return item;
}

// RoadCategory 实现
QJsonObject RoadCategory::toJson() const {
	QJsonObject obj;
	for (auto it = reportGroups.begin(); it != reportGroups.end(); ++it) {
		QJsonArray itemsArray;
		for (const ReportItem& item : it.value())
			itemsArray.append(item.toJson()); 
		 obj[it.key()] = itemsArray;
	}
	return obj;
}

RoadCategory RoadCategory::fromJson(const QString& name, const QJsonObject& json, HnProjectEnums::StandardParmTypeEnum& type, ROAD_WORK_TYPE& drawType) {
	RoadCategory category;
	category.categoryName = name;
	for (const QString& key : json.keys()) {
		 

		QJsonArray itemsArray = json[key].toArray();
		QList<ReportItem> items;
		int index = 0; 
		for (const QJsonValue val : itemsArray)
		{ 
			auto report = ReportItem::fromJson(val.toObject());
			report.index = index++; 
			QString typeStr = HnProjectEnums::roadTypeEnumToQString_ForExcel(type);

			QString drawTypeStr = "";
			switch (drawType)
			{
			case  ROAD_WORK_TYPE::ROAD_WORK_LARGE_RECT:
				drawTypeStr = QStringLiteral("人工模式");
				break;
			case  ROAD_WORK_TYPE::ROAD_WORK_SMALL_RECT:
				drawTypeStr = QStringLiteral("自动化模式");
				break;
			case  ROAD_WORK_TYPE::DESIGN:
				drawTypeStr = QStringLiteral("设计模式");
				break;
			default:
				break;
			}
		 
			QString ModelFilePath = QString("%1\\%2\\%3\\%4\\%5").arg(QStringLiteral("报表模板")).arg(typeStr).arg(drawTypeStr).arg(key).arg(report.modelName);
		//	QString ModelFilePath = QString("%1\\%2\\%3\\%4\\%5").arg(QStringLiteral("报表模板")).arg(typeStr).arg(key).arg(report.modelName);
			report.modelPath = ModelFilePath;
			items.append(report);
		}
		category.reportGroups[key] = items;
	}
	return category;
}

// AppConfig 实现
AppConfig AppConfig::loadFromFile(const QString& filePath, ROAD_WORK_TYPE type) 
{
	AppConfig config;
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Cannot open config file:" << filePath;
		return config;
	}
	QTextStream in(&file);
	in.setCodec("UTF-8");
	QString jsonStr = in.readAll();
	file.close();
	QByteArray jsonData = jsonStr.toUtf8();


	QJsonDocument doc = QJsonDocument::fromJson(jsonData);
	if (doc.isNull()) {
		qWarning() << "Invalid JSON format";
		return config;
	}

	QJsonObject root = doc.object();
	for (const QString& key : root.keys())
	{
		HnProjectEnums::StandardParmTypeEnum standard = HnProjectEnums::roadTypeQStringToEnum(key);
		config.categories[standard] = RoadCategory::fromJson(key, root[key].toObject(), standard,type);

	}
	return config;
}

bool AppConfig::saveToFile(const QString& filePath) const {
	QJsonObject root;
	for (auto it = categories.begin(); it != categories.end(); ++it)
	{
		QString type = HnProjectEnums::roadTypeEnumToQString_ForExcel(it.key());
		root[type] = it.value().toJson();
	}
	

	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly)) {
		qWarning() << "Cannot write to file:" << filePath;
		return false;
	}
	file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
	return true;
}