#pragma once
#include "hnDataManager.h"	//这个要放最前面，不然容易出问题
#include <QWidget>
#include "hnView.h"
#include "hnapplication_global.h"
#include "hnMile.h"
#include "../hnDataTable/hnRoadDiseaseTable.h"
#include "../hnCommon/hnRoadStruct.h"
#include <QLine>
#include <QLineF>
#include "hn2d3dCoordinates.h"
#include "hn2d3dPixBaseWidget.h"
#include "../HighAccConvertPlane/HighAccuracyPositioning.h"
 using namespace hnApp;

//作者 陈智超 
//(程文博 20240325)
class HNAPPLICATION_EXPORT hn2dPixWidget final: public hn2d3dPixBaseWidget
{
	Q_OBJECT

public:
	hn2dPixWidget(QWidget *parent = Q_NULLPTR);
	~hn2dPixWidget();

public:
	//加载路面影像
	void loadRoadPicture();

protected:
	//重载函数，绘图，往窗口上画东西
	void drawSomeThingOnImage(QImage &image) override;

protected:
	//鼠标点击事件
	void mousePressEvent(QMouseEvent *event) override;

	//鼠标双击事件
	void mouseDoubleClickEvent(QMouseEvent *event) override;

	// 鼠标释放事件
	void mouseReleaseEvent(QMouseEvent *event) override;

	//鼠标移动事件
	void mouseMoveEvent(QMouseEvent *event) override; 
	//滚轮事件
	void wheelEvent(QWheelEvent *event) override; 
	//鼠标离开事件
	void leaveEvent(QEvent *event) override; 

	// 键盘按键事件
	void keyPressEvent(QKeyEvent *event) override;

private:
	
	//判断临时病害是否跨多个材质或等级 0：人工模式	1：自动化模式      
	bool  isTmpDiseaseRoadTypeValid(const int &frameType) override;


	bool  isTmpDiseaseAreaValid (const hnCommon::hnRoadDiseaseInfo& disease) override;
private:
	//根据病害创建折线的点
	QVector<QPoint> createBrokenLinePoints(hnRoadDiseaseInfo &disease) override final;

private:
	//画人工模式病害的流程
	bool drawBigFrameProcess();

	//画人工模式病害 drawType : 0-人工模式 2-设计模式面状病害
	void drawBigFrameDisease(const vector<hnRoadDiseaseInfo> &diseases ,QImage &image, int drawType);
	
	//人工模式添加病害
	void bigFrameAddDisease(const QPoint &mousePoint) override final;

	//人工模式编辑病害
	void bigFrameEditProcess(const QPoint &mousePoint);

	//人工模式合并病害
	void bigFrameMergeDiseases(const QPoint &screenPoint) override final;

private:
	//绘制数据库加载内容
	void drawDatabaseLoadData(QImage &image) override final;

	//绘制临时内容
	void drawTmpData(QImage &image)override final;

	//绘制打标信息
	void drawMarkValue(QImage & image);
private:
	//hn2drect转大image上的rect
	QRect hn2dRectToImageQtRect(const hn2dRectI &hnRect);

	//hn2dPoint转QPoint 大image的qPoint
	QPoint hn2dPointToImageQtPoint(const hn2dPointWithMileI &hn2dPoint);


	QMap<double, QString>::const_iterator getPreviousStakeIterator(const QMap<double, QString>&map, int curMile);

private:
	//通用编辑病害的通用接口
	void editDisease(hnRoadDiseaseInfo &disease, const QPoint &mousePoint) override;

	// 自动化模式更新病害，主要用于右键拖拽删除自动化模式病害后需要重新计算参数，进行映射，读写数据库
	void updateLittleFrameDisease(hnRoadDiseaseInfo &disease);

private:
	//获取某个点的编码器里程
	double calculateEncoderMile(const QPoint &bigImagePoint) override;

	//获取某个点的绝对里程
	double caculateTrueMile(const QPoint &bigImagePoint) override;

private:
	// 生成状态栏信息
	QString generateStatusInfo(const QPoint &eventPos);

	//根据病害的坐标矩形，创建写入数据库的坐标数组
	vector<hn2dRectI> generateLargeFrameHn2dRectVector(const QRect &rect);

	//根据线状病害的线数组，创建写入数据库的hn2dRectI坐标数组
	vector<hn2dRectI> createLineDisease2dCoordVec(QVector<pixImagePoint> lineDiseasePoints);

	//根据线状病害的线数组，创建写入数组库的hn3dRectI坐标数组
	vector<hn3dRectI> createLineDisease3dCoordVec(QVector<pixImagePoint> lineDiseasePoints);

	//hnCommon::hn2dPointWithMileI
	hnCommon::hn2dPointWithMileI getHnPoint2dWithMileI(const QPoint &point);

	//输入一个针对于一个大image的点，输出hnmile
	hnMile getHnMileFromPoint(const QPoint &allImagePoint);

	//根据病害矩形（Qt）获取病害中心里程（编码器里程）
	double calculateBigFrameCenterMile(const QRect & rect);

	//计算人工模式病害的开始里程（编码器里程）
	double calculateBigFrameBeginMile(const QRect &rect) override;

	//计算人工模式的结束里程（编码器里程）
	double calculateBigFrameEndMile(const QRect &rect) override;

	//设置当前的hnMile
	void setCurrentHnMile();

	//获取当前界面的hnMile数组	
	std::vector<hnMile> getCurrentWidgetHnMiles();

