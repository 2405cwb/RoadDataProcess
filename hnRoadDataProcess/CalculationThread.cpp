
#include "CalculationThread.h"
#include <QMenu>
#include <QFile>
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
#include <QMetaObject>
#include <QMutexLocker>
#include <algorithm>
#include <cmath>
#include <deque>
#include <iterator>
#include <numeric>
#include <set>

namespace
{
class SlidingUpperMedianWindow
{
public:
	void append(double value)
	{
		m_values.push_back(value);
		m_sorted.insert(value);
	}

	void slide(double value)
	{
		if (!m_values.empty())
		{
			const double oldest = m_values.front();
			m_values.pop_front();
			auto iter = m_sorted.find(oldest);
			if (iter != m_sorted.end())
			{
				m_sorted.erase(iter);
			}
		}
		append(value);
	}

	int size() const
	{
		return static_cast<int>(m_values.size());
	}

	bool isEmpty() const
	{
		return m_values.empty();
	}

	double upperMedian() const
	{
		auto iter = m_sorted.begin();
		std::advance(iter, m_sorted.size() / 2);
		return *iter;
	}

private:
	std::deque<double> m_values;
	std::multiset<double> m_sorted;
};
}

CalculationThread::CalculationThread(CalculationType type, int taskId, hnPro::hnProject* project)
	: m_type(type), m_stopRequested(false), currentPorject(project), m_taskID(taskId)
{
	_Setting = HnXRSettings::getInstance();
	m_stdProcessValue = 0;
	m_spdProcessValue = 0;
	m_Project = project;
	setAutoDelete(false);
}

bool CalculationThread::isStopRequested() const
{
	return m_stopRequested.load(std::memory_order_relaxed);
}

void CalculationThread::StartIRMThread(hnPro::hnProject* pro, const QString iriPath, const QString & daqBasePath, const QString& resamplePath, int side)
{
	bool datasrc = JudgMTDval(iriPath, side);
	GenerateIRI_NEW(resamplePath.toStdString(), 10, "resample.txt", datasrc);
	emit progressUpdated(m_Project, m_type, 90, true);
}

