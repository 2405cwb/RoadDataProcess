
#include "CalculationThread.h"
#include "RutProfileDebugExporter.h"
#include <QMenu>
#include <QFile>
#include <QSaveFile>
#include <QTextStream>
#include <vector>
#include <QMessageBox>
#include <QDir>
#include <QApplication>
#include "..\hnConfigService\HnXRSettings.h"
#include <QTextCodec>
#include "../hnQtCommon/MyCommonMethods.h"
#include "../hnAlgorithm/hnComputeCUT.h"
#include "../hnAlgorithm/hnComputeIRI.h"
#include "../hnAlgorithm/FittingFunct.h"
#include <QSettings>
#include <fstream>
#include <filesystem>
#include "../hnQtCommon/MyPoint.h"
#include <QtCore/QtMath>  
#include <array>
#include <QTimer>  
namespace
{
	int RutRand100(int index, int salt)
	{
		uint32_t x =
			0x9E3779B9u ^
			static_cast<uint32_t>(index + 1) * 0x85EBCA6Bu ^
			static_cast<uint32_t>(salt + 1) * 0xC2B2AE35u;

		x ^= x >> 16;
		x *= 0x7FEB352Du;
		x ^= x >> 15;
		x *= 0x846CA68Bu;
		x ^= x >> 16;

		return static_cast<int>(x % 100u);
	}

	float RutNoise(int index, int salt)
	{
		return RutRand100(index, salt) * 0.01f - 0.5f;
	}

	void RemoveBigErrCore(
		float* rutval,
		int length,
		float errorRutTh2,
		int salt)
	{
		if (rutval == nullptr || length <= 0)
		{
			return;
		}

		float sumval = 0.0f;
		float oldval = rutval[0];
		float curval = rutval[0];

		for (int i = 0; i < length; ++i)
		{
			curval = rutval[i];

			if (curval < 0.1f)
			{
				if (i > 0)
				{
					curval =
						sumval / i +
						RutNoise(i, salt);
				}
				else
				{
					curval = oldval;
				}
			}
			else if (
				curval - oldval >=
				errorRutTh2)
			{
				if (oldval >= 0.1f)
				{
					if (i > 0)
					{
						curval =
							sumval / i +
							RutNoise(i, salt);
					}
					else
					{
						curval = oldval;
					}
				}
				else if (curval > errorRutTh2)
				{
					if (i > 0)
					{
						curval =
							sumval / i +
							RutNoise(i, salt);
					}
					else
					{
						curval = 5.0f;
					}
				}
			}

			rutval[i] = curval;
			oldval = curval;
			sumval += curval;
		}
	}
}
CalculationThread::CalculationThread(CalculationType type, int taskId, hnPro::hnProject* project)
	: m_type(type), m_stopRequested(false), currentPorject(project), m_taskID(taskId)
{
	setAutoDelete(false);
	_Setting = HnXRSettings::getInstance();
	m_stdProcessValue = 0;
	m_spdProcessValue = 0;
	m_Project = project;
}

void CalculationThread::StartIRMThread(hnPro::hnProject* pro, const QString iriPath, const QString& daqBasePath, const QString& resamplePath, int side)
{
	bool datasrc = JudgMTDval(iriPath, side);
	auto project2D = pro->get2DProject();

	int effectiveLength =pro->getCurProSetInfo().dLength;

	int markedRoadLength =
		std::abs(project2D->_EndMile - project2D->_StartMile);

	if (markedRoadLength > 0)
	{
		effectiveLength =
			(std::min)(effectiveLength, markedRoadLength);
	}

	GenerateIRI_NEW(
		resamplePath.toStdString(),
		10,
		"resample.txt",
		datasrc,
		effectiveLength);

	emit progressUpdated(
		m_Project,
		m_type,
		90,
		true);
	// ② 然后和 C# 一样执行 AdjustVal
	QString iriFile = daqBasePath + "/IRI_10m.txt";
	AdjustVal(
		iriFile,
		_Setting->ErrorIRI,
		0.001);

	//GenerateIRI_NEW(resamplePath.toStdString(), 10, "resample.txt", datasrc);
	emit progressUpdated(m_Project, m_type, 90, true);
}

void CalculationThread::CheckSetting(hnPro::hnProject* project)
{
	bool IsCopy = false;
	QString rpath = QApplication::applicationDirPath() + "\\Setting\\Project\\Setting.ini";
	bool tempb = QFile::exists(rpath);

	QString dpath = project->get2DProject()->getBasePath();
	QString fpath = dpath + "\\Setting.ini";
	QFile iniFile(fpath);
	if (iniFile.exists())
	{
		QStringList strs = MyCommonMethods::ReadAllLines(fpath);
		if (strs.length() > 2)
		{
			IsCopy = false;
		}
		else
		{
			IsCopy = true;
		}
	}
	else
	{
		IsCopy = true;
	}
	if (IsCopy)
	{
		if (QFile::exists(rpath))
		{
			if (QFile::exists(fpath))
			{
				QFile::remove(fpath);
			}
			QFile::copy(rpath, fpath);
		}
	}
	else
	{
		if (QFile::exists(rpath))
		{
			QFile::remove(rpath);
		}
		QFile::copy(fpath, rpath);
	}

	QStringList subdir = { "IRIMTD\\DAQ", "IRIMTD\\Laser" };
	for (int si = 0; si < subdir.length(); ++si)
	{
		for (int i = 0; i < 2; ++i)
		{
			IsCopy = false;
			rpath = QString("%1\\Setting\\%2%3\\Setting.ini").arg(QApplication::applicationDirPath()).arg(subdir[si]).arg(i);
			dpath = QString("%1\\%2%3").arg(dpath).arg(subdir[si]).arg(i);

			QDir tempDir(dpath);
			if (tempDir.exists())
			{
				fpath = dpath + "\\Setting.ini";
				if (QFile::exists(fpath))
				{
					IsCopy = false;
				}
				else
				{
					IsCopy = true;
				}
				if (IsCopy)
				{
					if (QFile::exists(rpath))
					{
						QFile::remove(fpath);
						QFile::copy(rpath, fpath);
					}
				}
				else
				{

					if (QFile::exists(fpath))
					{
						QFile::remove(fpath);
					}
					QFile::copy(rpath, fpath);
				}
			}
		}
	}

}


void CalculationThread::FilterLaserData(QString fpath, double thresh1/*=5*/, double thresh2/*=20*/)
{
	QStringList sdata;
	if (QFile::exists(fpath + ".bak"))
	{
		sdata = MyCommonMethods::ReadAllLines(fpath + ".bak");
	}
	else
	{
		sdata = MyCommonMethods::ReadAllLines(fpath);
	}
	int len = sdata.length();

	//不足1米
	if (len < 20)
		return;
	QVector<double> lasval(len);
	QVector<double> disval(len);
	QVector<double> roadval(len);
	QVector<double> dmival(len);
	QVector<double> diffval(len);

	double sum_lasval = 0;
	QStringList s;
	for (int i = 0; i < len; ++i)
	{
		s = sdata[i].split('\t');
		if (s.length() > 3)
		{
			try
			{

				lasval[i] = s[0].toDouble();
				disval[i] = s[1].toDouble();
				roadval[i] = s[2].toDouble();

				dmival[i] = s[3].toDouble();
			}
			catch (...)
			{
				if (i > 1)
				{
					lasval[i] = s[0].toDouble();
					disval[i] = s[1].toDouble();
					roadval[i] = s[2].toDouble();
					dmival[i] = s[3].toDouble();
				}
			}
			sum_lasval += lasval[i];
			if (i > 0)
			{
				diffval[i] = std::abs(lasval[i] - lasval[i - 1]);
			}
		}
	}

	double meanlasval = sum_lasval / len;
	for (int i = 0; i < 3; ++i)
	{
		if (std::abs(lasval[i] - meanlasval) > 100)
		{
			lasval[i] = meanlasval;
		}
	}

	//先根据相邻测距值的差大于指定阈值，将这种孤立的异常值剔除
	for (int i = 2; i < len; ++i)
	{
		if (diffval[i] >= thresh1 && diffval[i - 1] >= thresh1)
		{
			lasval[i - 1] = lasval[i - 2] - disval[i - 2] + disval[i - 1];
		}
	}

	//然后根据相邻的动态测距差值大于指定阈值，找出第一步成片的异常值的边缘位置，将这种异常值剔除
	for (int i = 1; i < len; ++i)
	{
		if (std::abs(lasval[i] - lasval[i - 1]) >= thresh2)
		{
			lasval[i] = lasval[i - 1] - disval[i - 1] + disval[i];
		}
	}

	for (int i = 0; i < len; ++i)
	{
		roadval[i] = lasval[i] - disval[i];
		QString lineData = QString("%1\t%2\t%3\t%4").arg(QString::number(lasval[i], 'f', 6)).arg(QString::number(disval[i], 'f', 6)).arg(QString::number(roadval[i], 'f', 6)).arg(QString::number(dmival[i], 'f', 0));


		sdata[i] = lineData;
	}

	if (!QFile::exists(fpath + ".bak"))
	{
		MyCommonMethods::moveFile(fpath, fpath + ".bak");
	}

	QTextCodec* codeT = QTextCodec::codecForName("utf-8");
	MyCommonMethods::writeAllLines(fpath, sdata, codeT);
}


//void CalculationThread::startCalculateIRI(QString dataPath, const QString& outPath, double dIntervel, vector<double> listIRI, bool datasrc)
//{
//
//
//	bool IsParmFile = false;
//	QString fparmpath = dataPath.replace("resample.txt", "Coeff.dat");
//	if (QFile::exists(fparmpath))
//	{
//		IsParmFile = true;
//	}
//	QStringList parms;
//	QVector< double> speedparms;
//	QVector< double> kparms;
//	QVector< double> bparms;
//	int parmnum = 0;
//	if (IsParmFile)
//	{
//		int idx = 0;
//		parms = MyCommonMethods::ReadAllLines(fparmpath);
//		parmnum = parms[idx].toInt();
//		speedparms.resize(parmnum);
//		kparms.resize(parmnum);
//		bparms.resize(parmnum);
//		for (int i = 0; i < parmnum; ++i)
//		{
//			speedparms[i] = parms[++idx].toDouble();
//		}
//		for (int i = 0; i < parmnum; ++i)
//		{
//			kparms[i] = parms[++idx].toDouble();
//		}
//		for (int i = 0; i < parmnum; ++i)
//		{
//			bparms[i] = parms[++idx].toDouble();
//		}
//	}
//
//	QString iriOutPath = QString("%1\\IRI_%2m.txt").arg(outPath).arg(QString::number(dIntervel));
//	hnComputeIRI iri;
//	vector<double> listIRIInfo;
//	int sumCount = dIntervel / 0.25;
//	QStringList results;
//	int  count = 0;
//	double speedval = 0;
//	double 	DeltLen = 0.25; 
//	double YSU = 0.0;
//	QVector<double> m_sZU; m_sZU.resize(4);
//	QVector<double>m_pZU; m_pZU.resize(4);
//	QVector<double>m_zSU;
//	QVector<double>m_oldZSU;
//	m_zSU.resize(4);
//	m_oldZSU.resize(4);
//	m_oldZSU[0] = listIRI[1] - listIRI[0];
//	m_oldZSU[1] = listIRI[1] - listIRI[0];
//	m_oldZSU[2] = 0;
//	m_oldZSU[3] = 0;
//	for (int i = 0; i < listIRI.size(); i++)  //10/0.25   100/0.25  1000/0.25 
//	{
//		if (i%sumCount == 0 && listIRIInfo.size() > 1)
//		{
//			count++;
//			double value = 0;
//			//iri.calculateIRI(dIntervel, listIRIInfo, value);
//
//			//计算10m的平整度
//			double irisum = 0.0;
//			int plusenum = (int)(dIntervel / DeltLen);
//
//			//iridata: 250mm采样间距纵断面
//			for (int i = 1; i < listIRIInfo.size(); ++i)
//			{
//				YSU = (listIRIInfo[i] - listIRIInfo[i - 1]) / DeltLen;
//				for (int zi = 0; zi < 4; ++zi)
//				{
//					m_zSU[zi] = 0;
//					for (int zj = 0; zj < 4; ++zj)
//					{
//						m_zSU[zi] += m_sZU[zi * 4 + zj] * m_oldZSU[zj];
//					}
//					m_zSU[zi] += m_pZU[zi] * YSU;
//				}
//				double valueTemp = abs(m_zSU[0] - m_zSU[2]);
//				irisum += valueTemp;
//
//				for (int zi = 0; zi < 4; ++zi)
//				{
//					m_oldZSU[zi] = m_zSU[zi];
//				}
//			}
//			value = irisum / plusenum;
//
//
//			//根据车速进行矫正 
//			if (datasrc)
//			{
//				value = value *m_IRI_k + m_IRI_b;
//			}
//			else
//			{
//				if (IsParmFile)
//				{
//					//根据车速获取速度系数k、b
//					double kparm = kparms[parmnum - 1];
//					double bparm = bparms[parmnum - 1];
//					for (int pi = 0; pi < parmnum; ++pi)
//					{
//						if (speedval <= speedparms[pi])
//						{
//							kparm = kparms[pi];
//							bparm = bparms[pi];
//							break;
//						}
//					}
//					value = value * kparm + bparm;
//				}
//				else
//				{
//					value = value*m_IRI_k + m_IRI_b;
//				}
//
//			}
//
//			QString result = QString::number(count) + "\t" + QString::number(value);
//			results.push_back(result);
//			listIRIInfo.clear();
//		}
//		listIRIInfo.push_back(listIRI[i]);
//	}
//
//	QTextCodec* codeT = QTextCodec::codecForName("utf-8");
//	MyCommonMethods::writeAllLines(iriOutPath, results, codeT);
//}


