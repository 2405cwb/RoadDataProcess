#pragma once
#pragma region 文件说明
/*! @hnBrowsePixWidget.h
********************************************************************************
<PRE>
模块名       : 
文件名       : hnBrowsePixWidget.h
相关文件     : 
文件实现功能 : 通用的显示图片的窗口 需和hnContinuouslyBrowsePixWidget配合使用
作者         : 陈智超
版本         : 1.0.0
--------------------------------------------------------------------------------
备注         : 
--------------------------------------------------------------------------------
修改记录 :
日期        版本     修改人              修改内容
2023/04/14	1.0.0	 陈智超				 创建初版
</PRE>
*******************************************************************************/

#pragma endregion

#pragma region 包含头文件
#include <QWidget>
#include <QDebug>
#include <QLabel>
#include <QFile>
#include <QDir>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QRectF>
#include <QPaintEvent>
#include <QTime>
#include <QImage>
#include <memory>
#include <QMutex>
#include <QDateTime>
#include <QOpenGLWidget>
#include <QCache>
#include "hncontinuousbrowsepix_global.h"
#include "../hnConfigService/HnXRSettings.h"
#pragma endregion



class HNCONTINUOUSBROWSEPIX_EXPORT hnBrowsePixWidget: public QWidget
{
	Q_OBJECT
public:
	hnBrowsePixWidget(QWidget *parent = Q_NULLPTR);
	~hnBrowsePixWidget();

/*公共接口*/
public:
	/*
	*接口名称：loadPix
	*接口介绍：加载图片，通过文件目录
	*参数1:图片路径文件夹的名字
	*返回值：无
	*/
	void loadPix(const QString &pixDirName);

	void loadPix(const QStringList &pixNames);

	/*
	*函数名称：reloadPixMap
	*函数简介：重新加载存储图片名称的map
	*参数一说明：[IN] 图片名称数组，建议调用hnProject的接口，获取当前区间图片数组
	*返回值：[bool] 成功为true 失败为false
	*/
	bool reloadPix(const std::vector<QString> &vecImageName);

	/*
	*函数名称：reloadPixMap
	*函数简介：重新加载存储图片名称的map,从当前工程当前区间的图片数组里面加载
	*返回值：[bool] 成功为true 失败为false
	*/
	bool reloadPix();

	/*
	*函数名称：clearPix
	*函数简介：清空储存图片的map
	*/
	void clearPix();
	
	/*
	*接口名称：getPixPos
	*接口简介：获取某一张图片的左下角坐标，坐标系以左下角为原点,是全局坐标
	*参数一介绍：[QString]图片名称       格式参照00001_450.947_22009764.jpg ，不需要带文件路径
	*返回值：[QPoint]图片的左下角坐标  如果没找到  就返回QPoint(-1 ,-1)
	*/
	QPoint getPixPos(const QString &pixName);

	/*
	*接口名称：singleImagePointToBigImagePoint
	*接口简介: 把一张小image的坐标转换成拼接iamge的坐标，例如：从一张image的坐标转成四张拼接image的坐标
	*参数一介绍：[QPoint][IN]在一张image中的坐标
	*参数二介绍：[QString][IN] 图片名称的绝对路径      
	*返回值：[QPoint]拼接image后的坐标 如果有异常，返回QPoint(-1,-1)
	*备注：
	*/
	QPoint singleImagePointToBigImagePoint(const QPoint &imagePoint, const QString &pixName);

	/*
	*接口名称：bigImagePointToSingleImagePoint
	*接口简介: 把拼接后image的坐标，转换成在某一张image的坐标
	*参数一介绍：[QPoint][IN]在拼接image中的坐标
	*参数二介绍：[QString][OUT] 图片名称的绝对路径  如果有异常，则返回空指针 
	*返回值：[QPoint]单独image的坐标 如果有异常，返回QPoint(-1,-1)  pixName 返回空的字符串""
	*备注：
	*/
	QPoint bigImagePointToSingleImagePoint(const QPoint &labelImagePoint, QString *pixName);