void CalculationThread::CheckSetting(hnPro::hnProject*  project)
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
		if (i%sumCount == 0)
		{
			irival = irisum / plusenum;
			if (datasrc)
			{

				irival = irival *m_IRI_k + m_IRI_b;
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

void CalculationThread::GenerateIRI_NEW(const std::string& fpath, int vallen, const std::string& fname, bool datasrc)
{
	// 步骤1: 加载速度修正参数（如果存在）
	auto result1 = LoadParameters(fpath, fname);
	bool isParmFile;
	std::vector<double>  speedparms, kparms, bparms;
	int parmnum;
	std::tie(isParmFile, speedparms, kparms, bparms, parmnum) = result1;

	// 步骤2: 加载和预处理原始数据
	double DeltLen = 0.1; // 采样间隔（0.1m 或 0.25m）
	auto result2 =  LoadData(fpath);
	std::vector<double >oridata, toridata;
	std::vector<int>oritime;
	std::vector<std::string> sdata;
	int len;
	std::tie(oridata, toridata, oritime, sdata, len) = result2;

	if (m_Project != nullptr)
	{
		int maxRawLen = static_cast<int>(std::round(m_Project->getCurProSetInfo().dEndEnclMile / 0.05));

		len = qMin(len, maxRawLen);

		if (len < 0)
			len = 0;

		oridata.resize(len);
		toridata.resize(len);
		oritime.resize(len);
		sdata.resize(len);
	}


	// 步骤3: 应用均值滤波（可选，默认注释，与原始版本一致）
	for (int i = 2; i < len - 2; ++i) {
		oridata[i] = (toridata[i - 2] + toridata[i - 1] + toridata[i] + toridata[i + 1] + toridata[i + 2]) / 5.0;
	}

	// 步骤3: 抽样到指定间隔（DeltLen）
	int qplusenum = static_cast<int>(DeltLen / 0.05); // 每0.1m抽样点数（0.05m原始间隔）

	auto result3 = ResampleData(oridata, oritime, len, qplusenum);
	std::vector<double> iridata;
	std::vector<int> iritime;
	std::tie(iridata, iritime) = result3;

	len = static_cast<int>(iridata.size());
	if (len <= 0)
	{
		return;
	}

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
	int start_i = (DeltLen == 0.1) ? 3 : 1; // 0.1m时从i=3开始以支持尾随YSU
	std::vector<double> ZSU(4, 0.0);

	for (int i = start_i; i < len; ++i) {
		// 每段结束时处理速度和IRI
		if (i % plusenum == 0) {
			double etime = ParseTime(iritime[i]);
			double speedval = (etime - stime) > 0 ? (vallen / (etime - stime) * 3.6) : 0.0;
			double irival = count > 0 ? irisum / count : 0.0;

			// 应用IRI修正（加速度或速度因子）
			irival = ApplyCorrection(irival, datasrc, speedval, isParmFile, speedparms, kparms, bparms, parmnum);

			// 写入输出
			swIRI << iricnt + 1 << " " << irival << "\n";
			swspeed << iricnt + 1 << " " << speedval << "\n";

			// 重置
			irisum = 0.0;
			count = 0;
			stime = etime;
			++iricnt;
		}

		// 计算YSU（输入坡度）
		double YSU = ComputeYSU(iridata, i, DeltLen);

		// 更新状态
		UpdateState(ZSU, oldZSU, YSU, DeltLen);

		// 累加IRI
		irisum += std::abs(ZSU[0] - ZSU[2]);
		++count;

		// 更新oldZSU
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
	int index = (DeltLen == 0.1) ?qMin(110, len - 1) : qMin(44, len - 1);
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
	if (i < 1) return 0.0;
	if (DeltLen == 0.1) {
		if (i < 3) return 0.0;
		return (iridata[i] - iridata[i - 3]) / 0.3;
	}
	else {
		return (iridata[i] - iridata[i - 1]) / DeltLen;
	}
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
		irival = irival *  1 + 0;
	}
	else {

		if (isParmFile && parmnum > 0 && kparms.size() >= static_cast<size_t>(parmnum) && bparms.size() >= static_cast<size_t>(parmnum))
		{
			double kparm = kparms[parmnum - 1];
			double bparm = bparms[parmnum - 1];
		/*	for (int pi = 0; pi < parmnum; ++pi)
			{
				if (speedval <= speedparms[pi])
				{
					kparm = kparms[pi];
					bparm = bparms[pi];
					break;
				}


			}*/
			kparm = std::accumulate(kparms.begin(), kparms.end(), 0.0) / kparms.size();
			//	irival = irival * kparm + bparm;
			irival = irival * kparm;
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

	std::unique_ptr<int[]>MP_idx(new int[20]{ 0 });
	std::unique_ptr<float[]> MP_val(new float[20]{ 0 });
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
		int cameracnt = valnum == 1 ? 2 : 1;
		int rbmatSize = 0;
		if (rutmode == 2)
		{
			matobj3d.reset(new double[hpix] {0});

			rbmat.reset(new unsigned   char[hpix * 8]{ 0 });
			rbmatSize = hpix * 8;
		}
		else
		{
			matobj.reset(new float[hpix] {0});
			rbmat.reset(new unsigned   char[hpix * 4]{ 0 });
			rbmatSize = hpix * 4;
		}
		//使用一维数组模拟二维数组
		c2wmatrix.resize(vpix*hpix, 0);
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
				dir1.mkdir(process[i]);
			}
			string sPath = c2w[i].toLocal8Bit();
			const char * path = sPath.c_str();
			std::ifstream fil(path, std::ios::binary);


			fil.seekg(0, std::ios::end);
			auto  fileSize = fil.tellg();
			fil.seekg(0, std::ios::beg);

			//计算预期大小
			size_t  expectedSize = vpix * hpix * (rutmode == 2 ? 8 : 4);
			if (fileSize < expectedSize)
			{
				qDebug() << "rut file too small. Expected" << expectedSize << "Actual" << fileSize;
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
							c2wmatrix[n*hpix + k] = static_cast<float>(matobj3d[k]);
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
							c2wmatrix[n*hpix + k] = matobj[k];
						}
					}
				}
				fileC2w.close();
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
					const char *  nowDatPath = datPath.c_str();
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
						const char *  nowDatPath1 = datPath1.c_str();
						//std::filesystem::
						std::ofstream filDat1;
						filDat1.open(nowDatPath1, std::ios::binary);
						//	filDat1.open(nowDatPath1);
						temp = hpix * 2;
						while (true)
						{
							if (isStopRequested())
							{
								return false;

							}
							if (filDat.eof())
							{
								break;
							}
							filDat.read(reinterpret_cast<char*> (rbarr.data()), temp);

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

				QString frutname = basePath + "\\RUT\\camera" + QString::number(i) + "\\orirut.txt";
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
				}
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
					return false;
				}
				QTextStream sworirut(&fworirut);

				if (!fwrut.open(QIODevice::WriteOnly | QIODevice::Text))
				{
					return false;
				}
				QTextStream swrut(&fwrut);
				int tempIndex = 0;
				int ceshi = 0;
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
								if (isStopRequested())
								{
									return false;

								}
								if (filDat.eof())
								{
									break;
								}
								filDat.read(reinterpret_cast<char*> (rbarr.data()), temp);

								memcpy(profile.data(), rbarr.data(), hpix * 2);

								for (m = 0, n = 0; m < hpix; ++m)
								{
									if (n == 2015)
									{
										int tta = 0;
									}
									if (profile[m] <= 0 || profile[m] >= vpix)
									{
										profobj[m] = 0x7fff;
										objlas[m] = 0x7fff;
									}
									else
									{
										float value1 = (c2wmatrix[profile[m] * hpix + m] - basezval);
										objlas[m] = value1;
										if (rutmode == 2)
										{
											objlas[n] = -objlas[n];
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

									QString line = QString::number(linecnt) + "0," + QString::number(arutval) + "\n";
									sworirut << line;
									swrut << line;

								}
								else
								{
									if (n > 1200)
									{
										tempIndex++;
										int _cept = _cep - (m - n);
										hnComputeCUT cut;

										brutval = cut.computerut3(hpix, objlas.data(), tobjlas.data(), _asp, _cept, arutval, crutval, lineK, _gslen, MP_idx.get(), MP_val.get());


										arutval = qAbs(arutval * _rutk + _rutb);//左
										crutval = qAbs(crutval * _rutk + _rutb);//右
										brutval = qAbs(brutval * _rutk + _rutb);//最大值
										double K = _Setting->rutKCorrect;
										double B = _Setting->rutBCorrect;  //

										arutval = qAbs(arutval *  K + B);//左
										crutval = qAbs(crutval * K + B);//右
										brutval = qAbs(brutval * K + B);//最大值
									}
									else
									{
										arutval = 0;
										crutval = 0;
										brutval = 0;
									}
									if (isStopRequested())
									{
										return false;

									}
									++linecnt;
									QString line = QString::number(linecnt) + "0," + QString::number(arutval) + "," + QString::number(brutval) + "," + QString::number(crutval) + "\n";
									sworirut << line;
									swrut << line;

								}
								if (linecnt % 1000 == 0)
								{
									emit progressUpdated(m_Project, m_type, (0.42 + fsbar2 + filDat.tellg()* fsbar), false);
									//if (Isbar) bar.SetRutVal(0.42 + fsbar2 + frstream.Position * fsbar);
								}
							}


						}
						filDat.close();

					}
					catch (...)
					{

					}

				}
				fwrut.close();
				fworirut.close();
			}
		}
	}
	catch (...)
	{
		throw;
	}
}

