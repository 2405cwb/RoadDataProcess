#pragma once

#include <QLabel>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include "hnqtcommon_global.h"
class HNQTCOMMON_EXPORT  hnCenterToast : public QLabel
{
	Q_OBJECT

public:
	explicit hnCenterToast(QWidget *parent = nullptr);

	void showMessage(const QString& title, const QString& context = QString(), int durationMs = 1800);

	~hnCenterToast();
protected:
	bool eventFilter(QObject *watched, QEvent *event) override;

private:
	void moveToCenter();
	void fadeOut();
private:
	QTimer * m_hideTimer = nullptr;

	QGraphicsOpacityEffect * m_opacityEffect = nullptr;

	QPropertyAnimation* m_animation = nullptr;
};
