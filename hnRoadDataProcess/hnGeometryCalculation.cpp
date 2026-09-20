#include "hnGeometryCalculation.h"

#include "../hnPavementCreate3d/hnPavementCamReader.h"
#include "../hnPcdCoordinate/hnPcdCoordinate.h"

#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QSaveFile>
#include <QTextStream>
#include <QMutexLocker>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <limits>

namespace
{
	double median(QVector<double> values)
	{
		if (values.isEmpty())
			return 0.0;
		const int middle = values.size() / 2;
		std::nth_element(values.begin(), values.begin() + middle, values.end());
		const double upper = values[middle];
		if (values.size() % 2 != 0)
			return upper;
		const double lower = *std::max_element(values.begin(), values.begin() + middle);
		return (lower + upper) * 0.5;
	}

	bool weightedLineFit(const QVector<hnPoint3d>& points, const QVector<double>& weights,
		double& slope, double& intercept)
	{
		if (points.size() < 2 || points.size() != weights.size())
			return false;
		double sw = 0.0, sx = 0.0, sz = 0.0, sxx = 0.0, sxz = 0.0;
		for (int i = 0; i < points.size(); ++i)
		{
			const double w = weights[i];
			sw += w;
			sx += w * points[i].x;
			sz += w * points[i].z;
			sxx += w * points[i].x * points[i].x;
			sxz += w * points[i].x * points[i].z;
		}
		const double denominator = sw * sxx - sx * sx;
		if (sw <= 0.0 || std::abs(denominator) < 1e-12)
			return false;
		slope = (sw * sxz - sx * sz) / denominator;
		intercept = (sz - slope * sx) / sw;
		return std::isfinite(slope) && std::isfinite(intercept);
	}

	struct FrameTransform
	{
		double values[3][4];
		POS_STRUCT_INFO pos;
	};

