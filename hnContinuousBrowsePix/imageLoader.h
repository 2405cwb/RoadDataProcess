#pragma once

#include <QObject>
#include <QMap>
#include <QFileInfoList>
//加载图片的类
class imageLoader : public QObject
{
	Q_OBJECT
public:
	imageLoader(QObject *parent = Q_NULLPTR);

public:
	//key 帧序号 从1开始  value:图片的绝对路径 图片支持jpg、jpeg
	QMap<int, QString> loadImageNamesToMap(const QString &pixDirName, const bool isReverse = false);
	QMap<int, QString> loadImageNamesToMap(const QStringList &pixNames, QMap< QString,int>& data);

private:
	QMap<int, QString> putPixNamesIntoMap(const QFileInfoList& pixInfoList);

};
