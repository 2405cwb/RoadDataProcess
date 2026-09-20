#pragma once

#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QVector>
#include <QtMath>

namespace hnPro
{
	struct hnDmiOffsetCorrection
	{
		double offset = 0.0;
		bool shouldCorrect = false;
		QString reason;

		double normalize(double rawDmi) const { return shouldCorrect ? rawDmi - offset : rawDmi; }

		static hnDmiOffsetCorrection analyzeMileStoneFile(const QString& filePath, double projectLength)
		{
			hnDmiOffsetCorrection result;
			if (projectLength <= 0.0) { result.reason = QStringLiteral("invalid project length"); return result; }
			QFile file(filePath);
			if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { result.reason = QStringLiteral("cannot read MileStoneCaliInfo.txt"); return result; }
			QVector<double> dmis;
			QTextStream in(&file);
			while (!in.atEnd())
			{
				const QStringList parts = in.readLine().simplified().split(' ', QString::SkipEmptyParts);
				bool ok = false;
				if (parts.size() >= 2) { const double dmi = parts[0].toDouble(&ok); if (ok) dmis.push_back(dmi); }
			}
			if (dmis.size() < 2) { result.reason = QStringLiteral("not enough calibration points"); return result; }
			const double candidate = dmis.first();
			const double tolerance = qMax(2.0, projectLength * 0.001);
			if (candidate <= tolerance) { result.reason = QStringLiteral("calibration already starts near zero"); return result; }
			if (qAbs((dmis.last() - candidate) - projectLength) > tolerance) { result.reason = QStringLiteral("calibration span does not match project length"); return result; }
			for (int i = 0; i < dmis.size(); ++i)
			{
				const double corrected = dmis[i] - candidate;
				if (corrected < -tolerance || corrected > projectLength + tolerance || (i > 0 && dmis[i] <= dmis[i - 1]))
				{ result.reason = QStringLiteral("calibration is not monotonic or would be out of range"); return result; }
			}
			result.offset = candidate;
			result.shouldCorrect = true;
			result.reason = QStringLiteral("fixed leading DMI offset detected");
			return result;
		}
	};
}
