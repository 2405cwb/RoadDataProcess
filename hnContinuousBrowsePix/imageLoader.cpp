#include "imageLoader.h"
#include <QFileInfo>
#include <QFileInfoList>
#include <QDir>

imageLoader::imageLoader(QObject *parent)
	: QObject(parent)
{
}

QMap<int, QString> imageLoader::loadImageNamesToMap(const QString &pixDirName, const bool isReverse)
{
	QDir dir(pixDirName);
	if (!dir.exists())
	{
		return QMap<int, QString>();
	}

	QStringList filters;
	filters << "*.jpg" << "*.jpeg";
	QFileInfoList pixInfoList = dir.entryInfoList(filters);
	QMap<int, QString> pixNameMap;
	pixNameMap = this->putPixNamesIntoMap(pixInfoList);

	if (isReverse)
	{
		std::reverse(pixNameMap.begin(), pixNameMap.end());
	}

	return pixNameMap;
}

QMap<int, QString> imageLoader::loadImageNamesToMap(const QStringList & pixNames, QMap< QString, int>& data)
{
	QMap<int, QString> pixNameMap;
	int pixCount = 1;
	for (QString pixName :pixNames)
	{
		pixNameMap.insert(pixCount, pixName);
		data.insert(pixName, pixCount);
		pixCount++;
	}
	return pixNameMap;
}

QMap<int, QString> imageLoader::putPixNamesIntoMap(const QFileInfoList& pixInfoList)
{
	QMap<int, QString> pixNameMap;
	QFileInfo fileInfo;
	for (auto idx = 0; idx < pixInfoList.size(); idx++)
	{
		fileInfo = pixInfoList.at(idx);
		QString fileName = fileInfo.absoluteFilePath();
		pixNameMap.insert(idx + 1, fileName);
	}
	return pixNameMap;
}