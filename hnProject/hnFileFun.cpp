#include "hnFileFun.h"
#include <QByteArray>
#include <QStringList>

namespace hnPro
{
	//解决乱码问题 qstr2str
	string qstr2str(QString qstr)
	{
		QByteArray cdata = qstr.toLocal8Bit();
		return string(cdata);
	}

	//中文 str2qstr
	QString str2qstr(string str)
	{
		return QString::fromLocal8Bit(str.data());
	}

	//长路径转最后路径
	void fullPath2LastPath(QString &qstrFullPath)
	{
		//没有则不改变 直接返回
		qstrFullPath = qstrFullPath.replace("/", "\\");
		qstrFullPath = qstrFullPath.replace("//", "\\");
		if (qstrFullPath.contains("\\"))
		{
			QStringList qstrList = qstrFullPath.split("\\");
			qstrFullPath = qstrList[qstrList.size() - 1];
		}

	}

	//长路径剔除最后路径
	void fullPathEraseLastPath(QString &qstrFullPath)
	{
		//没有则不改变 直接返回
		qstrFullPath = qstrFullPath.replace("/", "\\");
		qstrFullPath = qstrFullPath.replace("//", "\\");
		if (qstrFullPath.contains("\\"))
		{
			QStringList qstrList = qstrFullPath.split("\\");
			qstrFullPath = "";
			for (int i = 0; i < qstrList.size() - 1; i++)
			{
				if (i == qstrList.size() - 2)
				{
					qstrFullPath += qstrList[i];
				}
				else
				{
					qstrFullPath += qstrList[i] + "\\";
				}

			}

		}

	}

	//是否为数字
	bool isNumberFromQString(const QString& src)
	{
		const char* s = src.toUtf8().data();

		while (*s && *s >= '0' && *s <= '9')
		{
			s++;
		}

		return !bool(*s);
	}

	//是否为int
	bool isIntFromQString(const QString& src)
	{
		const char* s = src.toUtf8().data();

		if (string(s) == "-")
		{
			return false;
		}

		int nCoutHoriLine = 0;
		while ((*s && *s >= '0' && *s <= '9') || (*s && *s == '-'))
		{
			if (nCoutHoriLine != 0 && (*s && *s == '-'))
			{
				return false;
			}

			nCoutHoriLine++;

			s++;
		}

		return !bool(*s);
	}

	//是否为数字  可以为带小数点数字  可以带负数
	bool isFloatFromQString(const QString& src)
	{
		const char* s = src.toUtf8().data();

		int nCoutPoint = 0;
		int nCoutHoriLine = 0;
		while ((*s && *s == '-') || (*s && *s >= '0' && *s <= '9') || (*s && *s == '.'))
		{
			if (*s && *s == '.')
			{
				nCoutPoint++;
			}

			if (*s && *s == '-')
			{
				nCoutHoriLine++;
			}

			if (nCoutPoint > 1)
			{
				return false;
			}

			if (nCoutHoriLine > 1)
			{
				return false;
			}

			s++;
		}
		return !bool(*s);
	}

	//Txt2BegMile
	double getBegMileByTxt(const QString& src)
	{
		QString temp=src;
		temp.replace(".txt", "");
		
		//替换
		temp.replace("ModifyMileage", "");
		temp.replace("OriMileage", "");
		temp.replace("GRAY", "");

		QStringList list = temp.split("-");

		QString qstrMile = list[0];
		double dMile = qstrMile.toDouble();
		return dMile;
	}

	//Txt2EndMile
	double getEndMileByTxt(const QString& src)
	{
		QString temp = src;
		temp.replace(".txt", "");

		//替换
		temp.replace("ModifyMileage", "");
		temp.replace("OriMileage", "");

		QStringList list = temp.split("-");

		QString qstrMile = list[1];
		double dMile = qstrMile.toDouble();
		return dMile;
	}
	
	//Image2BegMile
	double getBegMileByImage(const QString& src)
	{
		QString temp = src;
		temp.replace(".bmp", "");

		//替换
		temp.replace("GRAY", "");

		//替换
		temp.replace("RGB", "");

		QStringList list = temp.split("-");

		QString qstrMile = list[0];
		double dMile = qstrMile.toDouble();
		return dMile;
	}

	//Image2EndMile
	double getEndMileByImage(const QString& src)
	{
		QString temp = src;
		temp.replace(".bmp", "");

		//替换
		temp.replace("GRAY", "");

		//替换
		temp.replace("RGB", "");

		QStringList list = temp.split("-");

		QString qstrMile = list[1];
		double dMile = qstrMile.toDouble();
		return dMile;
	}

