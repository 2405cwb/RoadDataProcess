#include "HnXRSettings.h"
#include <QException>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
HnXRSettings* HnXRSettings::_XRSetting = nullptr;

void HnXRSettings::Init()
{
	if (m_iniFilePath.isEmpty())
	{
		qWarning("config file: %s not exists");
		return;
	}
	m_Setting = configService::getPtr();
	m_Setting->loadCfg(m_iniFilePath);
}

HnXRSettings::HnXRSettings()
	: m_autoPlayIntervalMs(DefaultAutoPlayIntervalMs)
{

}

HnXRSettings & HnXRSettings::operator=(const HnXRSettings & value)
{
	 
	return *_XRSetting;

}

void HnXRSettings::readData()
{
	PartType = m_Setting->ReadInteger("UI", "PartType", 0) == 1 ? 1 : 0;
	PartType_Dmi_Len = m_Setting->ReadInteger("UI", "PartType_Dmi_Len", 200);
	if (PartType_Dmi_Len <= 0)
	{
		PartType_Dmi_Len = 200;
	}
	roadCrossingShow = m_Setting->ReadBool("UI", "RoadCrossingShow", true);
	DefaultPath = m_Setting->ReadString("SETTING", "DefaultPath", "");
	lastProjectName = m_Setting->ReadString("SETTING", "lastProjectName", "");
	lastProjectFn = m_Setting->ReadInteger("SETTING", "lastProjectFn", 0);
	OutPath = m_Setting->ReadString("SETTING", "OutPath", "");
	gpsFormat = m_Setting->ReadBool("SETTING", "gpsFormat", true);
	
	sheetRoundingOffType = m_Setting->ReadInteger("EXCEL", "sheetRoundingOffType", 0);
	sheetRoundingOffNum = m_Setting->ReadInteger("SETTING", "sheetRoundingOffNum", 0);
	sheetRoundingOffNum_Dr = m_Setting->ReadInteger("SETTING", "sheetRoundingOffNum_Dr", 0);
	diseaseMarkTxt = m_Setting->ReadString("SETTING", "diseaseMarkTxt", "");
	diseaseMarkTxts = m_Setting->ReadString("SETTING", "diseaseMarkTxts", "");
	Las_Filter = m_Setting->ReadBool("IRM", "Las_Filter", true);
	Las_Filter_Thresh0 = m_Setting->ReadInteger("IRM", "Las_Filter_Thresh0", 50);
	Las_Filter_Thresh1 = m_Setting->ReadInteger("IRM", "Las_Filter_Thresh1", 50);
	RQIJudgeType = m_Setting->ReadInteger("IRM", "RQIJudgeType", 0);
	IRIExcelSide = m_Setting->ReadInteger("IRM", "IRIExcelSide", 2);
	rutKCorrect = m_Setting->ReadDouble("IRM", "RutKCorrect", 0);
	rutBCorrect = m_Setting->ReadDouble("IRM", "RutBCorrect", 0);
	IsThresholdRut = m_Setting->ReadBool("IRM", "IsThresholdRut", false);
	ErrorRut = m_Setting->ReadDouble("IRM", "ErrorRut", 0);
	ErrorRutTh1 = m_Setting->ReadDouble("IRM", "ErrorRutTh1", 0);
	ErrorRutTh2 = m_Setting->ReadDouble("IRM", "ErrorRutTh2", 0);
	IsOutputLasval = m_Setting->ReadBool("IRM", "IsOutputLasval", false);
	ErrorMTD = m_Setting->ReadBool("IRM", "ErrorMTD", false);
	MPD_K = m_Setting->ReadDouble("IRM", "MPD_K", 1);
	MPD_B = m_Setting->ReadDouble("IRM", "MPD_B", 0);
	IRI_threshval = m_Setting->ReadDouble("IRM", "IRI_threshval", 0);
	ErrorIRI = m_Setting->ReadDouble("IRM", "ErrorIRI", 0);
	MpdInterveneFAactor = m_Setting->ReadString("IRM", "MpdInterveneFAactor", "");
	iriKCorrect = m_Setting->ReadDouble("IRM", "IriKCorrect", 0);
	iriBCorrect = m_Setting->ReadDouble("IRM", "IriBCorrect", 0);
	ErrorVal = m_Setting->ReadInteger("IRM", "ErrorVal", 0);
	rutLeftCorrect = m_Setting->ReadDouble("IRM", "rutLeftCorrect", 0);
	rutRightCorrect = m_Setting->ReadDouble("IRM", "rutRightCorrect", 0);

	czDisOutSelectExcel = m_Setting->ReadInteger("EXCEL", "czDisOutSelectExcel", 0);
	roadDisDegreeExcel = m_Setting->ReadInteger("EXCEL", "roadDisDegreeExcel", 0);
	roadSnKcShowExcel = m_Setting->ReadInteger("EXCEL", "roadSnKcShowExcel", 0);
	//outDisPicNameExcel = m_Setting->ReadBool("EXCEL", "outDisPicNameExcel", false);
	outSpeedAndMarkExcel = m_Setting->ReadBool("EXCEL", "outSpeedAndMarkExcel", false);
	outExcelNeedSort = m_Setting->ReadBool("EXCEL", "outExcelNeedSort", false);
	outMarkInfoFile =  m_Setting->ReadBool("EXCEL", "outMarkInfoFile", false);
	BrokenPlatetype = m_Setting->ReadInteger("EXCEL", " BrokenPlatetype", 0);
	PlateLength = m_Setting->ReadDouble("EXCEL", "PlateLength", 0);
	PlateWidth = m_Setting->ReadDouble("EXCEL", "PlateWidth", 0);
	outMileWithMark = m_Setting->ReadBool("EXCEL", "outMarkInfo", true);

	outRoadUnitMark = m_Setting->ReadBool("EXCEL", "outRoadUnitMark", true);
	outExcelFormatDmi = m_Setting->ReadBool("EXCEL", "outExcelFormatDmi", false);
	diseaseExcelOutPicture = m_Setting->ReadBool("EXCEL", "diseaseExcelOutPicture", false);
	diseaseExcelOutLocation = m_Setting->ReadBool("EXCEL", "diseaseExcelOutLocation", false);
	rutOutMode = m_Setting->ReadInteger("EXCEL", "rutOutMode", 0);

	diseaseMark = m_Setting->ReadBool("SETTING", "diseaseMark", false);
	 
	diseaseRectShow = m_Setting->ReadBool("SETTING", "diseaseRectShow", false);
	showGpsInfo = m_Setting->ReadBool("SETTING", "showGpsInfo", false);
	RutDisWidth = m_Setting->ReadDouble("SETTING", "RutDisWidth", 0.00);
	mile2dmiToInt = m_Setting->ReadBool("SETTING", "mile2dmiToInt", false);
	wheelScrollOneImage = m_Setting->ReadBool("SETTING", "wheelScrollOneImage", false);
	// Keep playback preferences beside the user INI, outside the legacy writer.
	QSettings playback(QFileInfo(m_iniFilePath).dir().filePath(QStringLiteral("Playback.ini")), QSettings::IniFormat);
	bool valid = false;
	const int interval = playback.value(QStringLiteral("Playback/IntervalMs"),
		int(DefaultAutoPlayIntervalMs)).toInt(&valid);
	m_autoPlayIntervalMs = valid && interval >= MinAutoPlayIntervalMs && interval <= MaxAutoPlayIntervalMs
		? interval : DefaultAutoPlayIntervalMs;
}

