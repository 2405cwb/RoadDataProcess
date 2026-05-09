#pragma once

#include <QObject>
#include "../hnCommon/hnRect.h"

class VMirror2dRectI : public QObject
{
	Q_OBJECT

public:
	VMirror2dRectI(QObject *parent = nullptr);
	~VMirror2dRectI();

public:
	const hnCommon::hn2dPointWithMileI VMirrorPoint(const hnCommon::hn2dPointWithMileI &point ,int pixHeight);

	const hnCommon::hn2dRectI VMirrorRect(const hnCommon::hn2dRectI& rect, int pixHeight);

	
};