	bool buildFrameTransform(hnPcdCoordinate& coordinate, const Eigen::Matrix<double, 4, 4>& matrix,
		std::vector<POS_STRUCT_INFO>& pos, double gpsTime, FrameTransform& transform)
	{
		int nearest = 0;
		if (!coordinate.linearInsertPos(gpsTime, pos, transform.pos, nearest))
			return false;

		// calcuCoord每次都会构造动态矩阵。一个断面内所有点共享POS，
		// 因此只变换原点和三个单位基向量，得到本断面的仿射矩阵。
		double basis[4][3] = { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 },
			{ 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 } };
		for (int i = 0; i < 4; ++i)
		{
			Eigen::Matrix<double, 4, 4> mutableMatrix = matrix;
			coordinate.calcuCoord(mutableMatrix, transform.pos, basis[i][0], basis[i][1], basis[i][2]);
			if (!std::isfinite(basis[i][0]) || !std::isfinite(basis[i][1]) || !std::isfinite(basis[i][2]))
				return false;
		}
		for (int axis = 0; axis < 3; ++axis)
		{
			transform.values[axis][0] = basis[1][axis] - basis[0][axis];
			transform.values[axis][1] = basis[2][axis] - basis[0][axis];
			transform.values[axis][2] = basis[3][axis] - basis[0][axis];
			transform.values[axis][3] = basis[0][axis];
		}
		return true;
	}

	bool transformPoint(const FrameTransform& transform, const POINT_STRUCT_XYZIT_INFO& source,
		hnPoint3d& target)
	{
		target.x = transform.values[0][0] * source.x + transform.values[0][1] * source.y
			+ transform.values[0][2] * source.z + transform.values[0][3];
		target.y = transform.values[1][0] * source.x + transform.values[1][1] * source.y
			+ transform.values[1][2] * source.z + transform.values[1][3];
		target.z = transform.values[2][0] * source.x + transform.values[2][1] * source.y
			+ transform.values[2][2] * source.z + transform.values[2][3];
		return std::isfinite(target.x) && std::isfinite(target.y) && std::isfinite(target.z);
	}

	bool cancelled(std::atomic_bool* flag)
	{
		return flag && flag->load();
	}

	bool reportProgress(const hnGeometryCalculator::ProgressCallback& callback,
		std::atomic_bool* flag, int value, const QString& text)
	{
		if (cancelled(flag))
			return false;
		return !callback || callback(qBound(0, value, 100), text);
	}

	bool readFrameAtMileage(hnPavementCamReader& reader, double mileage,
		std::vector<POINT_STRUCT_XYZIT_INFO>& points, int& pointCount)
	{
		const qint64 sumFrame = qRound64(mileage / 0.002);
		const int mainFrame = static_cast<int>(sumFrame / 40);
		const int subFrame = static_cast<int>(sumFrame % 40);
		pointCount = 0;
		return reader.getSubFramePoints(mainFrame, subFrame, 0, points, pointCount) && pointCount > 0;
	}

	bool sampleMedian(const QVector<double>& values, double& result)
	{
		if (values.isEmpty())
			return false;
		result = median(values);
		return std::isfinite(result);
	}

	bool aggregateMetric(const std::vector<hnRoadGeoParam>& raw, int begin, int end,
		double hnRoadGeoParam::* valueMember, bool hnRoadGeoParam::* validMember,
		int minimumValid, double& value)
	{
		QVector<double> values;
		for (int i = begin; i < end; ++i)
		{
			if (raw[i].*validMember && std::isfinite(raw[i].*valueMember))
				values.append(raw[i].*valueMember);
		}
		if (values.size() < minimumValid)
			return false;
		value = median(values);
		return true;
	}

	bool loadPosFiles(hnPcdCoordinate& coordinate, const QStringList& files,
		std::vector<POS_STRUCT_INFO>& pos, std::atomic_bool* cancelFlag,
		const hnGeometryCalculator::ProgressCallback& progress, QString& error)
	{
		qint64 totalBytes = 0;
		for (const QString& path : files)
			totalBytes += QFileInfo(path).size();
		qint64 consumedBytes = 0;
		for (const QString& path : files)
		{
			QFile file(path);
			if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				error = QStringLiteral("POS文件读取失败：%1").arg(path);
				return false;
			}
			while (!file.atEnd())
			{
				if (cancelled(cancelFlag))
					return false;
				const QByteArray line = file.readLine();
				POS_STRUCT_INFO info;
				if (info.serialize(line.constData()))
					pos.push_back(info);
				if ((pos.size() & 0x7ff) == 0 && totalBytes > 0)
				{
					const qint64 currentBytes = consumedBytes + file.pos();
					const int value = 2 + static_cast<int>(6.0 * currentBytes / totalBytes);
					if (!reportProgress(progress, cancelFlag, value, QStringLiteral("正在加载POS数据...")))
						return false;
				}
			}
			consumedBytes += file.size();
		}
		if (pos.size() < 3)
		{
			error = QStringLiteral("有效POS记录不足3条");
			return false;
		}
		coordinate.convertBlhToNeh(pos);
		return true;
	}
}

bool hnGeometryAlgorithms::fitHuber(const QVector<hnPoint3d>& input, double& slope,
	double& intercept, double& rmse, double& validPointRatio)
{
	slope = intercept = rmse = validPointRatio = 0.0;
	if (input.size() < 2)
		return false;

	QVector<double> zValues;
	for (const hnPoint3d& point : input)
		zValues.append(point.z);
	const double zMedian = median(zValues);
	QVector<double> deviations;
	for (double z : zValues)
		deviations.append(std::abs(z - zMedian));
	const double mad = median(deviations);

	QVector<hnPoint3d> points;
	const double grossLimit = mad > 1e-9 ? 3.5 * 1.4826 * mad : (std::numeric_limits<double>::max)();
	for (const hnPoint3d& point : input)
	{
		if (std::abs(point.z - zMedian) <= grossLimit)
			points.append(point);
	}
	validPointRatio = static_cast<double>(points.size()) / input.size();
	if (points.size() < 2)
		return false;

	QVector<double> weights(points.size(), 1.0);
	if (!weightedLineFit(points, weights, slope, intercept))
		return false;

	for (int iteration = 0; iteration < 10; ++iteration)
	{
		QVector<double> residuals;
		for (const hnPoint3d& point : points)
			residuals.append(point.z - (slope * point.x + intercept));
		const double residualMedian = median(residuals);
		QVector<double> absoluteResiduals;
		for (double residual : residuals)
			absoluteResiduals.append(std::abs(residual - residualMedian));
		const double scale = 1.4826 * median(absoluteResiduals);
		if (scale < 1e-9)
			break;
		const double huberLimit = 1.5 * scale;
		for (int i = 0; i < residuals.size(); ++i)
		{
			const double absolute = std::abs(residuals[i]);
			weights[i] = absolute <= huberLimit ? 1.0 : huberLimit / absolute;
		}
		const double oldSlope = slope;
		const double oldIntercept = intercept;
		if (!weightedLineFit(points, weights, slope, intercept))
			return false;
		if (std::abs(slope - oldSlope) < 1e-8 && std::abs(intercept - oldIntercept) < 1e-8)
			break;
	}

	double squaredError = 0.0;
	for (const hnPoint3d& point : points)
	{
		const double residual = point.z - (slope * point.x + intercept);
		squaredError += residual * residual;
	}
	rmse = std::sqrt(squaredError / points.size());
	return std::isfinite(rmse);
}

