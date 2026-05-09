#include <QPainter>
#include <QResizeEvent>
#include <QApplication>
#include <QDir>
#include <QRubberBand>
#include <QtDebug>
#include <vector>
#include<algorithm>
#include <iostream>
#include <QFileDialog>
#include <QDesktopWidget>
#include <thread>
#include <QMutexLocker>
#include "io.h"
#include "string.h"
#include "qmath.h"
#include "hnCommandDef.h"
#include "hnDataManager.h"
#include "hn3DCameraView.h"
#include "..\hnProject\hn3DProject.h"
#include "..\hnProject\hnProject.h"
//////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace hnPro;

namespace hnApp
{
	hn3DCameraView::hn3DCameraView(QWidget *parent)//m_scale(0.25)
		: hnView(parent), m_pCenter(0, 0), m_scale(0.12), m_LoadPic(NULL), m_screenScale(1.0), m_bPushMiddleButton(false), m_bFirstLoadImage(true)
		, m_dFirstScale(0.14), m_bShow(true), m_nViewID(1)
	{
		//背景
		QPalette myPalette = QPalette(palette());
		myPalette.setColor(QPalette::Background, QColor(33, 40, 48));
		setAutoFillBackground(true);
		setPalette(myPalette);

		//2020.8.17隐藏
		//this->setMinimumHeight(150);
		//this->setMinimumWidth(400);

		setContextMenuPolicy(Qt::CustomContextMenu);  // 设置右键菜单

													  //鼠标追踪
		setFocusPolicy(Qt::FocusPolicy::ClickFocus);
		setMouseTracking(true);

		//视图初始化  将类与本地消息重构 击穿 首次最难击穿 得到视图左上角原点屏幕坐标进行偏移  客户区域转屏幕区域  客户区域是除去标题栏之后的区域左上角为0,0  视图是屏幕左上角0,0
		POINT pt_view;
		pt_view.x = 0; pt_view.y = 0;
		ClientToScreen((HWND)this->winId(), &pt_view);

		//2020.8.17修正初始放缩值
		m_scale = m_dFirstScale;

		//图片中心设置
		/*m_scale = 0.12;*/
		m_pCenter = QPoint(0.0, 0.0);

		//中键点击点
		m_pMidClick = QPoint(0, 0);

		m_dOffsetX = 0.0;
		m_dOffsetY = 0.0;

		m_dRotate = 270.0;

		m_dOffsetXScale = 0.04;
		m_dOffsetYScale = 1.25;
	}

	hn3DCameraView::~hn3DCameraView()
	{
		if (m_LoadPic)
		{
			delete m_LoadPic;
			m_LoadPic = NULL;
		}

	}

	/************************************************************************/
	/*                            事件                              */
	/************************************************************************/

	// 绘制事件
	void hn3DCameraView::paintEvent(QPaintEvent *)
	{
		QPainter painterTrans(this);
		QPainter painter(this);

		//没有图片
		if (!m_LoadPic)
		{
			return;
		}

		// 设置偏移矩阵
		QTransform transform;

		transform.translate(m_dOffsetX, m_dOffsetY);
		transform.rotate(m_dRotate);
		painterTrans.setWorldTransform(transform);

		//画图
		drawImage(&painterTrans);

		if (m_bShow)
		{
			// 绘制添加病害
			drawAddDisease(&painter);

			// 绘制影像病害
			drawImageDisease(&painter);
		}

	}

	// 窗体大小改变事件
	void hn3DCameraView::resizeEvent(QResizeEvent * e)
	{
		if (abs(e->oldSize().height() - 0.0) < 0.0001)
		{
			m_scale = 0.12;
			return;
		}

		double scale = e->size().height() / (double)e->oldSize().height();
		if (scale > 0)
		{
			m_scale = m_scale * scale;
		}

	}

	// 鼠标滚动事件(放大缩小)
	void hn3DCameraView::wheelEvent(QWheelEvent *e)
	{
		if (!m_LoadPic)
		{
			return;
		}

		//获得新中心
		QPoint oldPos = e->pos();
		QPoint pUser = fromScreen2User(oldPos);

		// 直接调用当前工具进行三维浏览（此处是hdToolfly）
		int ndelta = e->delta();
		int global_x = 0.0;
		int global_y = 0.0;

		// 缩放影像
		scaleImage(ndelta);

		m_dOffsetX = m_LoadPic->width()* m_scale * m_dOffsetXScale;
		m_dOffsetY = m_LoadPic->height()* m_scale * m_dOffsetYScale;

		QPoint newPos = fromUser2Screen(pUser);

		m_pCenter = m_pCenter - (newPos - oldPos);

		// 事件处理完毕
		e->accept();

		update();
	}

	// 鼠标按下事件
	void hn3DCameraView::mousePressEvent(QMouseEvent *event)
	{
		if (event->button() == Qt::MidButton)
		{
			QPoint oldPos = event->pos();
			getOldCenterByMoveImage(oldPos.x(), oldPos.y());

			m_bPushMiddleButton = true;
		}
	}

	// 鼠标弹起事件
	void hn3DCameraView::mouseReleaseEvent(QMouseEvent *event)
	{
		m_bPushMiddleButton = false;
	}

	// 鼠标移动事件
	void hn3DCameraView::mouseMoveEvent(QMouseEvent *event)
	{
		if (m_bPushMiddleButton)
		{
			QPoint oldPos = event->pos();
			getNewCenterByMoveImage(oldPos.x(), oldPos.y());
		}

		if (m_LoadPic == NULL)
		{
			return;
		}

		update();
	}

