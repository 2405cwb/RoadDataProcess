#pragma once

#include <QObject>

class test_isMergeableDisease : public QObject
{
	Q_OBJECT

public:
	test_isMergeableDisease(QObject *parent = nullptr);
	~test_isMergeableDisease();

private slots:
	void noVMirror_isMergeableDisease();

	void VMirror_isMergeableDisease();
};