void hnGeometryAlgorithms::calculateCurvature(std::vector<hnRoadGeoParam>& samples, double halfWindowMeters)
{
	if (samples.empty())
		return;
	QVector<double> unwrapped(static_cast<int>(samples.size()), 0.0);
	int lastValidIndex = -1;
	for (int i = 0; i < static_cast<int>(samples.size()); ++i)
	{
		if (!samples[i].bCurvatureValid)
		{
			if (lastValidIndex >= 0)
				unwrapped[i] = unwrapped[lastValidIndex];
			continue;
		}
		if (lastValidIndex < 0)
		{
			unwrapped[i] = samples[i].dYaw;
			lastValidIndex = i;
			continue;
		}
		double delta = samples[i].dYaw - samples[lastValidIndex].dYaw;
		while (delta > 180.0) delta -= 360.0;
		while (delta < -180.0) delta += 360.0;
		unwrapped[i] = unwrapped[lastValidIndex] + delta;
		lastValidIndex = i;
	}

	for (int i = 0; i < static_cast<int>(samples.size()); ++i)
	{
		QVector<hnPoint3d> window;
		for (int j = 0; j < static_cast<int>(samples.size()); ++j)
		{
			if (!samples[j].bCurvatureValid || std::abs(samples[j].dMileage - samples[i].dMileage) > halfWindowMeters)
				continue;
			hnPoint3d point;
			point.x = samples[j].dMileage;
			point.y = 0.0;
			point.z = unwrapped[j] * M_PI / 180.0;
			window.append(point);
		}
		double slope = 0.0, intercept = 0.0, rmse = 0.0, ratio = 0.0;
		samples[i].bCurvatureValid = window.size() >= 5 && fitHuber(window, slope, intercept, rmse, ratio);
		if (samples[i].bCurvatureValid)
			samples[i].dC = slope;
	}
}

bool hnGeometryAlgorithms::aggregate(const std::vector<hnRoadGeoParam>& raw, double outputSpacing,
	std::vector<hnRoadGeoParam>& output, QString* errorMessage)
{
	output.clear();
	if (raw.empty() || outputSpacing <= 0.0)
	{
		if (errorMessage) *errorMessage = QStringLiteral("没有可汇总的几何样本或输出间距无效");
		return false;
	}
	int begin = 0;
	while (begin < static_cast<int>(raw.size()))
	{
		const double binStart = std::floor(raw[begin].dMileage / outputSpacing) * outputSpacing;
		int end = begin;
		while (end < static_cast<int>(raw.size()) && raw[end].dMileage < binStart + outputSpacing - 1e-8)
			++end;
		const int count = end - begin;
		const int minimumValid = qMax(1, (count + 1) / 2);
		hnRoadGeoParam sample;
		sample.dMileage = binStart;
		sample.bCurvatureValid = aggregateMetric(raw, begin, end, &hnRoadGeoParam::dC,
			&hnRoadGeoParam::bCurvatureValid, minimumValid, sample.dC);
		sample.bVAngleValid = aggregateMetric(raw, begin, end, &hnRoadGeoParam::dVAngle,
			&hnRoadGeoParam::bVAngleValid, minimumValid, sample.dVAngle);
		sample.bHAngleValid = aggregateMetric(raw, begin, end, &hnRoadGeoParam::dHAngle,
			&hnRoadGeoParam::bHAngleValid, minimumValid, sample.dHAngle);
		QVector<double> rmses;
		double ratioSum = 0.0;
		int ratioCount = 0;
		for (int i = begin; i < end; ++i)
		{
			if (raw[i].dFitRmse > 0.0) rmses.append(raw[i].dFitRmse);
			if (raw[i].dValidPointRatio > 0.0) { ratioSum += raw[i].dValidPointRatio; ++ratioCount; }
		}
		sample.dFitRmse = rmses.isEmpty() ? 0.0 : median(rmses);
		sample.dValidPointRatio = ratioCount == 0 ? 0.0 : ratioSum / ratioCount;
		output.push_back(sample);
		begin = end;
	}
	return true;
}