int HnXRSettings::autoPlayIntervalMs() const
{
	return m_autoPlayIntervalMs;
}

bool HnXRSettings::saveAutoPlayIntervalMs(int intervalMs)
{
	if (m_iniFilePath.isEmpty() || intervalMs < MinAutoPlayIntervalMs || intervalMs > MaxAutoPlayIntervalMs)
	{
		return false;
	}
	QSettings playback(QFileInfo(m_iniFilePath).dir().filePath(QStringLiteral("Playback.ini")), QSettings::IniFormat);
	playback.setValue(QStringLiteral("Playback/IntervalMs"), intervalMs);
	playback.sync();
	if (playback.status() != QSettings::NoError)
	{
		return false;
	}
	m_autoPlayIntervalMs = intervalMs;
	return true;
}

bool HnXRSettings::saveCityReportSettings(int partType, int length, bool crossingEntrances)
{
	// 旧配置写入器只更新已有键；同一配置文件用 QSettings 补齐新增键。
	QSettings settings(m_iniFilePath, QSettings::IniFormat);
	settings.setIniCodec("UTF-8");
	settings.setValue(QStringLiteral("UI/PartType"), partType == 1 ? 1 : 0);
	settings.setValue(QStringLiteral("UI/PartType_Dmi_Len"), length > 0 ? length : 200);
	settings.setValue(QStringLiteral("UI/RoadCrossingShow"), crossingEntrances ? QStringLiteral("True") : QStringLiteral("False"));
	settings.sync();
	if (settings.status() != QSettings::NoError)
	{
		return false;
	}
	PartType = partType == 1 ? 1 : 0;
	PartType_Dmi_Len = length > 0 ? length : 200;
	roadCrossingShow = crossingEntrances;
	return true;
}

