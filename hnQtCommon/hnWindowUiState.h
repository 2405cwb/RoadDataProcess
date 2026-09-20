#pragma once
#include <QWidget>
#include <QEvent>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QApplication>
#include <QDesktopWidget>

// 仅保存窗口尺寸；不保存工程数据，也不在显示后移动窗口。
class hnWindowUiState : public QObject
{
public:
    hnWindowUiState(QWidget* window, const QString& key, const QString& fileName = QString())
        : QObject(window), m_window(window), m_key(key), m_loaded(false), m_fileName(fileName)
    {
        if (m_fileName.isEmpty())
        {
            const QString folder = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                + QStringLiteral("/hnRoadDataProcess");
            QDir().mkpath(folder);
            m_fileName = folder + QStringLiteral("/UiWindows.ini");
        }
        m_window->installEventFilter(this);
    }
protected:
    bool eventFilter(QObject* object, QEvent* event) override
    {
        if (object == m_window && event->type() == QEvent::Show && !m_loaded)
        {
            m_loaded = true;
            QSettings settings(m_fileName, QSettings::IniFormat);
            const QSize size = settings.value(m_key + QStringLiteral("/Size"), m_window->size()).toSize();
            const QSize available = QApplication::desktop()->availableGeometry(m_window).size() - QSize(32, 80);
            if (size.isValid()) m_window->resize(size.boundedTo(available).expandedTo(m_window->minimumSize()));
        }
        if (object == m_window && event->type() == QEvent::Hide && m_loaded && !m_window->isMaximized())
        {
            QSettings settings(m_fileName, QSettings::IniFormat);
            settings.setValue(m_key + QStringLiteral("/Size"), m_window->size());
        }
        return QObject::eventFilter(object, event);
    }
private:
    QWidget* m_window;
    QString m_key;
    bool m_loaded;
    QString m_fileName;
};
