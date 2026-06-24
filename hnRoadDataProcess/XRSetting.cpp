#include "XRSetting.h"
#include "..\\hnCommon\\IniFiles.h"
#include <atlstr.h>
XRSetting::XRSetting()
{
}

XRSetting* XRSetting::_XRSetting = nullptr;

void XRSetting::readData()
{
#ifdef UNICODE
	hnCommon::IniFiles inisetting(L"D:\\TFS\\24-hnRoadDataProcess\\hnRoadDataProcess\\bin\\Debug-X64\\XRSetting.ini");
	
	ErrorVal = inisetting.ReadInteger(L"UI", L"ErrorVal", 0);
	ErrorIRI = inisetting.ReadDouble(L"UI", L"ErrorIRI", 0);
	ErrorMTD = inisetting.ReadDouble(L"UI", L"ErrorMTD", 0);
	ErrorRut = inisetting.ReadDouble(L"UI", L"ErrorRut", 0);
	ErrorRutTh1 = inisetting.ReadDouble(L"UI", L"ErrorRutTh1", 0);
	ErrorRutTh2 = inisetting.ReadDouble(L"UI", L"ErrorRutTh2", 0);
	IsThresholdRut = inisetting.ReadBool(L"UI", L"IsThresholdRut", false);
	ParmStyle = (StandardParmType)inisetting.ReadInteger(L"UI", L"ParmStyle", 0);
	ExcelType = inisetting.ReadInteger(L"UI", L"ExcelType", 0);
	IsExcelSort = inisetting.ReadBool(L"UI", L"IsExcelSort", false);
	IsStatistics = inisetting.ReadBool(L"UI", L"IsStatistics", false);
	IsRename = inisetting.ReadBool(L"UI", L"IsRename", false);
	YHType = inisetting.ReadInteger(L"UI", L"YHType", 0);
	DefaultPath = inisetting.ReadString(L"UI", L"DefaultPath",L"");
	ImgType = inisetting.ReadString(L"UI", L"ImgType", L"");

	LenExcelNum = inisetting.ReadInteger(L"UI", L"LenExcelNum", 0);
	for (int i=0;i<LenExcelNum;++i)
	{
		IsExcel.push_back(false);
		LenExcel.push_back("");
	}
	for (int i = 0; i < LenExcelNum; ++i)
	{
		CString str;
		str.Format(_TEXT("%d"), i);
		str.Insert(0, L"IsExcel");
		LPCTSTR iStr = LPCTSTR(str);
		CString str2;
		str2.Format(_TEXT("%d"), i);
		str2.Insert(0, L"LenExcel");
		LPCTSTR iStr2 = LPCTSTR(str2);

		IsExcel[i] = inisetting.ReadBool(L"UI",str, false);
		LenExcel[i] = inisetting.ReadString(L"UI", str2, L"");
	}

	DetectYear = inisetting.ReadString(L"UI", L"DetectYear", L"");
	DetectNum = inisetting.ReadString(L"UI", L"DetectNum", L"");
	DistrictCode = inisetting.ReadString(L"UI", L"DistrictCode", L"");
	DutyUnit = inisetting.ReadString(L"UI", L"DutyUnit", L"");
	RoadSideType = inisetting.ReadString(L"UI", L"RoadSideType", L"");
	CADLength = inisetting.ReadInteger(_T("UI"), _T("CADLength"), 0);
	IsRepair = inisetting.ReadBool(_T("UI"), _T("IsRepair"), false);
	OutRut = inisetting.ReadInteger(L"UI", L"OutRut", 0);
	Qufen_dis_degree = inisetting.ReadInteger(L"UI", L"Qufen_dis_degree", 0);
	Acc_IRI = inisetting.ReadInteger(L"UI", L"Acc_IRI", 0);
	Acc_IRI_K_1 = inisetting.ReadDouble(L"UI", L"Acc_IRI_K_1", 0);
	Acc_IRI_B_1 = inisetting.ReadDouble(L"UI", L"Acc_IRI_B_1", 0);
	IRIk = inisetting.ReadDouble(L"UI", L"IRIk", 0);
	IRIb = inisetting.ReadDouble(L"UI", L"IRIb", 0);

	Las_Filter = inisetting.ReadBool(L"UI", L"Las_Filter", false);
	Las_Filter_Thresh0 = inisetting.ReadDouble(L"UI", L"Las_Filter_Thresh0", 0);
	Las_Filter_Thresh1 = inisetting.ReadDouble(L"UI", L"Las_Filter_Thresh1", 0);
	Out_roadimg = inisetting.ReadInteger(L"UI", L"Out_roadimg", 0);
	Is_Multfolder = inisetting.ReadInteger(L"UI", L"Is_Multfolder", 0);
	IRI_threshval = inisetting.ReadDouble(L"UI", L"IRI_threshval", 0);
	cmop_rows = inisetting.ReadInteger(L"UI", L"cmop_rows", 0);
	SelectDrawDis = inisetting.ReadInteger(L"UI", L"SelectDrawDis", 0);
	ZJGT_dismodel = inisetting.ReadInteger(L"UI", L"ZJGT_dismodel", 0);
	BrokenPlatetype = inisetting.ReadInteger(L"UI", L"BrokenPlatetype", 0);
	PlateWidth = inisetting.ReadDouble(L"UI", L"PlateWidth", 0);
	PlateLength = inisetting.ReadDouble(L"UI", L"PlateLength", 0);
	RutDisWidth = inisetting.ReadDouble(L"UI", L"RutDisWidth", 0);
	Is_SnCarve = inisetting.ReadInteger(L"UI", L"Is_SnCarve", 0);
	IsShowAnalysis = inisetting.ReadBool(L"UI", L"IsShowAnalysis", false);

	MPD_K = inisetting.ReadDouble(L"UI", L"MPD_K", 0);
	MPD_B = inisetting.ReadDouble(L"UI", L"MPD_B", 0);

	IsWarning = inisetting.ReadBool(L"UI", L"IsWarning", false);
	PartType = inisetting.ReadInteger(L"UI", L"PartType", 0);
	PartType_Dmi_Len = inisetting.ReadInteger(L"UI", L"PartType_Dmi_Len", 0);
	Out_roadinfo = inisetting.ReadInteger(L"UI", L"Out_roadinfo", 0);
	sheetRoundingOffType = inisetting.ReadInteger(L"UI", L"sheetRoundingOffType", 0);
	sheetRoundingOffNum = inisetting.ReadInteger(L"UI", L"sheetRoundingOffNum", 0);

	StreetLenExcelNum = inisetting.ReadInteger(L"UI", L"StreetLenExcelNum", 0);
	for (int i = 0; i < StreetLenExcelNum; ++i)
	{
		StreetIsExcel.push_back(false);
		StreetLenExcel.push_back("");
	}
	//StreetIsExcel = new bool[StreetLenExcelNum];
	//StreetLenExcel = new string[StreetLenExcelNum];
	for (int i = 0; i < StreetLenExcelNum; ++i)
	{
		CString str;
		str.Format(_TEXT("%d"), i);
		str.Insert(0, L"StreetLenExcel");
		LPCTSTR iStr = LPCTSTR(str);
		CString str2;
		str2.Format(_TEXT("%d"), i);
		str2.Insert(0, L"StreetIsExcel");
		LPCTSTR iStr2 = LPCTSTR(str2);

		StreetLenExcel[i] = inisetting.ReadString(L"UI", iStr, L"");
		StreetIsExcel[i] = inisetting.ReadBool(L"UI",str2, false);
	}
	IsOutputLasval = inisetting.ReadBool(L"UI", L"IsOutputLasval", false);
	IsForbidOverLapping = inisetting.ReadBool(L"UI", L"IsForbidOverLapping", false);
	IsCrackRemark = inisetting.ReadBool(L"UI", L"IsCrackRemark", false);
	GPSJumpTime = inisetting.ReadInteger(L"UI", L"GPSJumpTime", 0);

	IRIExcelSide = inisetting.ReadInteger(L"UI", L"IRIExcelSide", 2);

	IsOutputDisAreaSubtotal = inisetting.ReadBool(L"UI", L"IsOutputDisAreaSubtotal", true);

	IsCheckIRIGPSTime = inisetting.ReadBool(L"UI",L"IsCheckIRIGPSTime", true);
	isImageCorrect = inisetting.ReadInteger(L"UI", L"isImageCorrect", 0);
	real_HM800 = inisetting.ReadString(L"UI", L"real_HM800", L"");
	real_MM800 = inisetting.ReadString(L"UI", L"real_MM800", L"");
	real_lm300 = inisetting.ReadString(L"UI", L"real_lm300", L"");
#else
	hnCommon::IniFiles inisetting(L"D:\\TFS\\24-hnRoadDataProcess\\hnRoadDataProcess\\bin\\Debug-X64\\XRSetting.ini");
		SkinName = inisetting.ReadString("UI", "SkinName", "");
	ICO = inisetting.ReadString("UI", "ICO", "");
	ICODX = inisetting.ReadString("UI", "ICODX", "");
	CompanyInfo = inisetting.ReadString("UI", "CompanyInfo", "");

	ErrorVal = inisetting.ReadInteger("UI", "ErrorVal", 0);
	ErrorIRI = inisetting.ReadDouble("UI", "ErrorIRI", 0);
	ErrorMTD = inisetting.ReadDouble("UI", "ErrorMTD", 0);
	ErrorRut = inisetting.ReadDouble("UI", "ErrorRut", 0);
	ErrorRutTh1 = inisetting.ReadDouble("UI", "ErrorRutTh1", 0);
	ErrorRutTh2 = inisetting.ReadDouble("UI", "ErrorRutTh2", 0);
	IsThresholdRut = inisetting.ReadBool("UI", "IsThresholdRut", false);
	ParmStyle = (StandardParmType)inisetting.ReadInteger("UI", "ParmStyle", 0);
	ExcelType = inisetting.ReadInteger("UI", "ExcelType", 0);
	IsExcelSort = inisetting.ReadBool("UI", "IsExcelSort", false);
	IsStatistics = inisetting.ReadBool("UI", "IsStatistics", false);
	IsRename = inisetting.ReadBool("UI", "IsRename", false);
	YHType = inisetting.ReadInteger("UI", "YHType", 0);
	DefaultPath = inisetting.ReadString("UI", "DefaultPath", "");
	ImgType = inisetting.ReadString("UI", "ImgType", "");

	LenExcelNum = inisetting.ReadInteger("UI", "LenExcelNum", 0);
	IsExcel = new bool[LenExcelNum];
	LenExcel = new string[LenExcelNum];
	for (int i = 0; i < LenExcelNum; ++i)
	{
		IsExcel[i] = inisetting.ReadBool("UI", "IsExcel" + i.ToString(), false);
		LenExcel[i] = inisetting.ReadString("UI", "LenExcel" + i.ToString(), "");
	}

	DetectYear = inisetting.ReadString("UI", "DetectYear", "");
	DetectNum = inisetting.ReadString("UI", "DetectNum", "");
	DistrictCode = inisetting.ReadString("UI", "DistrictCode", "");
	DutyUnit = inisetting.ReadString("UI", "DutyUnit", "");
	RoadSideType = inisetting.ReadString("UI", "RoadSideType", "");
	CADLength = inisetting.ReadInteger("UI", "CADLength", 0);
	IsRepair = inisetting.ReadBool("UI", "IsRepair", false);
	OutRut = inisetting.ReadInteger("UI", "OutRut", 0);
	Qufen_dis_degree = inisetting.ReadInteger("UI", "Qufen_dis_degree", 0);
	Acc_IRI = inisetting.ReadInteger("UI", "Acc_IRI", 0);
	Acc_IRI_K_1 = inisetting.ReadDouble("UI", "Acc_IRI_K_1", 0);
	Acc_IRI_B_1 = inisetting.ReadDouble("UI", "Acc_IRI_B_1", 0);
	IRIk = inisetting.ReadDouble("UI", "IRIk", 0);
	IRIb = inisetting.ReadDouble("UI", "IRIb", 0);

	Las_Filter = inisetting.ReadBool("UI", "Las_Filter", false);
	Las_Filter_Thresh0 = inisetting.ReadDouble("UI", "Las_Filter_Thresh0", 0);
	Las_Filter_Thresh1 = inisetting.ReadDouble("UI", "Las_Filter_Thresh1", 0);
	Out_roadimg = inisetting.ReadInteger("UI", "Out_roadimg", 0);
	Is_Multfolder = inisetting.ReadInteger("UI", "Is_Multfolder", 0);
	IRI_threshval = inisetting.ReadDouble("UI", "IRI_threshval", 0);
	cmop_rows = inisetting.ReadInteger("UI", "cmop_rows", 0);
	SelectDrawDis = inisetting.ReadInteger("UI", "SelectDrawDis", 0);
	ZJGT_dismodel = inisetting.ReadInteger("UI", "ZJGT_dismodel", 0);
	BrokenPlatetype = inisetting.ReadInteger("UI", "BrokenPlatetype", 0);
	PlateWidth = inisetting.ReadDouble("UI", "PlateWidth", 0);
	PlateLength = inisetting.ReadDouble("UI", "PlateLength", 0);
	RutDisWidth = inisetting.ReadDouble("UI", "RutDisWidth", 0);
	Is_SnCarve = inisetting.ReadInteger("UI", "Is_SnCarve", 0);
	IsShowAnalysis = inisetting.ReadBool("UI", "IsShowAnalysis", false);

	MPD_K = inisetting.ReadDouble("UI", "MPD_K", 0);
	MPD_B = inisetting.ReadDouble("UI", "MPD_B", 0);

	IsWarning = inisetting.ReadBool("UI", "IsWarning", false);
	PartType = inisetting.ReadInteger("UI", "PartType", 0);
	PartType_Dmi_Len = inisetting.ReadInteger("UI", "PartType_Dmi_Len", 0);
	Out_roadinfo = inisetting.ReadInteger("UI", "Out_roadinfo", 0);
	sheetRoundingOffType = inisetting.ReadInteger("UI", "sheetRoundingOffType", 0);
	sheetRoundingOffNum = inisetting.ReadInteger("UI", "sheetRoundingOffNum", 0);

	StreetLenExcelNum = inisetting.ReadInteger("UI", "StreetLenExcelNum", 0);
	StreetIsExcel = new bool[StreetLenExcelNum];
	StreetLenExcel = new string[StreetLenExcelNum];
	for (int i = 0; i < StreetLenExcelNum; ++i)
	{
		StreetLenExcel[i] = inisetting.ReadString("UI", "StreetLenExcel" + i.ToString(), "");
		StreetIsExcel[i] = inisetting.ReadBool("UI", "StreetIsExcel" + i.ToString(), false);
	}
	IsOutputLasval = inisetting.ReadBool("UI", "IsOutputLasval", false);
	IsForbidOverLapping = inisetting.ReadBool("UI", "IsForbidOverLapping", false);
	IsCrackRemark = inisetting.ReadBool("UI", "IsCrackRemark", false);
	GPSJumpTime = inisetting.ReadInteger("UI", "GPSJumpTime", 0);

	IRIExcelSide = inisetting.ReadInteger("UI", "IRIExcelSide", 2);

	IsOutputDisAreaSubtotal = inisetting.ReadBool("UI", "IsOutputDisAreaSubtotal", true);

	IsCheckIRIGPSTime = inisetting.ReadBool("UI", "IsCheckIRIGPSTime", true);
	isImageCorrect = inisetting.ReadInteger("UI", "isImageCorrect", 0);
	real_HM800 = inisetting.ReadString("UI", "real_HM800", "");
	real_MM800 = inisetting.ReadString("UI", "real_MM800", "");
	real_lm300 = inisetting.ReadString("UI", "real_lm300", "");
#endif // UNICODE
}