	/*
	*接口名称：screenPointToBigImagePoint
	*接口简介：label屏幕坐标转labelimage的坐标
	*参数一介绍：[QPoint][IN]在此label内屏幕坐标，比如鼠标指针处的坐标
	*返回值：转换完成后的以image为坐标系的坐标
	*/
	QPoint screenPointToBigImagePoint(const QPoint & point);

	/*
	*接口名称：bigImagePointToScreenPoint
	*接口简介：labelimage的坐标转label屏幕坐标
	*参数一介绍：[QPoint][IN]labelimage中的坐标
	*返回值：转换完成后label中的坐标，以label为坐标系
	*/
	QPoint bigImagePointToScreenPoint(const QPoint & point);

	/*
	*接口名称：screenToSingleImagePoint
	*接口简介：屏幕坐标转单张image的坐标
	*参数一介绍：[QPoint][IN]		屏幕中的坐标
	*参数二介绍：[QString &][OUT]	图片的绝对路径
	*返回值：转换完成后单张图片的坐标
	*/
	QPoint screenToSingleImagePoint(const QPoint & point , QString & pixName);


	/*
	*函数名称：setIsHiddenDisease
	*函数简介：设置是否隐藏病害
	*参数一说明：[bool][IN] 是否隐藏病害  隐藏为true 不隐藏为false
	*/
	void setIsHiddenDisease(const bool isHidden);
	/*
	*函数名称：getIsHiddenDisease
	*函数简介：获取是否隐藏病害
	*返回值：[bool]  true:状态为隐藏  false:状态为非隐藏
	*/
	bool getIsHiddenDisease();


	/*
	*函数名称：getCurrentMousePosFrameIdx
	*函数简介：获取当前鼠标位置的帧序号 (从 1开始）
	*返回值：当前鼠标位置的帧序号
	*/
	int getCurrentMousePosFrameIdx();

	/*
	*函数名称：getImageNameFromFrameIdx
	*函数简介：帧序号转图片名称
	*参数一说明：帧序号 从1开始
	*返回值：转换完成的图片名称  不带路径  
	*备注：如果不存在该帧序号，则返回空字符串
	*/
	QString getImageNameFromFrameIdx(const int frameIdx);

	/*
	*函数名称：getFrameIdxFromImageName
	*函数简介：图片名称转帧序号
	*参数一说明：图片名称
	*返回值：转换完成的帧序号 从1开始
	*备注：如果不存在该帧序号，则返回-1
	*/
	int getFrameIdxFromImageName(const QString imageName);

public:
	//获取当前底部帧序号值
	qreal getButtomFrameNumber();

public:
	//设置上下方向是否镜像
	void setVMirrored(const bool isMirrored);

	//获取上下方向是否镜像
	bool isVMirrored();

protected:
	//上下方向是否镜像
	bool m_isVMirrored;

public:
	//设置左右镜像
	void setHMirrored(const bool isMirrroed);

	//获取是否左右镜像
	bool isHMirrored();

protected:
	bool m_isHMirrored;

private:
	//初始化
	void init();

	//获取图片分辨率
	int getPixResoluion(const QString &pixDirName);

	//设置图片分辨率
	void setPixResolution(const int width, const int height);

	/*信号*/
signals:
	//信号：滚动条最大值的变化
	void sig_scrollBarMaxValueChanged(int pixNum);

signals:
	//信号：滚动条值变化
	void sig_scrollBarValueChanged(int value);

signals:
	//信号  鼠标所指的位置帧数变化
	void sig_mousePosFrameIdxUpdate(int frameIdx);

signals:
	//信号  鼠标处的局部image变化
	void sig_mousePosImageChanged(QImage image);

	/*槽函数*/
public slots:
	//更新图片  
	void drawPicture(QImage &image);

	//槽函数：更新当前的滚动条值
	void slot_updateCurrentScrollBar(const int scrollBarValue);

	public slots:
	//移动图片命令
	virtual void slot_moveMouse(bool up,bool is2D);
public:
	//设置预加载图片的帧数，前后各多少帧
	void setLoadFrameNum(const int num);

