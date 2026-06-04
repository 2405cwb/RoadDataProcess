#pragma once
#include "hnapplication_global.h"
#include "hnBrowsePixWidget.h"
#include "hnWorkMode.h"
#include "hnFrameMode.h"
#include "hnMagnify.h"
#include "drawDiseases.h"
#include "rectAlgorithm.h"
#include "hnAdjustImage.h"
#include "depthCaculate.h"
#include "mergeDisease.h"
#include <QKeyEvent>
#include "addDiseaseDialog.h"
#include "../hnProject/hn3DProject.h"
#include <QMetaObject>
#include "lineAlgorithm.h"
#include <qmath.h>
using namespace hnApp;

class HNAPPLICATION_EXPORT hn2d3dPixBaseWidget:
	public hnBrowsePixWidget,
	public hnWorkMode,
	public hnFrameMode,
	public hnMagnify,
	public drawDiseases,
	public rectAlgorithm,
	public hnAdjustImage,
	public depthCaculate,
	public mergeDisease,
	public lineAlgorithm
{
	Q_OBJECT
public:
	hn2d3dPixBaseWidget(QWidget *parent = Q_NULLPTR);
	~hn2d3dPixBaseWidget();

protected:
	enum WidgetType
	{
		WIDGET_2D,
		WIDGET_3D
	};

signals:
	//信号 数据库添加病害
	//void signal_addDisease(hnRoadDiseaseInfo disease,bool modify);

	//用户选中病害
	void signal_selectDisease(const hnRoadDiseaseInfo& disease);
	  
signals:
	//状态栏信息改变
	void signal_statusInfoChanged(QString &statusInfo);

 
protected:
	void keyPressEvent(QKeyEvent *event) override;
	
	void mousePressEvent(QMouseEvent *event) override;
 
	int diseaseImagePixels(const QImage &image,int screenPixels) const;
public slots:
	//取消画病害
	void slot_cancelDrawDiseases();

	void slot_deleteDisease(const hnRoadDiseaseInfo &disease);
public slots:
	void slot_moveMouse(bool up,bool is2D) override;
protected:
	//计算线状病害属性
	virtual hnRoadDiseaseInfo caculateLineDiseaseInfo(QVector<pixImagePoint> lineDiseasePoints, hnDiseaseSetInfo diseaseSetInfo) = 0;

protected:
	// 获取病害类型
	QStringList getDiseaseTypes();
	// 获取病害设置信息
	QVector<hnDiseaseSetInfo> getDiseaseSetInfos();

protected:
	// 添加线状病害
	void lineDiseaseAddDisease();
	void appendLittleFrameDrawingPoint(const pixImagePoint& point);
	bool hasLittleFramePointInPix(const QString& pixName) const;
protected:
	//绘制线状病害
	void drawLineDiseases(const vector<hnRoadDiseaseInfo> &diseases, QImage &image);

	//绘制临时线状病害
	void drawTmpLineDiseases(QImage &image);

	//2025.11.3 新增绘制最后一个点与鼠标连线（虚线）
	void drawTempDashLine(QImage &image);

	// 创建线状病害的点数组
	virtual QVector<QPoint> createBrokenLinePoints(hnRoadDiseaseInfo &disease) = 0;

	//输入一个针对于一个大image的点，输出hnmile
	virtual hnMile getHnMileFromPoint(const QPoint &allImagePoint) = 0;

	//线状病害计算里程
	double  calculateLineDiseaseCenterMile(QVector<pixImagePoint> lineDiseasePoints);
	double  calculateLineDiseaseBeginMile(QVector<pixImagePoint> lineDiseasePoints);
	double  calculateLineDiseaseEndMile(QVector<pixImagePoint> lineDiseasePoints);

	// 根据大image上的点 ，得到编码器里程
	virtual double caculateEncoderMileByBigImagePoint(const QPoint &bigImagePoint) = 0;

	//创造线病害点数组的里程map 从小到大排列
	QMap<int, double> createLineDiseaseMileMap(const QVector<pixImagePoint> &lineDiseasePoints);

	// 判断临时病害是否有效  
	virtual bool isTmpDiseaseRoadTypeValid(const int &frameType) = 0;

	//判断临时病害面积是否有效  
	virtual bool isTmpDiseaseAreaValid(const hnCommon::hnRoadDiseaseInfo& disease) = 0;


protected:
	// 当前是否正在小框自动化绘制
	bool isDrawingLittleFrameDisease() const;

	// 程序自动移动鼠标后，子类 mouseMoveEvent 里调用；
	// 返回 true 表示本次 mouseMove 已处理，子类应该直接 return。
	bool ignoreMouseMoveAfterAutoCursorMove(QMouseEvent* event);

	// 翻页后延迟把鼠标移动到最佳续画点
	void scheduleMoveCursorToBestContinuePointAfterBrowse(bool up, bool is2D);

	// 真正执行鼠标复位
	bool moveCursorToBestContinuePointAfterBrowse(bool up, bool is2D);

	// 当前可见图像区域，默认认为图片铺满 widget
	virtual QRect visibleImageWidgetRect() const;

	// 子类实现：把一个病害点转换成当前 widget 坐标
	// 注意：widgetPoint 可以在可见区域外，基类会 clamp 到最近可见点。
	virtual bool diseasePointToWidgetPointAfterBrowse(
		const pixImagePoint& point,
		bool up,
		QPoint& widgetPoint)  = 0;

	// 当前 widget 点转病害点，基类用 screenToSingleImagePoint 统一处理
	bool widgetPointToDiseasePoint(const QPoint& widgetPoint, pixImagePoint& point);

	// 把续画锚点同步到临时绘制数据
	void syncLittleFrameContinueAnchor(const QPoint& targetWidgetPoint);


	// Commit current D-rectangle selection before browsing to another page.
	void commitCurrentLittleRectDrawSelection();


	// Calculate current D-rectangle cells without drawing them.
	virtual QVector<QRect> currentLittleRectDrawSelection() = 0;

	// Append committed D-rectangle cells converted back to current big-image coordinates.
	void appendCommittedLittleRectDrawSelection(QVector<QRect>& rects);

	// Clear all temporary D-rectangle selection state.
	void clearLittleRectDrawSelection();

	void resetLittleFrameDrawState();

	// 重建 m_litteBigImagePoints
	void rebuildLittleFrameBigImagePoints();


protected:
	struct LittleFrameSingleRectSelection
	{
		QString pixName;
		QRect singleRect;

		bool operator==(const LittleFrameSingleRectSelection& other) const
		{
			return pixName == other.pixName && singleRect == other.singleRect;
		}
	};

	virtual QRect bigImageRectToSingleImageRect(const QRect& bigImageRect, QString* imageName) = 0;
	virtual QRect singleImageRectToBigImageRect(const QRect& singleImageRect, const QString& imageName) = 0;

	// 程序自动 setPos 后，下一次 mouseMove 不参与绘制
	bool m_ignoreNextMouseMoveAfterAutoCursorMove = false;
	QVector<LittleFrameSingleRectSelection> m_committedLittleFrameDiseaseRects;
protected:
	// 添加线状病害
	void addLineDisease(const QPoint &widgetPoint);

	// 通用的删除病害的方法
	void commonDeleteDisease(const QPoint &mousePoint,hnFrameMode::FrameMode frameMode) ;

	// 通用的编辑病害的方法
	void commonEditDisease(const QPoint &mousePoint, hnFrameMode::FrameMode frameMode);

	// 线状病害合并
	void mergeLineDisease(const QPoint &widgetPoint);

	//判断一个点是不是在线状病害附近
	bool isNearbyLineDisease(const QPoint &bigImagePoint, const hnRoadDiseaseInfo&disease);

	//判断屏幕上一个点是不是在一个病害的内部
	virtual bool isInDisease(const QPoint & screenPoint, const hnRoadDiseaseInfo& disease, const hnFrameMode::FrameMode &frameMode) = 0;

	//大自动化模式通用编辑病害的通用接口
	virtual void editDisease(hnRoadDiseaseInfo &disease, const QPoint &mousePoint) = 0;

	// 计算线状病害的总长度
	double caculateLineDiseaseLenth(QVector<pixImagePoint> lineDiseasePoints,WidgetType widgetType);

protected:

	//自动纠正三维视图的x坐标
	void autoCorrectXIn3dView(int &x);


	QPoint currentMousePos;
protected:
	// pixImagePoint 转 bigImagePoint
	QPoint pixImagePointToBigImagePoint(const pixImagePoint &point);
	void clearVisibleLittleFrameRectCache();
protected:
	//线状病害的临时点，这个点用于存储用户点击的点
	QVector<pixImagePoint> m_tmpLineDiseasePoints;

	// 绘制的线状病害的临时点，用于绘制
	QVector<pixImagePoint> m_tmpPaintLineDiseasePoints;

	//2025.11.3 画临时虚线（最后一个鼠标左键点击点与鼠标位置点）
	QVector<pixImagePoint> m_tempPoints;

	//绘制线状病害 记录最后一次用户按下后移动的点 
	QVector<pixImagePoint> m_tmpLastPaintLineDiseasePoints; 
	// 绘制线条的宽度，矩形的边框宽度也适用
	int m_lineWidth;
	// 字体大小
	int m_fontSize;

	QVector<QRect> m_cachedVisibleLittleFrameRects;
	QStringList m_cachedVisibleLittleFramePixNames;
protected:
	WidgetType m_widgetType;

	// 鼠标右键移动删除病害，是否按下标志
	bool m_isRightDeleteMouseDown;

	// 右键拖拽删除病害时只记录按下时自动化模式位置，鼠标滑动的位置不处理，不然会卡
	QPoint m_RightDeleteMousePoint;

	int m_pendingRightClcikDeleteSerial = 0;
	QPoint m_pendingRightClickDeletePoint = QPoint(-1,-1);
	
	//重新计算病害尺寸或 重新计算病后尺寸后写入数据库  
	//save为false为适配以前版本，为自动化模式外接矩形计算长度和宽度,不写入数据库
	//save为true,当用户修改了病害面积计算方式的时候重新计算病害尺寸并且写入数据库
	void reCalculateDiseaseSizeAndSave(hnCommon::hnRoadDiseaseInfo&  disease,bool save);


	//重新计算老版本的自动化模式尺寸，以前的版本没有 长度和宽度
	void reCalculateOldDiseaseSizeAndSave(const QVector<QRect>& diseaseRects,hnCommon::hnRoadDiseaseInfo&  disease);


	//计算病害长度和宽度，根据自动化模式的最大外接矩形
	void CalculateDiseaseSize(const QVector<QRect>&diseaseRects, hnCommon::hnRoadDiseaseInfo&  disease);
 	 
private:
		//根据最大外接矩形设置病害尺寸
		void setLittleDiseaseSize(const QVector<QRect>& diseaseRects, hnRoadDiseaseInfo& disease);
protected:
	//是否结束左键连续点击添加病害 
	bool m_isEndAddPoint;

	//bool m_diseaseCacheValid;
	//double m_cacheBeginEncoderMile;
	//double m_cacheEndEncoderMile;
	//int m_cacheFrameMode;
};

