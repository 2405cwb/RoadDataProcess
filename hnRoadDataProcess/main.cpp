//#define  _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

#include "hnRoadDataProcess.h"
#include <QtWidgets/QApplication>
#include "logMgr.h"
#include <windows.h>
//
//
//#ifdef _DEBUG
//#ifndef DBG_NEW
//#define  DBG_NEW new(_NORMAL_BLOCK,__FILE__,__LINE__)
//#define  new DBG_NEW
//#endif
//#endif // DEBUG


using namespace std;
void logOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	static thread_local bool isWritingLog = false;
	if (isWritingLog)
	{
		return;
	}

	//过滤掉libping的警告
	if (msg.contains("libpng"))	return;

	isWritingLog = true;
	try
	{
		QString text;

		//加入debug类型
		switch (type)
		{
		case QtInfoMsg:
			text.append("[Info]");
			break;

		case QtDebugMsg:
			text.append("[Debug]");
			break;

		case QtWarningMsg:
			text.append("[Warning]");
			break;

		case QtCriticalMsg:
			text.append("[Critical]");
			break;

		case QtFatalMsg:
			text.append("[Fatal]");
		}

		if (context.file != nullptr && context.line > 0)
		{
			text += QString("[%1:%2]").arg(QString::fromLocal8Bit(context.file)).arg(context.line);
		}

		//加入debug信息
		text += msg;

		//调用日志模块记录日志
		logMgr::instance()->writeLog(text);
	}
	catch (...)
	{
		//日志系统不能反过来导致 Qt 消息处理崩溃。
	}
	isWritingLog = false;

}
int main(int argc, char *argv[])
{
	SetProcessDPIAware();
	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
	//开启qt的高
	//安装消息过滤器  记录日志用
	qInstallMessageHandler(logOutput);
	//qDebug() << "WebEngine Version" <<QWebEngineProfile::defaultProfile()->httpUserAgent();
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);


	try { 
    QApplication a(argc, argv);

	//从文件中读取qss，应用
	QString qssFileName = QApplication::applicationDirPath() + "/config/blue.css";
	QFile qssFile(qssFileName);
	qssFile.open(QIODevice::ReadOnly);
	QString qss = qssFile.readAll();
	a.setStyleSheet(qss);
	qssFile.close();


	//主窗口启动
    hnRoadDataProcess* w  = new hnRoadDataProcess();
    w->show();

	 a.setWindowIcon(QIcon(":/icons/iconsNew/logo_xroe.ico"));
	//_CrtDumpMemoryLeaks();

	return a.exec();
	}
	catch (const std::exception& e)
	{
		auto waht =QString::fromLocal8Bit(  e.what());
		auto msg = QString::fromLocal8Bit("未捕获的异常导致程序退出\n\n%1").arg(QString::fromLocal8Bit(e.what()));
		auto msg1 = msg.toStdWString();
		::MessageBoxW(nullptr, msg1.c_str(), L"错误信息", MB_ICONERROR | MB_OK);
	//	QMessageBox::critical(nullptr, QString::fromLocal8Bit( "程序崩溃"), );
	}
	catch (...)
	{
		 
		::MessageBoxW(nullptr, L"未知异常,程序被迫退出。",L"程序崩溃", MB_ICONERROR | MB_OK);
	}
}
