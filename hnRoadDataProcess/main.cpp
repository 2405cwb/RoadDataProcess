//#define  _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

#include "hnRoadDataProcess.h"
#include <QtWidgets/QApplication>
#include "logMgr.h"
#include <windows.h>
#include <QDateTime>
#include <QAbstractEventDispatcher>
#include "../hnQtCommon/hnProgressStyle.h"
//
//
//#ifdef _DEBUG
//#ifndef DBG_NEW
//#define  DBG_NEW new(_NORMAL_BLOCK,__FILE__,__LINE__)
//#define  new DBG_NEW
//#endif
//#endif // DEBUG



namespace
{
	// Qt 5.8 必须在 QApplication 创建前把进程设为逐显示器 DPI 感知。
	// 原来的 SetProcessDPIAware 只按主屏缩放工作，跨 150%/100% 屏幕后会产生全局鼠标坐标偏移。
	bool hnEnablePerMonitorDpiAwareness()
	{
		typedef HRESULT(WINAPI* SetProcessDpiAwarenessFn)(int);
		HMODULE shcore = ::LoadLibraryW(L"shcore.dll");
		if (shcore)
		{
			SetProcessDpiAwarenessFn setProcessDpiAwareness =
				reinterpret_cast<SetProcessDpiAwarenessFn>(::GetProcAddress(shcore, "SetProcessDpiAwareness"));
			if (setProcessDpiAwareness)
			{
				// PROCESS_PER_MONITOR_DPI_AWARE = 2
				const HRESULT result = setProcessDpiAwareness(2);
				::FreeLibrary(shcore);
				if (SUCCEEDED(result) || result == E_ACCESSDENIED)
				{
					return true;
				}
			}
			else
			{
				::FreeLibrary(shcore);
			}
		}

		// Windows 7 等不支持逐显示器 DPI 的系统保留旧兜底。
		return ::SetProcessDPIAware() != FALSE;
	}

}
using namespace std;
void logOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	//过滤掉libping的警告
	if (msg.contains("libpng"))	return;

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

	//加入debug信息
	text += msg;

	//调用日志模块记录日志
	logMgr::instance()->writeLog(text);

}

int main(int argc, char *argv[])
{
	hnEnablePerMonitorDpiAwareness();
	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
	//开启qt的高
	//安装消息过滤器  记录日志用
	qInstallMessageHandler(logOutput);
	//qDebug() << "WebEngine Version" <<QWebEngineProfile::defaultProfile()->httpUserAgent();
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);


	try {
    QApplication a(argc, argv);
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][AppStart]" << "time=" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
	#endif

	//从文件中读取qss，应用
	QString qssFileName = QApplication::applicationDirPath() + "/config/blue.css";
	QFile qssFile(qssFileName);
	qssFile.open(QIODevice::ReadOnly);
	QString qss = qssFile.readAll();
	a.setStyleSheet(qss + hnProgressStyle::styleSheet());
	qssFile.close();


	//主窗口启动
    // Destroy WebEngine widgets before QApplication shuts down its global context.
    hnRoadDataProcess w;
    w.show();

	 a.setWindowIcon(QIcon(":/icons/iconsNew/logo_xroe.ico"));
	//_CrtDumpMemoryLeaks();

	const int exitCode = a.exec();
	// Stop routing Qt/Chromium shutdown messages into the process-lifetime
	// logger before QApplication and the main window start destructing.
	qInstallMessageHandler(nullptr);
	return exitCode;
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