void CalculationThread::startCalculateIRI(QString dataPath, const QString& outPath, double dIntervel, vector<double> listIRI, const QVector<double>speeds, bool datasrc)
{
	bool IsParmFile = false;
	QString fparmpath = dataPath.replace("resample.txt", "Coeff.dat");
	if (QFile::exists(fparmpath))
	{
		IsParmFile = true;
	}
	QStringList parms;
	QVector< double> speedparms;
	QVector< double> kparms;
	QVector< double> bparms;
	int parmnum = 0;
	if (IsParmFile)
	{
		int idx = 0;
		parms = MyCommonMethods::ReadAllLines(fparmpath);
		parmnum = parms[idx].toInt();
		speedparms.resize(parmnum);
		kparms.resize(parmnum);
		bparms.resize(parmnum);
		for (int i = 0; i < parmnum; ++i)
		{
			speedparms[i] = parms[++idx].toDouble();
		}
		for (int i = 0; i < parmnum; ++i)
		{
			kparms[i] = parms[++idx].toDouble();
		}
		for (int i = 0; i < parmnum; ++i)
		{
			bparms[i] = parms[++idx].toDouble();
		}
	}

	QString iriOutPath = QString("%1\\IRI_%2m.txt").arg(outPath).arg(QString::number(dIntervel));
	hnComputeIRI iri;

	int sumCount = dIntervel / 0.25;
	QStringList results;
	int  count = 0;
	double speedval = 0;

	double YSU = 0.0;

	int iricnt = 0;
	double irisum = 0;
	double irival = 0;
	double 	DeltLen = 0.25;
	int plusenum = (int)(dIntervel / DeltLen);//IRI距离内有多少个250mm 
	double stime = 0;
	double etime = 0;



	QVector< double> SZU = { 0.9966071, 1.091514e-02, -2.083274e-03, 3.190145e-04,
	   -0.5563044, 0.9438768, -0.8324718, 5.064701e-02,
	   2.153176e-02, 2.126763e-03, 0.7508714, 8.221888e-03,
	   3.335013, 0.3376467, -39.12762, 0.4347664 };
	QVector< double> PZU = { 5.47610e-03, 1.388776, 0.2275968, 35.79262 };

	QVector<double> ZSU;
	ZSU.resize(4);
	QVector<double> m_sZU;
	m_sZU.resize(4);
	QVector<double>m_pZU;
	m_pZU.resize(4);
	QVector<double>m_zSU;
	QVector<double>oldZSU;
	m_zSU.resize(4);
	oldZSU.resize(4);

	/*oldZSU[0] = listIRI[1] - listIRI[0];
	oldZSU[1] = listIRI[1] - listIRI[0];
	oldZSU[2] = 0;
	oldZSU[3] = 0;*/

	//20250818
	oldZSU[0] = (listIRI[44] - listIRI[0]) / 11;
	oldZSU[2] = (listIRI[44] - listIRI[0]) / 11;
	oldZSU[1] = 0;
	oldZSU[3] = 0;

	int speedInx = 0;

	for (int i = 1; i < listIRI.size(); i++)  //10/0.25   100/0.25  1000/0.25 
	{
		if (i % sumCount == 0)
		{
			irival = irisum / plusenum;
			if (datasrc)
			{

				irival = irival * m_IRI_k + m_IRI_b;
			}
			else
			{
				if (IsParmFile)
				{
					if (speedInx < speeds.size())
					{
						speedval = speeds.at(speedInx);
						speedInx++;
					}

					//根据车速获取速度系数k、b
					double kparm = kparms[parmnum - 1];
					double bparm = bparms[parmnum - 1];
					for (int pi = 0; pi < parmnum; ++pi)
					{
						if (speedval <= speedparms[pi])
						{
							kparm = kparms[pi];
							bparm = bparms[pi];
							break;
						}
					}
					irival = irival * kparm + bparm;
				}
				else
				{
					irival = irival * m_IRI_k + m_IRI_b;
				}
			}
			QString line = QString("%1 %2").arg(QString::number(++iricnt)).arg(QString::number(irival));
			//QString result = QString::number(count) + "\t" + QString::number(value);
			results.append(line);
			irisum = 0;
		}

		YSU = (listIRI[i] - listIRI[i - 1]) / DeltLen;
		for (int zi = 0; zi < 4; ++zi)
		{
			ZSU[zi] = 0;
			for (int zj = 0; zj < 4; ++zj)
			{
				ZSU[zi] += SZU[zi * 4 + zj] * oldZSU[zj];
			}
			ZSU[zi] += PZU[zi] * YSU;
		}
		irisum += qAbs(ZSU[0] - ZSU[2]);

		for (int zi = 0; zi < 4; ++zi)
		{
			oldZSU[zi] = ZSU[zi];
		}
	}

	QTextCodec* codeT = QTextCodec::codecForName("utf-8");
	MyCommonMethods::writeAllLines(iriOutPath, results, codeT);
}



tuple_bool_vec_vec_vec_int CalculationThread::LoadParameters(const std::string& fpath, const std::string& fname)
{
	bool isParmFile = false;
	std::string fparmpath = fpath.substr(0, fpath.find_last_of("/\\")) + "/Coeff.dat";  // 假设路径替换
	std::vector<double> speedparms, kparms, bparms;
	int parmnum = 0;

	QFile file(QString::fromStdString(fparmpath));
	if (file.exists()) {
		isParmFile = true;
		if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QTextStream in(&file);
			QStringList lines;
			while (!in.atEnd()) {
				lines << in.readLine();
			}
			file.close();

			if (lines.size() >= 1) {
				try {
					parmnum = std::stoi(lines[0].toStdString());
					speedparms.resize(parmnum);
					kparms.resize(parmnum);
					bparms.resize(parmnum);
					int idx = 1;
					for (int i = 0; i < parmnum; ++i) {
						speedparms[i] = std::stod(lines[idx++].toStdString());
					}
					for (int i = 0; i < parmnum; ++i) {
						kparms[i] = std::stod(lines[idx++].toStdString());
					}
					for (int i = 0; i < parmnum; ++i) {
						bparms[i] = std::stod(lines[idx++].toStdString());
					}
				}
				catch (...) {
					QMessageBox::warning(nullptr, "Error", QString::fromStdString("读取文件出错，请检查！\r\n" + fparmpath));
				}
			}
		}
	}
	return{ isParmFile, speedparms, kparms, bparms, parmnum };
}

void CalculationThread::GenerateIRI_NEW(const std::string& fpath, int vallen, const std::string& fname, bool datasrc, int effectiveLength)
{
	// 步骤1: 加载速度修正参数（如果存在）
	auto result1 = LoadParameters(fpath, fname);
	bool isParmFile;
	std::vector<double>  speedparms, kparms, bparms;
	int parmnum;
	std::tie(isParmFile, speedparms, kparms, bparms, parmnum) = result1;

	// 步骤2: 加载和预处理原始数据
	double DeltLen = 0.1; // 采样间隔（0.1m 或 0.25m）

	auto result2 = LoadData(fpath);
	std::vector<double >oridata, toridata;
	std::vector<int>oritime;
	std::vector<std::string> sdata;
	int len;
	std::tie(oridata, toridata, oritime, sdata, len) = result2;

	// -----------------------------------------------------
// 按工程有效里程裁剪原始 resample 数据
// resample.txt 原始采样间隔为 0.05m
// -----------------------------------------------------
	int maxRawLen = static_cast<int>(
		std::round(effectiveLength / 0.05)
		);

	len = (std::min)(len, maxRawLen);

	// 裁剪所有相关数组，必须一起裁剪
	oridata.resize(len);
	toridata.resize(len);
	oritime.resize(len);
	sdata.resize(len);

	// 步骤3: 应用均值滤波（可选，默认注释，与原始版本一致）
	// 标准 IRI 默认直接使用原始纵断面；启用此遗留滤波后，结果将不再与未滤波 LP 的标准复算严格一致。
	const bool enableLegacyFivePointMeanFilter = true;
	if (enableLegacyFivePointMeanFilter) {
		// 原始 5 点（0.25m 窗口）均值滤波逻辑，保留仅用于复现历史结果。
		for (int i = 2; i < len - 2; ++i) {
			oridata[i] = (toridata[i - 2] + toridata[i - 1] + toridata[i] + toridata[i + 1] + toridata[i + 2]) / 5.0;
		}
	}

	// 步骤3: 抽样到指定间隔（DeltLen）
	int qplusenum = static_cast<int>(DeltLen / 0.05); // 每0.1m抽样点数（0.05m原始间隔）

	auto result3 = ResampleData(oridata, oritime, len, qplusenum);
	std::vector<double> iridata;
	std::vector<int> iritime;
	std::tie(iridata, iritime) = result3;

	len = static_cast<int>(iridata.size());

	// 步骤4: 初始化状态变量
	std::vector<double> oldZSU = InitializeState(iridata, DeltLen, len);

	// 步骤5: 初始化时间和输出文件
	double stime = ParseTime(iritime[0]);
	std::string savefname = fpath.substr(0, fpath.rfind('/')) + "/IRI_" + std::to_string(vallen) + "m.txt";
	QFile fwIRI(QString::fromStdString(savefname));
	fwIRI.open(QIODevice::WriteOnly);
	QTextStream swIRI(&fwIRI);

	savefname = fpath.substr(0, fpath.rfind('/')) + "/Speed_" + std::to_string(vallen) + "m.txt";
	QFile fwspeed(QString::fromStdString(savefname));
	fwspeed.open(QIODevice::WriteOnly);
	QTextStream swspeed(&fwspeed);

	// 步骤6: 主循环计算IRI和速度
	int plusenum = static_cast<int>(vallen / DeltLen); // 每段点数（e.g., 10m / 0.1m = 100）
	int iricnt = 0;
	double irisum = 0.0;
	int count = 0;
	std::vector<double> ZSU(4, 0.0);

	// 从第一个相邻差分开始，保证 0.1m 输入的每个状态步都参与递推。

	const int start_i = (DeltLen == 0.1) ? 3 : 1;

	for (int i = start_i; i < len; ++i)
	{
		// 与当前 C# 一致：到达边界时先输出已累计的前一段。
		if (i % plusenum == 0)
		{
			const double etime = ParseTime(iritime[i]);
			const double speedval = (etime - stime) > 0.0
				? (vallen / (etime - stime) * 3.6)
				: 0.0;

			double irival = count > 0 ? irisum / count : 0.0;

			irival = ApplyCorrection(
				irival,
				datasrc,
				speedval,
				isParmFile,
				speedparms,
				kparms,
				bparms,
				parmnum);

			swIRI << iricnt + 1 << " " << irival << "\n";
			swspeed << iricnt + 1 << " " << speedval << "\n";

			irisum = 0.0;
			count = 0;
			stime = etime;
			++iricnt;
		}

		// 与当前 C# 一致：当前状态步进入下一段。
		const double YSU = ComputeYSU(iridata, i, DeltLen);
		UpdateState(ZSU, oldZSU, YSU, DeltLen);

		irisum += std::abs(ZSU[0] - ZSU[2]);
		++count;
		oldZSU = ZSU;
	}
	// 步骤7: 处理最后一个不完整段
	if (count > 0) {
		double etime = ParseTime(iritime[len - 1]);
		double partial_distance = count * DeltLen;
		double speedval = (etime - stime) > 0 ? (partial_distance / (etime - stime) * 3.6) : 0.0;
		double irival = irisum / count;

		// 应用IRI修正
		irival = ApplyCorrection(irival, datasrc, speedval, isParmFile, speedparms, kparms, bparms, parmnum);

		// 写入输出
		swIRI << ++iricnt << " " << irival << "\n";
		swspeed << iricnt << " " << speedval << "\n";
	}

	// 步骤8: 关闭输出文件
	fwIRI.close();
	fwspeed.close();

	// 步骤9: 生成250mm抽样文件
	savefname = fpath.substr(0, fpath.rfind('/')) + "/ReSample250.txt";
	QFile fw250(QString::fromStdString(savefname));
	fw250.open(QIODevice::WriteOnly);
	QTextStream sw250(&fw250);
	for (int i = 0; i < static_cast<int>(sdata.size()); ++i) {
		if (i % qplusenum == 0) {
			sw250 << QString::fromStdString(sdata[i]) << "\n";
		}
	}
	fw250.close();
}

