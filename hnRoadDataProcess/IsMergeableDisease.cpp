#include "IsMergeableDisease.h"
#include"QPoint"
#include"qmath.h"
#include"QVector"
#include"QRect"



IsMergeableDisease::IsMergeableDisease(QObject *parent)
	: QObject(parent)
{
}

IsMergeableDisease::~IsMergeableDisease()
{
}

bool IsMergeableDisease::IsMergeable(const hnCommon::hnRoadDiseaseInfo & diseaseDown, const hnCommon::hnRoadDiseaseInfo & diseaseUp,
	bool isVMirror, int pixHeight, double roadWidth, int &minIndexUp, int &minIndexDown,int gYThreshold,int gLeftRightpixelThreshold)
{
	const double YThreshold = gYThreshold;					// 单位：像素
	const double leftXpixelThreshold = gLeftRightpixelThreshold;			// 单位：像素
	const double rightXpixelThreshold = gLeftRightpixelThreshold;			// 单位：像素
	
	const double mileThreshold = roadWidth;

	bool isMileCorrent = false;
	bool isYPixelCorrect = false;
	bool isLeftXpixelCorrect = false;
	bool isRightXpixelCorrect = false;

	// 确定是否是相邻图像
	isMileCorrent = (qAbs(qAbs(diseaseUp.dDmi - diseaseDown.dDmi) - mileThreshold) < 0.01);
	if (!isMileCorrent)
	{
		return false;			// 图像桩号不满足要求，直接返回，减少无效运算
	}


#if  0
	if (isVMirror)
	{
		for (int i = 0; i < diseaseDown.vec2dRect.size(); i++)
		{
			hnCommon::hn2dRectI *rectI = (hnCommon::hn2dRectI *)(&diseaseDown.vec2dRect[i]);
			// 自动化模式的y坐标进行变换，里程等信息不变
			rectI->p0.y = pixHeight - rectI->p0.y;
			rectI->p1.y = pixHeight - rectI->p1.y;
			rectI->p2.y = pixHeight - rectI->p2.y;
			rectI->p3.y = pixHeight - rectI->p3.y;
		}
		for (int i = 0; i < diseaseUp.vec2dRect.size(); i++)
		{
			hnCommon::hn2dRectI *rectI = (hnCommon::hn2dRectI *)(&diseaseUp.vec2dRect[i]);
			// 自动化模式的y坐标进行变换，里程等信息不变
			rectI->p0.y = pixHeight - rectI->p0.y;
			rectI->p1.y = pixHeight - rectI->p1.y;
			rectI->p2.y = pixHeight - rectI->p2.y;
			rectI->p3.y = pixHeight - rectI->p3.y;
		}
	}
#endif

	// 寻找两个病害中相距最近的点以及距离值
	double minDis = verticalMinDistance(diseaseDown, diseaseUp, pixHeight, minIndexUp, minIndexDown);
	// 不能确定画病害时是从上到下还是从下到上，所以直接使用第一个点相减有些问题
	// double minDis = pixHeight - diseaseUp.vec2dRect.at(0).p3.y + diseaseDown.vec2dRect.at(0).p0.y;
	isYPixelCorrect = qAbs(minDis) < YThreshold;


#if 0
	if (qAbs(diseaseDown.vec2dRect.at(0).p0.x - diseaseUp.vec2dRect.at(0).p0.x) < leftXpixelThreshold)
	{
		isLeftXpixelCorrect = true;
	}

	if (qAbs(diseaseDown.vec2dRect.at(0).p1.x - diseaseUp.vec2dRect.at(0).p1.x) < leftXpixelThreshold)
	{
		isRightXpixelCorrect = true;
	}
#else
	// x 方向最小距离
	int xDis = qAbs(diseaseDown.vec2dRect[minIndexDown].p0.x - diseaseUp.vec2dRect[minIndexUp].p0.x);
	if (xDis < leftXpixelThreshold)
	{
		isLeftXpixelCorrect = true;
		isRightXpixelCorrect = true;
	}

#endif

	if (isMileCorrent && isYPixelCorrect && isLeftXpixelCorrect && isRightXpixelCorrect)
	{
		return true;
	}
	else
	{
		return false;
	}
}



