#ifndef LOGMGR_H
#define LOGMGR_H
//管理层模块
#include <QObject>
#include <QDateTime>
#include "logService.h"
#include "hnlogservice_global.h"
#define QDATETIMS qPrintable(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))

//注意：此路径是可执行文件路径后面接的路径
#define CFG_PATH "/config/log_mgr_cfg.ini"

using namespace std;

class HNLOGSERVICE_EXPORT logMgr : public logService
{
public:
    explicit logMgr(QObject *parent = 0);

    static logMgr* instance();
public:
    //日志模块初始化
    void initlogMgr();

    //写日志
    void writeLog(QString msg);
public:

    //获取日志文件绝对路径
    QString getFileName();

    //获取日志模块输出方向
    void getOutDir();

    //获取日志文件最大大小
    void getLogMaxSize();

    //检查文件大小
    void checkLogSize();

    //检查输出方向，并输出
    void checkOutDirection(QString msg);

    //按指定格式输出日志
    void formatLog(QString &msg);
private:
    //日志输出方向
    QString m_outDirection;

    //日志文件大小
    int m_logSize = 0;

    //日志文件最大大小 单位:Byte 根据需要自行换算
    int m_maxLogSize;
};

#endif // LOGMGR_H