void XRSetting::writeData()
{
#ifdef UNICODE
	hnCommon::IniFiles inisetting(L"D:\\TFS\\24-hnRoadDataProcess\\hnRoadDataProcess\\bin\\Debug-X64\\XRSetting.ini");
	
	
	inisetting.WriteInteger(L"UI", L"ErrorVal", ErrorVal);
	inisetting.WriteDouble(L"UI", L"ErrorIRI", ErrorIRI);
	inisetting.WriteDouble(L"UI", L"ErrorMTD", ErrorMTD);
	inisetting.WriteDouble(L"UI", L"ErrorRut", ErrorRut);
	inisetting.WriteDouble(L"UI", L"ErrorRutTh1", ErrorRutTh1);
	inisetting.WriteDouble(L"UI", L"ErrorRutTh2", ErrorRutTh2);
	inisetting.WriteBool(L"UI", L"IsThresholdRut", IsThresholdRut);
	inisetting.WriteInteger(L"UI", L"ParmStyle", (int)ParmStyle);
	inisetting.WriteInteger(L"UI", L"ExcelType", ExcelType);
	inisetting.WriteBool(L"UI", L"IsExcelSort", IsExcelSort);
	inisetting.WriteBool(L"UI", L"IsStatistics", IsStatistics);
	inisetting.WriteBool(L"UI", L"IsRename", IsRename);
	inisetting.WriteInteger(L"UI", L"YHType", YHType); 
	auto temp = inisetting.string2LPCTSTR(DefaultPath);
	inisetting.WriteString(L"UI", L"DefaultPath",temp);
	temp = inisetting.string2LPCTSTR(ImgType);
	inisetting.WriteString(L"UI", L"ImgType",temp);

	inisetting.WriteInteger(L"UI", L"LenExcelNum", LenExcelNum);
	for (int i = 0; i < LenExcelNum; ++i)
	{
		CStringW str;
		str.Format(_TEXT("%d"), i);
		str.Insert(0, L"IsExcel");
		LPCTSTR iStr = LPCTSTR(str);
		inisetting.WriteBool(L"UI", iStr, IsExcel[i]);
		CString str2;
		str2.Format(_TEXT("%d"), i);
		str2.Insert(0, L"LenExcel");
		LPCTSTR iStr2 = LPCTSTR(str2);
		temp = inisetting.string2LPCTSTR(LenExcel[i]);
		inisetting.WriteString(L"UI", iStr2, temp);
	}

    temp =  inisetting.string2LPCTSTR(DetectYear);
	inisetting.WriteString(L"UI", L"DetectYear", temp);
	temp = inisetting.string2LPCTSTR(DetectNum);
	inisetting.WriteString(L"UI", L"DetectNum", temp);
	temp = inisetting.string2LPCTSTR(DistrictCode);
	inisetting.WriteString(L"UI", L"DistrictCode",temp);
	temp = inisetting.string2LPCTSTR(DutyUnit);
	inisetting.WriteString(L"UI", L"DutyUnit", temp);
	temp = inisetting.string2LPCTSTR(RoadSideType);
	inisetting.WriteString(L"UI", L"RoadSideType",temp );
	inisetting.WriteInteger(L"UI", L"CADLength", CADLength);
	inisetting.WriteBool(L"UI", L"IsRepair", IsRepair);
	inisetting.WriteInteger(L"UI", L"OutRut", OutRut);
	inisetting.WriteInteger(L"UI", L"Qufen_dis_degree", Qufen_dis_degree);
	inisetting.WriteInteger(L"UI", L"Acc_IRI", Acc_IRI);
	inisetting.WriteDouble(L"UI", L"Acc_IRI_K_1", Acc_IRI_K_1);
	inisetting.WriteDouble(L"UI", L"Acc_IRI_B_1", Acc_IRI_B_1);
	inisetting.WriteDouble(L"UI", L"IRIk", IRIk);
	inisetting.WriteDouble(L"UI", L"IRIb", IRIb);

	inisetting.WriteBool(L"UI", L"Las_Filter", Las_Filter);
	inisetting.WriteDouble(L"UI", L"Las_Filter_Thresh0", Las_Filter_Thresh0);
	inisetting.WriteDouble(L"UI", L"Las_Filter_Thresh1", Las_Filter_Thresh1);
	inisetting.WriteInteger(L"UI", L"Out_roadimg", Out_roadimg);
	inisetting.WriteInteger(L"UI", L"Is_Multfolder", Is_Multfolder);
	inisetting.WriteDouble(L"UI", L"IRI_threshval", IRI_threshval);
	inisetting.WriteInteger(L"UI", L"cmop_rows", cmop_rows);
	inisetting.WriteInteger(L"UI", L"SelectDrawDis", SelectDrawDis);
	inisetting.WriteInteger(L"UI", L"ZJGT_dismodel", ZJGT_dismodel);
	inisetting.WriteInteger(L"UI", L"BrokenPlatetype", BrokenPlatetype);
	inisetting.WriteDouble(L"UI", L"PlateWidth", PlateWidth);
	inisetting.WriteDouble(L"UI", L"PlateLength", PlateLength);
	inisetting.WriteDouble(L"UI", L"RutDisWidth", RutDisWidth);
	inisetting.WriteInteger(L"UI", L"Is_SnCarve", Is_SnCarve);
	inisetting.WriteBool(L"UI", L"IsShowAnalysis", IsShowAnalysis);

	inisetting.WriteDouble(L"UI", L"MPD_K", MPD_K);
	inisetting.WriteDouble(L"UI", L"MPD_B", MPD_B);

	inisetting.WriteBool(L"UI", L"IsWarning", IsWarning);
	inisetting.WriteInteger(L"UI", L"PartType", PartType);
	inisetting.WriteInteger(L"UI", L"PartType_Dmi_Len", PartType_Dmi_Len);
	inisetting.WriteInteger(L"UI", L"Out_roadinfo", Out_roadinfo);
	inisetting.WriteInteger(L"UI", L"sheetRoundingOffType", sheetRoundingOffType);
	inisetting.WriteInteger(L"UI", L"sheetRoundingOffNum", sheetRoundingOffNum);
	inisetting.WriteInteger(L"UI", L"StreetLenExcelNum", StreetLenExcelNum);
	for (int i = 0; i < StreetLenExcelNum; ++i)
	{
		CString str;
		str.Format(_TEXT("%d"), i);
		str.Insert(0, L"StreetLenExcel");
		LPCTSTR iStr = LPCTSTR(str);
		temp = inisetting.string2LPCTSTR(StreetLenExcel[i]);
		inisetting.WriteString(L"UI", iStr,temp);
		CString str2;
		str2.Format(_TEXT("%d"), i);
		str2.Insert(0, L"StreetIsExcel");
		LPCTSTR iStr2 = LPCTSTR(str2);

		inisetting.WriteBool(L"UI", iStr2, StreetIsExcel[i]);
	}
	inisetting.WriteBool(L"UI", L"IsOutputLasval", IsOutputLasval);
	inisetting.WriteBool(L"UI", L"IsForbidOverLapping", IsForbidOverLapping);
	inisetting.WriteBool(L"UI", L"IsCrackRemark", IsCrackRemark);
	inisetting.WriteInteger(L"UI", L"GPSJumpTime", GPSJumpTime);

	inisetting.WriteInteger(L"UI", L"IRIExcelSide", IRIExcelSide);

	inisetting.WriteBool(L"UI", L"IsOutputDisAreaSubtotal", IsOutputDisAreaSubtotal);

	inisetting.WriteBool(L"UI", L"IsCheckIRIGPSTime", IsCheckIRIGPSTime);
	inisetting.WriteInteger(L"UI", L"isImageCorrect", isImageCorrect);
	temp = inisetting.string2LPCTSTR(real_HM800);
	inisetting.WriteString(L"UI", L"real_HM800",temp );
	temp = inisetting.string2LPCTSTR(real_MM800);
	inisetting.WriteString(L"UI", L"real_MM800", temp);
	temp = inisetting.string2LPCTSTR(real_lm300);
	inisetting.WriteString(L"UI", L"real_lm300", temp);
	delete temp;
	temp = NULL;
#else
	hnCommon::IniFiles inisetting(L"D:\\TFS\\24-hnRoadDataProcess\\hnRoadDataProcess\\bin\\Debug-X64\\XRSetting.ini");
	inisetting.WriteString(L"UI", L"SkinName", SkinName);
	inisetting.WriteString(L"UI", L"ICO", ICO);
	inisetting.WriteString(L"UI", L"ICODX", ICODX);
	inisetting.WriteString(L"UI", L"CompanyInfo", CompanyInfo);

	inisetting.WriteInteger(L"UI", L"ErrorVal", ErrorVal);
	inisetting.WriteDouble(L"UI", L"ErrorIRI", ErrorIRI);
	inisetting.WriteDouble(L"UI", L"ErrorMTD", ErrorMTD);
	inisetting.WriteDouble(L"UI", L"ErrorRut", ErrorRut);
	inisetting.WriteDouble(L"UI", L"ErrorRutTh1", ErrorRutTh1);
	inisetting.WriteDouble(L"UI", L"ErrorRutTh2", ErrorRutTh2);
	inisetting.WriteBool(L"UI", L"IsThresholdRut", IsThresholdRut);
	inisetting.WriteInteger(L"UI", L"ParmStyle", (int)ParmStyle);
	inisetting.WriteInteger(L"UI", L"ExcelType", ExcelType);
	inisetting.WriteBool(L"UI", L"IsExcelSort", IsExcelSort);
	inisetting.WriteBool(L"UI", L"IsStatistics", IsStatistics);
	inisetting.WriteBool(L"UI", L"IsRename", IsRename);
	inisetting.WriteInteger(L"UI", L"YHType", YHType);
	inisetting.WriteString(L"UI", L"DefaultPath", DefaultPath);
	inisetting.WriteString(L"UI", L"ImgType", ImgType);

	inisetting.WriteInteger(L"UI", L"LenExcelNum", LenExcelNum);
	for (int i = 0; i < LenExcelNum; ++i)
	{
		CString str;
		str.Format(_TEXT("%s"), i);
		str.Insert(0, L"IsExcel");
		LPCTSTR iStr = LPCTSTR(str);
		inisetting.WriteBool(L"UI", iStr, IsExcel[i]);
		CString str2;
		str2.Format(_TEXT("%s"), i);
		str2.Insert(0, L"LenExcel");
		LPCTSTR iStr2 = LPCTSTR(str2);

		inisetting.WriteString(L"UI", iStr2, LenExcel[i]);
	}

	inisetting.WriteString(L"UI", L"DetectYear", DetectYear);
	inisetting.WriteString(L"UI", L"DetectNum", DetectNum);
	inisetting.WriteString(L"UI", L"DistrictCode", DistrictCode);
	inisetting.WriteString(L"UI", L"DutyUnit", DutyUnit);
	inisetting.WriteString(L"UI", L"RoadSideType", RoadSideType);
	inisetting.WriteInteger(L"UI", L"CADLength", CADLength);
	inisetting.WriteBool(L"UI", L"IsRepair", IsRepair);
	inisetting.WriteInteger(L"UI", L"OutRut", OutRut);
	inisetting.WriteInteger(L"UI", L"Qufen_dis_degree", Qufen_dis_degree);
	inisetting.WriteInteger(L"UI", L"Acc_IRI", Acc_IRI);
	inisetting.WriteDouble(L"UI", L"Acc_IRI_K_1", Acc_IRI_K_1);
	inisetting.WriteDouble(L"UI", L"Acc_IRI_B_1", Acc_IRI_B_1);
	inisetting.WriteDouble(L"UI", L"IRIk", IRIk);
	inisetting.WriteDouble(L"UI", L"IRIb", IRIb);

	inisetting.WriteBool(L"UI", L"Las_Filter", Las_Filter);
	inisetting.WriteDouble(L"UI", L"Las_Filter_Thresh0", Las_Filter_Thresh0);
	inisetting.WriteDouble(L"UI", L"Las_Filter_Thresh1", Las_Filter_Thresh1);
	inisetting.WriteInteger(L"UI", L"Out_roadimg", Out_roadimg);
	inisetting.WriteInteger(L"UI", L"Is_Multfolder", Is_Multfolder);
	inisetting.WriteDouble(L"UI", L"IRI_threshval", IRI_threshval);
	inisetting.WriteInteger(L"UI", L"cmop_rows", cmop_rows);
	inisetting.WriteInteger(L"UI", L"SelectDrawDis", SelectDrawDis);
	inisetting.WriteInteger(L"UI", L"ZJGT_dismodel", ZJGT_dismodel);
	inisetting.WriteInteger(L"UI", L"BrokenPlatetype", BrokenPlatetype);
	inisetting.WriteDouble(L"UI", L"PlateWidth", PlateWidth);
	inisetting.WriteDouble(L"UI", L"PlateLength", PlateLength);
	inisetting.WriteDouble(L"UI", L"RutDisWidth", RutDisWidth);
	inisetting.WriteInteger(L"UI", L"Is_SnCarve", Is_SnCarve);
	inisetting.WriteBool(L"UI", L"IsShowAnalysis", IsShowAnalysis);

	inisetting.WriteDouble(L"UI", L"MPD_K", MPD_K);
	inisetting.WriteDouble(L"UI", L"MPD_B", MPD_B);

	inisetting.WriteBool(L"UI", L"IsWarning", IsWarning);
	inisetting.WriteInteger(L"UI", L"PartType", PartType);
	inisetting.WriteInteger(L"UI", L"PartType_Dmi_Len", PartType_Dmi_Len);
	inisetting.WriteInteger(L"UI", L"Out_roadinfo", Out_roadinfo);
	inisetting.WriteInteger(L"UI", L"sheetRoundingOffType", sheetRoundingOffType);
	inisetting.WriteInteger(L"UI", L"sheetRoundingOffNum", sheetRoundingOffNum);
	inisetting.WriteInteger(L"UI", L"StreetLenExcelNum", StreetLenExcelNum);
	for (int i = 0; i < StreetLenExcelNum; ++i)
	{
		CString str;
		str.Format(_TEXT("%s"), i);
		str.Insert(0, L"StreetLenExcel");
		LPCTSTR iStr = LPCTSTR(str);
		inisetting.WriteString(L"UI", iStr, StreetLenExcel[i]);
		CString str2;
		str2.Format(_TEXT("%s"), i);
		str2.Insert(0, L"StreetIsExcel");
		LPCTSTR iStr2 = LPCTSTR(str2);

		inisetting.WriteBool(L"UI", iStr2, StreetIsExcel[i]);
	}
	inisetting.WriteBool(L"UI", L"IsOutputLasval", IsOutputLasval);
	inisetting.WriteBool(L"UI", L"IsForbidOverLapping", IsForbidOverLapping);
	inisetting.WriteBool(L"UI", L"IsCrackRemark", IsCrackRemark);
	inisetting.WriteInteger(L"UI", L"GPSJumpTime", GPSJumpTime);

	inisetting.WriteInteger(L"UI", L"IRIExcelSide", IRIExcelSide);

	inisetting.WriteBool(L"UI", L"IsOutputDisAreaSubtotal", IsOutputDisAreaSubtotal);

	inisetting.WriteBool(L"UI", L"IsCheckIRIGPSTime", IsCheckIRIGPSTime);
	inisetting.WriteInteger(L"UI", L"isImageCorrect", isImageCorrect);
	inisetting.WriteString(L"UI", L"real_HM800", real_HM800);
	inisetting.WriteString(L"UI", L"real_MM800", real_MM800);
	inisetting.WriteString(L"UI", L"real_lm300", real_lm300);
#endif
}

XRSetting* XRSetting::getInstance()
{
	if (!_XRSetting)
	{
		_XRSetting = new XRSetting();
		return _XRSetting;
	}
}

XRSetting::~XRSetting()
{

	_XRSetting = nullptr;
	delete _XRSetting;
}
