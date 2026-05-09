#pragma once

#include <QObject>

class test_VMirror2dRectI : public QObject
{
	Q_OBJECT

public:
	test_VMirror2dRectI(QObject *parent = nullptr);
	~test_VMirror2dRectI();

private slots:
	void VMirrorPoint();

	void VMirrorRect();
	
};