void CalculationThread::AdjustRutVal(int valnum)
{
	QStringList LRutsr;
	QVector<QString> newLRutsr;
	QStringList RRutsr;
	QStringList newRRutsr;
	QString basePath = currentPorject->get2DProPath();
	QString path(QString("%1\\Rut\\camera0\\orioldrut.txt").arg(basePath));
	QString path1(QString("%1\\Rut\\camera0\\orirut.txt").arg(basePath));
	QFile file;
	QFile file1;
	file.setFileName(path);
	file1.setFileName(path1);
	if (file.exists())
	{
		LRutsr = MyCommonMethods::ReadAllLines(path);

	}
	else if (file1.exists())
	{
		LRutsr = MyCommonMethods::ReadAllLines(path1);
	}
	else
	{
		return;
	}
	newLRutsr.resize(LRutsr.size());
	if (valnum < 2)
	{

		QString rpath(QString("%1\\Rut\\camera1\\orioldrut.txt").arg(basePath));
		QString rpath1(QString("%1\\Rut\\camera1\\orirut.txt").arg(basePath));
		QFile rfile;
		QFile rfile1;
		rfile.setFileName(rpath);
		rfile1.setFileName(rpath1);
		if (rfile.exists())
		{
			RRutsr = MyCommonMethods::ReadAllLines(rpath);

		}
		else if (rfile1.exists())
		{
			RRutsr = MyCommonMethods::ReadAllLines(rpath1);
		}
		else
		{
			return;
		}
		newRRutsr.reserve(RRutsr.size());
	}
	int lenval = 0;
	if (valnum < 2)
	{
		lenval = qMax(LRutsr.size(), RRutsr.size());
		if (LRutsr.size() < 2)
			return;
		if (RRutsr.size() < 2)
			return;
	}
	else
	{
		lenval = LRutsr.size();
		if (LRutsr.size() < 2)
			return;
	}

	std::unique_ptr<float[]> Lrutvals(new float[lenval]);
	std::unique_ptr<float[]> Rrutvals(new float[lenval]);
	QString LRutstrline, RRutstrline;
	float Lrutoldval = 0, Rrutoldval = 0;
	float Lrutcurval = 0, Rrutcurval = 0;

	QStringList trut;

	for (int i = 0; i < lenval; ++i)
	{
		if (valnum < 2)//双车辙
		{
			if (i < LRutsr.size())
			{
				LRutstrline = LRutsr[i];
				trut = LRutstrline.split(',');
				try
				{
					Lrutcurval = trut[1].toFloat();
				}
				catch (...)
				{
					Lrutcurval = Lrutoldval;
				}

				if (_Setting->IsThresholdRut)
				{
					if (Lrutcurval > 0)
					{
						Lrutcurval = Lrutcurval / qCeil(Lrutcurval / _Setting->ErrorRut);
					}
				}

				Lrutvals[i] = Lrutcurval;
				Lrutoldval = Lrutcurval;
			}
			if (i < RRutsr.size())
			{
				RRutstrline = RRutsr[i];
				trut = RRutstrline.split(",");
				try
				{
					Rrutcurval = trut[1].toFloat();
				}
				catch (...)
				{
					Rrutcurval = Rrutoldval;
				}

				if (_Setting->IsThresholdRut)
				{
					if (Rrutcurval > 0)
					{
						Rrutcurval = Rrutcurval / qCeil(Rrutcurval / _Setting->ErrorRut);
					}
				}

				Rrutvals[i] = Rrutcurval;
				Rrutoldval = Rrutcurval;
			}
		}
		else //单车辙
		{
			if (i < LRutsr.size())
			{
				LRutstrline = LRutsr[i];
				trut = LRutstrline.split(",");
				try
				{
					Lrutcurval = trut[1].toFloat();
					Rrutcurval = trut[3].toFloat();
				}
				catch (...)
				{
					Lrutcurval = Lrutoldval;
					Rrutcurval = Rrutoldval;
				}

				if (_Setting->IsThresholdRut)
				{
					if (Lrutcurval > 0)
					{
						Lrutcurval = Lrutcurval / qCeil(Lrutcurval / _Setting->ErrorRut);
					}
					if (Rrutcurval > 0)
					{
						Rrutcurval = Rrutcurval / qCeil(Rrutcurval / _Setting->ErrorRut);
					}
				}

				Lrutvals[i] = Lrutcurval;
				Rrutvals[i] = Rrutcurval;

				Lrutoldval = Lrutcurval;
				Rrutoldval = Rrutcurval;
			}
		}

	}
	RemoveBigErr(Lrutvals.get(), lenval);
	RemoveBigErr(Rrutvals.get(), lenval);
	std::uniform_int_distribution<>::param_type newPara(1, 100);
	dis.param(newPara);
	// 左右侧车辙之间调整比较
	for (int i = 0; i < lenval; ++i)
	{
		Lrutoldval = Lrutvals[i];
		Rrutoldval = Rrutvals[i];

		if (Lrutoldval > Rrutoldval)
		{
			if (Rrutoldval > 1.0)
			{
				if ((Lrutoldval - Rrutoldval) >= _Setting->ErrorRutTh1)
				{
					auto seed = std::chrono::system_clock::now().time_since_epoch().count();
					std::mt19937 gen(seed);
					Lrutoldval = Rrutoldval + dis(gen) * 0.01f - 0.5f;
				}
			}
			else if (Rrutoldval == 0.0)
			{
				if (i > 0)
				{
					auto seed = std::chrono::system_clock::now().time_since_epoch().count();
					std::mt19937 gen(seed);
					Rrutoldval = Rrutvals[i - 1] + dis(gen) * 0.01f - 0.5f;
				}
			}
		}
		else
		{
			if (Lrutoldval > 1.0)
			{
				if ((Rrutoldval - Lrutoldval) >= _Setting->ErrorRutTh1)
				{
					auto seed = std::chrono::system_clock::now().time_since_epoch().count();
					std::mt19937 gen(seed);
					Rrutoldval = Lrutoldval + dis(gen)* 0.01f - 0.5f;
				}
			}
			else if (Lrutoldval == 0.0)
			{
				if (i > 0)
				{
					auto seed = std::chrono::system_clock::now().time_since_epoch().count();
					std::mt19937 gen(seed);
					Lrutoldval = Lrutvals[i - 1] + dis(gen) * 0.01f - 0.5f;
				}
			}
		}
		Lrutvals[i] = qAbs(Lrutoldval);
		Rrutvals[i] = qAbs(Rrutoldval);

		if (valnum < 2)//双车辙
		{
			if (i < newLRutsr.size())
			{

				newLRutsr[i] = QString::number(i + 1) + "0," + QString::number(Lrutvals[i]);
			}
			if (i < newRRutsr.size())
			{
				newLRutsr[i] = QString::number(i + 1) + "0," + QString::number(Rrutvals[i]);
			}
		}
		else
		{
			newLRutsr[i] = QString::number(i + 1) + "0," + QString::number(Lrutvals[i]) + "," + QString::number(qMax(Lrutvals[i], Rrutvals[i])) + "," + QString::number(Rrutvals[i]);
		}
	}

	path1 = QString("%1\\Rut\\camera0\\orioldrut.txt").arg(basePath);
	QString path2 = QString("%1\\Rut\\camera0\\orirut.txt").arg(basePath);
	file1.setFileName(path1);
	if (!file1.exists())
	{
		QFile::copy(path2, path1);
	}
	MyCommonMethods::writeAllLines(path2, newLRutsr);

	if (valnum < 2)//双车辙
	{
		path1 = QString("%1\\Rut\\camera1\\orioldrut.txt").arg(basePath);
		path2 = QString("%1\\Rut\\camera1\\orirut.txt").arg(basePath);
		file1.setFileName(path1);
		if (!file1.exists())
		{
			QFile::copy(path2, path1);
		}
		MyCommonMethods::writeAllLines(path2, newRRutsr);
	}

}

