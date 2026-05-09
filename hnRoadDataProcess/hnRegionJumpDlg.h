#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QValidator>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialog>
#include <QRadioButton>


class hnRegionJumpDlg : public QDialog
{
	Q_OBJECT

public:
	hnRegionJumpDlg(QWidget *parent = Q_NULLPTR);
	~hnRegionJumpDlg();

signals:
	void signal_updateScrollValue(int value);
signals:
	void signal_road3dFrameIdxChanged(int frameidx);

public:
	//获取编码器里程
	double getRegion();

private:
	void initUI();

	void initLayout();

private:
	void slot_onJumpPushButtonClicked();

private:
	QRadioButton *m_trueMileRadioButton;
	QRadioButton *m_encoderMileRadioButton;
	QPushButton *m_jumpPushButton;
	QLineEdit *m_regionLineEdit;

	//编码器里程
	double m_region;
};
