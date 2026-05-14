#pragma once
#include "hnapplication_global.h"
#include <QString>
#include <QPoint>
#include <QRect>
#include <QVector>
#include <QImage>
#include <QPainter>
#include "hnMile.h"
#include "../hnCommon/hnRoadStruct.h"
#include <vector>
#include "pixImagePoint.h" 
using namespace hnCommon;
using namespace std;


class HNAPPLICATION_EXPORT drawDiseases
{
public:
	drawDiseases();
	~drawDiseases();

public:
	// 获取用户点击的编码器里程，用于二三维里程矫正
	double getEncoderMile();


protected:
	//人工模式添加病害
	virtual void bigFrameAddDisease(const QPoint &mousePoint) = 0;

protected:
	// 计算自动化模式病害的大image数组
	virtual QVector<QRect> caculateLittleFrameBigImageRects(const hnRoadDiseaseInfo& disease) = 0;

protected:
	//绘制数据库加载内容
	virtual void drawDatabaseLoadData(QImage &image) = 0;
	//绘制临时内容
	virtual void drawTmpData(QImage &image) = 0;

protected:
	//往图片上画矩形数组里面的所有矩形
	void drawRectsOnImage(QImage &image, const QVector<QRect> rects,int boarderWidth, const QColor &rectColor,Qt::PenStyle style);

protected:
	// 获取某个点的编码器里程
	virtual double calculateEncoderMile(const QPoint &bigImagePoint) = 0;

	// 获取某个点的绝对里程
	virtual double caculateTrueMile(const QPoint &bigImagePoint) = 0;

protected:
	//计算人工模式的开始里程（编码器里程）
	virtual double calculateBigFrameBeginMile(const QRect &rect) = 0;

	//计算人工模式的结束里程（编码器里程）
	virtual double calculateBigFrameEndMile(const QRect &rect) = 0;

	//计算自动化模式的开始里程（编码器里程）
	virtual double calculateLittleFrameBeginMile(const QVector<QRect> rects) = 0;

	//计算自动化模式的结束里程（编码器里程）
	virtual double calculateLittleFrameEndMile(const QVector<QRect> rects) = 0;



public:
	//创造人工模式3d数组
	virtual vector<hn3dRectI> generateLargeFrameHn3dRectVector(const QRect &rect) = 0;

protected:
	//病害第一个点的hnMile
	hnMile m_firstHnMile;

	//当前hnMile的数组
	QVector<hnMile> m_hnMileVector;

	//当前视图的病害
	std::vector<hnRoadDiseaseInfo> m_currentWidgetDiseases;

protected:
	//病害起始点
	pixImagePoint m_diseaseStartPoint;

	//病害终止点
	pixImagePoint m_diseaseEndPoint;

	//点击左键添加病害点
	pixImagePoint m_diseaseAddPoint;

	//是否正在画临时病害
	bool m_isDrawingDisease;

protected:
	//当前临时自动化模式病害的矩形数组 这里的坐标系是针对大image的
	QVector<QRect> m_currentLittleFrameRects;

	//单张图片的自动化模式数组 这里的自动化模式是针对小的image的
	QVector<QRect> m_singleImageLittleFrameRects;

	//临时病害的自动化模式数组 这里的自动化模式是针对大的image的
	QVector<QRect> m_tmpLittleFrameDiseaseRects;

	//画临时自动化模式时的折线的点的集合,这个是当前视图拼接image上面的
	QVector<QPoint> m_litteBigImagePoints;

	//画临时自动化模式时单张图片上的点
	QVector<pixImagePoint> m_littleSingleImagePoints;

	//编码器里程，用于二三维里程差值矫正
	double m_encoderMile;

protected:
	// 判断一个病害是不是选中的合并病害
	bool isSeclectedMergeDisease(const hnRoadDiseaseInfo &disease);

protected:
	//选中的病害数组，用户合并病害
	QVector<hnRoadDiseaseInfo> m_seclectedDiseases;

public:
	//清理二三维矫正选中的点
	void claerSelectPoint();
protected:
	//绘制二三维矫正选中的点
	void drawLineOnImage(const QLine &line, int lineWidth,const QColor &color,QImage &image);
	//二三维矫正选中的点
	pixImagePoint m_seclectPoint;
};

