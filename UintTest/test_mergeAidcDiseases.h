#pragma once

#include <QObject>

class test_mergeAidcDiseases : public QObject
{
	Q_OBJECT

public:
	test_mergeAidcDiseases(QObject *parent = nullptr);
	~test_mergeAidcDiseases();

private slots:
	void isMergeableDisease_true();
};
