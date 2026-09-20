#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QVector>
#include "test_isMergeableDisease.h"
#include "test_mergeTwoDiseases.h"
#include "test_VMirror2dRectI.h"
#include "test_LittleFrameRenderPathBuilder.h"
#include "test_DrPciCalculator.h"
#include "test_LineCameraConfig.h"
#include "test_VirtualImageSequence.h"
#include "test_GeometryCalculation.h"
#include "test_RutProfileTrace.h"
#include "test_RutProfileDebugExporter.h"
#include <QtTest/QTest>

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	const QString classFilter = QString::fromLocal8Bit(qgetenv("HN_UNIT_TEST_CLASS"));
	auto classEnabled = [&classFilter](const QString& className)
	{
		return classFilter.isEmpty() || classFilter == className;
	};

	QVector<QObject*>  testObjects;

	test_isMergeableDisease obj_test_isMergeableDisease;
	if (classEnabled(QStringLiteral("test_isMergeableDisease"))) testObjects.push_back(&obj_test_isMergeableDisease);

	test_mergeTwoDiseases obj_test_mergeTwoDiseases;
	if (classEnabled(QStringLiteral("test_mergeTwoDiseases"))) testObjects.push_back(&obj_test_mergeTwoDiseases);

	test_VMirror2dRectI obj_test_VMirror2dRectI;
	if (classEnabled(QStringLiteral("test_VMirror2dRectI"))) testObjects.push_back(&obj_test_VMirror2dRectI);

	test_LittleFrameRenderPathBuilder obj_test_LittleFrameRenderPathBuilder;
	if (classEnabled(QStringLiteral("test_LittleFrameRenderPathBuilder"))) testObjects.push_back(&obj_test_LittleFrameRenderPathBuilder);

	test_DrPciCalculator obj_test_DrPciCalculator;
	if (classEnabled(QStringLiteral("test_DrPciCalculator"))) testObjects.push_back(&obj_test_DrPciCalculator);

	test_LineCameraConfig obj_test_LineCameraConfig;
	if (classEnabled(QStringLiteral("test_LineCameraConfig"))) testObjects.push_back(&obj_test_LineCameraConfig);

	test_VirtualImageSequence obj_test_VirtualImageSequence;
	if (classEnabled(QStringLiteral("test_VirtualImageSequence"))) testObjects.push_back(&obj_test_VirtualImageSequence);

	test_GeometryCalculation obj_test_GeometryCalculation;
	if (classEnabled(QStringLiteral("test_GeometryCalculation"))) testObjects.push_back(&obj_test_GeometryCalculation);

	test_RutProfileTrace obj_test_RutProfileTrace;
	if (classEnabled(QStringLiteral("test_RutProfileTrace"))) testObjects.push_back(&obj_test_RutProfileTrace);

	test_RutProfileDebugExporter obj_test_RutProfileDebugExporter;
	if (classEnabled(QStringLiteral("test_RutProfileDebugExporter"))) testObjects.push_back(&obj_test_RutProfileDebugExporter);

	int status = 0;
	for (auto object : qAsConst(testObjects))
	{
		status |= QTest::qExec(object, argc, argv);

		qDebug() << "";
	}

	return status;
}
