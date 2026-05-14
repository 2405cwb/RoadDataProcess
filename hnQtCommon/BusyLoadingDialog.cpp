
#include "BusyLoadingDialog.h"

#include <QApplication>
#include <QEventLoop>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QScreen>
#include <QShowEvent>
#include <QVBoxLayout>

BusyLoadingDialog::BusyLoadingDialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::CustomizeWindowHint);
	setWindowModality(Qt::ApplicationModal);
	setModal(true);

	setAttribute(Qt::WA_TranslucentBackground, true);
	setAttribute(Qt::WA_ShowWithoutActivating, false);

	setFixedSize(380, 150);

	QVBoxLayout* rootLayout = new QVBoxLayout(this);
	rootLayout->setContentsMargins(12, 12, 12, 12);

	QFrame* panel = new QFrame(this);
	panel->setObjectName("loadingPanel");

	QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(panel);
	shadow->setBlurRadius(28);
	shadow->setOffset(0, 8);
	shadow->setColor(QColor(60, 150, 210, 90));
	panel->setGraphicsEffect(shadow);

	QVBoxLayout* panelLayout = new QVBoxLayout(panel);
	panelLayout->setContentsMargins(28, 24, 28, 24);
	panelLayout->setSpacing(14);

	QLabel * iconLabel = new QLabel(this);
	iconLabel->setFixedSize(12, 12);
	iconLabel->setStyleSheet(R"(
		QLabel{
background-color: #25A9E0;
border-radius: 6px;}
)");

	m_titleLabel = new QLabel("请稍候", panel);
	m_titleLabel->setObjectName("titleLabel");
	m_titleLabel->setAlignment(Qt::AlignCenter);

	m_messageLabel = new QLabel("正在处理，请稍候……", panel);
	m_messageLabel->setObjectName("messageLabel");
	m_messageLabel->setAlignment(Qt::AlignCenter);
	m_messageLabel->setWordWrap(true);

	m_progressBar = new QProgressBar(panel);
	m_progressBar->setTextVisible(false);
	m_progressBar->setRange(0, 0); // 关键：忙碌模式，不需要设置百分比
	m_progressBar->setFixedHeight(8);


	QHBoxLayout * titleLayout = new QHBoxLayout();
	titleLayout->setContentsMargins(0, 0, 0, 0);
	titleLayout->setSpacing(8);
	titleLayout->addStretch();
	titleLayout->addWidget(iconLabel);
	titleLayout->addWidget(m_titleLabel);
	titleLayout->addStretch();

	panelLayout->addLayout(titleLayout);


	panelLayout->addWidget(m_messageLabel);
	panelLayout->addSpacing(4);
	panelLayout->addWidget(m_progressBar);

	rootLayout->addWidget(panel);

	setStyleSheet(R"(
        QFrame#loadingPanel {
            background-color: rgba(245,251,255,248);
			border: 1px solid rgba(120,190,235,160);
            border-radius: 16px;
        }

        QLabel#titleLabel {
            color: #0B5C9D;
            font-family: Microsoft YaHei;
            font-size: 18px;
            font-weight: 600;
        }

        QLabel#messageLabel {
            color: #2E5E7E;
            font-family: Microsoft YaHei;
            font-size: 14px;
        }

        QProgressBar {
            border: none;
            background-color: rgba(180,220,245,120);
            border-radius: 4px;
			height:8px;
        }

        QProgressBar::chunk {
            background-color: #25A9E0;
            border-radius: 4px;
        }
    )");
}

void BusyLoadingDialog::setTitleText(const QString& title)
{
	if (m_titleLabel)
	{
		m_titleLabel->setText(title);
	}

	processUiEvents();
}

void BusyLoadingDialog::setMessage(const QString& message)
{
	if (m_messageLabel)
	{
		m_messageLabel->setText(message);
	}

	processUiEvents();
}

void BusyLoadingDialog::showLoading(const QString& message)
{
	if (!message.isEmpty())
	{
		setMessage(message);
	}

	centerToParent();
	show();
	raise();
	activateWindow();

	processUiEvents();
}

void BusyLoadingDialog::showEvent(QShowEvent* event)
{
	QDialog::showEvent(event);
	centerToParent();
}

void BusyLoadingDialog::centerToParent()
{
	QRect targetRect;

	QWidget* p = parentWidget();
	if (p)
	{
		targetRect = p->geometry();
	}
	else if (QApplication::primaryScreen())
	{
		targetRect = QApplication::primaryScreen()->availableGeometry();
	}
	else
	{
		return;
	}

	move(targetRect.center() - rect().center());
}

void BusyLoadingDialog::processUiEvents()
{
	// 让提示框先绘制出来，并更新文字。
	// ExcludeUserInputEvents 可以避免用户在加载过程中乱点界面。
	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
}


// ================= BusyLoadingGuard =================

BusyLoadingGuard::BusyLoadingGuard(QWidget* parent,
	const QString& title,
	const QString& message)
{
	m_dialog = new BusyLoadingDialog(parent);
	m_dialog->setTitleText(title);
	m_dialog->showLoading(message);
}

BusyLoadingGuard::~BusyLoadingGuard()
{
	if (m_dialog)
	{
		m_dialog->close();
		BusyLoadingDialog::processUiEvents();

		delete m_dialog;
		m_dialog = nullptr;
	}
}

void BusyLoadingGuard::setTitleText(const QString& title)
{
	if (m_dialog)
	{
		m_dialog->setTitleText(title);
	}
}

void BusyLoadingGuard::setMessage(const QString& message)
{
	if (m_dialog)
	{
		m_dialog->setMessage(message);
	}
}