tuple_vec_vec_vec_int_int CalculationThread::LoadData(const std::string& fpath)
{
	QFile file(QString::fromStdString(fpath));
	std::vector<std::string> sdata;
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&file);
		while (!in.atEnd()) {
			sdata.push_back(in.readLine().toStdString());
		}
		file.close();
	}
	int len = sdata.size();
	std::vector<double> oridata(len);
	std::vector<double> toridata(len);
	std::vector<int> oritime(len);

	for (int i = 0; i < len; ++i) {
		std::string line = sdata[i];
		std::string delim = "\t";
		size_t pos = 0;
		std::vector<std::string> s;
		while ((pos = line.find(delim)) != std::string::npos) {
			s.push_back(line.substr(0, pos));
			line.erase(0, pos + delim.length());
		}
		s.push_back(line);  // 最后一个token

		if (s.size() <= 1) {
			delim = " ";
			pos = 0;
			s.clear();
			while ((pos = line.find(delim)) != std::string::npos) {
				s.push_back(line.substr(0, pos));
				line.erase(0, pos + delim.length());
			}
			s.push_back(line);
		}

		if (s.size() > 3) {
			try {
				oridata[i] = std::stod(s[2]);
				toridata[i] = oridata[i];
				oritime[i] = std::stoi(s[3]);
			}
			catch (...) {
				if (i > 1) {
					oridata[i] = oridata[i - 1] * 2 - oridata[i - 2];
					toridata[i] = oridata[i];
					oritime[i] = oritime[i - 1] * 2 - oritime[i - 2];
				}
			}
		}
	}
	return{ oridata, toridata, oritime, sdata, len };
}

tuple_vec_int CalculationThread::ResampleData(const std::vector<double>& oridata, const std::vector<int>& oritime, int len, int qplusenum)
{
	int newLen = (len + qplusenum - 1) / qplusenum;
	std::vector<double> iridata(newLen);
	std::vector<int> iritime(newLen);
	for (int i = 0, j = 0; i < len; i += qplusenum, ++j) {
		if (i < len) {
			iridata[j] = oridata[i];
			iritime[j] = oritime[i];
		}
	}
	return{ iridata, iritime };
}

std::vector<double> CalculationThread::InitializeState(const std::vector<double>& iridata, double DeltLen, int len)
{
	std::vector<double> oldZSU(4, 0.0);
	int index = (DeltLen == 0.1) ? qMin(110, len - 1) : qMin(44, len - 1);
	if (index > 0 && index < len) {
		oldZSU[0] = (iridata[index] - iridata[0]) / 11.0;
		oldZSU[2] = (iridata[index] - iridata[0]) / 11.0;
	}
	oldZSU[1] = 0.0;
	oldZSU[3] = 0.0;
	return oldZSU;
}

double CalculationThread::ParseTime(int timeValue)
{
	if (m_speedtype) {
		return timeValue * 1.0 / m_Frequency;
	}
	else {
		return (timeValue / 10000000) * 3600.0 +
			(timeValue / 100000 % 100) * 60.0 +
			(timeValue / 1000 % 100) +
			(timeValue % 1000) * 0.001;
	}
}

double CalculationThread::ComputeYSU(const std::vector<double>& iridata, int i, double DeltLen)
{
	if (DeltLen == 0.1)
	{
		return (iridata[i] - iridata[i - 3]) / 0.3;
	}

	return (iridata[i] - iridata[i - 1]) / DeltLen;
}



void CalculationThread::UpdateState(std::vector<double>& ZSU, const std::vector<double>& oldZSU, double YSU, double DeltLen)
{
	const std::array<double, 16> SZU = {
		0.9966071, 1.091514e-02, -2.083274e-03, 3.190145e-04,
		-0.5563044, 0.9438768, -0.8324718, 5.064701e-02,
		2.153176e-02, 2.126763e-03, 0.7508714, 8.221888e-03,
		3.335013, 0.3376467, -39.12762, 0.4347664
	};

	const  std::array<double, 4> PZU = {
		5.47610e-03, 1.388776, 0.2275968, 35.79262
	};

	const  std::array<double, 16> SZU100 = {
		0.9994014, 0.004442351, 0.0002188854, 5.72179E-05,
		-0.2570548, 0.975036, 0.007966216, 0.02458427,
		0.003960378, 0.0003814527, 0.9548048, 0.004055587,
		1.687312, 0.1638951, -19.34264, 0.7948701
	};

	const  std::array<double, 4> PZU100 = {
		0.0003793992, 0.2490886, 0.04123478, 17.65532
	};


	const auto& szu = (DeltLen == 0.1) ? SZU100 : SZU;
	const auto& pzu = (DeltLen == 0.1) ? PZU100 : PZU;
	for (size_t zi = 0; zi < 4; ++zi) {
		ZSU[zi] = 0.0;
		for (size_t zj = 0; zj < 4; ++zj) {
			ZSU[zi] += szu[zi * 4 + zj] * oldZSU[zj];
		}
		ZSU[zi] += pzu[zi] * YSU;
	}
}

