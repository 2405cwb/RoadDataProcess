#ifndef LOGSERVICE_H
#define LOGSERVICE_H
//日志底层模块
#define LOG_FILE_NAME "/home/ecoman/log/debug.log"

#include <QObject>
#include <iostream>
#include <QFile>
#include <QTcpSocket>
#include <QWriteLocker>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include "cfgBaseService.h"
#include "hnlogservice_global.h"
class HNLOGSERVICE_EXPORT logService : public QObject,public cfgBaseService
{
    Q_OBJECT
public:
    explicit logService(QObject *parent = 0);

public:
    //写日志到文件  使用之前要先调用setLogFileName和openLogFile
    void logToFile(QString msg);
    //写日志到控制台
    void logToConsole(const QString msg);
    //写日志到tcp客户端
    void logToTcp(QString msg, const QString host, const quint16 port);
public:
    void openLogFile();
    void setLogFileName(const QString name);
protected:
    QFile m_logFile;
    QTcpSocket* m_socket = nullptr;
    //**文件读写锁**//
    QReadWriteLock rwlock;
    //写日志到文件的锁
    QMutex m_mtx;

};

#endif // LOGSERVICE_H