	//判断屏幕上一个点是不是在一个病害的内部
	bool isInDisease(const QPoint & screenPoint, const hnRoadDiseaseInfo& disease, const FrameMode &frameMode) override;

	//根据屏幕上的点，得出编码器里程
	double caculateEncoderMileByScreenPoint(const QPoint &screenPoint);

	// 根据大image上的点 ，得到编码器里程
	double caculateEncoderMileByBigImagePoint(const QPoint &bigImagePoint);

private:
	//根据二维病害的坐标矩形，创建写入数据库的三维坐标数组
	//如果其中有一个点无效的话，则返回空的数组
	vector<hn3dRectI> generateLargeFrameHn3dRectVector(const QRect &rect);

	//获取hn3dPointWithMileI  point是2d视图上大image上的point
	//如果有问题，返回值的bottomEncoderMile为-1
	hnCommon::hn3dPointWithMileI getHnPoint3dWithMileI(const QPoint &point);
	
	//人工模式病害
private:
	//画临时病害
	void drawTmpBigFrameDisease(QImage &image);
	
	//绘制自动化模式 以人工模式方式绘制  取其中的所有小方格
	QRect 	drawTmpLittleBigFrameDisease(QImage &image);


	
private:
	//key 图片名字   value ：hnMile 里程
	QMap<QString, hnMile> m_pixNameHnMileMap;
	//key:编码器桩号：0 2 4 		value:图片名字
	QMap<double, QString> m_milePixNameMap;
private:
	//计算人工模式病害属性
	hnCommon::hnRoadDiseaseInfo caculateBigFrameDiseaseAttribute(const QRect& diseaseRect, const hnDiseaseSetInfo &diseaseSetInfo,const QString& makinfo);

	//计算线状病害属性
	hnRoadDiseaseInfo caculateLineDiseaseInfo(QVector<pixImagePoint> lineDiseasePoints, hnDiseaseSetInfo diseaseSetInfo) override;

protected:
	bool diseasePointToWidgetPointAfterBrowse(
		const pixImagePoint& point,
		bool up,
		QPoint& widgetPoint)  override;
private:
	//画自动化模式的流程
	bool littleFrameProcess();

	//画自动化模式病害
	void drawLittleFrameDisease(vector<hnRoadDiseaseInfo> &diseases, QImage &image);

	 
	//自动化模式编辑病害
	void littleFrameEditDisease(const QPoint &mousePoint);

	// 自动化模式病害右键按下拖拽删除，只删除病害中的一个矩形，删除整个病害使用左键
	void littleFrameRightButtonDragDelete(const QPoint &mousePoint);

	//自动化模式合并病害
	void littleFrameMergeDiseases(const QPoint &screenPoint) override final;

	//计算两个病害以及病害之间的的所有自动化模式矩形
	QVector<QRect> caculateLittleFrameRects(QVector<hnRoadDiseaseInfo> diseases) override final;

	//计算自动化模式病害的大image数组 
	QVector<QRect> caculateLittleFrameBigImageRects(const hnRoadDiseaseInfo& disease) override final;

	//计算自动化模式病害属性
	hnCommon::hnRoadDiseaseInfo caculateLittleFrameDiseaseAttribute(const QVector<QRect>& diseaseRects, const hnDiseaseSetInfo &diseaseSetInfo,const QString& MarkInfo);

	//根据自动化模式病害的坐标矩形数组，创建写入数据库的2d坐标数组
	vector<hn2dRectI> generateLittleFrameHn2dRectVector(const QVector<QRect> &rects);

	//根据自动化模式病害的坐标矩形数组，创建写入数据库的3d坐标数组
	vector<hn3dRectI> generateLittleFrameHn3dRectVector(const QVector<QRect> &rects);

	//计算自动化模式病害的中心里程
	double caculateLittleFrameMiddleMile(const QVector<QRect> &rects);

	//计算自动化模式的开始里程（编码器里程）
	double calculateLittleFrameBeginMile(const QVector<QRect> rects) override;

	//计算自动化模式的结束里程（编码器里程）
	double calculateLittleFrameEndMile(const QVector<QRect> rects) override;

private:
	//大张图rect 转 单张图的rect	这个大张图的rect的四个点要在同一张图片上
	QRect bigImageRectToSingleImageRect(const QRect &bigImageRect, QString *imageName);

	//单张图的rect 转大张图的rect	
	QRect singleImageRectToBigImageRect(const QRect &singleImageRect, const QString &imageName);

	//计算某张图片的大张图的自动化模式数组
	QVector<QRect> calculateBigImageRects(const QString &imageName);

	//点击位置是否为有效区域
	  bool isValidArea(QMouseEvent * event);

	  void selectDisease(const QPoint & mousePoint);
private slots:
       void slotDiseaseChanged();
private:
	//创造单个图片的自动化模式数组	这里的自动化模式是针对小的image的
	QVector<QRect> createSingleImageLittleFrameRect();

	//创造给定点的自动化模式数组 这里的自动化模式是针对大的image的
	QVector<QRect> createLittleFrameRects(const QVector<pixImagePoint>& pixImagePoints);


	//获取鼠标位置的病害
	hnRoadDiseaseInfo getMousePosDisease( const QPoint & mousePoint);

	HnXRSettings * m_xrSetting;
	 
	std::unique_ptr<HighAccuracyPositioning>m_highAccuracy;

	QLabel * m_lblCoordinates;

	QString m_latitude = "";
	QString m_longitude = "";
	QString m_centerH = "";

};
