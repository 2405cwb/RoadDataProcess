#ifndef HNSTREETCAMERAVIEW_H
#define HNSTREETCAMERAVIEW_H

#include <QWidget>
#include <QMenu>
#include <QAction>
#include <vector>
#include <map>
#include <QProgressDialog>
#include <QMessageBox>
#include <QDockWidget>
#include <QByteArray>
#include <QEvent>
#include "hnapplication_global.h"
#include "hnView.h"
#include "hnWorkMode.h"
#include "hnImagePainter.h"
#include "hnMile.h"
#include "addDiseaseDialog.h"
#include "hnDataManager.h"
#include "hnStreetCameraView.h"
#include "..\hnProject\hn2DProject.h"
#include "..\hnProject\hnProject.h"
#include "hnAddStreetDiseaseDialog.h"
#include <QFontMetrics>
#include "opencv2/opencv.hpp"
using namespace std;

namespace hnApp
{
	// 前置声明
	class HNAPPLICATION_EXPORT hnStreetCameraView : public hnView, public hnWorkMode
	{
		Q_OBJECT

	public:
		hnStreetCameraView(QWidget *parent);
		~hnStreetCameraView();

		//大小提示
		QSize sizeHint() const
		{
			return QSize(350, 150);
		}

	public:
		//ptInImage
		bool ptInImage(const QPoint& p);

		// 添加图片 bResetCurImage表示重置当前图片
		bool addImage(bool needRotate,const QString& picPath);

		// 添加图片
		bool addImage(bool needRotate, int nImageIndex);

		// 设置显示状态
		void setShowState(bool bShow);

		// 获取显示状态
		bool getShowState() { return m_bShow; }

		// 更新绘制数据
		void updateDrawData();

		// 重新加载数据
		void reloadData(bool needRotate, STREET_VIEW_TYPE nViewType);

		void setStreetInterval(double interval);

	public:
		//清空信息
		void clear();

		//获得原始图片中心 鼠标中键按下 用于移动图片
		void getOldCenterByMoveImage(int X, int Y);

		//获得新的移动点 鼠标中键移动 用于移动图片
		void getNewCenterByMoveImage(int X, int Y);

		//放大缩小图片
		void scaleImage(short zDelta);

		// 自定义坐标转换屏幕坐标
		QPoint fromUser2Screen(const QPoint& pti);

		// 屏幕坐标转换为自定义坐标
		QPoint fromScreen2User(const QPoint& pti);

		void setCalculateRoadWidthMode(bool mode) { m_calculateRoadWidthMode = mode; }

		void setUnitScale(double unitPerPixel,const QString& unitName);
	private:

		enum class MeasureState
		{
			Idle, //什么都没做
			Measuring, //已经点击了第一个点 正在移动
			Finished //已经点击了第二个点 ，显示结果
		};
		// 绘制图片
		void drawImage(QPainter * painter);

		void cancelMeasure();
	protected:
		/************************************************************************/
		/*									   事件                              */
		/************************************************************************/
		// 绘制事件
		virtual void paintEvent(QPaintEvent *);

		// 窗体大小改变事件
		virtual void resizeEvent(QResizeEvent *);

		// 鼠标滚动事件
		virtual void wheelEvent(QWheelEvent *);

		// 鼠标按下事件
		virtual void mousePressEvent(QMouseEvent *event);

		// 鼠标弹起事件
		virtual void mouseReleaseEvent(QMouseEvent *event);

		// 鼠标移动事件
		virtual void mouseMoveEvent(QMouseEvent *event);

		// 键盘按下事件 空格键 浏览模式 图片回到正常水平
		void keyPressEvent(QKeyEvent *e);

	signals:
		void updateImageView(int);

		// 更新影像
		void updateShowImg(int);

		 

		void sig_mousePosImageChanged(QImage widget);
	signals:
		//信号 数据库添加病害
		void signal_addDisease(hnRoadDiseaseInfo disease,bool modify);
	signals:
		//信号 数据库删除病害
		void signal_deleteDisease(hnRoadDiseaseInfo disease);

	public slots:
		void hnStreetCameraView::setOriginalWidgetWidthHeight(const int w, const int h);


	//更新图片亮度
		void	slot_updatePictureBrightness(int value);
	private:
		//添加病害
		void addDisease();

		//删除病害
		void deleteDisease();

		void deleteDisease(const QPoint &point);

		QImage updateBrightness(QImage &image);
 
	private:
		// 缩放比例
		double m_scale;

		// 中心点
		QPoint m_pCenter;

		//中键点击点
		QPoint m_pMidClick;

		//加载图片
		QPixmap* m_LoadPic;

		// 中键是否按下
		bool m_bPushMiddleButton;

		// 偏移量
		double m_dOffsetX;
		double m_dOffsetY;

		// 显示状态
		bool m_bShow;

		// 影像类型
		STREET_VIEW_TYPE m_nStreetViewType;

		// 当前影像里程
		int m_nCurImageDmi;

		// 当前影像
		QStringList m_listImage;

		//
		bool m_bFirstLoadImage;

		//景观图片的桩号hnMile
		QVector<hnMile> m_streetMiles;

		//图片绝对路径和hnMile的对应关系
		QMap<QString, hnMile> m_pixPathStreetMilesMap;

		//当前视图病害
		QVector<hnRoadDiseaseInfo> m_currentWidgetDiseases;

		// 当前视图病害
		// QRect：病害文字的外边缘矩形框
		// hnRoadDiseaseInfo: 病害信息
		// QString : 病害的名字 里程+名字+分数
		QVector< QPair<QRect, QPair<hnRoadDiseaseInfo, QString>>> m_currentDiseasesVector;


		QImage  hnStreetCameraView::getOriginalImage(QPoint mousePos, const QImage &tmpImageWithoutDisease,
			const int originalWidgetWidth, const int originalWidgetHeight);

		
		//原始比例显示窗口的宽
		int m_originalWidgetWidth;

		//原始比例显示窗口的高
		int m_originalWidgetHeight;

		//界面属于左景观还是右边景观，0 左 1右
		STREET_VIEW_TYPE  m_nViewType;
		//图片间隔
		double m_pictureInterval;

		//图片亮度
		int PictureBrightnessFactor = 0;
		
		//存储处理后 用于显示的图像
		QPixmap*  m_displayPic; //

		//图片是否需要旋转
		bool  m_needRotate;

		//测量长度模式
		bool m_calculateRoadWidthMode;
		MeasureState m_state = MeasureState::Idle;
		QPointF m_startPoint;
		QPointF m_endPoint;
		QPointF m_currentPoint;
		double m_unitPerPixel = 1.0; 
		QString m_unitName = "m	";
	 

		QString m_curPicPath;


	};
}

#endif // HNVIEWPIC_H
