#include "hnwidget3dview.h"
#include "..\hnApplication\hnApplication.h"
#include "..\hdFramework\hdTool.h"

using namespace hnApp;

hnWidget3DView::hnWidget3DView(QWidget *parent)
	: QDockWidget(parent)
{
	// 设置视图默认光标为十字
	this->setCursor(Qt::CrossCursor);

	// 设置视图更新状态为不可用
	this->setUpdatesEnabled(false);

	// 初始化视图尺寸
//	this->resize(1200,600);
	this->setMinimumHeight(400);
	this->setMinimumWidth(600);

	// 设置鼠标位置追踪
	this->setMouseTracking(true);

	// 焦点获取
	setFocusPolicy(Qt::FocusPolicy::ClickFocus);

	// 设置右键菜单
	setContextMenuPolicy(Qt::CustomContextMenu);

	// 与主框架绑定信号槽，用于更新设置当前活动视图;
	registerSignalSlots();
}

hnWidget3DView::~hnWidget3DView()
{

}

void hnWidget3DView::registerSignalSlots()
{
	// 主窗口得到当前鼠标点位置;
	connect(this, SIGNAL(mouseMoveF(QPointF*)), this->parentWidget(), SLOT(statusBarManager(QPointF*)));

	// 主窗口得到当前活动视图;
	QMetaObject::Connection cnt = connect(this, SIGNAL(widegtviewActive(WId)), this->parentWidget(), SLOT(widgetviewActive(WId)));
}

void hnWidget3DView::setMousePointF(QPointF& pointf)
{
	m_mouse_posf = pointf;
	emit mouseMoveF(&m_mouse_posf);
}

// 本地视图消息重构
bool hnWidget3DView::nativeEvent(const QByteArray& eventType,void* message,long* result)
{
	// 接收所有windows消息并发送至底层处理
	if (eventType == "windows_generic_MSG")
	{
		// 消息类型转换
		MSG* pMsg = (MSG*)message;
		{
			hnApplication::getApp()->WindowProc((HWND)this->winId(),pMsg->message,pMsg->wParam,pMsg->lParam);
		}

		//处理内置命令发送的消息
		//buildInCommandMsgProc(pMsg->message,pMsg->wParam,pMsg->lParam);

		return QWidget::nativeEvent(eventType,message,result);
	}

	return QWidget::nativeEvent(eventType,message,result);
}

// 鼠标中键盘滚轮事件,负责视图滚轮处理
void hnWidget3DView::wheelEvent(QWheelEvent *event)
{
	// 得到鼠标滚动方向值
	int ndelta = event->delta();
	WPARAM wParam  = (DWORD_PTR)((ndelta & 0xffff)<<16);
	// 得到鼠标所在位置屏幕坐标
	int global_x = event->globalX();
	int global_y = event->globalY();
	LPARAM lParam = (DWORD_PTR)(((global_y & 0xffff)<<16) | (global_x & 0xffff));
	// 模仿window过程-处理过程无法中断，换为直接调用底层工具进行处理
	//hdApp::getAppInstance()->windowProc((HWND)this->winId(),WM_MOUSEWHEEL,wParam,lParam);

	// 视图角点
	POINT pt_view; 
	pt_view.x = 0; pt_view.y = 0;

	// 得到视图左上角原点屏幕坐标进行偏移
	ClientToScreen((HWND)this->winId(), &pt_view);
	global_x -= pt_view.x;
	global_y -= pt_view.y;

	// 直接调用当前工具进行三维浏览（此处是hdToolfly）
	hnApplication::getApp()->GetCurrentTool()->OnMouseWheel(1,ndelta,global_x,global_y);

	// 事件处理完毕
	event->accept();
}

// 键盘按下事件
void hnWidget3DView::keyPressEvent(QKeyEvent* e)
{
	//switch(e->key())
	//{
	//case Qt::Key_Up:
	//	{
	//		hnApplication::getApp()->m_pScanList->PrevPano();
	//		break;
	//	}
	//case Qt::Key_Down:
	//	{
	//		hnApplication::getApp()->m_pScanList->NextPano();
	//		break;
	//	}
	//case Qt::Key_Left:
	//	{
	//		hnApplication::getApp()->m_pScanList->PrevPano();
	//		break;
	//	}
	//case Qt::Key_Right:
	//	{
	//		hnApplication::getApp()->m_pScanList->NextPano();
	//		break;
	//	}
	//}
}

// 键盘松开事件
void hnWidget3DView::keyReleaseEvent(QKeyEvent* e)
{

}

void hnWidget3DView::mousePressEvent(QMouseEvent *event)
{
	// 鼠标位置str
	QString str = "(" + QString::number(event->x()) + "," + QString::number(event->y()) + "+";

	if (event->button() == Qt::LeftButton)         	// 鼠标左键按下
	{
		this->setCursor(Qt::PointingHandCursor);

		// 设置当前活动视图;
		emit widegtviewActive(this->winId());
	}
	else if (event->button() == Qt::MidButton)     	// 鼠标中键按下
	{
		this->setCursor(Qt::OpenHandCursor);
	}
	else if (event->button() == Qt::RightButton)    // 鼠标右键按下
	{

	}

	//testSplitPointCloud(event->x(),event->y());
}

void hnWidget3DView::mouseMoveEvent(QMouseEvent *event)
{
	POINT pt;
	pt.x = event->x();
	pt.y = event->y();
	//QPointF ptF = event->screenPos();

	QPointF pta = event->pos();
	setMousePointF(pta);

	if (event->button() == Qt::LeftButton)         	// 鼠标左键按下
	{
		this->setCursor(Qt::PointingHandCursor);
	}
	else if (event->button() == Qt::MidButton)     	// 鼠标中键按下
	{
		this->setCursor(Qt::OpenHandCursor);
	}
	else if (event->button() == Qt::RightButton)    // 鼠标右键按下
	{

	}
}