double CalculationThread::ApplyCorrection(double irival, bool datasrc, double speedval, bool isParmFile, const std::vector<double>& speedparms, const std::vector<double>& kparms, const std::vector<double>& bparms, int parmnum)
{
	if (datasrc) {
		irival = irival * 1 + 0;
	}
	else {

		if (isParmFile)
		{
			double kparm = kparms[parmnum - 1];
			double bparm = bparms[parmnum - 1];
			for (int pi = 0; pi < parmnum; ++pi)
			{
				if (speedval <= speedparms[pi])
				{
					kparm = kparms[pi];
					bparm = bparms[pi];
					break;
				}


			}
			irival = irival * kparm + bparm;
			//kparm = std::accumulate(kparms.begin(), kparms.end(), 0.0) / kparms.size();

			//irival = irival * kparm;
		}
		else
		{
			//irival = irival * m_IRI_k + m_IRI_b;
		}
	}
	return irival;
}
bool CalculationThread::ComputeRut(bool Isbar, int valnum, int rutmode)
{
	std::vector< float>c2wmatrix;
	std::unique_ptr<float[]>matobj;
	std::unique_ptr<double[]>matobj3d;
	std::unique_ptr<unsigned char[]>rbmat;

	std::unique_ptr<int[]>MP_idx(new int[20] { 0 });
	std::unique_ptr<float[]> MP_val(new float[20] { 0 });
	float lineK = 0.0f;
	float _rutk = 1.0f;
	float _rutb = 0.0f;
	try
	{
		QString basePath = currentPorject->get2DProPath();
		QString c2w[] = { QString("%1\\camera0\\c2cali.c2w").arg(basePath),QString("%1\\camera1\\c2cali.c2w").arg(basePath) };
		QString dat[] = { QString("%1\\camera0\\data").arg(basePath), QString("%1\\camera1\\data").arg(basePath) };
		QString cfg[] = { QString("%1\\camera0\\rutcfg.ini").arg(basePath), QString("%1\\camera1\\rutcfg.ini").arg(basePath) };
		QString process[] = { QString("%1\\RUT\\camera0\\data").arg(basePath),QString("%1\\RUT\\camera1\\data").arg(basePath) };
		int cameracnt = valnum == 1 ? 2 : 1;

		// 计算前一次性检查全部必需输入，避免无数据时直接显示“完成”。
		QStringList inputIssues;
		for (int cameraIndex = 0; cameraIndex < cameracnt; ++cameraIndex)
		{
			if (!QDir(dat[cameraIndex]).exists())
			{
				inputIssues.append(QStringLiteral("缺少原始车辙数据目录：%1").arg(dat[cameraIndex]));
			}
			else if (MyCommonMethods::getMyAllDirFile(dat[cameraIndex], QStringList("*.dat")).isEmpty())
			{
				inputIssues.append(QStringLiteral("未找到可计算的 DAT 原始文件：%1").arg(dat[cameraIndex]));
			}

			if (!QFile::exists(c2w[cameraIndex]))
			{
				inputIssues.append(QStringLiteral("缺少车辙标定文件：%1").arg(c2w[cameraIndex]));
			}
			if (!QFile::exists(cfg[cameraIndex]))
			{
				inputIssues.append(QStringLiteral("缺少车辙配置文件：%1").arg(cfg[cameraIndex]));
			}
		}

		if (!inputIssues.isEmpty())
		{
			emit error(QStringLiteral("工程【%1】车辙计算未启动：\r\n%2\r\n\r\n请补齐以上文件后重新计算。")
				.arg(currentPorject->get2DProName(), inputIssues.join(QStringLiteral("\r\n"))));
			return false;
		}


		QSettings settings(cfg[0], QSettings::IniFormat);

		//设置要读取的组名 
		settings.beginGroup(QString("camera"));
		float hpix = settings.value("hpixel").toFloat();
		if (hpix == 0)
			hpix = 2048;
		float vpix = settings.value("calivpix").toFloat();
		if (vpix == 0)
			vpix = 3200;
		settings.endGroup();
		settings.beginGroup(QString("rut"));
		float _scaleval = settings.value("scaleval").toFloat();
		if (_scaleval == 0)
			_scaleval = 10;
		float basezval = settings.value("basezval").toFloat();
		int rbmatSize = 0;
		if (rutmode == 2)
		{
			matobj3d.reset(new double[hpix] {0});

			rbmat.reset(new unsigned   char[hpix * 8] { 0 });
			rbmatSize = hpix * 8;
		}
		else
		{
			matobj.reset(new float[hpix] {0});
			rbmat.reset(new unsigned   char[hpix * 4] { 0 });
			rbmatSize = hpix * 4;
		}
		//使用一维数组模拟二维数组
		c2wmatrix.resize(vpix * hpix, 0);
		emit progressUpdated(m_Project, m_type, 0.001, false);
		std::vector<short> profile(hpix, 0);
		std::vector<short> profobj(hpix, 0);
		//std::unique_ptr<float[]>objlas(new float[hpix] {0});
		std::vector<float> objlas(hpix, 0);
		std::vector<float>  tobjlas(hpix, 0);
		//	std::unique_ptr< unsigned char[]> rbarr(new   unsigned  char[hpix * 2]{ 0 });
		std::vector<unsigned char>rbarr(hpix * 2, 0);
		//	std::unique_ptr< char[]> wbarr(new  char[hpix * 2]{ 0 });
		std::vector< unsigned  char>wbarr(hpix * 2, 0);
		RutProfileDebugExporter profileDebugExporter(currentPorject, basePath,
			static_cast<int>(hpix), static_cast<int>(vpix), rutmode);
		QString profileDebugError;
		if (!profileDebugExporter.initialize(profileDebugError))
		{
			emit error(QStringLiteral("工程【%1】车辙计算未启动：\r\n%2")
				.arg(currentPorject->get2DProName(), profileDebugError));
			return false;
		}
		const bool profileDebugEnabled = profileDebugExporter.isEnabled();
		//像方激光线转物方激光线
		int i = 0, j = 0, n = 0, k = 0, m = 0, temp = 0;
		long linetotalnum = 0;
		for (i = 0; i < cameracnt; ++i)
		{
			linetotalnum = 0;
			//如果相机原始数据没有存在工程内就不处理当前相机的数据
			QDir dir(dat[i]);
			if (!dir.exists())
			{
				continue;
			}
			//如果标定文件和配置文件没有存在工程内就不处理当前相机的数据
			QFile file1(c2w[i]);
			QFile file2(cfg[i]);
			if (!file1.exists() || !file2.exists())
			{
				continue;
			}
			QDir dir1(process[i]);
			if (!dir1.exists())
			{
				if (!QDir().mkpath(process[i]))
				{
					emit error(QStringLiteral("工程【%1】车辙计算未启动：\r\n无法创建输出目录：%2")
						.arg(currentPorject->get2DProName(), process[i]));
					return false;
				}
			}
			string sPath = c2w[i].toLocal8Bit();
			const char* path = sPath.c_str();
			std::ifstream fil(path, std::ios::binary);
			if (!fil.is_open())
			{
				emit error(QStringLiteral("工程【%1】车辙计算未启动：\r\n无法读取标定文件：%2")
					.arg(currentPorject->get2DProName(), c2w[i]));
				return false;
			}


			fil.seekg(0, std::ios::end);
			auto  fileSize = fil.tellg();
			fil.seekg(0, std::ios::beg);

			//计算预期大小
			size_t  expectedSize = vpix * hpix * (rutmode == 2 ? 8 : 4);
			if (fileSize < expectedSize)
			{
				emit error(QStringLiteral("工程【%1】车辙计算未启动：\r\n标定文件大小不足：%2\r\n期望至少 %3 字节，实际 %4 字节。")
					.arg(currentPorject->get2DProName(), c2w[i])
					.arg(static_cast<qulonglong>(expectedSize))
					.arg(static_cast<qlonglong>(fileSize)));
				return false;
			}
			QFile fileC2w(c2w[i]);

			if (fileC2w.open(QIODevice::ReadOnly))
			{
				QDataStream stream(&fileC2w);
				stream.setByteOrder(QDataStream::LittleEndian);
				for (int n = 0; n < vpix; ++n)
				{
					if (rutmode == 2)
					{
						temp = hpix * 8;
						//将字节转换成double
						if (stream.readRawData(reinterpret_cast<char*>(rbmat.get()), temp) != temp)
						{
							qDebug() << "read error line:" << n;
							break;
						}
						std::memcpy(matobj3d.get(), rbmat.get(), temp);
						for (int k = 0; k < hpix; ++k)
						{
							c2wmatrix[n * hpix + k] = static_cast<float>(matobj3d[k]);
						}
					}
					else
					{
						temp = hpix * 4;
						if (stream.readRawData(reinterpret_cast<char*>(rbmat.get()), temp) != temp)
						{
							qDebug() << "read error line:" << n;
							break;
						}
						if (n == 279)
						{
							for (int ddds = 0; ddds < 10; ++ddds)
							{
								auto datasds = rbmat.get()[ddds];
							}
						}
						//将字节转换成float
						std::memcpy(matobj.get(), rbmat.get(), temp);
						for (int k = 0; k < hpix; ++k)
						{
							c2wmatrix[n * hpix + k] = matobj[k];
						}
					}
				}
				fileC2w.close();
			}
			else
			{
				emit error(QStringLiteral("工程【%1】车辙计算未启动：\r\n无法读取标定文件：%2")
					.arg(currentPorject->get2DProName(), c2w[i]));
				return false;
			}
			/*		if (fil.is_open())
					{
						if (rutmode == 2)
						{
							temp = hpix * 8;
							for (n = 0; n < vpix; ++n)
							{
								fil.read(reinterpret_cast<char*> (rbmat.get()), temp);
								memcpy(matobj3d.get(), rbmat.get(), rbmatSize);
								for (k = 0; k < hpix; ++k)
								{
									double tempss = matobj3d.get()[k];

									c2wmatrix[n*hpix + k] = static_cast<float>(matobj3d.get()[k]);
								}
							}

						}
						else
						{
							temp = hpix * 4;
							for (n = 0; n < vpix; ++n)
							{
								fil.read(reinterpret_cast<char*> (rbmat.get()), temp);

								memcpy(matobj.get(), rbmat.get(), rbmatSize);
								if (n==65)
								{
									auto * datass = rbmat.get();
								auto *  datadd= matobj.get();
								for (int dsda = 0 ; dsda <16; ++dsda)
								{

									auto valueddd = (int)rbmat.get()[dsda];
									qDebug() << "rbmat[" << dsda << "]:" << valueddd;
								}

								}
								for (k = 0; k < hpix; ++k)
								{

									{
										c2wmatrix[n*hpix + k] = static_cast<float>(matobj.get()[k]);;


									}
								}
							}
						}
					}*/
			emit progressUpdated(m_Project, m_type, 0.011, false);
			fil.close();


			QString _dtwname = "";
			QStringList _dats = MyCommonMethods::getMyAllDirFile(dat[i], QStringList("*.dat"));
			_dats.sort();
			for (j = 0; j < _dats.size(); ++j)
			{
				try
				{
					int datfilecnt = 0;
					int index = _dats[j].lastIndexOf("/");
					if (index < 0)
					{
						index = _dats[j].lastIndexOf(QStringLiteral("//"));
					}
					_dtwname = _dats[j].mid(index + 1);
					_dtwname = _dtwname.mid(0, _dtwname.lastIndexOf('.'));
					/*	bool fflag = false;
						QFile  orifile(_dats[j]);
						QFile newfile;
						if (QFile::exists(QString("%1\\%2.dtw").arg(process[i]).arg(_dtwname)))
						{
							fflag = true;
							newfile.setFileName(QString("%1\\%2.dtw").arg(process[i]).arg(_dtwname));
						}

						if (fflag &&qAbs(orifile.size() - newfile.size()) < 1000)
						{

							linetotalnum += newfile.size() / (hpix * 2);
							emit progressUpdated(m_Project, m_type, 0.021 + 0.2 * datfilecnt++ / _dats.size()* i / cameracnt, false);
							continue;
						}*/
					string datPath = _dats[j].toLocal8Bit();
					const char* nowDatPath = datPath.c_str();
					QString nowPath0(process[i] + "\\");
					QDir dirRes(nowPath0);
					MyCommonMethods::createMultipleFolders(nowPath0);
					//读取所有dat文件

					QFile fs(_dats[j]);

					std::ifstream filDat(nowDatPath, std::ios::binary);
					if (filDat)
					{
						QString nowDtwPath(process[i] + "\\" + _dtwname + ".dtw");
						QString nowPath0(process[i] + "\\");
						QDir dirRes(nowPath0);
						if (!dirRes.exists())
						{
							MyCommonMethods::createMultipleFolders(nowPath0);
						}

						string datPath1 = nowDtwPath.toLocal8Bit();
						const char* nowDatPath1 = datPath1.c_str();
						//std::filesystem::
						std::ofstream filDat1;
						filDat1.open(nowDatPath1, std::ios::binary);
						//	filDat1.open(nowDatPath1);
						temp = hpix * 2;
						while (true)
						{
							if (m_stopRequested)
							{
								return false;
							}

							// 先读，再判断本次是否真正读满一帧。
							// std::ifstream::eof() 只有在“读失败以后”才会置位，
							// 旧写法会在每个 DAT 文件末尾把 rbarr 中上一帧再处理一次。
							filDat.read(reinterpret_cast<char*>(rbarr.data()), temp);
							if (filDat.gcount() != temp)
							{
								break;
							}

							memcpy(profile.data(), rbarr.data(), hpix * 2);

							for (m = 0; m < hpix; ++m)
							{
								if (profile[m] <= 0 || profile[m] >= vpix)
								{
									profobj[m] = 0x7fff;
								}
								else
								{
									profobj[m] = (short)((c2wmatrix[profile[m] * hpix + m] - basezval) * _scaleval);
								}
							}
							memcpy(wbarr.data(), profobj.data(), hpix * 2);
							if (filDat1.is_open())
							{
								//写所有dtw文件
								filDat1.write(reinterpret_cast<char*> (wbarr.data()), hpix * 2);

								//写入数据
							}

						}
						filDat1.close();
						filDat.close();

					}

					int count = 0.021 + 0.2 * datfilecnt++ / _dats.size() * i / cameracnt;
					emit progressUpdated(m_Project, m_type, count, false);
				}
				catch (...)
				{
					//	throw; 
					THROW_EX("车辙计算失败！");
				}

			}

			//用物方激光线计算车辙值
			for (i = 0; i < cameracnt; ++i)
			{
				//判读如果车辙计算结果文件存在就不用再计算

			/*	QString frutname = basePath + "\\RUT\\camera" + QString::number(i) + "\\orirut.txt";
				QFile file(frutname);
				if (file.exists())
				{
					QStringList orirutstrs = MyCommonMethods::ReadAllLines(frutname);
					if (orirutstrs.size() > 0 && orirutstrs.size() >= linetotalnum - 10)
					{
						file.close();
						emit progressUpdated(m_Project, m_type, 0.9, false);
						continue;
					}
				}*/
				int profilenum = 0;
				QString _dtwname = "";
				int linecnt = 0;
				float arutval = 0;
				float brutval = 0;
				float crutval = 0;
				float _threshval = 0;

				//	std::vector<float> _gsfilter();
				std::unique_ptr<float[]> _gsfilter;
				int _partlen = 256, _asp = 0, _aep = 0, _bsp = 0, _bep = 0, _csp = 0, _cep = 0, _gslen = 0, _ThrPoint = 0;
				QSettings settings0(cfg[i], QSettings::IniFormat);

				//设置要读取的组名 
				settings0.beginGroup(QString("camera"));
				_asp = settings0.value("rutastart").toInt();

				_cep = settings0.value("rutcend").toInt();
				if (_cep == 0)
					_cep = 2048;


				_aep = settings0.value("rutaend").toInt();
				if (_aep == 0)
					_aep = 2048;
				_bsp = settings0.value("rutbstart").toInt();

				_bep = settings0.value("rutbend").toInt();
				if (_bep == 0)
					_bep = 2048;
				_csp = settings0.value("rutcstart").toInt();

				profilenum = settings0.value("calcstep").toInt();
				if (profilenum == 0)
					profilenum = 50;
				_rutk = settings0.value("rutk").toFloat();
				if (_rutk == 0)
					_rutk = 1.0f;
				_rutb = settings0.value("rutb").toFloat();

				settings0.endGroup();
				settings0.beginGroup(QString("sync"));
				int  plusstep = settings0.value("plusstep").toInt();
				if (plusstep == 0)
				{
					plusstep = 10;
				}
				profilenum = profilenum / plusstep;
				settings0.endGroup();
				settings0.beginGroup(QString("rut"));
				_scaleval = settings0.value("scaleval").toInt();
				if (_scaleval == 0)
					_scaleval = 10;
				_gslen = settings0.value("gslen").toInt();
				if (_gslen == 0)
					_gslen = 32;
				_gslen = _gslen / 2 * 2 + 1;
				std::unique_ptr<MyQtCommon::MyPoint[]> _pt(new MyQtCommon::MyPoint[hpix]);
				for (int pii = 0; pii < hpix; ++pii)
				{
					MyQtCommon::MyPoint& nowPt = _pt[pii];
					nowPt = MyQtCommon::MyPoint(0, 0);
				}
				_partlen = _aep - _asp - 2;//157、170、186
				_threshval = settings0.value("threshval").toInt();
				if (_threshval == 0)
					_threshval = 28;
				_ThrPoint = settings0.value("threshpointnum").toInt();
				if (_ThrPoint == 0)
					_ThrPoint = _partlen / 4;
				_gsfilter.reset(new float[_gslen]);
				CreateGaussFilter(_gsfilter, _gslen, 0.3f);
				emit progressUpdated(m_Project, m_type, 0.42, false);
				QStringList _dats = MyCommonMethods::getMyAllDirFile(dat[i], QStringList("*.dat"));
				_dats.sort();
				QFile fwrut(basePath + "\\RUT\\camera" + QString::number(i) + "\\rut.txt");
				QFile fworirut(basePath + "\\RUT\\camera" + QString::number(i) + "\\orirut.txt");

				if (!fworirut.open(QIODevice::WriteOnly | QIODevice::Text))
				{
					emit error(QStringLiteral("工程【%1】车辙计算失败：\r\n无法写入结果文件：%2")
						.arg(currentPorject->get2DProName(), fworirut.fileName()));
					return false;
				}
				QTextStream sworirut(&fworirut);

				if (!fwrut.open(QIODevice::WriteOnly | QIODevice::Text))
				{
					emit error(QStringLiteral("工程【%1】车辙计算失败：\r\n无法写入结果文件：%2")
						.arg(currentPorject->get2DProName(), fwrut.fileName()));
					return false;
				}
				QTextStream swrut(&fwrut);
				for (j = 0; j < _dats.size(); ++j)
				{
					try
					{
						_dtwname = _dats[j].mid(_dats[j].lastIndexOf("/") + 1);
						_dtwname = _dtwname.mid(0, _dtwname.lastIndexOf('.'));
						//读取所有dat文件
						//QString datPath = process[i] + "\\" + _dtwname + ".dtw";
						QString datPath = _dats[j];
						string tempDatPath = datPath.toLocal8Bit();
						std::ifstream filDat(tempDatPath.c_str(), std::ios::binary);

						if (filDat)
						{
							double fsbar2 = 0.58 * j / _dats.size();
							double fsbar = 0.58 / _dats.size();

							auto s_pos = filDat.tellg();
							filDat.seekg(0, std::ios::end);
							auto length = filDat.tellg() - s_pos;
							fsbar = fsbar / length;
							temp = hpix * 2;
							//将位置移动到开头
							filDat.seekg(0, std::ios::beg);
							while (true)
							{
								if (m_stopRequested)
								{
									return false;
								}

								// 先读，再判断本次是否真正读满一帧。
								// std::ifstream::eof() 只有在“读失败以后”才会置位，
								// 旧写法会在每个 DAT 文件末尾把 rbarr 中上一帧再处理一次。
								filDat.read(reinterpret_cast<char*>(rbarr.data()), temp);
								if (filDat.gcount() != temp)
								{
									break;
								}

								memcpy(profile.data(), rbarr.data(), hpix * 2);
								const qint64 currentFrameIndex = linecnt;
								const bool exportCurrentFrame =
									profileDebugEnabled
									&& profileDebugExporter.shouldExport(currentFrameIndex);
								for (m = 0, n = 0; m < hpix; ++m)
								{
									if (profile[m] <= 0 ||
										profile[m] >= vpix)
									{
										profobj[m] = 0x7fff;
										objlas[m] = 0x7fff;

									}
									else
									{
										float c2wValue =
											c2wmatrix[
												profile[m] * hpix + m
											];

										float value1 =
											c2wValue - basezval;

										objlas[m] = value1;


										if (rutmode == 2)
										{
											objlas[m] = -objlas[m];
										}

										tobjlas[n] = objlas[m];
										++n;
									}
								}
								if (valnum == 1)
								{
									if (n > 1200)
									{
										int _aept = _aep - (m - n);
										hnComputeCUT cut;
										arutval = cut.computerut(objlas.data(), _gsfilter.get(), _gslen, _asp, _aept, _threshval, _partlen, _ThrPoint, _pt.get(), tobjlas.data());
										arutval = qAbs(arutval * _rutk + _rutb);
									}
									else
									{
										arutval = 0;
									}

									++linecnt;

									QString line = QString::number(linecnt) + "0," + QString::number(arutval, 'f', 3) + "\n";
									sworirut << line;
									swrut << line;

								}
								else
								{
									if (n > 1200)
									{
										// 与 C# 保持完全一致：沿用 2025 调用口径，
										// 按当前帧无效点数量缩短右侧计算终点。
										int _cept = _cep - (m - n);

										hnComputeCUT cut;
										hnRutProfileTrace profileTrace;
										if (exportCurrentFrame)
										{
											brutval = cut.computerut3(hpix, objlas.data(), tobjlas.data(),
												_asp, _cept, arutval, crutval, lineK, _gslen,
												MP_idx.get(), MP_val.get(), &profileTrace);
											if (!profileDebugExporter.writeFrame(currentFrameIndex, datPath,
												profile, profileTrace.worldHeight, &profileTrace,
												brutval, profileDebugError))
											{
												emit error(QStringLiteral("工程【%1】车辙断面调试导出失败：\r\n%2")
													.arg(currentPorject->get2DProName(), profileDebugError));
												return false;
											}
										}
										else
										{
											brutval = cut.computerut3(hpix, objlas.data(), tobjlas.data(),
												_asp, _cept, arutval, crutval, lineK, _gslen,
												MP_idx.get(), MP_val.get());
										}

										// 第一层：工程 rutcfg.ini 中的 rutk / rutb
										arutval = qAbs(arutval * _rutk + _rutb);
										crutval = qAbs(crutval * _rutk + _rutb);
										brutval = qAbs(brutval * _rutk + _rutb);

										// 第二层：软件设置中的全局 K / B
										double K = 1;
										double B = 0;

										arutval = qAbs(arutval * K + B);
										crutval = qAbs(crutval * K + B);
										brutval = qAbs(brutval * K + B);

									}
								else
								{
									arutval = 0;
									crutval = 0;
									brutval = 0;
									if (exportCurrentFrame
										&& !profileDebugExporter.writeFrame(currentFrameIndex, datPath,
											profile, objlas, nullptr, brutval, profileDebugError))
									{
										emit error(QStringLiteral("工程【%1】车辙断面调试导出失败：\r\n%2")
											.arg(currentPorject->get2DProName(), profileDebugError));
										return false;
									}
								}
									if (m_stopRequested)
									{
										return false;

									}
									++linecnt;
									QString line = QString::number(linecnt) + "0," + QString::number(arutval, 'f', 3) + "," + QString::number(brutval, 'f', 3) + "," + QString::number(crutval, 'f', 3) + "\n";
									sworirut << line;
									swrut << line;

								}
								if (linecnt % 1000 == 0)
								{
									emit progressUpdated(m_Project, m_type, (0.42 + fsbar2 + filDat.tellg() * fsbar), false);
									//if (Isbar) bar.SetRutVal(0.42 + fsbar2 + frstream.Position * fsbar);
								}
							}


						}
						else
						{
							emit error(QStringLiteral("工程【%1】车辙计算失败：\r\n无法读取原始数据文件：%2")
								.arg(currentPorject->get2DProName(), datPath));
							return false;
						}
						filDat.close();

					}
					catch (const std::exception& exception)
					{
						emit error(QStringLiteral("工程【%1】车辙计算失败：\r\n处理原始数据文件失败：%2\r\n%3")
							.arg(currentPorject->get2DProName(), _dats[j], QString::fromUtf8(exception.what())));
						return false;
					}
					catch (...)
					{
						emit error(QStringLiteral("工程【%1】车辙计算失败：\r\n处理原始数据文件失败：%2")
							.arg(currentPorject->get2DProName(), _dats[j]));
						return false;
					}

				}
				fwrut.close();
				fworirut.close();
			}
		}
		if (!profileDebugExporter.finish(true, QString(), profileDebugError))
		{
			emit error(QStringLiteral("工程【%1】车辙断面调试目录发布失败：\r\n%2")
				.arg(currentPorject->get2DProName(), profileDebugError));
			return false;
		}
		return true;
	}
	catch (const std::exception& exception)
	{
		emit error(QStringLiteral("工程【%1】车辙计算失败：\r\n%2")
			.arg(currentPorject->get2DProName(), QString::fromUtf8(exception.what())));
		return false;
	}
	catch (...)
	{
		emit error(QStringLiteral("工程【%1】车辙计算失败：发生未知异常。")
			.arg(currentPorject->get2DProName()));
		return false;
	}
}

