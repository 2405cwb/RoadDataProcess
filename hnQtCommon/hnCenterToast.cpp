#include "hnCenterToast.h"

#include <QEvent>
#include <QWidget>
 
hnCenterToast::hnCenterToast(QWidget *parent /*= nullptr*/):QLabel(parent)
{
	setAttribute(Qt::WA_TransparentForMouseEvents, true);
	setAttribute(Qt::WA_ShowWithoutActivating, true);

	setAlignment(Qt::AlignCenter);
	setWordWrap(true);
	setTextFormat(Qt::RichText);

	setStyleSheet(R"(
        QLabel {
            background-color: rgba(25, 25, 25, 210);
            color: white;
            border-radius: 14px;
            padding: 18px 28px;
            font-family: Microsoft YaHei;
            font-size: 16px;
        }
    )");

	m_opacityEffect = new QGraphicsOpacityEffect(this);
	setGraphicsEffect(m_opacityEffect);
	m_opacityEffect->setOpacity(0.0);

	m_animation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
	m_animation->setDuration(180);

	m_hideTimer = new QTimer(this);
	m_hideTimer->setSingleShot(true);

	connect(m_hideTimer, &QTimer::timeout, this, &hnCenterToast::fadeOut);

	connect(m_animation, &QPropertyAnimation::finished, this, [this]()
	{
		if (m_animation->endValue().toDouble() <= 0.0)
		{
			hide();
		}
	});

	if (parent)
	{
		parent->installEventFilter(this);
	}

	hide();
}

void hnCenterToast::showMessage(const QString& title,
	const QString& content,
	int durationMs)
{
	if (parentWidget())
	{
		int maxWidth = parentWidget()->width() * 0.6;
		maxWidth = qBound(320, maxWidth, 620);
		setMaximumWidth(maxWidth);
	}

	QString html;

	html += QString(R"(
        <div style="font-size:20px;font-weight:600;margin-bottom:8px;">
            %1
        </div>
    )").arg(title.toHtmlEscaped());

	if (!content.isEmpty())
	{
		html += QString(R"(
            <div style="font-size:14px;color:#dddddd;line-height:1.6;">
                %1
            </div>
        )").arg(content.toHtmlEscaped().replace("\n", "<br>"));
	}

	setText(html);
	adjustSize();
	moveToCenter();

	raise();
	show();

	m_hideTimer->stop();
	m_animation->stop();

	m_opacityEffect->setOpacity(0.0);
	m_animation->setDuration(160);
	m_animation->setStartValue(0.0);
	m_animation->setEndValue(1.0);
	m_animation->start();

	m_hideTimer->start(durationMs);
}

hnCenterToast::~hnCenterToast()
{

}

void hnCenterToast::fadeOut()
{
	m_animation->stop();
	m_animation->setDuration(260);
	m_animation->setStartValue(m_opacityEffect->opacity());
	m_animation->setEndValue(0.0);
	m_animation->start();
}

void hnCenterToast::moveToCenter()
{
	QWidget* p = parentWidget();
	if (!p)
		return;

	int x = (p->width() - width()) / 2;
	int y = (p->height() - height()) / 2;

	move(x, y);
}

bool hnCenterToast::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == parentWidget() && event->type() == QEvent::Resize)
	{
		moveToCenter();
	}

	return QLabel::eventFilter(watched, event);
}