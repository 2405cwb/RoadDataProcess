#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QVector>
#include "test_isMergeableDisease.h"
#include "test_mergeTwoDiseases.h"
#include "test_VMirror2dRectI.h"
#include "test_LittleFrameRenderPathBuilder.h"
#include "test_DrPciCalculator.h"
#include <QtTest/QTest>

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	QVector<QObject*>  testObjects;

	test_isMergeableDisease obj_test_isMergeableDisease;
	testObjects.push_back(&obj_test_isMergeableDisease);

	test_mergeTwoDiseases obj_test_mergeTwoDiseases;
	testObjects.push_back(&obj_test_mergeTwoDiseases);

	test_VMirror2dRectI obj_test_VMirror2dRectI;
	testObjects.push_back(&obj_test_VMirror2dRectI);

	test_LittleFrameRenderPathBuilder obj_test_LittleFrameRenderPathBuilder;
	testObjects.push_back(&obj_test_LittleFrameRenderPathBuilder);

	test_DrPciCalculator obj_test_DrPciCalculator;
	testObjects.push_back(&obj_test_DrPciCalculator);

	int status = 0;
	for (auto object : qAsConst(testObjects))
	{
		status |= QTest::qExec(object, argc, argv);

		qDebug() << "";
	}

	return status;
}
