#pragma once
#include <QString>
#include <QPoint>

struct pixImagePoint
{
	QString pixName;	//图片的名称，绝对路径
	QPoint pixPoint;	//单张图片内点的坐标
	bool operator==(const pixImagePoint &other) const {
		return (pixName == other.pixName&& pixPoint==other.pixPoint);
	}
};