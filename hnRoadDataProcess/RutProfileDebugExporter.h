#pragma once

#include <QFile>
#include <QString>
#include <QVector>
#include <vector>

namespace hnPro
{
	class hnProject;
}

struct hnRutProfileTrace;

// 解析隐藏测试配置，并将命中的车辙断面按步骤流式写成 CloudCompare ASCII 文件。
class RutProfileDebugExporter
{
public:
	RutProfileDebugExporter(hnPro::hnProject* project, const QString& projectBasePath,
		int horizontalPixels, int verticalPixels, int rutMode);
	~RutProfileDebugExporter();

	// 配置文件不存在时返回成功并保持禁用；存在时完成全部校验和输出文件创建。
	bool initialize(QString& errorMessage);
	bool isEnabled() const;
	bool shouldExport(qint64 frameIndex) const;
	// 供配置解析和单元测试共同使用的桩号、帧号换算规则。
	static bool parsePileText(const QString& text, double& trueMile);
	static qint64 firstFrameAtOrAfter(double dmi, double dmiScale);
	static qint64 lastFrameAtOrBefore(double dmi, double dmiScale);

	// 写入一个全局帧的原始断面、算法快照和特征结果。
	bool writeFrame(qint64 frameIndex, const QString& sourceDat,
		const std::vector<short>& rawProfile, const std::vector<float>& worldHeight,
		const hnRutProfileTrace* trace, float rawRut, QString& errorMessage);

	// 正常完成时发布正式目录；失败时保留带 _failed 后缀的诊断现场。
	bool finish(bool success, const QString& failureMessage, QString& errorMessage);

private:
	struct FrameRange
	{
		qint64 startFrame;
		qint64 endFrame;
		int step;
	};

	bool parseConfiguration(QString& errorMessage);
	bool addRange(const QStringList& fields, int lineNumber, QString& errorMessage);
	void mergeRanges();
	bool createOutputFiles(QString& errorMessage);
	bool openOutputFile(QFile& file, const QString& fileName, const QByteArray& header,
		QString& errorMessage);
	bool writeRawStage(qint64 frameIndex, double dmi, double trueMile,
		const std::vector<short>& values, QString& errorMessage);
	bool writeFloatStage(QFile& file, qint64 frameIndex, double dmi, double trueMile,
		const std::vector<float>& values, bool stageApplied, QString& errorMessage);
	int countValidRaw(const std::vector<short>& values) const;
	int countValidFloat(const std::vector<float>& values) const;
	QString csvField(const QString& value) const;
	void closeFiles();
	bool renameRunDirectory(const QString& suffix, QString& errorMessage);

	hnPro::hnProject* m_project;
	QString m_projectBasePath;
	QString m_configPath;
	QString m_runDirectory;
	QString m_finalDirectory;
	int m_horizontalPixels;
	int m_verticalPixels;
	int m_rutMode;
	double m_dmiScale;
	double m_projectLength;
	bool m_enabled;
	bool m_finalized;
	QVector<FrameRange> m_ranges;
	QFile m_rawSensorFile;
	QFile m_worldHeightFile;
	QFile m_firstDetrendedFile;
	QFile m_firstOutlierFile;
	QFile m_secondDetrendedFile;
	QFile m_secondOutlierFile;
	QFile m_filteredFile;
	QFile m_featuresFile;
	QFile m_manifestFile;
};
