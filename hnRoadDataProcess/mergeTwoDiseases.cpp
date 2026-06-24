#include "mergeTwoDiseases.h"
#include <QMap>
#include "VMirror2dRectI.h"

mergeTwoDiseases::mergeTwoDiseases(QObject *parent)
	: QObject(parent)
{
}

mergeTwoDiseases::~mergeTwoDiseases()
{
}


//合并两个病害
hnCommon::hnRoadDiseaseInfo mergeTwoDiseases::merge(const hnCommon::hnRoadDiseaseInfo & up, const hnCommon::hnRoadDiseaseInfo & down, 
	double roadWidth, int pixHeight ,bool isVMirror)
{
	hnCommon::hnRoadDiseaseInfo result = up;

	//处理坐标
	// 1.double 里程，米 2. int 图片上的y坐标  3. double  图片底部里程，米
	QMap<double, QPair<int,double>> yMileMap;	//这里的整体里程顺序和目标y的顺序是相反的
	QMap<int, double> xMileMap;

	if (up.vec2dRect.empty() || down.vec2dRect.empty())
	{
		return result;
	}

	VMirror2dRectI vmirror;
	auto upRect = up.vec2dRect.at(0);
	if (isVMirror)
	{
		upRect = vmirror.VMirrorRect(upRect, pixHeight);
	}
	yMileMap.insert((upRect.p0.m_dmi / roadWidth) * pixHeight + (pixHeight - upRect.p0.y), QPair<int, double>(upRect.p0.y, upRect.p0.m_dmi));
	yMileMap.insert((upRect.p1.m_dmi / roadWidth) * pixHeight + (pixHeight - upRect.p1.y), QPair<int, double>(upRect.p1.y, upRect.p1.m_dmi));
	yMileMap.insert((upRect.p2.m_dmi / roadWidth) * pixHeight + (pixHeight - upRect.p2.y), QPair<int, double>(upRect.p2.y, upRect.p2.m_dmi));
	yMileMap.insert((upRect.p3.m_dmi / roadWidth) * pixHeight + (pixHeight - upRect.p3.y), QPair<int, double>(upRect.p3.y, upRect.p3.m_dmi));

	double pointEncoderMile;
	pointEncoderMile = (upRect.p0.m_dmi / roadWidth) * pixHeight + (pixHeight - upRect.p0.y);

	auto downRect = down.vec2dRect.at(0);
	if (isVMirror)
	{
		downRect = vmirror.VMirrorRect(downRect, pixHeight);
	}
	yMileMap.insert((downRect.p0.m_dmi / roadWidth) * pixHeight + (pixHeight - downRect.p0.y), QPair<int, double>(downRect.p0.y, downRect.p0.m_dmi));
	yMileMap.insert((downRect.p1.m_dmi / roadWidth) * pixHeight + (pixHeight - downRect.p1.y), QPair<int, double>(downRect.p1.y, downRect.p1.m_dmi));
	yMileMap.insert((downRect.p2.m_dmi / roadWidth) * pixHeight + (pixHeight - downRect.p2.y), QPair<int, double>(downRect.p2.y, downRect.p2.m_dmi));
	yMileMap.insert((downRect.p3.m_dmi / roadWidth) * pixHeight + (pixHeight - downRect.p3.y), QPair<int, double>(downRect.p3.y, downRect.p3.m_dmi));

	xMileMap.insert(upRect.p0.x, upRect.p0.m_dmi);
	xMileMap.insert(upRect.p1.x, upRect.p1.m_dmi);
	xMileMap.insert(upRect.p2.x, upRect.p2.m_dmi);
	xMileMap.insert(upRect.p3.x, upRect.p3.m_dmi);

	xMileMap.insert(downRect.p0.x, downRect.p0.m_dmi);
	xMileMap.insert(downRect.p1.x, downRect.p1.m_dmi);
	xMileMap.insert(downRect.p2.x, downRect.p2.m_dmi);
	xMileMap.insert(downRect.p3.x, downRect.p3.m_dmi);

	// 左上角
	result.vec2dRect.at(0).p0.x = xMileMap.firstKey();
	result.vec2dRect.at(0).p0.y = yMileMap.last().first;
	result.vec2dRect.at(0).p0.m_dmi = yMileMap.last().second;

	// 右上角
	result.vec2dRect.at(0).p1.x = xMileMap.lastKey();
	result.vec2dRect.at(0).p1.y = yMileMap.last().first;
	result.vec2dRect.at(0).p1.m_dmi = yMileMap.last().second;

	// 右下角
	result.vec2dRect.at(0).p2.x = xMileMap.lastKey();
	result.vec2dRect.at(0).p2.y = yMileMap.first().first;
	result.vec2dRect.at(0).p2.m_dmi = yMileMap.first().second;

	// 左下角
	result.vec2dRect.at(0).p3.x = xMileMap.firstKey();
	result.vec2dRect.at(0).p3.y = yMileMap.first().first;
	result.vec2dRect.at(0).p3.m_dmi = yMileMap.first().second;

	const double beginMile = down.dDmiStart;
	const double endMile = up.dDmiEnd;
	const double centerMile = 0.5 * (beginMile + endMile);

	result.dDmiStart = beginMile;
	result.dDmiEnd = endMile;
	result.dMileage = centerMile;

	if (isVMirror)
	{
		result.vec2dRect.at(0) = vmirror.VMirrorRect(result.vec2dRect.at(0),pixHeight);
	}

	return result;
}