	//Image2Ttx nType 0 OriMileage 1 ModifyMileage
	QString image2Txt(const QString& src,int nType)
	{
		QString temp = src;
		temp.replace("GRAY", "");
		temp.replace("RGB", "");
		temp.replace(".bmp", "");

		temp += ".txt";
		if (nType == 0)
		{
			temp = "OriMileage" + temp;
		}
		else if (nType == 1)
		{
			temp = "ModifyMileage" + temp;
		}

		return temp;
	}

	//txt2Image 
	QString txt2Image(const QString& src, bool bRGB)
	{
		QString qstrTxt = src;
		qstrTxt.replace(".txt", "");
		qstrTxt.replace("OriMileage", "");
		qstrTxt.replace("ModifyMileage", "");

		if (!bRGB)
		{
			qstrTxt = "GRAY" + qstrTxt;
		}
		else if (bRGB)
		{
			qstrTxt = "RGB" + qstrTxt;
		}

		qstrTxt += ".bmp";
		return qstrTxt;
	}

	//Txt2TreeName
	QString txt2TreeName(const QString& src)
	{
		QString temp = src;
		temp.replace("OriMileage", "");
		temp.replace("ModifyMileage", "");
		temp.replace(".txt", "");
		return temp;
	}

	//里程转公里 4位
	QString mile2Kilometre(double dInput_Mileage)
	{
		int nOutput_KM = int(dInput_Mileage / 1000);
		QString qstrOutput_KM;

		//已经四舍五入
		if (nOutput_KM >= 10000)
		{
			//保留后4位
			qstrOutput_KM = QString::number(nOutput_KM);
			qstrOutput_KM = qstrOutput_KM.mid(qstrOutput_KM.length() - 4, 4);
		}
		else if (nOutput_KM >= 1000 && nOutput_KM<10000)
		{
			qstrOutput_KM = QString::number(nOutput_KM);
		}
		else if (nOutput_KM >= 100 && nOutput_KM < 1000)
		{
			qstrOutput_KM = "0" + QString::number(nOutput_KM);
		}
		else if (nOutput_KM < 100 && nOutput_KM >= 10)
		{
			qstrOutput_KM = "00" + QString::number(nOutput_KM);
		}
		else/* if (nOutput_KM < 10)*/
		{
			qstrOutput_KM = "000" + QString::number(nOutput_KM);
		}

		return qstrOutput_KM;
	}

	//CP3用qstring3位表示
	QString cp3ID2QstringHundred(int nCp3Id)
	{
		QString qstrId = "";
		if (nCp3Id >= 1000)
		{
			//保留后3位
			qstrId = QString::number(nCp3Id);
			qstrId = qstrId.mid(qstrId.length() - 3, 3);
		}
		else if (nCp3Id >= 100 && nCp3Id < 1000)
		{
			qstrId = QString::number(nCp3Id);
		}
		else if (nCp3Id < 100 && nCp3Id >= 10)
		{
			qstrId = "0" + QString::number(nCp3Id);
		}
		else/* if (nOutput_KM < 10)*/
		{
			qstrId = "00" + QString::number(nCp3Id);
		}

		return qstrId;
	}

	//id用qstring3位表示
	QString id2QstringHundred(int nId)
	{
		QString qstr = "";
		if (nId >= 1000)
		{
			//保留后3位
			qstr = QString::number(nId);
			qstr = qstr.mid(qstr.length() - 3, 3);
		}
		else if (nId >= 100 && nId<1000)
		{
			qstr = QString::number(nId);
		}
		else if (nId < 100 && nId >= 10)
		{
			qstr = "0" + QString::number(nId);
		}
		else/* if (nOutput_KM < 10)*/
		{
			qstr = "00" + QString::number(nId);
		}

		return qstr;
	}

	//id用qstring6位表示
	QString id2QstringMillionHundred(int nId)
	{
		QString qstr = "";
		if (nId >= 1000000)
		{
			//保留后6位
			qstr = QString::number(nId);
			qstr = qstr.mid(qstr.length() - 6, 6);
		}
		else if (nId < 1000000 && nId >= 100000)
		{
			qstr = QString::number(nId);
		}
		else if (nId < 100000 && nId >= 10000)
		{
			qstr = "0" + QString::number(nId);
		}
		else if (nId < 10000 && nId >= 1000)
		{
			qstr = "00" + QString::number(nId);
		}
		else  if (nId < 1000 && nId >= 100)
		{
			qstr = "000" + QString::number(nId);
		}
		else  if (nId < 100 && nId >= 10)
		{
			qstr = "0000" + QString::number(nId);
		}
		else
		{
			qstr = "00000" + QString::number(nId);
		}

		return qstr;
	}
}

