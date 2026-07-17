#pragma once

#include <QObject>

class test_GeometryCalculation : public QObject
{
	Q_OBJECT
private slots:
	void fitsSignedSlopesDeterministically();
	void aggregatesOneMeterSamplesIntoTenMeters();
	void unwrapsHeadingAndCalculatesCurvature();
	void writesLegacyAndQualityFilesAtomically();
	void integratesRealProjectWhenConfigured();
};
