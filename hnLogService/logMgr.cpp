#include "logMgr.h"
#include <QCoreApplication>
#include "../hnQtCommon/MyCommonMethods.h"

logMgr::logMgr(QObject *parent)
{
    Q_UNUSED(parent);
	
	this->initlogMgr();
}

logMgr *logMgr::instance()
{
    static logMgr l;
    return &l;
}
//1.判断日志文件大小  2.设置日志格式  3.判断日志输出方向输出日志
void logMgr::writeLog(QString msg)
{
    this->checkLogSize();

    this->formatLog(msg);

    this->checkOutDirection(msg);

}

QString logMgr::getFileName()
{
    QString relativePath =  this->getValueDft("LOG","LOG_FILE_PATH","/home/ecoman/log/debug.log");

	 QString cfgPath= MyCommonMethods::GetUserPath() + relativePath;
	// return QCoreApplication::applicationDirPath() + relativePath;
	 return cfgPath;
}

void logMgr::getOutDir()
{
    //获取日志输出方向
    m_outDirection = this->getValueDft("LOG","LOG_OUTPUT_DIRECTION","File");
}

void logMgr::getLogMaxSize()
{
    //获取日志最大大小
    m_maxLogSize = this->getValueDft("LOG","LOG_FILE_MAX_SIZE","4096").toInt(nullptr,10)*1024;
}

void logMgr::initlogMgr()
{
	QString logCfgPath = QCoreApplication::applicationDirPath() + CFG_PATH;

    this->loadCfgFile(logCfgPath);

    this->getOutDir();

    this->getLogMaxSize();

    this->setLogFileName(this->getFileName());

    this->openLogFile();

}

void logMgr::checkLogSize()
{
	m_logSize = m_logFile.size();

    if(m_maxLogSize <= m_logSize)//存在且日志文件超过@MAXLOGFILESIZE@则删除重建
    {
        std::cout << "log file out of range,remove and build new one" << std::endl;
        this->m_logFile.close();
        //m_logFlie.close();
        //delate file
        m_logFile.remove();
        if(!m_logFile.open(QIODevice::ReadWrite | QIODevice::Append))
        {
            cout << "log file open failed" << endl;
        }   
    }
}

void logMgr::checkOutDirection(QString msg)
{
    if(m_outDirection.indexOf("Console")>=0)
    {
        this->logToConsole(msg);
    }
    else if(m_outDirection.indexOf("File")>=0)
    {
        this->logToFile(msg);
        m_logSize += msg.size();
    }
    else if(m_outDirection.indexOf("Net")>=0)
    {
        this->logToTcp(msg,m_outDirection.section(":",1,1),m_outDirection.section(":",2,2).toUInt());
    }
    else
    {
        this->logToFile(msg);
        m_logSize += msg.size();
    }
}

void logMgr::formatLog(QString &msg)
{
    msg = QString(QDATETIMS) + ":" + msg;
}