void HnXRSettings::writeData()
{
	saveCityReportSettings(PartType, PartType_Dmi_Len, roadCrossingShow);

	m_Setting->WriteInteger("SETTING", "sheetRoundingOffType", sheetRoundingOffType);
	m_Setting->WriteInteger("SETTING", "sheetRoundingOffNum", sheetRoundingOffNum);
	m_Setting->WriteInteger("SETTING", "sheetRoundingOffNum_Dr", sheetRoundingOffNum_Dr);

	m_Setting->WriteBool("SETTING", "gpsFormat", gpsFormat);
	m_Setting->WriteString("SETTING", "DefaultPath", DefaultPath);
	m_Setting->WriteString("SETTING", "lastProjectName", lastProjectName);
	m_Setting->WriteInteger("SETTING", "lastProjectFn", lastProjectFn);
	m_Setting->WriteString("SETTING", "OutPath", OutPath);
	m_Setting->WriteBool("SETTING", "diseaseMark", diseaseMark);
	m_Setting->WriteString("SETTING", "diseaseMarkTxt", diseaseMarkTxt);
	m_Setting->WriteBool("SETTING", "diseaseRectShow", diseaseRectShow);
	m_Setting->WriteBool("SETTING", "showGpsInfo", showGpsInfo);
	m_Setting->WriteBool("SETTING", "mile2dmiToInt", mile2dmiToInt);
	m_Setting->WriteBool("SETTING", "wheelScrollOneImage", wheelScrollOneImage);

	m_Setting->WriteBool("IRM", "Las_Filter", Las_Filter);
	m_Setting->WriteDouble("IRM", "Las_Filter_Thresh0", Las_Filter_Thresh0);
	m_Setting->WriteDouble("IRM", "Las_Filter_Thresh1", Las_Filter_Thresh1);
	m_Setting->WriteInteger("IRM", "ErrorVal", ErrorVal);
	m_Setting->WriteInteger("IRM", "RQIJudgeType", RQIJudgeType);
	m_Setting->WriteInteger("IRM", "IRIExcelSide", IRIExcelSide);
	m_Setting->WriteDouble("IRM", "RutKCorrect", rutKCorrect);
	m_Setting->WriteDouble("IRM", "RutBCorrect", rutBCorrect);
	m_Setting->WriteDouble("IRM", "ErrorRut", ErrorRut);
	m_Setting->WriteBool("IRM", "IsThresholdRut", IsThresholdRut);
	m_Setting->WriteDouble("IRM", "ErrorRutTh1", ErrorRutTh1);
	m_Setting->WriteDouble("IRM", "ErrorRutTh2", ErrorRutTh2);
	m_Setting->WriteBool("IRM", "IsOutputLasval", IsOutputLasval);
	m_Setting->WriteBool("IRM", "ErrorMTD", ErrorMTD);
	m_Setting->WriteDouble("IRM", "IRI_threshval", IRI_threshval);
	m_Setting->WriteString("IRM", "MpdInterveneFAactor", MpdInterveneFAactor);
	m_Setting->WriteDouble("IRM", "ErrorIRI", ErrorIRI);
	m_Setting->WriteDouble("IRM", "rutLeftCorrect", rutLeftCorrect);
	m_Setting->WriteDouble("IRM", "rutRightCorrect", rutRightCorrect);


	m_Setting->WriteInteger("EXCEL", "czDisOutSelectExcel", czDisOutSelectExcel);
	m_Setting->WriteInteger("EXCEL", "roadDisDegreeExcel", roadDisDegreeExcel);
	m_Setting->WriteInteger("EXCEL", "roadSnKcShowExcel", roadSnKcShowExcel);
	//m_Setting->WriteBool("EXCEL", "outDisPicNameExcel", outDisPicNameExcel);
	m_Setting->WriteBool("EXCEL", "outSpeedAndMarkExcel", outSpeedAndMarkExcel);
	m_Setting->WriteBool("EXCEL", "outMarkInfoFile", outMarkInfoFile);
	m_Setting->WriteBool("EXCEL", "outExcelNeedSort", outExcelNeedSort);
	m_Setting->WriteInteger("EXCEL", "BrokenPlatetype", BrokenPlatetype);
	m_Setting->WriteDouble("EXCEL", "PlateWidth", PlateWidth);
	m_Setting->WriteDouble("EXCEL", "PlateLength", PlateLength);

	m_Setting->WriteBool("EXCEL", "outMarkInfo", outMileWithMark);

	m_Setting->WriteBool("EXCEL", "outRoadUnitMark", outRoadUnitMark);

	m_Setting->WriteBool("EXCEL", "outExcelFormatDmi", outExcelFormatDmi);

	m_Setting->WriteBool("EXCEL", "diseaseExcelOutPicture", diseaseExcelOutPicture);
	m_Setting->WriteBool("EXCEL", "diseaseExcelOutLocation", diseaseExcelOutLocation);
	m_Setting->WriteInteger("EXCEL", "rutOutMode", rutOutMode);
}


HnXRSettings * HnXRSettings::getInstance()
{
	if (!_XRSetting)
	{
		_XRSetting = new HnXRSettings();
		return _XRSetting;
	}
	return _XRSetting;
}

HnXRSettings::~HnXRSettings()
{ 
	delete m_Setting;
	delete _XRSetting; 
}

void HnXRSettings::SetConfigFilePath(const QString& path)
{
	this->m_iniFilePath = path;
}
