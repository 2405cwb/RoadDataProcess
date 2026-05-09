#include "VMirror2dRectI.h"

VMirror2dRectI::VMirror2dRectI(QObject *parent)
	: QObject(parent)
{
}

VMirror2dRectI::~VMirror2dRectI()
{
}

const hnCommon::hn2dPointWithMileI VMirror2dRectI::VMirrorPoint(const hnCommon::hn2dPointWithMileI & point, int pixHeight)
{
	hnCommon::hn2dPointWithMileI result;

	result = point;
	result.y = pixHeight - result.y;

	return result;
}

const hnCommon::hn2dRectI VMirror2dRectI::VMirrorRect(const hnCommon::hn2dRectI & rect, int pixHeight)
{
	hnCommon::hn2dRectI result = rect;

	result.p0 = this->VMirrorPoint(result.p0, pixHeight);
	result.p1 = this->VMirrorPoint(result.p1, pixHeight);
	result.p2 = this->VMirrorPoint(result.p2, pixHeight);
	result.p3 = this->VMirrorPoint(result.p3, pixHeight);

	return result;
}
