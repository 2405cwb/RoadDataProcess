#ifndef HN2DCAMERAVIEW_H
#define HN2DCAMERAVIEW_H

#include <QWidget>
#include <vector>
#include <map>
#include "hnapplication_global.h"
#include "hnView.h"

using namespace std;

// 前置声明
class QToolBar;
class QAction;
class QLabel;
class QLineEdit;
class QComboBox;

class HNAPPLICATION_EXPORT hn2DCameraView : public hnView
{
	Q_OBJECT

public:
	hn2DCameraView(QWidget *parent);
	~hn2DCameraView();
	
	//大小提示
	QSize sizeHint() const
	{
		return QSize(350, 150);
	}

public:
	//ptInImage
	bool ptInImage(const QPoint& p);

	// 添加图片 bResetCurImage表示重置当前图片
	bool addImage(const QString& picPath);

	// 设置显示状态
	void setShowState(bool bShow);

	// 获取显示状态
	bool getShowState() { return m_bShow; }

	// 获取视图id
	int  getViewID() { return m_nViewID; }

	// 设置视图id
	void setViewID(int nViewID) { m_nViewID = nViewID; }

	// 更新绘制数据
	void updateDrawData();

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

private:
	
	// 绘制图片
	void drawImage(QPainter * painter);

	// 绘制添加病害
	void drawAddDisease(QPainter* painter);

	// 绘制影像病害
	void drawImageDisease(QPainter* painter);

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

private:
	// 工具条
	QToolBar* m_operateBar;

    // 桩号定位
	QLabel* m_realMileLable;
	QLineEdit* m_realMileEdit;

	// 里程定位
	QLabel* m_enclMileLable;
	QLineEdit* m_enclMileEdit;

	// 跳转
	QAction* m_jumpAct;

	// 路面类型
	QComboBox* m_cmbRoadSurfaceAct;

	// 上一帧
	QAction* m_preImageAct;

	// 下一帧
	QAction* m_nextImageAct;
	
	// 自动播放
	QAction* m_autoPlayAct;

	// 快进
	QAction* m_fastForwardAct;

	// 慢进
	QAction* m_slowForwardAct;


private:
	//屏幕缩放因子
	double m_screenScale;

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

	// 是否第一次加载图片
	bool m_bFirstLoadImage;

	// 偏移量
	double m_dOffsetX;
	double m_dOffsetY;

	// 旋转角度
	double m_dRotate;

	// 偏移量XY缩放比例
	double m_dOffsetXScale;
	double m_dOffsetYScale;

	// 新增初始的放缩值
	double m_dFirstScale;

	// 显示
	bool m_bShow;

	// 视图id
	int m_nViewID;

};

#endif // HNVIEWPIC_H
