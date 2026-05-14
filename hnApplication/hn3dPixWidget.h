#pragma once
#include "hnDataManager.h"	//这个要放最前面，不然会出问题
#include <QWidget>
#include "hnapplication_global.h"
#include <QMouseEvent>
#include "hnMile.h"
#include "hnImagePainter.h"
#include "../hnDataTable/hnRoadDiseaseTable.h"
#include "../hnCommon/hnRoadStruct.h"
#include "hn3dImageMode.h"
#include "hn2d3dCoordinates.h"
#include "hnMagnify.h"
#include <QTimer>
#include <QApplication>
#include "../hnCommon/hnRoadStruct.h"
#include "ShowCtrlPointInfoDlg.h"
#include "projectType.h"
#include "hn2d3dPixBaseWidget.h"

class HNAPPLICATION_EXPORT hn3dPixWidget final: 
public hn2d3dPixBaseWidget,
public hn3dImageMode,
public projectType
{
	Q_OBJECT
public:
	//构造函数
	hn3dPixWidget(QWidget *parent = Q_NULLPTR);

	//加载3d影像
	void load3DImagePictures();

	//设置进度条的值
	void setScrollBarValue(const int scrollBarValue);

public:
	//设置2dMileVector
	void setMileVector(QVector<hnMile> &hnMiles);

private:
	// 生成状态栏信息
	QString generateStatusInfo(const QPoint &eventPos);

protected:
	//重载函数，绘图，往窗口上画东西
	void drawSomeThingOnImage(QImage &image) override;

protected:
	//重载函数 鼠标单击事件
	void mousePressEvent(QMouseEvent *event) override;


	//鼠标双击事件
	void mouseDoubleClickEvent(QMouseEvent *event) override;

	// 鼠标释放事件
	void mouseReleaseEvent(QMouseEvent * event);

	//重载函数  鼠标移动事件
	void mouseMoveEvent(QMouseEvent *event) override;

	//滚轮事件
	void wheelEvent(QWheelEvent *event) override;

	//离开事件
	void leaveEvent(QEvent *event) override;

	// 键盘按键事件
	void keyPressEvent(QKeyEvent * event);

private:
	// 根据病害创建折线的点
	QVector<QPoint> createBrokenLinePoints(hnRoadDiseaseInfo &disease) override final;

private:
	//绘制数据库加载内容
	void drawDatabaseLoadData(QImage &image) override final;
	//绘制临时内容
	void drawTmpData(QImage &image)override final;

private:
	//判断临时病害是否跨多个材质或等级 0：人工模式	1：自动化模式
	bool isTmpDiseaseRoadTypeValid(const int &frameType) override;


	bool  isTmpDiseaseAreaValid(const hnCommon::hnRoadDiseaseInfo& disease) override;

	//点击位置是否为有效区域
	bool isValidArea(QMouseEvent * event);

	void selectDisease(const QPoint & mousePoint);

	private slots:
	void slotDiseaseChanged();
protected:
	bool diseasePointToWidgetPointAfterBrowse(
		const pixImagePoint& point,
		bool up,
		QPoint& widgetPoint)  override;
private:
	//画人工模式病害
	void drawBigFrameDisease(const vector<hnRoadDiseaseInfo> &diseases, QImage &image);

	//人工模式添加病害
	void bigFrameAddDisease(const QPoint &mousePoint) override;

	//人工模式编辑病害
	void bigFrameEditProcess(const QPoint &mousePoint);

	//人工模式合并病害
	void bigFrameMergeDiseases(const QPoint &screenPoint) override;

private:
	//大自动化模式通用编辑病害的通用接口
	void editDisease(hnRoadDiseaseInfo &disease, const QPoint &mousePoint) override;

	// 自动化模式更新病害，主要用于右键拖拽删除自动化模式病害后需要重新计算参数，进行映射，读写数据库
	void updateLittleFrameDisease(hnRoadDiseaseInfo & disease);

private:
	//hn3drect转大image上的rect
	QRect hn3dRectToBigImageQtRect(const hn3dRectI &hnRect);

	//hn3dPoint转QPoint 大image的qPoint
	QPoint hn3dPointToBigImageQtPoint(const hn3dPointWithMileI &hn3dPoint);

	//控制点转 大image的QPoint
	QPoint ctrlPointToBigImagePoint(const hnKZDDataInfo &ctrlPoint);

	//根据病害的坐标矩形，创建写入数据库的3d坐标数组
	vector<hn3dRectI> generateLargeFrameHn3dRectVector(const QRect &rect);

	//获取hn3dPointWithMileI
	hnCommon::hn3dPointWithMileI getHnPoint3dWithMileI(const QPoint &point);

	//输入一个针对于一个大image的点，输出hnmile
	hnMile getHnMileFromPoint(const QPoint &allImagePoint);

	// 人工模式边界自适应 二三维可用
	QRect bigFrameDiseaseAutoWidth(const QRect &bigImageRect);

private:
	//根据线状病害的点数组 创建写入数据库的3d数组
	vector<hn3dRectI> createLineDisease3dCoord(const QVector<pixImagePoint> &lineDiseasePoints);

	//根据线状病害的点数组 创建写入数据库的2d数组 映射
	vector<hn2dRectI> createLineDisease2dCoord(const QVector<pixImagePoint> &lineDiseasePoints);

private:
	//获取某个点的编码器里程
	double calculateEncoderMile(const QPoint &bigImagePoint) override;

	//获取某个点的桩号
	double caculateTrueMile(const QPoint &bigImagePoint) override;

	//根据病害矩形（Qt）获取病害里程（编码器里程）
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
	bool isInDisease(const QPoint & screenPoint, const hnRoadDiseaseInfo& disease,const FrameMode &frameMode);

	//根据屏幕上的点，得出编码器里程
	double caculateEncoderMileByScreenPoint(const QPoint &screenPoint) override;

	//计算某个点的里程，这个函数不管是二三维和单三维都可以用
	double caculateEncoderMile(const pixImagePoint &point);

	// 计算某个点的里程 这个函数不管是二三维和单三维都可以用
	double caculateEncoderMileByBigImagePoint(const QPoint &bigImagePoint);

public:
	double encoderMileToTrueMile(double encoderMile);
	double trueMileToEncoderMile(double trueMile);

private:
	//根据病害的坐标矩形，创建写入数据库的2d坐标数组 rect是大image中的矩形
	vector<hn2dRectI> generateLargeFrameHn2dRectVector(const QRect &rect);

	//获取hn2dPointWithMileI  point 是大image中的点
	//如果有问题，返回值的m_dmi为-1
	hnCommon::hn2dPointWithMileI getHnPoint2dWithMileI(const QPoint &point);

	//根据三维图片名字获取对应的hnMile 这个图片名字 灰度图和深度图都可以,传入绝对路径
	hnMile getHnMileBy3dPixName(const QString &pix3dName);


	//获取鼠标位置的病害  inDisease : 
	hnRoadDiseaseInfo getMousePosDisease(const QPoint & mousePoint);
private:
	QMap < double, hnMile> encoderMilesToEncoderMileHnMileMap();

	QVector<double> grayImageNamesToEncoderMileVector();

private:
	//key 编码器里程   value ：hnMile 里程
	QMap<double, hnMile> m_encoderMileHnMileMap;

	//三维的里程vector
	QVector<double> m_3dEncoderMileVector;

	//三维的编码器里程 和图片名字的对应关系
	//key 编码器里程 value 三维灰度图的名称，不含路径
	QMap<double, QString > m_encoderMileGrayPixNameMap;

private:
	//画人工模式病害的流程
	bool drawBigFrameProcess();

	//画人工模式临时病害
	void drawTmpLargeFrameDisease(QImage &image);

	QRect drawTmpLittleBigFrameDisease(QImage &image);
private:
	//计算人工模式病害属性
	hnCommon::hnRoadDiseaseInfo caculateBigFrameDiseaseAttribute(const QRect& diseaseRect, const hnDiseaseSetInfo &diseaseSetInfo,const QString& diseaseInfo);

	//计算线状病害属性
	hnRoadDiseaseInfo caculateLineDiseaseInfo(QVector<pixImagePoint> lineDiseasePoints, hnDiseaseSetInfo diseaseSetInfo) override;

private:
	QVector<hnMile> m_2dMileVector;

	// 所有灰度图像名称 不含图片路径，不含GRAY 例：0.000-8.000.jpg
	QVector<QString> m_vecGreyImageName;

	// 所有深度图名称 不含图片路径，不含RGB 例：0.000-8.000.jpg
	QVector<QString> m_vecRGBImageName;

	// 灰度图路径
	QString m_strGreyImaePath;

	// 深度图路径
	QString m_strRGBImagePath;

private:
	//画自动化模式的流程
	bool littleFrameProcess();

	//自动化模式编辑病害
	void littleFrameEditDisease(const QPoint &mousePoint);

	// 自动化模式病害右键按下拖拽删除，只删除病害中的一个矩形，删除整个病害使用左键
	void littleFrameRightButtonDragDelete(const QPoint & mousePoint);

	// 自动化模式合并病害
	void littleFrameMergeDiseases(const QPoint &screenPoint) override final;

	// 计算两个病害以及病害之间的的所有自动化模式矩形 
	QVector<QRect> caculateLittleFrameRects(QVector<hnRoadDiseaseInfo> diseases) override final;

	// 画自动化模式病害
	void drawLittleFrameDisease(const vector<hnRoadDiseaseInfo> &diseases, QImage &image);

	//计算自动化模式病害的大image数组 
	QVector<QRect> caculateLittleFrameBigImageRects(const hnRoadDiseaseInfo& disease) override final;

	// 计算自动化模式病害属性
	hnCommon::hnRoadDiseaseInfo caculateLittleFrameDiseaseAttribute(const QVector<QRect>& diseaseRects, const hnDiseaseSetInfo &diseaseSetInfo,const QString& diseaseMark);

	// 根据自动化模式病害的坐标矩形数组(大image)，创建写入数据库的2d坐标数组  
	vector<hn2dRectI> generateLittleFrameHn2dRectVector(const QVector<QRect> &rects);

	//根据自动化模式病害的坐标矩形数组(大image)，创建写入数据库的3d坐标数组 
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

private:
	//创造单个图片的自动化模式数组	这里的自动化模式是针对小的image的
	QVector<QRect> createSingleImageLittleFrameRect();

	//创造给定点的自动化模式数组 这里的自动化模式是针对大的image的
	QVector<QRect> createLittleFrameRects(QVector<pixImagePoint> pixImagePoints);

private:
	// 添加控制点
	void addCtrlPoint(const QPoint &mousePoint);

	// 删除控制点
	void deleteCtrlPoint(const QPoint &mousePoint);	
	// 删除控制点
	void deleteCtrlPoint(const hnKZDDataInfo& KZDDataInfo);

	// 处理选中的控制点
	// 使用函数指针作为参数
	void hundleSelectCtrlPoint(const QPoint &mousePoint,void(hn3dPixWidget::*hundleFunc)(const hnKZDDataInfo& KZDDataInfo));

	// 编辑控制点（查看控制点信息）
	void editCtrlPoint(const QPoint &mousePoint);
	// 编辑控制点（查看控制点信息）
	void editCtrlPoint(const hnKZDDataInfo& KZDDataInfo);

	// 绘制控制点
	void drawCtrlPoint(QImage &image, const std::vector<hnKZDDataInfo> &ctrlPoints);

	// 不带路径RGB/GREY的图片名称转为带路径，带RGB GREY的图片名称
	QString pixName3dConvertWithPath(const char* pixName);

	// 带前缀的图片名称转为不带前缀，不含RGB GREY的图片
	QString pixName3dConvertWithoutPath(const QString &pixName);

	//当前的控制点
	std::vector<hnKZDDataInfo> m_currentCtrlPoints;
};
