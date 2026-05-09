#pragma once

#include <QObject>


class test_mergeTwoDiseases : public QObject
{
	Q_OBJECT

public:
	test_mergeTwoDiseases(QObject *parent = nullptr);
	~test_mergeTwoDiseases();

private slots:
	void noVMirror_merge();

	void VMirror_merge();
};
