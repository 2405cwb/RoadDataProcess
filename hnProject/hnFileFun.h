#ifndef HNIO_H
#define HNIO_H

#include "hnproject_global.h"
#include <iostream>
#include <vector>

using namespace std;

namespace hnPro
{
	//解决乱码问题 qstr2str
	string HNPROJECT_EXPORT qstr2str(QString qstr);

	//中文 str2qstr
	QString HNPROJECT_EXPORT str2qstr(string str);

	//长路径转最后路径
	void HNPROJECT_EXPORT fullPath2LastPath(QString &qstrFullPath);

	//长路径剔除最后路径
	void HNPROJECT_EXPORT fullPathEraseLastPath(QString &qstrFullPath);

	//是否为Float类型
	bool HNPROJECT_EXPORT isFloatFromQString(const QString& src);

	//是否为数字
	bool HNPROJECT_EXPORT isNumberFromQString(const QString& src);

	//是否为int
	bool HNPROJECT_EXPORT isIntFromQString(const QString& src);

	//Txt2BegMile
	double HNPROJECT_EXPORT getBegMileByTxt(const QString& src);

	//Txt2EndMile
	double HNPROJECT_EXPORT getEndMileByTxt(const QString& src);

	//Image2BegMile
	double HNPROJECT_EXPORT getBegMileByImage(const QString& src);

	//Image2EndMile
	double HNPROJECT_EXPORT getEndMileByImage(const QString& src);

	//Image2Ttx nType 0 OriMileage 1 ModifyMileage
	QString HNPROJECT_EXPORT image2Txt(const QString& src, int nType);

	//txt2Image 
	QString HNPROJECT_EXPORT txt2Image(const QString& src, bool bRGB);

	//Txt2TreeName
	QString HNPROJECT_EXPORT txt2TreeName(const QString& src);

	//保留小数点后3位有效数字
	void HNPROJECT_EXPORT saveSignificantFigures(double& dVal,int nSignificantFigures=3);

	//里程转公里 4位
	QString HNPROJECT_EXPORT mile2Kilometre(double dInput_Mileage);

	//CP3用qstring3位表示
	QString HNPROJECT_EXPORT cp3ID2QstringHundred(int nCp3Id);

	//id用qstring3位表示
	QString HNPROJECT_EXPORT id2QstringHundred(int nId);

	//id用qstring6位表示
	QString HNPROJECT_EXPORT id2QstringMillionHundred(int nId);
}

#endif // HNIO_H
