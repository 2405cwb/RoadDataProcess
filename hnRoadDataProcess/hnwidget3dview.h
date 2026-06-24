#ifndef HNWIDGET3DVIEW_H
#define HNWIDGET3DVIEW_H

#include <QObject>
#include <QWidget>
#include "QtWidgets"

class hnWidget3DView : public QDockWidget
{
	Q_OBJECT

public:
	hnWidget3DView(QWidget *parent);
	~hnWidget3DView();

signals:
	//// 鼠标移动信号
	//void mouseMove(POINT* point);

	// 鼠标移动信号(重载);
	void mouseMoveF(QPointF* pointf);

	// 当前活动视图
	void widegtviewActive(WId hwnd);

private:
	// 命令槽注册绑定:注意-命令函数和槽函数需要保证参数类型一致，槽函数参数要小于等于命令函数参数;
	void registerSignalSlots();

	// 设置鼠标位置(重载);
	void setMousePointF(QPointF& pointf);

protected:
	// 本地视图消息重构
	virtual bool nativeEvent(const QByteArray& eventType,void* message,long* result);

	// 鼠标滚轮事件
	virtual void wheelEvent(QWheelEvent *event);
	
	// 键盘按下事件
	virtual void keyPressEvent(QKeyEvent* e);

	// 键盘松开事件
	virtual void keyReleaseEvent(QKeyEvent* e);

	// 鼠标按下事件
	virtual void mousePressEvent(QMouseEvent *event);

	// 鼠标移动事件
	virtual void mouseMoveEvent(QMouseEvent *event);

private:
	// 鼠标位置;
	QPointF m_mouse_posf;
};

#endif // HNWIDGET3DVIEW_H
