#pragma once

#include <QObject>

class test_RutProfileDebugExporter : public QObject
{
	Q_OBJECT

private slots:
	void parsesPileText();
	void convertsDmiToFrameBoundaries();
};