	//设置被选中的病害id
	void setSelectedDiseaseId(int diseaseId);
protected:
	//获取鼠标位置的原始比例窗口
	QImage getOriginalImage(const QPoint mousePos,const QImage &tmpImageWithoutDisease ,
		const int originalWidgetWidth, const int originalWidgetHeight);
private:
	void drawAllPixOnLabel(QImage &labelImage,const int framePixHeight,const int framePixWidth, const double heightScale);

	//封装私有函数   更新鼠标位置帧数值
	void updateMousePosFrameIdx(QMouseEvent *event);
	void updateMousePosFrameIdx();


	//坐标转换  当前鼠标坐标转化到显示图片的image坐标
	QPoint transformPos(const QPoint &point);

	//封装函数，从labelimage的坐标转换成帧序号 
	int getFrameIdxFromImagePoint(const QPoint &point);

	//浮点数取余数 返回a除以b的余数 比如  2.5/1.1   返回0.3
	qreal getMod(qreal a, qreal b);

protected:
	//判断屏幕上的点是否在图像上
	bool isValidPoint(const QPoint &screenPoint);

private:
	//多线程  根据底部帧序号更新imagemap 
	void updateImageMapBasedOnBottomFrameIdx();

protected:
	//往图片上画东西， 供子类重载
	virtual void drawSomeThingOnImage(QImage &image);

	//加载工程后需要进行的操作，供子类重载
	virtual void afterLoadPictures();

protected:
	//绘图事件
	void paintEvent(QPaintEvent * event) override final;
private:
	//延迟再次刷新界面
	void delayReupdate();

private:
	//储存病害矩形坐标的点和图片名字信息的map
	QMap<QPoint, QString> m_rectPointMap;

private:
	//是否隐藏病害
	bool m_isHiddenDisease;

protected:
	//储存照片名字的map  value为图片名字的绝对路径
	QMap<int, QString> m_pixNameMap;

	QMap<QString,int> m_reversePixNameMap;


	//当前视图的图片名字的map
	QMap<int, QString> m_currentWidgetPixNames;


	
private:
	//储存image的map，帧数对应QImage  帧数从1开始
	QMap<int, QImage> m_imageMap;

	//QCache<int, QImage> m_imageMap;

	//imagemap最大帧序号 帧数从1开始
	int m_imageMapMaxFrameIdx;

	//imageMap的互斥锁
	QMutex m_imageMapMutex;

	//当前进度条的值
	int m_currentScrollBarValue;

	//上次滚动条的值
	int m_lastScrollBarValue;

	//当前鼠标帧数值
	int m_currentMousePosFrameIdx;

protected:
	//带病害的临时image
	QImage m_tmpPixWithDiseaseImage;

	//label底部的帧序号
	qreal m_buttomFrameIdx;

	qreal m_lastButtonFrameIdx;

	//临时image
	QImage m_tmpPixImageWithoutDisease;

	//图片宽度
	int m_pixWidth;

	//图片高度
	int m_pixHeight;

	//高度比例 每个像素代表多少米
	double m_heightScale;

	//宽度比例 每个像素代表多少米
	double m_widthScale;

	// 当前视图起始终止里程
	double m_beginEncoderMile;
	double m_endEncoderMile;

	//当前视图的帧数
	int m_currentWidgetFrameNum;

	//前后的要加载的帧数
	int m_loadFrameNum;

	//是否允许画路面图片
	bool m_isAllowDrawPix;


	//画小框过程中是是否进行了翻页操作
	bool isSuspended = false;

	QPoint lastPoint_Suspending;
	QPoint current_Suspending;

	//2025.11.3 是否允许画最后点击点与鼠标位置连线（虚线）
	bool m_isAllowDrawDashLine;

	//临时内容画板
	QImage m_tmpContectImage;

public:
	//设置原始比例的宽高
	void setOriginalWidgetWidthHeight(const int w,const int h);

protected:
	//原始比例显示窗口的宽
	int m_originalWidgetWidth;

	//原始比例显示窗口的高
	int m_originalWidgetHeight;

private:
	//上次的时间
	QDateTime m_lastTime;
	 
public:
	//获取是否允许联动
	bool getIsAllowLinked();

protected:
	//是否允许联动
	bool m_isAllowLinked;
	int selectedDiseaseId = -1;
protected:
		HnXRSettings* m_setting;
};
