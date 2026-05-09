#pragma once

#include "hnapplication_global.h"
#include <QLineF>
#include <QPointF>


class HNAPPLICATION_EXPORT lineAlgorithm
{
public:
	lineAlgorithm();
	~lineAlgorithm();

public:
	//点到线的最小距离
	double distanceFromPointToLine(const QPointF& point, const QLineF& line);
};

