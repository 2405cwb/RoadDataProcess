#include "lineAlgorithm.h"



lineAlgorithm::lineAlgorithm()
{
}


lineAlgorithm::~lineAlgorithm()
{
}

double lineAlgorithm::distanceFromPointToLine(const QPointF & point, const QLineF & line)
{
	double result;
	double x1 = line.p1().x();
	double y1 = line.p1().y();
	double x2 = line.p2().x();
	double y2 = line.p2().y();
	double x0 = point.x();
	double y0 = point.y();

	double numerator = std::abs((x2 - x1)*(y1 - y0) - (x1 - x0)*(y2 - y1));
	double denominator = std::sqrt((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1));

	result = numerator / denominator;
	return  result;
}