void CalculationThread::AdjustRutVal(int valnum)
{
	QString basePath = currentPorject->get2DProPath();

	QString leftPath =
		QString("%1\\Rut\\camera0\\orirut.txt")
		.arg(basePath);

	QString leftBackupPath =
		QString("%1\\Rut\\camera0\\orioldrut.txt")
		.arg(basePath);

	if (!QFile::exists(leftPath))
	{
		return;
	}

	// orioldrut ÿ�θ��ǣ�ֻ���汾�� AdjustRutVal ǰ��ԭʼ�����
	if (QFile::exists(leftBackupPath))
	{
		QFile::remove(leftBackupPath);
	}

	if (!QFile::copy(leftPath, leftBackupPath))
	{
		return;
	}

	QStringList LRutsr =
		MyCommonMethods::ReadAllLines(leftPath);

	QStringList RRutsr;
	QString rightPath;
	QString rightBackupPath;

	if (valnum < 2)//˫�������
	{
		rightPath =
			QString("%1\\Rut\\camera1\\orirut.txt")
			.arg(basePath);

		rightBackupPath =
			QString("%1\\Rut\\camera1\\orioldrut.txt")
			.arg(basePath);

		if (!QFile::exists(rightPath))
		{
			return;
		}

		if (QFile::exists(rightBackupPath))
		{
			QFile::remove(rightBackupPath);
		}

		if (!QFile::copy(rightPath, rightBackupPath))
		{
			return;
		}

		RRutsr =
			MyCommonMethods::ReadAllLines(rightPath);
	}

	if (LRutsr.size() < 2)
	{
		return;
	}

	if (valnum < 2 && RRutsr.size() < 2)
	{
		return;
	}

	int lenval =
		valnum < 2
		? qMax(LRutsr.size(), RRutsr.size())
		: LRutsr.size();

	std::unique_ptr<float[]> Lrutvals =
		std::make_unique<float[]>(lenval);

	std::unique_ptr<float[]> Rrutvals =
		std::make_unique<float[]>(lenval);

	float Lrutoldval = 0.0f;
	float Rrutoldval = 0.0f;
	float Lrutcurval = 0.0f;
	float Rrutcurval = 0.0f;

	for (int i = 0; i < lenval; ++i)
	{
		if (valnum < 2)
		{
			if (i < LRutsr.size())
			{
				QStringList trut =
					LRutsr[i].split(',');

				bool ok = false;
				Lrutcurval =
					trut.value(1).toFloat(&ok);

				if (!ok)
				{
					Lrutcurval = Lrutoldval;
				}

				if (_Setting->IsThresholdRut &&
					Lrutcurval > 0.0f)
				{
					Lrutcurval =
						Lrutcurval /
						qCeil(
							Lrutcurval /
							_Setting->ErrorRut);
				}

				Lrutvals[i] = Lrutcurval;
				Lrutoldval = Lrutcurval;
			}

			if (i < RRutsr.size())
			{
				QStringList trut =
					RRutsr[i].split(',');

				bool ok = false;
				Rrutcurval =
					trut.value(1).toFloat(&ok);

				if (!ok)
				{
					Rrutcurval = Rrutoldval;
				}

				if (_Setting->IsThresholdRut &&
					Rrutcurval > 0.0f)
				{
					Rrutcurval =
						Rrutcurval /
						qCeil(
							Rrutcurval /
							_Setting->ErrorRut);
				}

				Rrutvals[i] = Rrutcurval;
				Rrutoldval = Rrutcurval;
			}
		}
		else
		{
			if (i < LRutsr.size())
			{
				QStringList trut =
					LRutsr[i].split(',');

				bool okL = false;
				bool okR = false;

				Lrutcurval =
					trut.value(1).toFloat(&okL);

				Rrutcurval =
					trut.value(3).toFloat(&okR);

				if (!okL)
				{
					Lrutcurval = Lrutoldval;
				}

				if (!okR)
				{
					Rrutcurval = Rrutoldval;
				}

				if (_Setting->IsThresholdRut)
				{
					if (Lrutcurval > 0.0f)
					{
						Lrutcurval =
							Lrutcurval /
							qCeil(
								Lrutcurval /
								_Setting->ErrorRut);
					}

					if (Rrutcurval > 0.0f)
					{
						Rrutcurval =
							Rrutcurval /
							qCeil(
								Rrutcurval /
								_Setting->ErrorRut);
					}
				}

				Lrutvals[i] = Lrutcurval;
				Rrutvals[i] = Rrutcurval;

				Lrutoldval = Lrutcurval;
				Rrutoldval = Rrutcurval;
			}
		}
	}

	RemoveBigErrCore(
		Lrutvals.get(),
		lenval,
		_Setting->ErrorRutTh2,
		101);

	RemoveBigErrCore(
		Rrutvals.get(),
		lenval,
		_Setting->ErrorRutTh2,
		202);

	QStringList newLRutsr;
	QStringList newRRutsr;

	newLRutsr.reserve(LRutsr.size());

	if (valnum < 2)
	{
		newRRutsr.reserve(RRutsr.size());
	}

	for (int i = 0; i < lenval; ++i)
	{
		Lrutoldval = Lrutvals[i];
		Rrutoldval = Rrutvals[i];

		if (Lrutoldval > Rrutoldval)
		{
			if (Rrutoldval > 1.0f)
			{
				if ((Lrutoldval - Rrutoldval) >=
					_Setting->ErrorRutTh1)
				{
					Lrutoldval =
						Rrutoldval +
						RutNoise(i, 303);
				}
			}
			else if (Rrutoldval == 0.0f &&
				i > 0)
			{
				Rrutoldval =
					Rrutvals[i - 1] +
					RutNoise(i, 505);
			}
		}
		else
		{
			if (Lrutoldval > 1.0f)
			{
				if ((Rrutoldval - Lrutoldval) >=
					_Setting->ErrorRutTh1)
				{
					Rrutoldval =
						Lrutoldval +
						RutNoise(i, 404);
				}
			}
			else if (Lrutoldval == 0.0f &&
				i > 0)
			{
				Lrutoldval =
					Lrutvals[i - 1] +
					RutNoise(i, 606);
			}
		}

		// ����ȫ�� K/B ֻ�� AdjustRutVal ����Ӧ��һ�Ρ�
		Lrutvals[i] =
			static_cast<float>(
				_Setting->rutKCorrect *
				qAbs(Lrutoldval) +
				_Setting->rutBCorrect);

		Rrutvals[i] =
			static_cast<float>(
				_Setting->rutKCorrect *
				qAbs(Rrutoldval) +
				_Setting->rutBCorrect);

		if (valnum < 2)
		{
			if (i < LRutsr.size())
			{
				newLRutsr.append(
					QString("%1%2,%3")
					.arg(i + 1)
					.arg("0")
					.arg(QString::number(Lrutvals[i], 'f', 3)));
			}

			if (i < RRutsr.size())
			{
				newRRutsr.append(
					QString("%1%2,%3")
					.arg(i + 1)
					.arg("0")
					.arg(QString::number(Rrutvals[i], 'f', 3)));
			}
		}
		else
		{
			newLRutsr.append(
				QString("%1%2,%3,%4,%5")
				.arg(i + 1)
				.arg("0")
				.arg(QString::number(Lrutvals[i], 'f', 3))
				.arg(QString::number(qMax(Lrutvals[i], Rrutvals[i]), 'f', 3))
				.arg(QString::number(Rrutvals[i], 'f', 3)));
		}
	}

	QTextCodec* codeT =
		QTextCodec::codecForName("utf-8");

	MyCommonMethods::writeAllLines(
		leftPath,
		newLRutsr,
		codeT);

	if (valnum < 2)
	{
		MyCommonMethods::writeAllLines(
			rightPath,
			newRRutsr,
			codeT);
	}
}
void CalculationThread::CreateGaussFilter(std::unique_ptr<float[]>& gaus, int size, float sigma)
{
	double PI = 4.0 * atan(1.0); //Բ���ʦи�ֵ
	int center = size / 2;
	float sum = 0, tsigma = 0;
	double temp1 = 0, temp2 = 0;

	sigma = (float)(sqrt(log(2.0) / 2) / (sigma));
	temp1 = PI / sigma;
	temp2 = sqrt(PI) / sigma;
	for (int i = 0; i < size; i++)
	{
		gaus[i] = (float)(i - center) / center;
		tsigma = (float)(gaus[i] * temp1);
		gaus[i] = (float)(temp2 * exp(-tsigma * tsigma));
		sum += gaus[i];
	}

	for (int i = 0; i < size; i++)
	{
		gaus[i] = gaus[i] / sum;
	}
}
 

