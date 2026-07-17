#include "test_LineCameraConfig.h"

#include "../hnProject/hnLineCameraConfig.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTextStream>
#include <QtTest/QTest>

namespace
{
	QString createLineCameraProject(QTemporaryDir& temporaryDir, const QString& width,
		const QString& height, const QString& diff, bool createCamera1 = false)
	{
		const QString root = temporaryDir.path();
		QDir().mkpath(QDir(root).filePath(QStringLiteral("RoadImg/Camera0/Image_0000")));
		QFile config(hnPro::hnLineCameraConfig::cameraModuleConfigPath(root, 0));
		if (config.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream stream(&config);
			stream.setCodec("UTF-8");
			stream << "[Cam]\n"
				<< "MmPerPix_W=" << width << ";横向mm/pix\n"
				<< "MmPerPix_H=" << height << QStringLiteral("；纵向mm/pix\n")
				<< "DiffValue=" << diff << QStringLiteral("；左右偏移差 单模块=0\n");
		}
		if (createCamera1)
		{
			QDir().mkpath(QDir(root).filePath(QStringLiteral("RoadImg/Camera1")));
			QFile camera1Config(hnPro::hnLineCameraConfig::cameraModuleConfigPath(root, 1));
			if (camera1Config.open(QIODevice::WriteOnly | QIODevice::Text))
			{
				QTextStream stream(&camera1Config);
				stream.setCodec("UTF-8");
				stream << "[Cam]\nMmPerPix_W=9.9\nMmPerPix_H=8.8\nDiffValue=123.4\n";
			}
		}
		QImage image(4096, 20, QImage::Format_RGB32);
		image.fill(Qt::black);
		image.save(QDir(root).filePath(QStringLiteral("RoadImg/Camera0/Image_0000/000001.png")));
		return root;
	}
}

void test_LineCameraConfig::readsConfigAndCalculatesWidths()
{
	QTemporaryDir temporaryDir;
	QVERIFY(temporaryDir.isValid());
	const QString root = createLineCameraProject(temporaryDir, QStringLiteral("0.1"), QStringLiteral("1"), QStringLiteral("0"));

	hnPro::hnLineCameraInfo info = hnPro::hnLineCameraConfig::load(root);
	QVERIFY(info.isLineCamera);
	QVERIFY2(info.cameraConfigValid, qPrintable(info.errorMessage));
	QVERIFY(!info.validAreaConfigured);
	QCOMPARE(info.imageWidth, 4096);
	QCOMPARE(info.imageHeight, 20);

	info.leftPixel = 0;
	info.rightPixel = 4096;
	info.validAreaConfigured = true;
	QVERIFY(qAbs(info.roadWidthMeters() - 0.4096) < 1e-9);
	QString errorMessage;
	QVERIFY2(hnPro::hnLineCameraConfig::saveValidArea(info, &errorMessage), qPrintable(errorMessage));

	info = hnPro::hnLineCameraConfig::load(root);
	QVERIFY(info.validAreaConfigured);
	info.leftPixel = 200;
	info.rightPixel = 3800;
	QVERIFY(qAbs(info.roadWidthMeters() - 0.36) < 1e-9);
	QVERIFY(qAbs(info.roadXFromPixel(200) - 0.0) < 1e-9);
	QVERIFY(qAbs(info.distanceFromRight(200) - 0.36) < 1e-9);
	QVERIFY(qAbs(info.imageLengthMeters() - 0.02) < 1e-9);
}

void test_LineCameraConfig::rejectsInvalidScaleAndKeepsCamera1AsInterface()
{
	QTemporaryDir invalidScaleDir;
	hnPro::hnLineCameraInfo invalidScale = hnPro::hnLineCameraConfig::load(
		createLineCameraProject(invalidScaleDir, QStringLiteral("0"), QStringLiteral("1"), QStringLiteral("0")));
	QVERIFY(invalidScale.isLineCamera);
	QVERIFY(!invalidScale.cameraConfigValid);
	QVERIFY(invalidScale.errorMessage.contains(QStringLiteral("MmPerPix_W")));

	QTemporaryDir dualModuleDir;
	hnPro::hnLineCameraInfo dualModule = hnPro::hnLineCameraConfig::load(
		createLineCameraProject(dualModuleDir, QStringLiteral("0.53"), QStringLiteral("1"), QStringLiteral("10"), true));
	QVERIFY(dualModule.isLineCamera);
	QVERIFY2(dualModule.cameraConfigValid, qPrintable(dualModule.errorMessage));
	QVERIFY(dualModule.camera0.configValid);
	QVERIFY(dualModule.camera1.configExists);
	QVERIFY(dualModule.camera1.configValid);
	QVERIFY(dualModule.hasSecondaryCamera());
	QCOMPARE(dualModule.mmPerPixelWidth, 0.53);
	QCOMPARE(dualModule.mmPerPixelHeight, 1.0);
	QCOMPARE(dualModule.diffValueMm, 10.0);
}

void test_LineCameraConfig::calculatesCenteredBoundsFromRoadWidth()
{
	hnPro::hnLineCameraInfo info;
	info.imageWidth = 4096;
	info.mmPerPixelWidth = 0.53;
	int left = -1;
	int right = -1;
	QVERIFY(info.centeredBoundsForRoadWidth(1.908, left, right));
	QCOMPARE(left, 248);
	QCOMPARE(right, 3848);
	QVERIFY(info.centeredBoundsForRoadWidth(2.17088, left, right));
	QCOMPARE(left, 0);
	QCOMPARE(right, 4096);
	QVERIFY(!info.centeredBoundsForRoadWidth(3.75, left, right));
	QVERIFY(!info.centeredBoundsForRoadWidth(0.0001, left, right));
}
