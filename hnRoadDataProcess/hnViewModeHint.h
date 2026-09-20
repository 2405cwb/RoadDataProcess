#pragma once
#include <QLabel>
#include <QEvent>
#include "../hnApplication/hnWorkMode.h"

// 提示不接收鼠标，避免遮挡影像中的绘制和选择操作。
class hnViewModeHint : public QLabel
{
public:
    explicit hnViewModeHint(QWidget* view) : QLabel(view)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setTextFormat(Qt::PlainText);
        setWordWrap(true);
        setStyleSheet(QStringLiteral("QLabel { background: #F1F4F7; color: #303E4B; border: 1px solid #BBC6D1; "
            "padding: 6px; font-family: 'Microsoft YaHei'; font-size: 12px; }"));
        view->installEventFilter(this);
    }
    void updateMode(bool hasProject, hnWorkMode::WorkMode mode, const QString& dataHint = QString())
    {
        QString text;
        if (!hasProject) text = QStringLiteral("尚未打开工程\n请点击底部“打开工程”，或使用工程管理菜单。");
        else
        {
            switch (mode)
            {
            case hnWorkMode::ADD_MODE: text = QStringLiteral("添加病害 · F1"); break;
            case hnWorkMode::DELETE_MODE: text = QStringLiteral("删除病害 · F2：点击病害即删除"); break;
            case hnWorkMode::EDIT_MODE: text = QStringLiteral("编辑病害 · F3：点击修改类型，拖动调整形状"); break;
            case hnWorkMode::MOVE: text = QStringLiteral("移动病害：拖动调整位置"); break;
            case hnWorkMode::MERGE: text = QStringLiteral("合并病害 · F4：依次选择两个病害"); break;
            case hnWorkMode::GET_MILE: text = QStringLiteral("校准里程：选择对应位置"); break;
            case hnWorkMode::ADD_CTRL_POINT: text = QStringLiteral("添加控制点"); break;
            default: text = QStringLiteral("浏览模式"); break;
            }
            if (mode != hnWorkMode::NO_MODE) text += QStringLiteral("\n使用“视图管理 → 结束编辑”返回浏览。");
        }
        if (!dataHint.isEmpty()) text += QStringLiteral("\n") + dataHint;
        if (text != this->text()) setText(text);
        reposition();
    }
protected:
    bool eventFilter(QObject* object, QEvent* event) override
    {
        if (object == parentWidget() && event->type() == QEvent::Resize) reposition();
        return QLabel::eventFilter(object, event);
    }
private:
    void reposition()
    {
        setFixedWidth(qMax(120, qMin(310, parentWidget()->width() - 24)));
        adjustSize();
        move(qMax(0, parentWidget()->width() - width() - 12), 42);
        raise();
    }
};
