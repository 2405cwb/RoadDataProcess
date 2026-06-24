#include "SynTrigInfo.h"
#include "QDateTime"

SynTrigInfo::SynTrigInfo(const SynTrigInfo& STrig, const SynTrigInfo& ETrig, double insertdmi)
{
	double x = (insertdmi - STrig._trigdmi) * 1.0 / (ETrig._trigdmi - STrig._trigdmi);

	QDateTime sdate = QDateTime::fromString(STrig._trigdate + STrig._trigtime, "yyyyMMddHHmmsszzz");
	QDateTime edate = QDateTime::fromString(ETrig._trigdate + ETrig._trigtime, "yyyyMMddHHmmsszzz");

	//计算时间差并插值
	qint64 pdateMilliseconds = static_cast<qint64>(sdate.msecsTo(edate) * x);
	QDateTime cdate = sdate.addMSecs(pdateMilliseconds);

	//更新当前实例时间和日期
	_trigdate = cdate.toString("yyyyMMdd");
	_trigtime = cdate.toString("HHmmsszzz");
	_trigdmi = insertdmi; 
	

}

SynTrigInfo::SynTrigInfo(const QString& frameIndex, const QString& dataDate, const QString& trigtime, const QString& trigdmi)
{
	_trigdate = dataDate;
	_trigtime = trigtime;
	_trigdmi = trigdmi.toDouble();
	QByteArray hexData = frameIndex.toUtf8();
	
	bool ok = false;
	_FrameIndex = hexData.toInt(&ok, 16);
}