void CalculationThread::CreateGaussFilter(std::unique_ptr<float[]> &  gaus, int size, float sigma)
{
	double PI = 4.0 * atan(1.0); //圆周率π赋值
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
		gaus[i] = (float)(temp2 *exp(-tsigma * tsigma));
		sum += gaus[i];
	}

	for (int i = 0; i < size; i++)
	{
		gaus[i] = gaus[i] / sum;
	}
}

bool CalculationThread::JudgMTDval(QString prj, int side)
{

	QString resamplefname = QString("%1\\Laser0\\MTD_100.txt").arg(prj).arg(QString::number(side));
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
	float sumval = 0.0f;
	float oldval = rutval[0];
	float curval = rutval[0];
	std::uniform_int_distribution<>::param_type newPara(1, 100);
	dis.param(newPara);
	for (int i = 0; i < length; ++i)
	{
		curval = rutval[i];

		if (curval < 0.1)
		{
			if (i > 0)
			{
				auto seed = std::chrono::system_clock::now().time_since_epoch().count();
				std::mt19937 gen(seed);
				curval = sumval / i + dis(gen) * 0.01f - 0.5f;
			}
			else
			{
				curval = oldval;
			}
		}
		else if (curval - oldval >= _Setting->ErrorRutTh2)
		{
			if (oldval >= 0.1)
			{
				if (i > 0)
				{
					auto seed = std::chrono::system_clock::now().time_since_epoch().count();
					std::mt19937 gen(seed);
					curval = sumval / i + dis(gen) * 0.01f - 0.5f;
				}
				else
				{
					curval = oldval;
				}
			}
			else
			{
				if (curval > _Setting->ErrorRutTh2)
				{
					if (i > 0)
					{
						auto seed = std::chrono::system_clock::now().time_since_epoch().count();
						std::mt19937 gen(seed);

						curval = sumval / i + dis(gen) * 0.01f - 0.5f;
					}
					else
					{
						curval = 5.0f;
					}

				}
			}
		}

		rutval[i] = curval;
		oldval = curval;
		sumval = sumval + curval;
	}
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

		for (int i = 0 ; i <3 ; i++)
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


void CalculationThread::reportSideProgress(int side, double progress)
{
	if (m_type != CalculationThread::Smtd && m_type != CalculationThread::Smpd)
	{
		emit progressUpdated(m_Project, m_type, progress, false);
		return;
	}

	double aggregateProgress = progress;
	{
		QMutexLocker locker(&m_progressMutex);
		if (m_metricSideIndexes.isEmpty() || side < 0 || side >= m_metricSideProgress.size())
		{
			aggregateProgress = progress;
		}
		else
		{
			const double boundedProgress = qBound(0.0, progress, 1.0);
			m_metricSideProgress[side] = qMax(m_metricSideProgress[side], boundedProgress);

			double sumProgress = 0.0;
			for (int activeSide : qAsConst(m_metricSideIndexes))
			{
				if (activeSide >= 0 && activeSide < m_metricSideProgress.size())
				{
					sumProgress += m_metricSideProgress[activeSide];
				}
			}
			aggregateProgress = sumProgress / m_metricSideIndexes.size();
		}
	}

	emit progressUpdated(m_Project, m_type, aggregateProgress, false);
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
	if (file.exists() && file.size() > 1)
	{
		reportSideProgress(side, 1.0);
		return;
	}
	QVector<QString>lasfile;
	MyCommonMethods::GetAllIRIFiles(prj, side, "Laser", "las", lasfile);
	reportSideProgress(side, 0.1);
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

	QString tmpstr;
	QVector<QString> lasstrlist;
	ulong framecnt = 0;
	QFile fmtd(fname);
	int lasFileCount = lasfile.size();

	if (fmtd.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		if (isStopRequested())
		{

			return;
		}

		QStringList outList;
		SlidingUpperMedianWindow value1M; //始终记录前500个点  用来获取中间值

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
			QDataStream in(&fileLf);
			in.setByteOrder(QDataStream::LittleEndian);
			QVector<QString> temps;
			int ceshiInt = 0;
			//更新总迭代次数
			for (int i = 0; i < framenum; ++i, ++framecnt)
			{
				if (isStopRequested())
				{
					return;
				}
				fileLf.seek(fileLf.pos() + 16);
				in >> ttval;
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
						value1M.append(ttval);
					}
				}
				if (value1M.size() >= 500)
				{
					value1M.slide(ttval);
				}
				if (_Setting->IsOutputLasval)
				{
					tmpstr = QString::number(ttval);
				}
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
				if (!value1M.isEmpty())
				{
					oldttval = value1M.upperMedian();
				}
				if (_Setting->IsOutputLasval)
				{
					tmpstr = tmpstr + "\t" + QString::number(ttval);;
					lasstrlist.push_back(tmpstr);
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
							reportSideProgress(side, value);
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
		MyCommonMethods::writeAllLines(fname, outList);
		fmtd.close();
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
		reportSideProgress(side, 1.0);
		return;
	}
	QVector<QString>lasfile;
	MyCommonMethods::GetAllIRIFiles(prj, side, "Laser", "las", lasfile);
	reportSideProgress(side, 0.1);


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
		SlidingUpperMedianWindow value1M; //始终记录前25个点  用来获取中间值

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
				if (isStopRequested())
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
						value1M.append(ttval);
					}
				}
				if (value1M.size() >= 500)
				{
					value1M.slide(ttval);
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


				if (!value1M.isEmpty())
				{
					oldttval = value1M.upperMedian();
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
						reportSideProgress(side, value);
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
	QFile file(fname);
	if (!file.exists())
	{
		return;
	}

	QStringList oristrs = MyCommonMethods::ReadAllLines(fname);
	int len = oristrs.size();
	std::unique_ptr<QString[]> newstrs = std::make_unique<QString[]>(len);
	std::unique_ptr<double[]> orival = std::make_unique<double[]>(len);
	for (int i = 0; i < len; ++i)
	{
		orival[i] = oristrs[i].mid(oristrs[i].lastIndexOf(' ') + 1).toDouble();
	}
	if (len < 2) return;

	double lastval = orival[0];
	double sumval = 0;
	// orival[0] = MainForm.rdval.NextDouble() * 10;
	for (int i = 1; i < len; ++i)
	{

		//
		//if (orival[i]>12)
		//{
		//    orival[i] = MainForm.rdval.NextDouble() * 10;
		//}
		// 如果当前值大于异常阈值，则调整当前值为前面所有值的平均值

		std::uniform_int_distribution<>::param_type newPara(1, 100);
		dis.param(newPara);
		auto seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::mt19937 gen(seed);

		orival[i] = orival[i] > Thrval ? lastval + (dis(gen) - 50) * scale : orival[i];
		sumval += orival[i];
		lastval = sumval / i;
	}

	for (int i = 0; i < len; ++i)
	{
		newstrs[i] = QString("%1 %2").arg(oristrs[i].split(' ')[0]).arg(QString::number(orival[i]));
	}
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		return;
	}
	QTextStream sr(&file);
	for (int i = 0; i < len; ++i)
	{
		sr << newstrs[i] + "\t";
	}
	file.close();
}