GeometryCalculationResult hnGeometryCalculator::calculate(const GeometryCalculationInput& input,
	std::atomic_bool* cancelFlag, const ProgressCallback& progress) const
{
	GeometryCalculationResult result;
	if (input.posFiles.isEmpty() || input.camPath.isEmpty() || input.scanParameterPath.isEmpty()
		|| input.projectLength <= 0.0 || input.options.sampleSpacing <= 0.0)
	{
		result.errorMessage = QStringLiteral("几何计算输入不完整");
		return result;
	}
	if (!reportProgress(progress, cancelFlag, 0, QStringLiteral("正在校验几何计算输入...")))
	{
		result.status = GeometryCalculationStatus::Cancelled;
		return result;
	}

	hnPcdCoordinate coordinate;
	std::vector<POS_STRUCT_INFO> pos;
	if (!loadPosFiles(coordinate, input.posFiles, pos, cancelFlag, progress, result.errorMessage))
	{
		result.status = cancelled(cancelFlag) ? GeometryCalculationStatus::Cancelled : GeometryCalculationStatus::ReadFailed;
		return result;
	}
	std::sort(pos.begin(), pos.end(), [](const POS_STRUCT_INFO& left, const POS_STRUCT_INFO& right) {
		return left.dGpsSecond < right.dGpsSecond;
	});
	pos.erase(std::unique(pos.begin(), pos.end(), [](const POS_STRUCT_INFO& left, const POS_STRUCT_INFO& right) {
		return std::abs(left.dGpsSecond - right.dGpsSecond) < 1e-7;
	}), pos.end());

	Eigen::Matrix<double, 4, 4> pointsToPos;
	const QByteArray parameterPath = QFile::encodeName(input.scanParameterPath);
	if (!coordinate.setiScanParaPath(0, parameterPath.constData(), pointsToPos))
	{
		result.status = GeometryCalculationStatus::ReadFailed;
		result.errorMessage = QStringLiteral("扫描标定参数读取失败：%1").arg(input.scanParameterPath);
		return result;
	}

	hnPavementCamReader reader;
	const QByteArray camPath = QFile::encodeName(input.camPath);
	if (!reader.Open(camPath.constData()))
	{
		result.status = GeometryCalculationStatus::ReadFailed;
		result.errorMessage = QStringLiteral("点云相机文件读取失败：%1").arg(input.camPath);
		return result;
	}
	reader.setSubFrameCacheEnabled(true);
	result.scanLength = reader.GetScanLines() * 40.0 * 0.002;
	if (result.scanLength <= 0.0)
	{
		result.status = GeometryCalculationStatus::InvalidInput;
		result.errorMessage = QStringLiteral("点云扫描长度无效：%1m")
			.arg(result.scanLength, 0, 'f', 3);
		return result;
	}
	const double calculationLength = qMin(input.projectLength, result.scanLength);
	if (result.scanLength + 0.001 < input.projectLength)
	{
		result.warningMessage = QStringLiteral(
			"工程长度为%1m，点云扫描长度为%2m，末端%3m无点云覆盖。\n"
			"本次已按点云有效范围0-%2m计算，超出部分不生成几何结果。")
			.arg(input.projectLength, 0, 'f', 3)
			.arg(result.scanLength, 0, 'f', 3)
			.arg(input.projectLength - result.scanLength, 0, 'f', 3);
	}

	// 点云末端可能早于工程封闭里程，仅对两者共同覆盖范围采样。
	const int sampleCount = static_cast<int>(std::floor(calculationLength / input.options.sampleSpacing)) + 1;
	result.rawSamples.reserve(sampleCount);
	std::vector<POINT_STRUCT_XYZIT_INFO> sourcePoints;
	for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
	{
		if (cancelled(cancelFlag))
		{
			result.status = GeometryCalculationStatus::Cancelled;
			return result;
		}
		hnRoadGeoParam sample;
		sample.dMileage = sampleIndex * input.options.sampleSpacing;
		if (sample.dMileage >= result.scanLength)
			break;

		QVector<hnPoint3d> longitudinalPoints;
		const int longitudinalCount = qMax(1, qRound(input.options.longitudinalWindow / 0.1));
		for (int j = 0; j < longitudinalCount; ++j)
		{
			const double mileage = sample.dMileage + j * 0.1;
			if (mileage >= result.scanLength)
				break;
			int pointCount = 0;
			if (!readFrameAtMileage(reader, mileage, sourcePoints, pointCount))
				continue;
			FrameTransform frameTransform;
			if (!buildFrameTransform(coordinate, pointsToPos, pos, sourcePoints[0].timeSecond, frameTransform))
				continue;
			QVector<double> elevations;
			for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex)
			{
				if (std::abs(sourcePoints[pointIndex].x) > 0.01)
					continue;
				hnPoint3d transformed;
				if (transformPoint(frameTransform, sourcePoints[pointIndex], transformed))
					elevations.append(transformed.z);
			}
			double elevation = 0.0;
			if (sampleMedian(elevations, elevation))
			{
				hnPoint3d point;
				point.x = mileage;
				point.y = 0.0;
				point.z = elevation;
				longitudinalPoints.append(point);
			}
		}
		double slope = 0.0, intercept = 0.0, rmse = 0.0, validRatio = 0.0;
		sample.bVAngleValid = longitudinalPoints.size() >= 6
			&& hnGeometryAlgorithms::fitHuber(longitudinalPoints, slope, intercept, rmse, validRatio);
		if (sample.bVAngleValid)
			sample.dVAngle = slope;

		int crossPointCount = 0;
		if (readFrameAtMileage(reader, sample.dMileage, sourcePoints, crossPointCount))
		{
			QVector<hnPoint3d> crossPoints;
			FrameTransform frameTransform;
			if (!buildFrameTransform(coordinate, pointsToPos, pos, sourcePoints[0].timeSecond, frameTransform))
			{
				result.rawSamples.push_back(sample);
				continue;
			}
			sample.dYaw = frameTransform.pos.dHeading;
			sample.bCurvatureValid = true; // 表示航向角有效，曲率稍后统一计算。
			hnPoint3d previous;
			bool hasPrevious = false;
			double lateralDistance = 0.0;
			for (int pointIndex = 0; pointIndex < crossPointCount; ++pointIndex)
			{
				hnPoint3d transformed;
				if (!transformPoint(frameTransform, sourcePoints[pointIndex], transformed))
					continue;
				if (hasPrevious)
				{
					const double dx = transformed.x - previous.x;
					const double dy = transformed.y - previous.y;
					lateralDistance += std::sqrt(dx * dx + dy * dy);
				}
				hnPoint3d fitPoint;
				fitPoint.x = lateralDistance;
				fitPoint.y = 0.0;
				fitPoint.z = transformed.z;
				crossPoints.append(fitPoint);
				previous = transformed;
				hasPrevious = true;
			}
			if (crossPoints.size() >= 30 && lateralDistance >= 1.0)
			{
				slope = intercept = rmse = validRatio = 0.0;
				sample.bHAngleValid = hnGeometryAlgorithms::fitHuber(crossPoints, slope, intercept, rmse, validRatio);
				if (sample.bHAngleValid)
				{
					sample.dHAngle = slope;
					sample.dFitRmse = rmse;
					sample.dValidPointRatio = validRatio;
				}
			}
		}
		result.rawSamples.push_back(sample);
		const int percent = 10 + static_cast<int>(80.0 * (sampleIndex + 1) / sampleCount);
		if (!reportProgress(progress, cancelFlag, percent,
			QStringLiteral("正在计算第%1/%2个1米断面...").arg(sampleIndex + 1).arg(sampleCount)))
		{
			result.status = GeometryCalculationStatus::Cancelled;
			return result;
		}
	}

	if (result.rawSamples.empty())
	{
		result.status = GeometryCalculationStatus::InsufficientData;
		result.errorMessage = QStringLiteral("点云范围内没有可计算的几何断面");
		return result;
	}
	hnGeometryAlgorithms::calculateCurvature(result.rawSamples);
	if (!reportProgress(progress, cancelFlag, 93, QStringLiteral("正在汇总10米几何结果...")))
	{
		result.status = GeometryCalculationStatus::Cancelled;
		return result;
	}
	if (!hnGeometryAlgorithms::aggregate(result.rawSamples, input.options.outputSpacing,
		result.outputSamples, &result.errorMessage))
	{
		result.status = GeometryCalculationStatus::InsufficientData;
		return result;
	}
	if (!reportProgress(progress, cancelFlag, 97, QStringLiteral("正在写入几何结果...")))
	{
		result.status = GeometryCalculationStatus::Cancelled;
		return result;
	}
	if (!writeResults(input.resultPath, result.outputSamples, &result.errorMessage))
	{
		result.status = GeometryCalculationStatus::WriteFailed;
		return result;
	}
	result.status = GeometryCalculationStatus::Success;
	reportProgress(progress, cancelFlag, 100, QStringLiteral("几何线型计算完成"));
	return result;
}