bool IsMergeableDisease::BigFrameIsMergeable(const hnCommon::hnRoadDiseaseInfo & diseaseDown, const hnCommon::hnRoadDiseaseInfo & diseaseUp,
	 int pixHeight, double imageInterval, int gYThreshold, int gLeftRightpixelThreshold)
{
	if (diseaseDown.vec2dRect.size() <= 0 || diseaseUp.vec2dRect.size() <= 0)
	{
		return false;
	}

	const double YThreshold = gYThreshold;					// 单位：像素
	const double leftXpixelThreshold = gLeftRightpixelThreshold;			// 单位：像素
	const double rightXpixelThreshold = gLeftRightpixelThreshold;			// 单位：像素


	bool isMileCorrent = false;
	bool isYPixelCorrect = false;
	bool isXpixelCorrect = false;

	// 确定是否是相邻图像
	isMileCorrent = imageInterval > 0.0 &&
		(qAbs(qAbs(diseaseUp.dDmi - diseaseDown.dDmi) - imageInterval) < 0.01);
	if (!isMileCorrent)
	{
		return false;			// 图像桩号不满足要求，直接返回，减少无效运算
	}

	// 两个矩形在大图像上的 y 坐标差值
	int DiffBigImage_y = diseaseDown.vec2dRect[0].p0.y + pixHeight - diseaseUp.vec2dRect[0].p3.y;
	if (DiffBigImage_y < YThreshold)
	{
		isYPixelCorrect = true;
	}

	// 病害矩形框 x 坐标值
	int diseaseUpX_left = diseaseUp.vec2dRect[0].p0.x;
	int diseaseUpX_right = diseaseUp.vec2dRect[0].p1.x;
	int diseaseDownX_left = diseaseDown.vec2dRect[0].p0.x;
	int diseaseDownX_right = diseaseDown.vec2dRect[0].p1.x;
	
	// 病害在 x 方向是否满足
	// 两个矩形错开，且间距小于阈值
	if (diseaseUpX_left > diseaseDownX_right&&diseaseUpX_left - diseaseDownX_right < gLeftRightpixelThreshold)
	{
		isXpixelCorrect = true;
	}
	if (diseaseUpX_right < diseaseDownX_left&&diseaseDownX_left - diseaseUpX_right < gLeftRightpixelThreshold)
	{
		isXpixelCorrect = true;
	}
	// 两个矩形没有错开
	if (diseaseUpX_left >= diseaseDownX_left&&diseaseUpX_left <= diseaseDownX_right)
	{
		isXpixelCorrect = true;
	}
	if (diseaseUpX_right >= diseaseDownX_left&&diseaseUpX_right <= diseaseDownX_right)
	{
		isXpixelCorrect = true;
	}

	if (isMileCorrent && isYPixelCorrect && isXpixelCorrect)
	{
		return true;
	}
	else
	{
		return false;
	}
}



// 计算两个自动化模式病害在垂直方向的最小距离
double IsMergeableDisease::verticalMinDistance(const hnCommon::hnRoadDiseaseInfo & diseaseDown, const hnCommon::hnRoadDiseaseInfo & diseaseUp, int pixHeight, int &minIndexUp, int &minIndexDown)
{
	vector<hnCommon::hn2dRectI>  vec2dRectDown = diseaseDown.vec2dRect;
	vector<hnCommon::hn2dRectI>  vec2dRectUp = diseaseUp.vec2dRect;
	double minDis = 1e+6;
	//int minIndexUp = 0, minIndexDown = 0;			// 记录最近点索引
	for (int i = 0; i < vec2dRectDown.size(); i++)
	{
		for (int j = 0; j < vec2dRectUp.size(); j++)
		{
			// 使用矩形中心点计算距离,下方图像加上一个图像高度
			QPoint t_DownCenter((vec2dRectDown[i].p0.x + vec2dRectDown[i].p3.x) / 2, pixHeight + (vec2dRectDown[i].p0.y + vec2dRectDown[i].p3.y) / 2);
			QPoint t_UpCenter((vec2dRectUp[j].p0.x + vec2dRectUp[j].p3.x) / 2, (vec2dRectUp[j].p0.y + vec2dRectUp[j].p3.y) / 2);
			QPoint dv = t_DownCenter - t_UpCenter;
			double tDis = qSqrt(dv.x()*dv.x() + dv.y()*dv.y());

			if (tDis < minDis)
			{
				minDis = tDis;			// 计算最小距离和对应的点
				minIndexUp = j;
				minIndexDown = i;
			}
		}
	}
	return minDis;

}



// 根据最近点，合并两个病害
hnCommon::hnRoadDiseaseInfo IsMergeableDisease::pureMerge(const hnCommon::hnRoadDiseaseInfo & up, const hnCommon::hnRoadDiseaseInfo & down, int minIndexUp, int minIndexDown, int pixHeight)
{
	QVector<QRect> UpRect;
	QVector<QRect> DownRect;

	// 单张图像自动化模式集合
	//QVector<QRect> RectSet = createSingleImageLittleFrameRect();

	return up;

}






