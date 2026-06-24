#include <QtCore/QCoreApplication>
#include "test_isMergeableDisease.h"
#include "test_mergeTwoDiseases.h"
#include "test_VMirror2dRectI.h"
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


	for (auto object : qAsConst(testObjects))
	{
		QTest::qExec(object, argc, argv);

		qDebug() << "";
	}

	return a.exec();
}
