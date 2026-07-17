#include "test_GeometryCalculation.h"
#include "../hnRoadDataProcess/hnGeometryCalculation.h"
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QtTest/QTest>
#include <cmath>

void test_GeometryCalculation::fitsSignedSlopesDeterministically()
{
	QVector<hnPoint3d> points;
	for (int i = 0; i < 100; ++i)
	{
		hnPoint3d point;
		point.x = i * 0.05; point.y = 0.0; point.z = -0.02 * point.x + 1.5;
		if (i == 10 || i == 70) point.z += 2.0;
		points.append(point);
	}
	double firstSlope = 0.0, firstIntercept = 0.0, rmse = 0.0, ratio = 0.0;
	QVERIFY(hnGeometryAlgorithms::fitHuber(points, firstSlope, firstIntercept, rmse, ratio));
	QVERIFY(qAbs(firstSlope + 0.02) < 1e-6);
	QVERIFY(ratio > 0.9);
	double secondSlope = 0.0, secondIntercept = 0.0;
	QVERIFY(hnGeometryAlgorithms::fitHuber(points, secondSlope, secondIntercept, rmse, ratio));
	QCOMPARE(firstSlope, secondSlope);
	QCOMPARE(firstIntercept, secondIntercept);

	QVector<hnPoint3d> horizontal;
	for (int i = 0; i < 30; ++i)
	{
		hnPoint3d point; point.x = i; point.y = 0.0; point.z = 3.0; horizontal.append(point);
	}
	QVERIFY(hnGeometryAlgorithms::fitHuber(horizontal, firstSlope, firstIntercept, rmse, ratio));
	QVERIFY(qAbs(firstSlope) < 1e-12);
}

void test_GeometryCalculation::aggregatesOneMeterSamplesIntoTenMeters()
{
	std::vector<hnRoadGeoParam> raw(100);
	for (int i = 0; i < 100; ++i)
	{
		raw[i].dMileage = i; raw[i].dC = i; raw[i].dVAngle = 0.01; raw[i].dHAngle = -0.02;
		raw[i].bCurvatureValid = raw[i].bVAngleValid = raw[i].bHAngleValid = true;
	}
	std::vector<hnRoadGeoParam> output;
	QString error;
	QVERIFY2(hnGeometryAlgorithms::aggregate(raw, 10.0, output, &error), qPrintable(error));
	QCOMPARE(static_cast<int>(output.size()), 10);
	for (int i = 0; i < 10; ++i) QCOMPARE(output[i].dMileage, i * 10.0);
	QCOMPARE(output[0].dC, 4.5);
	QCOMPARE(output[9].dC, 94.5);
}

void test_GeometryCalculation::unwrapsHeadingAndCalculatesCurvature()
{
	std::vector<hnRoadGeoParam> samples(30);
	for (int i = 0; i < 30; ++i)
	{
		samples[i].dMileage = i;
		double yaw = 178.0 + i * 0.5;
		while (yaw > 180.0) yaw -= 360.0;
		samples[i].dYaw = yaw; samples[i].bCurvatureValid = true;
	}
	hnGeometryAlgorithms::calculateCurvature(samples);
	const double expected = 0.5 * 3.14159265358979323846 / 180.0;
	QVERIFY(samples[15].bCurvatureValid);
	QVERIFY(qAbs(samples[15].dC - expected) < 1e-8);
}

void test_GeometryCalculation::writesLegacyAndQualityFilesAtomically()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	std::vector<hnRoadGeoParam> samples(2);
	for (int i = 0; i < 2; ++i)
	{
		samples[i].dMileage = i * 10.0;
		samples[i].bCurvatureValid = samples[i].bVAngleValid = samples[i].bHAngleValid = true;
	}
	const QString path = QDir(directory.path()).filePath(QStringLiteral("Geoalig_10m.txt"));
	QString error;
	QVERIFY2(hnGeometryCalculator::writeResults(path, samples, &error), qPrintable(error));
	QFile legacy(path);
	QVERIFY(legacy.open(QIODevice::ReadOnly));
	QCOMPARE(legacy.readAll().count('\n'), 2);
	QVERIFY(QFile::exists(QDir(directory.path()).filePath(QStringLiteral("Geoalig_10m.quality.csv"))));
}

void test_GeometryCalculation::integratesRealProjectWhenConfigured()
{
	const QString root = QString::fromLocal8Bit(qgetenv("HN_GEOMETRY_SAMPLE_ROOT"));
	if (root.isEmpty())
		QSKIP("HN_GEOMETRY_SAMPLE_ROOT is not configured");

	const QString project3d = QDir(root).filePath(QStringLiteral("MmsRSening-00000000-20231122100228"));
	GeometryCalculationInput input;
	input.posFiles << QDir(project3d).filePath(QStringLiteral("POS/IE/1.pos"));
	input.camPath = QDir(project3d).filePath(QStringLiteral("PointCloud/1/Mms-Cam-1.cam"));
	input.scanParameterPath = QDir(project3d).filePath(QStringLiteral("Mms-Para.db"));
	input.projectLength = qgetenv("HN_GEOMETRY_SAMPLE_LENGTH").toDouble();
	QVERIFY(input.projectLength > 0.0);

	QTemporaryDir outputDirectory;
	QVERIFY(outputDirectory.isValid());
	input.resultPath = QDir(outputDirectory.path()).filePath(QStringLiteral("Geoalig_10m.txt"));
	std::atomic_bool cancelled(false);
	int lastProgress = -1;
	bool progressMonotonic = true;
	QElapsedTimer timer;
	timer.start();
	const GeometryCalculationResult result = hnGeometryCalculator().calculate(input, &cancelled,
		[&lastProgress, &progressMonotonic](int progress, const QString&) {
			progressMonotonic = progressMonotonic && progress >= lastProgress;
			lastProgress = progress;
			return true;
		});
	qInfo("real geometry calculation: status=%d scanLength=%.3f rows=%d elapsedMs=%lld",
		static_cast<int>(result.status), result.scanLength, static_cast<int>(result.outputSamples.size()), timer.elapsed());
	QVERIFY2(result.status == GeometryCalculationStatus::Success, qPrintable(result.errorMessage));
	QVERIFY(progressMonotonic);
	QVERIFY(lastProgress == 100);
	QVERIFY(QFile::exists(input.resultPath));
	QVERIFY(QFile::exists(QDir(outputDirectory.path()).filePath(QStringLiteral("Geoalig_10m.quality.csv"))));
	QVERIFY(!result.outputSamples.empty());
	QVERIFY(result.outputSamples.back().dMileage + 10.0 >= input.projectLength);
}
