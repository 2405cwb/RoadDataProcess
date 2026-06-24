#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIntValidator>


//放大镜参数设置对话框
class hnMagnifySettingDlg : public QDialog
{
	Q_OBJECT

public:
	hnMagnifySettingDlg(QWidget *parent = Q_NULLPTR);

public:
	void getMagnifySetting(int &pixelSize, int &offset, int &magnifynition);
	void setMagnifySetting(const int pixelSize, const int offset, const int magnifynition);

private slots:
	void slot_onOkPushButtonClicked();

	void slot_onCancelPushButtonClicked();

private:
	void initValidator();

private:
	QLabel *m_pixelSizeLabel;			//放大区域边长label
	QLineEdit *m_pixelSizeLineEdit;		//放大区域边长lineEdit
	QLabel *m_offsetLabel;				//放大偏移距离label
	QLineEdit *m_offsetLineEdit;		//放大偏移距离lineEdit
	QLabel *m_magnificationLabel;		//放大倍数label
	QLineEdit *m_magnificationLineEdit;	//放大倍数lineEdit
	QPushButton *m_okPushButton;		//确定按钮
	QPushButton *m_cancelPushButton;	//取消按钮
};
