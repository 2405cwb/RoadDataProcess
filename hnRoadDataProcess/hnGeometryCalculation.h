#pragma once

#include "hnCalRoadGeometry.h"

#include <QMutex>
#include <QThread>
#include <QStringList>
#include <QVector>
#include <atomic>
#include <functional>

struct GeometryCalculationInput
{
	QStringList posFiles;
	QString camPath;
	QString scanParameterPath;
	QString resultPath;
	double projectLength = 0.0;
	GeometryCalculationOptions options;
};

struct GeometryCalculationResult
{
	GeometryCalculationStatus status = GeometryCalculationStatus::InvalidInput;
	QString errorMessage;
	QString warningMessage;
	double scanLength = 0.0;
	std::vector<hnRoadGeoParam> rawSamples;
	std::vector<hnRoadGeoParam> outputSamples;
};

class hnGeometryAlgorithms
{
public:
	static bool fitHuber(const QVector<hnPoint3d>& points, double& slope, double& intercept,
		double& rmse, double& validPointRatio);
	static bool aggregate(const std::vector<hnRoadGeoParam>& rawSamples, double outputSpacing,
		std::vector<hnRoadGeoParam>& outputSamples, QString* errorMessage = nullptr);
	static void calculateCurvature(std::vector<hnRoadGeoParam>& samples, double halfWindowMeters = 5.0);
};

class hnGeometryCalculator
{
public:
	typedef std::function<bool(int, const QString&)> ProgressCallback;

	GeometryCalculationResult calculate(const GeometryCalculationInput& input,
		std::atomic_bool* cancelFlag, const ProgressCallback& progress) const;
	static bool writeResults(const QString& resultPath,
		const std::vector<hnRoadGeoParam>& samples, QString* errorMessage);
};

// 不使用自定义信号，避免旧工程额外的moc依赖；界面通过QTimer读取线程安全快照。
class hnGeometryCalculationThread : public QThread
{
public:
	explicit hnGeometryCalculationThread(const GeometryCalculationInput& input, QObject* parent = nullptr);
	void requestCancel();
	int progressValue() const;
	QString progressText() const;
	GeometryCalculationResult result() const;

protected:
	void run() override;

private:
	GeometryCalculationInput m_input;
	mutable QMutex m_mutex;
	std::atomic_bool m_cancelled;
	int m_progress;
	QString m_progressText;
	GeometryCalculationResult m_result;
};
