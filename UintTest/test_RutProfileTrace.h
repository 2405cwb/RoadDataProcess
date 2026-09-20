#pragma once

#include <QObject>

class test_RutProfileTrace : public QObject
{
	Q_OBJECT

private slots:
	void traceDoesNotChangeRutResult();
};
