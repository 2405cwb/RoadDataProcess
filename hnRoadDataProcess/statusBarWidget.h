#pragma once

#include <QLabel>
#include <QMap>
#include <QWidget>
#include <QApplication>
class statusBarWidget : public QLabel
{
	Q_OBJECT
		/*枚举 状态栏显示类型*/
		enum statusType
	{
		MOUSE_FRAME_IDX,	//当前鼠标位置帧数
		MOUSE_POS,			//鼠标位置坐标
	};
public:
	statusBarWidget(QWidget *parent = Q_NULLPTR);
	~statusBarWidget();

	public slots:
	//更新label文字内容
	void updateLabelTextSlot();
	//更新label文字内容
	void updateLabelTextSlot(const QString &text);

	/*信号槽连接*/
private:
	void connectSignalSlot();

	/*信号*/
signals:

	/*槽函数*/
	public slots :

		/*类自用功能函数*/
private:

	/*初始化*/
private:
	void init();

	/*释放内存*/
private:
	void releaseMemory();

	/*成员变量*/
private:
	QMap<statusType, QString> m_statusInfo;
	
};
