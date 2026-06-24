#include "dxfExcelInfo.h"

dxfExcelInfo::dxfExcelInfo(QObject *parent)
	: QObject(parent)
{
}

dxfExcelInfo::~dxfExcelInfo()
{
}

int dxfExcelInfo::getExcelMiles(const QString & fileName, int & beginMile, int & endMile)
{
	beginMile = 0;
	endMile = 0;

	Document docment(fileName);
	if (false == docment.load())
	{
		return -1;
	}
	if (false == docment.selectSheet(QString::fromLocal8Bit("工程信息")))
	{
		return -2;
	}
	const auto beginValue = docment.cellAt(7, 2)->readValue();
	if (false == beginValue.isValid())
	{
		return -3;
	}
	const auto endValue = docment.cellAt(16, 2)->readValue();
	if (false == endValue.isValid())
	{
		return -4;
	}
	const QString begin = beginValue.toString();
	const QString end = endValue.toString();

	beginMile = begin.toInt();
	endMile = end.toInt();

	return 0;
}

int dxfExcelInfo::kMileToDoubleMile(const QString & kMile)
{
	int mile = 0;
	const QStringList list = kMile.split('+');
	if (2 != list.size())
	{
		return mile;
	}
	if (false == list.at(0).contains("K"))
	{
		return mile;
	}
	mile = list.at(0).toInt() * 1000 + list.at(0).toInt();
	return mile;
}
