#pragma once

#include <QDialog>
#include "hnqtcommon_global.h"
class QLabel;
class QProgressBar;

class HNQTCOMMON_EXPORT BusyLoadingDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BusyLoadingDialog(QWidget* parent = nullptr);

	void setTitleText(const QString& title);
	void setMessage(const QString& message);
	void setProgressRange(int minimum, int maximum);
	void setProgressValue(int value);
	void showLoading(const QString& message = QString());

	static void processUiEvents();

protected:
	void showEvent(QShowEvent* event) override;

private:
	void centerToParent();

private:
	QLabel* m_titleLabel = nullptr;
	QLabel* m_messageLabel = nullptr;
	QProgressBar* m_progressBar = nullptr;
};


// RAII 辅助类：创建时显示，析构时自动关闭
class  HNQTCOMMON_EXPORT BusyLoadingGuard
{
	Q_DISABLE_COPY(BusyLoadingGuard)

public:
	explicit BusyLoadingGuard(QWidget* parent,
		const QString& title = "请稍候",
		const QString& message = "正在处理，请稍候……");

	~BusyLoadingGuard();

	void setTitleText(const QString& title);
	void setMessage(const QString& message);
	void setProgressRange(int minimum, int maximum);
	void setProgressValue(int value);

private:
	BusyLoadingDialog* m_dialog = nullptr;
};