void CalculationThread::run()
{
	struct StoppedEmitter
	{
		CalculationThread* thread;
		~StoppedEmitter()
		{
			QMetaObject::invokeMethod(thread, "stopped", Qt::QueuedConnection);
		}
	} stoppedEmitter{ this };
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
			if (!loadParm(true,temp ))
			{
				return;
			}
			if (isStopRequested())
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
		if (!isStopRequested())
		{
			emit progressUpdated(m_Project, m_type, 100, true);
		}

		break;

	}

	case CalculationThread::Rut:
	{
		QString iriPath = currentPorject->get2DProject()->getLeftRutPath();
		auto cur2dPro = currentPorject->get2DProject();
		bool dataproc1;
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
			break;
		}
		if (!isStopRequested())
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
		{
			QMutexLocker locker(&m_progressMutex);
			m_metricSideIndexes.clear();
			m_metricSideProgress = QVector<double>(3, 0.0);
			for (int side = 0; side < 3; ++side)
			{
				if (side == 2 && !currentPorject->get2DProject()->_IsMMTD)
				{
					continue;
				}
				m_metricSideIndexes.append(side);
			}
		}
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
			auto thread = std::make_unique<QThread>();
			QObject::connect(thread.get(), &QThread::started, [this, i, iriPath, thread = thread.get()]()
			{
				if (isStopRequested())
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

						}
						if (m_type == CalculationThread::Smpd)
						{
							ComputeMPD(iriPath, i, 10, lasthreshval);
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
		if (!isStopRequested())
		{
			emit progressUpdated(m_Project, m_type, 100, true);
		}
		break;
	}
	case CalculationThread::Jhxx:
	{
		if (isStopRequested())
		{
			return;
		}
		if (!isStopRequested())
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
	m_stopRequested.store(true, std::memory_order_relaxed);
}


CalculationThread::~CalculationThread()
{

}
