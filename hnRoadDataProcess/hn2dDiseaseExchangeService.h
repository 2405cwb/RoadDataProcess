#pragma once

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>
#include <functional>

#include "../hnCommon/hnRoadStruct.h"
#include "../hnQtCommon/hnMile.h"

namespace hnPro
{
	class hnProject;
}

namespace hnDiseaseImportRules
{
	bool meetsRuralDiseaseAreaRequirement(HnProjectEnums::StandardParmTypeEnum standard,
		const QByteArray& tableName, double area);
}

class hn2dDiseaseExchangeService
{
public:
	struct Result
	{
		Result();

		bool success;
		int roadCount;
		int streetCount;
		int skippedCount;
		QStringList warnings;
		QString error;
		QString backupPath;
		bool stopped;
	};

	explicit hn2dDiseaseExchangeService(hnPro::hnProject* project);

	// 将成果库病害覆盖输出为二维软件逐图文本。
	Result exportDiseases(int frameType);
	// 输出当前工程数据库中的自定义景观明细及图片。
	Result exportCustomStreetReport(const QString& filePath);

	// 导入二维软件景观病害文本；路面病害仍由既有导入器负责。
	Result importStreetDiseases();

	// 原样覆盖当前目标工程；回调返回 true 时回滚当前工程。
	Result replaceDiseases(const std::function<bool()>& cancelled = std::function<bool()>());

private:
	struct ImageRef
	{
		ImageRef();
		QString imagePath;
		double dmi;
		double trueMile;
		int side;
	};

	typedef QMap<QString, QStringList> TextFileMap;

	QVector<ImageRef> roadImages() const;
	QVector<ImageRef> streetImages(int side) const;
	ImageRef closestImage(const QVector<ImageRef>& images, double dmi) const;

	bool buildBigFrameFiles(const QVector<hnCommon::hnRoadDiseaseInfo>& diseases,
		const QVector<ImageRef>& images, TextFileMap& files, QStringList& warnings, int& skippedCount) const;
	bool buildLittleFrameFiles(const QVector<hnCommon::hnRoadDiseaseInfo>& diseases,
		const QVector<ImageRef>& images, TextFileMap& files, QStringList& warnings, int& skippedCount) const;
	bool buildStreetFiles(const QVector<hnCommon::hnRoadDiseaseInfo>& diseases,
		TextFileMap& normalFiles, TextFileMap& userFiles, QStringList& warnings, int& skippedCount) const;

	bool writeTextFiles(const QString& rootPath, const QStringList& suffixes,
		const TextFileMap& files, QString& error) const;
	bool writeUtf8File(const QString& filePath, const QStringList& lines, QString& error) const;

	void loadStreetChannel(const QVector<ImageRef>& images, int defaultSide,
		QMap<QString, QVector<hnCommon::hnRoadDiseaseInfo>>& diseases,
		Result& result) const;
	void loadStreetFile(const QString& filePath, const ImageRef& image, int defaultSide,
		bool userSign, QMap<QString, QVector<hnCommon::hnRoadDiseaseInfo>>& diseases,
		Result& result) const;
	bool parseStreetLine(const QString& line, const ImageRef& image, int defaultSide,
		bool userSign, hnCommon::hnRoadDiseaseInfo& disease, QString& tableName) const;
	bool parseRoadLine(const QString& line, const ImageRef& image,
		hnCommon::hnRoadDiseaseInfo& disease, QString& error) const;

	QString formatStake(double trueMile) const;
	QString roadSurfaceName(int surface) const;
	QString roadDiseaseName(const hnCommon::hnRoadDiseaseInfo& disease) const;
	int exchangeSide(const hnCommon::hnRoadDiseaseInfo& disease) const;
	bool isUserStreetDisease(const hnCommon::hnRoadDiseaseInfo& disease) const;

	hnPro::hnProject* m_project;
};