bool CalculationThread::JudgMTDval(QString prj, int side)
{

	//QString resamplefname = QString("%1\\Laser0\\MTD_100.txt").arg(prj).arg(QString::number(side));
	QString resamplefname =
		QString("%1\\Laser%2\\MTD_100.txt")
		.arg(prj)
		.arg(side);
	if (!QFile::exists(resamplefname))
	{
		return false;
	}
	QStringList data;
	QStringList  s;
	data = MyCommonMethods::ReadAllLines(resamplefname);
	if (data.size() > 2)
	{
		double tt = 0;
		int num = 0;
		for (int i = 0; i < data.size(); ++i)
		{
			try
			{
				s = data[i].split('\t');
				if (s.size() > 1)
				{
					tt += s[1].toDouble();
					num++;
				}

			}
			catch (...)
			{
			}
		}
		tt = tt / num;
		if (tt < _Setting->IRI_threshval)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

void CalculationThread::RemoveBigErr(float* rutval, int length)
{
	RemoveBigErrCore(
		rutval,
		length,
		_Setting->ErrorRutTh2,
		101);
}


bool CalculationThread::loadParm(bool isIRI, int side)
{
	QString dirName;
	QDir dir;
	m_mmutex.lock();
	if (isIRI)
	{
		dirName = QString("%1\\IRIMTD\\DAQ%2").arg(currentPorject->get2DProject()->getBasePath()).arg(QString::number(side));
		QString fileName;
		dir.setPath(dirName);
		if (dir.exists())
		{
			fileName = QString("%1\\IRIMTD\\DAQ%2\\Setting.ini").arg(currentPorject->get2DProject()->getBasePath()).arg(QString::number(side));
			QFile file;
			file.setFileName(fileName);
			if (!file.exists())
			{
				QString errorMsg = QStringLiteral("丢失配置文件：\r\n%1\r\n请从其他工程相同位置拷贝【Setting.ini】至此目录").arg(fileName);
				emit error(errorMsg);
				m_mmutex.unlock();
				return false;
			}
			QStringList iris = MyCommonMethods::ReadAllLines(fileName);
			QSettings settings(fileName, QSettings::IniFormat);
			settings.beginGroup(QString("IRISpeedCali"));
			m_IRI_k = 0;
			m_IRI_k = settings.value("IRIk").toDouble();
			if (m_IRI_k == 0)
			{
				m_IRI_k = 0.8969;
			}
			m_IRI_b = 0;
			m_IRI_b = settings.value("IRIb").toDouble();
			if (m_IRI_b == 0)
			{
				m_IRI_b = 0.359;
			}
			m_speedtype = false;
			m_speedtype = settings.value("SpeedType").toBool();
			m_Frequency = 0;
			settings.endGroup();
			settings.beginGroup(QString("SampleFrequency"));
			m_Frequency = settings.value("Frequency").toInt();
			if (m_Frequency == 0)
			{
				m_Frequency = 2000;
			}
			settings.endGroup();
			file.close();
		}
	}


	else
	{
		_MTDCali.resize(3);
		_MPDCali.resize(3);
		_mmPerPoint.resize(3);

		for (int i = 0; i < 3; i++)
		{
			_MTDCali[i].resize(2);
			_MPDCali[i].resize(2);
			dirName = QString("%1\\IRIMTD\\Laser%2").arg(currentPorject->get2DProject()->getBasePath()).arg(QString::number(i));
			dir.setPath(dirName);
			if (dir.exists())
			{
				QString	mtdfname = dirName + "\\MTD_10m.txt";
				QString iniFileName = dirName + "\\Setting.ini";
				QStringList mtds = MyCommonMethods::ReadAllLines(iniFileName);
				QSettings settings(iniFileName, QSettings::IniFormat);
				settings.beginGroup(QString("Parm"));
				_MTDCali[i][0] = settings.value("MTD_k").toDouble();
				_MTDCali[i][1] = settings.value("MTD_b").toDouble();
				_MPDCali[i][0] = settings.value("MTD_k").toDouble();
				_MPDCali[i][1] = settings.value("MTD_b").toDouble();
				_mmPerPoint[i] = settings.value("PMode").toInt();
				settings.endGroup();
				// 如果存在这个文件，有两种情况：1、旧软件计算的，MPD不加系数，2、新软件计算的，MPD要加系数
				// 如果不存在这个文件，那么就之间用这个新软件计算，MTD和MPD都会加系数 
				QFile file;
				file.setFileName(mtdfname);
				if (file.exists())
				{
					int tmp = -1;
					tmp = settings.value("Ver").toInt();
					if (tmp == -1)//不存在这个配置，说明用旧软件计算的构造深度，MPD不加系数，如果存在这个配置，说明是用新软件计算的构造深度，MPD加系数
					{
						try
						{
							_MPDCali[i][0] = settings.value("MTD_k").toDouble();
						}
						catch (...)
						{
							_MPDCali[i][0] = 1;
							m_mmutex.unlock();

						}
						try
						{
							_MPDCali[i][1] = settings.value("MTD_b").toDouble();
						}
						catch (...)
						{

							_MPDCali[i][1] = 0;
							m_mmutex.unlock();

						}
					}
					file.close();
				}
			}
		}



	}
	m_mmutex.unlock();

	return true;
}


double CalculationThread::GetLaserThresh(const QString& fpath)
{
	double res = 100;
	QStringList sdata;
	if (QFile::exists(fpath))
	{
		sdata = MyCommonMethods::ReadAllLines(fpath);
	}
	else
	{
		return res;
	}

	size_t len = sdata.size();
	//不足1米
	if (len < 20)
		return res;

	std::unique_ptr<double[]> lasval = std::make_unique<double[]>(len);
	QStringList s;
	for (int i = 0; i < len; ++i)
	{
		s = sdata[i].split('\t');
		if (s.size() > 3)
		{
			try
			{
				lasval[i] = s[0].toDouble();
			}
			catch (...)
			{
				if (i > 1)
				{
					lasval[i] = s[0].toDouble();
				}
			}
		}
	}

	res = 0;
	double tmpval = 0;
	for (int i = 1; i < len; ++i)
	{
		tmpval = qAbs(lasval[i] - lasval[i - 1]);
		if (res < tmpval)
		{
			res = tmpval;
		}
	}

	return res;
}

void CalculationThread::ComputeMTD(const QString& prj, int side, int featurelen, double threshval)
{
	int SizeLaserData = 24;
	QString  fname = QString("%1\\Laser%2\\MTD_%3m.txt").arg(prj).arg(side).arg(featurelen);
	QFile file(fname);
	const bool hasMtdResult = file.exists() && file.size() > 1;
	const QString lasvalPath = QStringLiteral("%1/Laser%2/lasval.txt").arg(prj).arg(side);
	const QFileInfo lasvalInfo(lasvalPath);
	if (hasMtdResult && lasvalInfo.isFile() && lasvalInfo.size() > 0)
	{
		emit progressUpdated(m_Project, m_type, 1.0, false);
		return;
	}
	QVector<QString>lasfile;
	MyCommonMethods::GetAllIRIFiles(prj, side, "Laser", "las", lasfile);
	emit progressUpdated(m_Project, m_type, 0.1, false);
	//m_stdMutex.lock();
	//m_stdProcessValue += 10;
	//emit progressUpdated(m_type, m_stdProcessValue, true);
	//m_stdMutex.unlock();  

	int MTDPOINT = 300 / _mmPerPoint[side] + 1;
	std::unique_ptr<int[]> laserX = std::make_unique<int[]>(MTDPOINT);
	for (int i = 0; i < MTDPOINT; i++)
	{
		laserX[i] = i - MTDPOINT / 2;
	}
	int m_PointNN = MTDPOINT * MTDPOINT;

	bool m_IsMTDFrame0 = true;
	double tempvaly = 0, tempval = 0, ttval = 0, oldttval = 200;
	int tempvalx = 0;
	int m_laserYIdx = 0;
	double m_laserYSum = 0;
	double m_laserYYSum = 0;
	double m_laserXYSum = 0;
	double m_laserXXYSum = 0;

	double m_SMTDdSum = 0;
	int m_SMTDdCnt = 0;
	int m_SMTDSubcnt = 0;//不足一个SMTD的点数计数
	int m_SMTDValCnt = 0;//有效的SMTD个数
	int m_featureDis = featurelen;

	int m_laspnum = 300 / (MTDPOINT - 1);
	int m_SMTDdnum = (int)(m_featureDis * 1000 / 300);//总共的SMTD个数
	int m_SMTDSubnum = (m_featureDis * 1000 - m_SMTDdnum * 300) / m_laspnum;//不足一个SMTD的点数

	double m_CurMTD;
	int m_MTDCnt = 0;
	int filecnt = 0;

	int ptcnt = 0;
	double filtersum = 0.0;
	double filtermean = 0.0;

	// 与二维格式一致：第一列为限幅后的采样值，第二列为去噪后的值。
	// 流式写入临时文件，完整成功后再替换正式文件，停止时自动丢弃半成品。
	if (lasfile.isEmpty())
	{
		emit error(QStringLiteral("无法输出纹理原始数据：未找到激光原始文件。\n%1/Laser%2").arg(prj).arg(side));
		return;
	}
	QSaveFile lasvalFile(lasvalPath);
	lasvalFile.setDirectWriteFallback(false);
	if (!lasvalFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		emit error(QStringLiteral("无法创建纹理原始数据文件：\n%1\n%2").arg(lasvalPath, lasvalFile.errorString()));
		return;
	}
	QTextStream lasvalStream(&lasvalFile);
	lasvalStream.setCodec("UTF-8");
	double rawLaserValue = 0.0;
	qint64 lasvalCount = 0;
	ulong framecnt = 0;
	QFile fmtd(fname);
	int lasFileCount = lasfile.size();

	if (hasMtdResult || fmtd.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		if (m_stopRequested)
		{

			return;
		}

		QStringList outList;
		QVector<double> value1M; //始终记录前500个点  用来获取中间值

		for (QString lf : lasfile)
		{
			QFileInfo lasf(lf);
			qint64 filesize = lasf.size();
			if (filesize <= 0 || filesize % SizeLaserData != 0)
			{
				emit error(QStringLiteral("激光原始文件为空或记录不完整，无法输出纹理原始数据：\n%1").arg(lf));
				return;
			}
			qint64 framenum = filesize / SizeLaserData;
			QFile fileLf(lf);
			if (!fileLf.open(QIODevice::ReadOnly))
			{
				emit error(QStringLiteral("无法读取激光原始文件：\n%1\n%2").arg(lf, fileLf.errorString()));
				return;
			}
			QDataStream in(&fileLf);
			in.setByteOrder(QDataStream::LittleEndian);
			QVector<QString> temps;
			int ceshiInt = 0;
			//更新总迭代次数   
			for (qint64 i = 0; i < framenum; ++i, ++framecnt)
			{
				if (m_stopRequested)
				{
					return;
				}
				fileLf.seek(fileLf.pos() + 16);
				in >> ttval;
				if (in.status() != QDataStream::Ok)
				{
					emit error(QStringLiteral("读取激光采样记录失败：\n%1").arg(lf));
					return;
				}
				ceshiInt++;
				if (ttval < 201) ttval = 200;
				else if (ttval > 399) ttval = 400;
				else
				{

					if (ptcnt < 25)
					{
						filtersum += ttval;

						++ptcnt;
					}
					if (value1M.size() < 500) //记录500-》1m数据 
					{
						value1M.push_back(ttval);
					}
				}
				if (value1M.size() >= 500)
				{
					value1M.remove(0);
					value1M.push_back(ttval);
				}
				rawLaserValue = ttval;
				//取25个点，是因为平整度的纵断面原始是50mm的采样间距，构造的激光数字信号是2mm的采样间距，所以取25个点，
				//先把平整度的纵断面原始激光数据做个去噪，然后得到平整度纵断面数据里面差距最大值作为构造的激光数字信号去噪的差值阈值
				if (ptcnt >= 25)
				{
					filtermean = filtersum / ptcnt;
					if (qAbs(filtermean - ttval) > threshval)
					{
						ttval = oldttval;
					}
					filtersum = filtersum + ttval - filtermean;
				}
				QVector<double> tempSort(value1M);
				std::sort(tempSort.begin(), tempSort.end());
				if (value1M.size() > 0)
				{
					oldttval = tempSort[tempSort.size() / 2];
				}
				lasvalStream << QString::number(rawLaserValue, 'g', 17) << QLatin1Char('\t')
					<< QString::number(ttval, 'g', 17) << QLatin1Char('\n');
				++lasvalCount;
				if (lasvalStream.status() != QTextStream::Ok)
				{
					emit error(QStringLiteral("写入纹理原始数据失败：\n%1\n%2").arg(lasvalPath, lasvalFile.errorString()));
					return;
				}
				if (m_IsMTDFrame0)
				{
					if (++m_SMTDSubcnt >= m_SMTDSubnum)
					{
						m_IsMTDFrame0 = false;
					}
				}
				else
				{
					tempvaly = ttval;
					tempvalx = laserX[m_laserYIdx];
					m_laserYSum += tempvaly;
					m_laserYYSum += tempvaly * tempvaly;
					tempvaly *= tempvalx;
					m_laserXYSum += tempvaly;
					m_laserXXYSum += tempvaly * tempvalx;

					if (++m_laserYIdx == MTDPOINT)
					{
						tempval = (m_PointNN - 1) * m_laserYSum - 12 * m_laserXXYSum;
						tempval = 5 * tempval * tempval / (4 * (m_PointNN - 4));
						tempval = (12 * m_laserXYSum * m_laserXYSum + tempval) / (m_PointNN - 1);
						tempval = (MTDPOINT * m_laserYYSum - m_laserYSum * m_laserYSum - tempval) / m_PointNN;

						tempval = tempval > 0 ? tempval : 0;
						m_SMTDdSum += qSqrt(tempval);
						++m_SMTDValCnt;

						if (++m_SMTDdCnt == m_SMTDdnum)
						{
							double value = 0.1 + 0.9 / lasfile.size() * (filecnt + (double)i / framenum);
							emit progressUpdated(m_Project, m_type, value, false);
							m_CurMTD = m_SMTDdSum / m_SMTDValCnt * _MTDCali[side][0] + _MTDCali[side][1];
							++m_MTDCnt;
							QString lineStr = QString::number(m_MTDCnt) + QString(" ") + QString::number(m_CurMTD);
							outList.push_back(lineStr);

							m_SMTDdSum = 0;
							m_SMTDdCnt = 0;
							m_SMTDValCnt = 0;
							m_SMTDSubcnt = 0;
							m_IsMTDFrame0 = true;
						}
						tempvaly = ttval;
						tempvalx = laserX[0];
						m_laserYSum = tempvaly;
						m_laserYYSum = tempvaly * tempvaly;
						tempvaly *= tempvalx;
						m_laserXYSum = tempvaly;
						m_laserXXYSum = tempvaly * tempvalx;
						m_laserYIdx = 1;

					}

				}
			}
			fileLf.close();
			filecnt++;

		}
		if (m_stopRequested)
		{
			return;
		}
		lasvalStream.flush();
		if (lasvalCount == 0 || lasvalStream.status() != QTextStream::Ok)
		{
			emit error(QStringLiteral("纹理原始数据为空或写入失败：\n%1").arg(lasvalPath));
			return;
		}
		if (!lasvalFile.commit())
		{
			emit error(QStringLiteral("保存纹理原始数据失败：\n%1\n%2").arg(lasvalPath, lasvalFile.errorString()));
			return;
		}
		// 已有 MTD 结果时只补齐纹理文件，保留原有指标结果。
		if (!hasMtdResult)
		{
			MyCommonMethods::writeAllLines(fname, outList);
			fmtd.close();
		}
	}
	else
	{
		emit error(QStringLiteral("无法创建构造深度结果文件：\n%1\n%2").arg(fname, fmtd.errorString()));
		return;
	}
	if (hasMtdResult)
	{
		emit progressUpdated(m_Project, m_type, 1.0, false);
		return;
	}
	//AdjustVal(fname, _Setting->ErrorMTD, 0.0005);
	QString ininame = QString("%1\\Laser%2\\Setting.ini").arg(prj).arg(side);
	QSettings mtdparm(ininame, QSettings::IniFormat);
	mtdparm.beginGroup("Parm");
	mtdparm.setValue("Ver", 1);
	mtdparm.endGroup();

}

void CalculationThread::ComputeMPD(const QString& prj, int side, int featurelen, double threshval)
{
	int SizeLaserData = 24;
	QString  fname = QString("%1\\Laser%2\\MPD_%3m.txt").arg(prj).arg(side).arg(featurelen);
	QFile file(fname);
	if (file.exists() && file.size() > 1)
	{
		emit progressUpdated(m_Project, m_type, 1.0, false);
		return;
	}
	QVector<QString>lasfile;
	MyCommonMethods::GetAllIRIFiles(prj, side, "Laser", "las", lasfile);
	emit progressUpdated(m_Project, m_type, 0.1, false);


	double ttval = 0;
	double oldttval = 200;

	int m_featureDis = featurelen;
	vector<double> ratio;

	ulong MPDPointNum = (ulong)(100 / _mmPerPoint[side]);
	ulong MPDPointNumHalf = MPDPointNum / 2;
	double YMax1 = 0.0;
	double YMax2 = 0.0;
	double YMean = 0.0;
	double curMPDB = 0.0;
	double curMPD = 0.0;
	int MPDBCnt = 0;
	int MPDCnt = 0;
	int MPDBNUM = featurelen * 1000 / 100;

	int ptcnt = 0;
	double filtersum = 0.0;
	double filtermean = 0.0;

	std::vector<double> laserX(MPDPointNum, 0);
	for (ulong i = 0; i < MPDPointNum; i++)
	{
		laserX[i] = i;
	}
	std::vector<double> laserY(MPDPointNum, 0);
	int filecnt = 0;
	ulong framecnt = 0;
	QFile fmpd(fname);
	if (fmpd.open(QIODevice::Text | QIODevice::WriteOnly))
	{

		QStringList resultData;
		QVector<double> value1M; //始终记录前25个点  用来获取中间值				

		for (QString lf : lasfile)
		{
			QFileInfo lasf(lf);
			long filesize = lasf.size();
			long framenum = filesize / SizeLaserData;
			QFile fileLf(lf);
			if (!fileLf.open(QIODevice::ReadOnly))
			{
				continue;
			}
			QDataStream br(&fileLf);
			//List<double> jzlb5Value = new List<double>();
			br.setByteOrder(QDataStream::LittleEndian);

			double tMile = 0;
			for (int i = 0; i < framenum; ++i, ++framecnt)
			{
				if (m_stopRequested)
				{
					return;
				}
				fileLf.seek(fileLf.pos() + 16);
				br >> ttval;

				if (ttval < 201) ttval = 200;
				else if (ttval > 399) ttval = 400;
				else
				{
					//取25个点，是因为平整度的纵断面原始是50mm的采样间距，构造的激光数字信号是2mm的采样间距，所以取25个点，
					//先把平整度的纵断面原始激光数据做个去噪，然后得到平整度纵断面数据里面差距最大值作为构造的激光数字信号去噪的差值阈值
					if (ptcnt < 25)
					{
						filtersum += ttval;

						++ptcnt;
					}
					if (value1M.size() < 500) //记录500-》1m数据 
					{
						value1M.push_back(ttval);
					}
				}
				if (value1M.size() >= 500)
				{
					value1M.removeAt(0);
					value1M.push_back(ttval);
				}
				if (ptcnt >= 25)
				{
					tMile += 0.002;
					filtermean = filtersum / ptcnt;
					if (qAbs(filtermean - ttval) > threshval)
					{
						ttval = oldttval;
					}

					filtersum = filtersum + ttval - filtermean;
				}


				QVector<double> tempSort(value1M);

				std::sort(tempSort.begin(), tempSort.end());

				if (value1M.size() > 0)
				{
					oldttval = tempSort[tempSort.size() / 2];
				}
				if (framecnt < MPDPointNum)
				{
					laserY[framecnt] = ttval;
				}
				else
				{
					YMax1 = -10000;
					YMax2 = -10000;
					YMean = 0;
					ratio = FittingFunct::TowTimesCurve(laserY, laserX);
					for (ulong j = 0; j < MPDPointNum; j++)
					{
						laserY[j] = laserY[j] - (ratio[0] + ratio[1] * laserX[j] + ratio[2] * laserX[j] * laserX[j]);
					}
					for (ulong j = 0; j < MPDPointNumHalf; j++)
					{
						YMean += laserY[j];
						if (laserY[j] > YMax1)
							YMax1 = laserY[j];
					}
					for (ulong j = MPDPointNumHalf; j < MPDPointNum; j++)
					{
						YMean += laserY[j];
						if (laserY[j] > YMax2)
							YMax2 = laserY[j];
					}
					YMean = YMean / MPDPointNum;
					curMPDB = (YMax1 + YMax2) / 2 - YMean;

					curMPD = curMPD + curMPDB;
					++MPDBCnt;

					if (MPDBCnt == MPDBNUM)
					{
						double value = 0.1 + 0.9 / lasfile.size() * (filecnt + (double)i / framenum);
						emit progressUpdated(m_Project, m_type, value, false);
						++MPDCnt;
						curMPD = curMPD / MPDBCnt;
						curMPD = curMPD * _Setting->MPD_K + _Setting->MPD_B;
						curMPD = curMPD * _MPDCali[side][0] + _MPDCali[side][1];

						QString lineStr = QString::number(MPDCnt) + QString(" ") + QString::number(curMPD);

						resultData.push_back(lineStr);

						MPDBCnt = 0;
						curMPD = 0;
					}

					framecnt = 0;
					laserY[framecnt] = ttval;
				}

			}
			fileLf.close();

			filecnt++;
		}
		MyCommonMethods::writeAllLines(fname, resultData);
		fmpd.close();
		//	AdjustVal(fname, _Setting->ErrorMTD, 0.0005);
	}

}

void CalculationThread::AdjustVal(QString fname, double Thrval, double scale)
{
	// C#：
	// if (_Setting.ErrorVal != 1)
	//     return;
	//
	// 注意：
	// 如果 HnXRSettings 中字段名字不是 ErrorVal，
	// 请替换成你 C++ 设置类中的对应字段。
	if (_Setting->ErrorVal != 1)
	{
		return;
	}

	QFile file(fname);

	if (!file.exists())
	{
		return;
	}

	QStringList oristrs =
		MyCommonMethods::ReadAllLines(fname);

	int len = oristrs.size();

	if (len < 2)
	{
		return;
	}

	QVector<double> orival(len);

	// 读取 IRI
	// 文件格式：
	// 1 2.345
	// 2 2.456
	for (int i = 0; i < len; ++i)
	{
		QString line = oristrs[i].trimmed();

		int pos = line.lastIndexOf(' ');

		if (pos >= 0)
		{
			orival[i] =
				line.mid(pos + 1).toDouble();
		}
		else
		{
			orival[i] = 0.0;
		}
	}

	double lastval = orival[0];
	double sumval = 0.0;

	// C# Random.Next(100) 范围是 0~99
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> randomDist(0, 99);

	// --------------------------------------------------
	// 第一部分：异常 IRI 修正
	// 对应 C#：
	//
	// orival[i] = orival[i] > Thrval
	//     ? lastval +
	//       (MainForm.rdval.Next(100) - 50) * scale
	//     : orival[i];
	// --------------------------------------------------
	for (int i = 1; i < len; ++i)
	{
		if (orival[i] > Thrval)
		{
			int randValue = randomDist(gen);

			orival[i] =
				lastval +
				(randValue - 50) * scale;
		}

		sumval += orival[i];

		lastval = sumval / i;
	}

	QStringList newstrs;

	newstrs.reserve(len);

	// --------------------------------------------------
	// 第二部分：最终 IRI K/B 修正
	//
	// 对应 C#：
	//
	// orival[i] =
	//     _Setting.iriKCorrect * orival[i]
	//     + _Setting.iriBCorrect;
	// --------------------------------------------------
	for (int i = 0; i < len; ++i)
	{
		orival[i] =
			_Setting->iriKCorrect * orival[i]
			+ _Setting->iriBCorrect;

		QString index =
			oristrs[i]
			.trimmed()
			.section(' ', 0, 0);

		QString line =
			QString("%1 %2")
			.arg(index)
			.arg(QString::number(
				orival[i],
				'g',
				15));

		newstrs.append(line);
	}

	// 一行一个 IRI
	MyCommonMethods::writeAllLines(
		fname,
		newstrs);
}


void CalculationThread::run()
{
	if (m_stopRequested) return;
	/*m_smutex.lock();
	CheckSetting(currentPorject);
	m_smutex.unlock();*/
	switch (m_type)
	{
	case CalculationThread::IRI:
	{
		QString iriPath = currentPorject->get2DProject()->getIRIPath();
		for (int i = 0; i < 2; ++i)
		{
			int temp = i;
			if (!loadParm(true, temp))
			{
				return;
			}
			if (m_stopRequested)
			{
				return;
			}
			int t = i;
			QString nowIriPath = QStringLiteral("%1/DAQ%2").arg(iriPath).arg(t);

			QDir dir(nowIriPath);
			if (dir.exists())
			{
				QString fpath = nowIriPath + "/resample.txt";
				if (_Setting->Las_Filter)
				{
					FilterLaserData(fpath, _Setting->Las_Filter_Thresh0, _Setting->Las_Filter_Thresh1);
				}
				else
				{
					emit progressUpdated(m_Project, m_type, 10, true);
					if (QFile::exists(fpath + ".bak"))
					{
						QFile::remove(fpath);
						MyCommonMethods::moveFile(fpath + ".bak", fpath);
					}
				}
				StartIRMThread(currentPorject, iriPath, nowIriPath, fpath, t);
			}
		}
		if (!m_stopRequested)
		{
			emit progressUpdated(m_Project, m_type, 100, true);
		}

		break;

	}

	case CalculationThread::Rut:
	{
		auto cur2dPro = currentPorject->get2DProject();
		bool dataproc1 = false;
		switch (cur2dPro->_RutMode)
		{
		case 0:
			dataproc1 = ComputeRut(true, 3, cur2dPro->_RutMode);
			if (dataproc1)
			{
				  AdjustRutVal(3);
			}
			break;
		case  1:
			dataproc1 = ComputeRut(true, 1, cur2dPro->_RutMode);
			if (dataproc1)
			{
				 AdjustRutVal(1); 
			}
			break;
		case  2:
			dataproc1 = ComputeRut(true, 3, cur2dPro->_RutMode);
			if (dataproc1)
			{
				 AdjustRutVal(3);
			}
			break;
		default:
			emit error(QStringLiteral("工程【%1】车辙计算未启动：不支持的车辙模式 %2。")
				.arg(currentPorject->get2DProName())
				.arg(cur2dPro->_RutMode));
			break;
		}
		if (dataproc1 && !m_stopRequested)
		{
			emit progressUpdated(m_Project, m_type, 100, true);

		}


		break;
	}
	case CalculationThread::Smtd:
	case CalculationThread::Smpd:
	{
		QString iriPath = currentPorject->get2DProject()->getIRIPath();

		if (currentPorject->get2DProject()->_IsMMTD)
		{
		}
		else
		{
		}
		std::vector<std::unique_ptr<QThread>> threads;
		if (!loadParm(false, -1))
		{
			return;
		}
		for (int i = 0; i < 3; ++i)
		{
			int temp = i;

			if (i == 2)
			{
				if (!currentPorject->get2DProject()->_IsMMTD)
				{
					continue;
				}
			}
			auto thread = std::make_unique<QThread>(this);
			QObject::connect(thread.get(), &QThread::started, [this, i, iriPath, thread = thread.get()]()
				{
					if (m_stopRequested)
					{
						thread->quit();
						return;
					}
					QDir dir;
					QString laserPath = iriPath + QString("\\Laser%1").arg(i);
					double lasthreshval = 50;
					dir.setPath(laserPath);
					if (dir.exists())
					{
						QString fpath = iriPath + QString("\\DAQ%1\\resample.txt").arg(i);
						if (i == 2)
						{
							fpath = iriPath + QString("\\DAQ%1\\resample.txt").arg(0);
						}
						QFile daqPath(fpath);
						if (daqPath.exists())
						{
							lasthreshval = GetLaserThresh(fpath);

							if (m_type == CalculationThread::Smtd)
							{
								ComputeMTD(iriPath, i, 10, lasthreshval);
								thread->quit();

							}
							if (m_type == CalculationThread::Smpd)
							{
								ComputeMPD(iriPath, i, 10, lasthreshval);
								thread->quit();
							}

						}
					}
					thread->quit();
				}
			);
			threads.push_back(std::move(thread));
		}

		for (auto& thread : threads)
		{
			thread->start();
		}


		for (auto& thread : threads)
		{
			thread->wait();

		}
		if (!m_stopRequested)
		{
			emit progressUpdated(m_Project, m_type, 100, true);
		}
		break;
	}
	case CalculationThread::Jhxx:
	{
		if (m_stopRequested)
		{
			return;
		}
		if (!m_stopRequested)
		{
			emit progressUpdated(m_Project, m_type, 100, true);
		}
		break;

	}
	default:
	{
		break;
	}

	}
}
void CalculationThread::stop()
{
	m_stopRequested = true;
}


CalculationThread::~CalculationThread()
{

}
