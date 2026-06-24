#include "logService.h"
#include <QFileInfo>
#include <QDir>
logService::logService(QObject *parent) : QObject(parent)
{

}

void logService::logToFile(QString msg)
{
    QMutexLocker locker(&m_mtx);

    QWriteLocker writelock(&rwlock);
    //写入
    QTextStream stream(&m_logFile);
    stream.setCodec("utf-8");
    //m_logFile.write(msg.toLatin1());
    //写入到文件中时，要加换行
    stream << msg.toUtf8() << endl;
    //刷新
    m_logFile.flush();
}

void logService::logToConsole(const QString msg)
{
    //cout如果想打印中文，要打印std:string类型的变量
    std::cout << msg.toStdString() << std::endl;
}

void logService::logToTcp(QString msg, const QString host, const quint16 port)
{
    if(msg.indexOf("\r\n")<0)
        msg += "\r\n";
    if(m_socket == nullptr)
    {
        m_socket = new QTcpSocket;
        m_socket->connectToHost(host,port);
        m_socket->waitForConnected();
    }
    if(m_socket->isValid())
    {
        //通过TCP方式发送日志
        m_socket->write(msg.toLatin1().data());
        m_socket->flush();
    }
    else {
        m_socket->disconnectFromHost();
        m_socket->close();
        delete m_socket;
        m_socket = nullptr;
    }
}

void logService::openLogFile()
{
    if(!m_logFile.exists() || !m_logFile.isOpen())
    {
        if(!m_logFile.open(QIODevice::ReadWrite | QIODevice::Append))
        {
            std::cout << "log file open failed" << std::endl;
        }
    }
}

void logService::setLogFileName(const QString name)
{
    m_logFile.setFileName(name);
	QFileInfo fileInfo(name);
	QDir dir = fileInfo.dir();
	if (!dir.exists())
	{
		if (!dir.mkpath("."))
		{
			
		} 
	}
	if (!m_logFile.open(QIODevice::WriteOnly |QIODevice::Text))
	{
		int a = 0;
	}
	 
}
