#pragma once

#include <QObject>

class test_LineCameraConfig : public QObject
{
	Q_OBJECT

private slots:
	void readsConfigAndCalculatesWidths();
	void rejectsInvalidScaleAndKeepsCamera1AsInterface();
	void calculatesCenteredBoundsFromRoadWidth();
};