	// 键盘按下事件 空格键 浏览模式 图片回到正常水平
	void hn3DCameraView::keyPressEvent(QKeyEvent *e)
	{

	}

	/************************************************************************/
	/*                            fun                              */
	/************************************************************************/

	// 自定义坐标转换屏幕坐标
	QPoint hn3DCameraView::fromUser2Screen(const QPoint& pti)
	{
		QPoint pt;

		pt.rx() = pti.x() * m_scale - m_pCenter.y();
		pt.ry() = pti.y() * m_scale + m_pCenter.x();

		QTransform transform;
		transform.translate(m_dOffsetX, m_dOffsetY);
		transform.rotate(m_dRotate);

		pt = transform.map(pt);

		return pt;
	}

	// 屏幕坐标转换为自定义坐标
	QPoint hn3DCameraView::fromScreen2User(const QPoint& pti)
	{
		QPoint pt;
		pt.rx() = pti.x();
		pt.ry() = pti.y();

		QTransform transform;
		transform.translate(m_dOffsetX, m_dOffsetY);
		transform.rotate(m_dRotate);
		transform = transform.inverted();

		pt = transform.map(pt);

		pt.rx() = (pt.x() + m_pCenter.y()) / m_scale + 0.5;
		pt.ry() = (pt.y() - m_pCenter.x()) / m_scale + 0.5;

		return pt;
	}

	//获得原始图片中心 鼠标中键按下 用于移动图片
	void hn3DCameraView::getOldCenterByMoveImage(int X, int Y)
	{
		m_pMidClick = QPoint(X, Y) - m_pCenter;
	}

	//获得新的移动点 鼠标中键移动 用于移动图片
	void hn3DCameraView::getNewCenterByMoveImage(int X, int Y)
	{
		QPoint pCenter;
		pCenter = QPoint(X, Y);

		QPoint MidClickPoint = m_pMidClick;

		QPoint tempPt = pCenter - MidClickPoint;

		m_pCenter = tempPt;

		update();

	}

	//放大缩小图片
	void hn3DCameraView::scaleImage(short zDelta)
	{
		double dScale = m_scale;

		if (zDelta > 0.0)
		{
			dScale += 0.05;
		}
		else
		{
			dScale -= 0.05;

			//最小为0.09
			if (dScale < 0.09)
			{
				return;
			}

			if (dScale < 0.0)
			{
				dScale += 0.05;
				return;
			}
		}

		//小于该值
		if (dScale <= 0.034)
		{
			return;
		}

		//设置值
		m_scale = dScale;

	}

	/************************************************************************/
	/*                                public                                 */
	/************************************************************************/

	// 添加图片 bResetCurImage表示重置当前图片
	bool hn3DCameraView::addImage(const QString& picPath)
	{
		//法3
		if (NULL != m_LoadPic)
		{
			delete m_LoadPic;
			m_LoadPic = NULL;
		}

		m_LoadPic = new QPixmap;

		QString strImagePath = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getGreyImagePath();
		strImagePath = strImagePath + "/" + picPath;

		//从文件中加载
		QImage imageTemp;
		imageTemp.load(picPath);
		*m_LoadPic = QPixmap::fromImage(imageTemp.scaled(QSize(imageTemp.width(), imageTemp.height()), Qt::KeepAspectRatio));

		//图片大小设置
		if (m_bFirstLoadImage)
		{
			/*m_scale = 0.16*m_screenScale;*/
			//2020.8.17 修正初始放缩值
			m_scale = m_dFirstScale*m_screenScale;
			m_pCenter = QPoint(0.0, 0.0);
			m_bFirstLoadImage = false;

			m_dOffsetX = m_LoadPic->width()* m_scale * m_dOffsetXScale;
			m_dOffsetY = m_LoadPic->height()* m_scale * m_dOffsetYScale;
		}

		//设置鼠标的形态  十字形态
		this->setCursor(Qt::CrossCursor);

		m_dRotate = 270.0;

		m_dOffsetXScale = 0.04;
		m_dOffsetYScale = 1.25;

		//更新
		updateDrawData();

		return true;

	}

	//清空信息
	void hn3DCameraView::clear()
	{

		//清空图片
		if (m_LoadPic)
		{
			delete m_LoadPic;
			m_LoadPic = NULL;
		}

		//图片大小设置
		m_scale = 0.12*m_screenScale;
		m_pCenter = QPoint(0.0, 0.0);

		m_bFirstLoadImage = true;

		//更新
		update();

	}

	//ptInImage
	bool hn3DCameraView::ptInImage(const QPoint& p)
	{
		if (m_LoadPic == NULL)
		{
			return false;
		}

		if (p.x() < 0 || p.y() < 0 || p.x() >= m_LoadPic->width() || p.y() >= m_LoadPic->height())
		{
			return false;
		}

		return true;
	}

	// 绘制图片
	void hn3DCameraView::drawImage(QPainter * painter)
	{
		painter->save();

		//重新画图
		QRect rect;

		rect.setRect(-m_pCenter.y(), m_pCenter.x(), m_LoadPic->width() * m_scale, m_LoadPic->height() * m_scale);

		painter->drawPixmap(rect, *m_LoadPic);

		painter->restore();

	}

	// 绘制添加病害
	void hn3DCameraView::drawAddDisease(QPainter* painter)
	{

	}

	// 绘制影像病害
	void hn3DCameraView::drawImageDisease(QPainter* painter)
	{

	}

	// 设置显示状态
	void hn3DCameraView::setShowState(bool bShow)
	{
		m_bShow = bShow;

		update();
	}


	// 更新绘制数据
	void hn3DCameraView::updateDrawData()
	{

		update();
	}
}
