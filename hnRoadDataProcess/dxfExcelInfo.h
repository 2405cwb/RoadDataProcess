#pragma once

#include <QObject>
#include "xlsxdocument.h"
#include "xlsxworksheet.h"
QXLSX_USE_NAMESPACE


class dxfExcelInfo : public QObject
{
	Q_OBJECT

public:
	dxfExcelInfo(QObject *parent);
	~dxfExcelInfo();

public:
	/*
	* 函数名：getExcelMiles
	* 函数说明：获取某个excel表中的开始里程和结束里程
	* 参数1：[IN] excel文件的名字
	* 参数2：[OUT] 开始里程
	* 参数2：[OUT] 结束里程
	* 返回值:正常返回0，否则为不正常
	*/
	static int getExcelMiles(const QString &fileName, int &beginMile, int &endMile);

private:
	/*
	* 函数名：kMileToDoubleMile
	* 函数说明：k里程转double里程 比如 k51 + 480 实际为51480
	* 参数1：[IN] k类型的里程 
	* 返回值: 实际里程，异常返回0
	*/
	static int kMileToDoubleMile(const QString &kMile);
};