bool hnGeometryCalculator::writeResults(const QString& resultPath,
	const std::vector<hnRoadGeoParam>& samples, QString* errorMessage)
{
	const QString qualityPath = QFileInfo(resultPath).dir().filePath(
		QFileInfo(resultPath).completeBaseName() + QStringLiteral(".quality.csv"));
	QSaveFile qualityFile(qualityPath);
	if (!qualityFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		if (errorMessage) *errorMessage = QStringLiteral("无法写入质量文件：%1").arg(qualityPath);
		return false;
	}
	QTextStream quality(&qualityFile);
	quality << "Mileage,CurvatureValid,LongitudinalValid,CrossValid,FitRmse,ValidPointRatio\n";
	for (const hnRoadGeoParam& sample : samples)
	{
		quality << QString::number(sample.dMileage, 'f', 3) << ','
			<< (sample.bCurvatureValid ? 1 : 0) << ',' << (sample.bVAngleValid ? 1 : 0) << ','
			<< (sample.bHAngleValid ? 1 : 0) << ',' << QString::number(sample.dFitRmse, 'g', 12) << ','
			<< QString::number(sample.dValidPointRatio, 'g', 12) << '\n';
	}
	if (!qualityFile.commit())
	{
		if (errorMessage) *errorMessage = QStringLiteral("质量文件提交失败：%1").arg(qualityPath);
		return false;
	}

	QSaveFile resultFile(resultPath);
	if (!resultFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		if (errorMessage) *errorMessage = QStringLiteral("无法写入几何结果：%1").arg(resultPath);
		return false;
	}
	QTextStream output(&resultFile);
	for (const hnRoadGeoParam& sample : samples)
	{
		output << QString::number(sample.dMileage, 'f', 3) << ','
			<< QString::number(sample.dC, 'g', 12) << ',' << QString::number(sample.dVAngle, 'g', 12) << ','
			<< QString::number(sample.dHAngle, 'g', 12) << '\n';
	}
	if (!resultFile.commit())
	{
		if (errorMessage) *errorMessage = QStringLiteral("几何结果提交失败：%1").arg(resultPath);
		return false;
	}
	return true;
}

hnGeometryCalculationThread::hnGeometryCalculationThread(const GeometryCalculationInput& input, QObject* parent)
	: QThread(parent), m_input(input), m_cancelled(false), m_progress(0)
{
}

void hnGeometryCalculationThread::requestCancel()
{
	m_cancelled.store(true);
}

int hnGeometryCalculationThread::progressValue() const
{
	QMutexLocker locker(&m_mutex);
	return m_progress;
}

QString hnGeometryCalculationThread::progressText() const
{
	QMutexLocker locker(&m_mutex);
	return m_progressText;
}

GeometryCalculationResult hnGeometryCalculationThread::result() const
{
	QMutexLocker locker(&m_mutex);
	return m_result;
}

void hnGeometryCalculationThread::run()
{
	hnGeometryCalculator calculator;
	GeometryCalculationResult calculated = calculator.calculate(m_input, &m_cancelled,
		[this](int value, const QString& text) {
			QMutexLocker locker(&m_mutex);
			m_progress = value;
			m_progressText = text;
			return !m_cancelled.load();
		});
	QMutexLocker locker(&m_mutex);
	m_result = calculated;
}
