#pragma  once
#include "hnOutExcelMile.h"
#include "../hnApplication/hnDataManager.h"
#include "../hnCommon/hnTypeDefs.h"
#include "../hnQtCommon/MyCommonMethods.h"
hnOutExcelMile::hnOutExcelMile(hnPro::hnProject * project, HnProjectEnums::StandardParmTypeEnum roadStandard):m_project(project),Type(roadStandard) 
{
	
	PCIExcelStr ="";
	DRScore = 0;
	SpeedVal = 0;
	LeftIriValue = 0;
	RightIriValue = 0;
	IriExcelStr = "";
	IriEvaluateStr = "";
	judgeIirValue = 0;   
	LeftPbValue=0;
	RightPbValue=0;
	judgePbStr="";
	PbEvaluateStr=""; 
	m_sRutVal = 0;
	LeftRutValue=0;
	RightRutValue=0;
	judgeRutValue=0;
	QString RutExcelStr="";
	QString RutEvaluateStr="";
	hasBPTT = false;
   CenterMtdValue = 0;
	LeftMtdValue=0;
	RightMtdValue=0;
	PwiValueStr="";
	 PwiEvaluateStr="";
	 LeftMpdValue=0;
	 RightMpdValue=0;
	 CenterMpdValue=0;
	 MpdValueStr="";
	 MpdEvaluateStr="";
	 Direction = -1;
	 RoadGrad = -1;
	 RoadDegreestr = "";
	 StartMile = 0;
	 StartDmi = 0;
	 EndMile = 0;
	 EndDmi = 0;
	 Curvature = -1;
	 LongitudianalSlope = -1;
	 CrossSlope = -1;
	m_xrSetting = HnXRSettings::getInstance(); 
}

hnOutExcelMile::hnOutExcelMile() :m_project(nullptr) 
{
	StartMile = 0;
	StartDmi = 0;
	EndMile = 0;
	EndDmi = 0;
	IriExcelStr = "";
	IriEvaluateStr = "";
	judgeIirValue = 0;
	LeftPbValue = 0;
	RightPbValue = 0;
	judgePbStr = "";
	PbEvaluateStr = "";
	m_sRutVal = 0;
	PCIExcelStr="";
	LeftMtdValue = 0;
	RightMtdValue = 0; 
	PwiValueStr = "";
	PwiEvaluateStr = "";
	LeftMpdValue = 0;
	RightMpdValue = 0;
	CenterMpdValue = 0;
	MpdValueStr = "";
	MpdEvaluateStr = "";
	RoadGrad = -1;
	RoadDegreestr = "";
	DRScore = 0;
	SpeedVal = 0;
	PBIScoreStr = ""; 
	CenterMtdValue = 0;
	Curvature = -1; 
	LongitudianalSlope = -1;
	CrossSlope = -1;
	Direction = -1;
	m_xrSetting = HnXRSettings::getInstance(); 
}

hnOutExcelMile::~hnOutExcelMile()
{
	 
}

 
void hnOutExcelMile::setStartMile(double value)
{
	this->StartMile = value;
	this->setRoadLength(qAbs(this->StartMile - this->EndMile));
}

double hnOutExcelMile::getStartMile() const
{
	return this->StartMile;
}

void hnOutExcelMile::setEndMile(double mile)
{
	this->EndMile = mile;
	this->setRoadLength(qAbs(this->StartMile - this->EndMile));

}

double hnOutExcelMile::getEndMile() const
{
	return this->EndMile;
}

void hnOutExcelMile::setStartDmi(double value)
{
	this->StartDmi = value;
}

double hnOutExcelMile::getStartDmi() const
{
	return StartDmi;
}

void hnOutExcelMile::setEndDmi(double dmi)
{
	this->EndDmi = dmi;
}

double hnOutExcelMile::getEndDmi() const
{
	return EndDmi;
}

double hnOutExcelMile::getRoadLength() const
{
	return this->RoadLength;
}

void hnOutExcelMile::setUnitStr(QString str)
{
	this->UnitStr = str;
}

QString hnOutExcelMile::getUnitStr() const
{
	return this->UnitStr;
}

double hnOutExcelMile::getSpeed()
{
	return this->SpeedVal;
}

void hnOutExcelMile::setSpeed(double value)
{
	this->SpeedVal =MyCommonMethods::rountToNDecimalPlaces( value, m_xrSetting->sheetRoundingOffNum);
}

void hnOutExcelMile::setStartGpsInfo( _EXCELGPS_ value)
{
	this->StartGpsInfo = value;
}

void hnOutExcelMile::setEndGpsInfo( _EXCELGPS_ value)
{
	this->EndGpsInfo= value;
}

void hnOutExcelMile::setLeftIriValue(double value)
{
	LeftIriValue = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum);
}

double hnOutExcelMile::getLeftIriValue()
{
	return LeftIriValue;
}

void hnOutExcelMile::setRightIriValue(double value)
{
	RightIriValue = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum);
}

double hnOutExcelMile::getRightIriValue()
{
	return RightIriValue;
}

double hnOutExcelMile::getJudgeIirValue()
{
	return judgeIirValue;
}

double hnOutExcelMile::getMaxIriValue()
{
	return qMax(LeftIriValue, RightIriValue);
}

QString hnOutExcelMile::getIriExcelStr()
{
	 return  IriExcelStr;
}

 double hnOutExcelMile::getDRScore()
 { 
	 return   DRScore;
 }

 void hnOutExcelMile::setDrScore(double value)
 { 
	 DRScore = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum_Dr);
 }

 QString hnOutExcelMile::getPCIExcelStr()
 {
	 return PCIExcelStr;
 }

  QString hnOutExcelMile::getPciEvaluateStr(QString colStr,int row)
 { 
		 QStringList pciLevels = QString::fromLocal8Bit(m_roadTypeSetInfo.strPCILevel).split(' ');

		 switch (Type)
		 {
		 case HnProjectEnums::None:
			 break;
		 case HnProjectEnums::DegreeRoad2018:
			 if (pciLevels.size() > 3)
			 {
				 PciEvaluateStr = QStringLiteral("=IF(%1%2>=%3,\"优\",IF(%1%2>=%4,\"良\",IF(%1%2>=%5,\"中\",IF(%1%2>=%6,\"次\",\"差\"))))")
					 .arg(colStr).arg(QString::number(row)).arg(pciLevels.at(0)).arg(pciLevels.at(1)).arg(pciLevels.at(2)).arg(pciLevels.at(3));
			 }

			 break;
		 case HnProjectEnums::CityRoad:
			 /*
			 string.Format("=IF(D{0}>={1},\"A\",IF(D{0}>={2},\"B\",IF(D{0}>={3},\"C\",\"D\")))",
			 i + 3, _PCIGrade[roadpart[i].roaddegree][0], _PCIGrade[roadpart[i].roaddegree][1], _PCIGrade[roadpart[i].roaddegree][2]);
			 */
			 if (pciLevels.size() > 2)
			 {
				 PciEvaluateStr = QStringLiteral("=IF(%1%2>=%3,\"A\",IF(%1%2>=%4,\"B\",IF(%1%2>=%5,\"C\",\"D\")))")
					 .arg(colStr).arg(QString::number(row)).arg(pciLevels.at(0)).arg(pciLevels.at(1)).arg(pciLevels.at(2));
			 }
		
			 break;
		 case HnProjectEnums::RuralRoadlowLevel:
			 if (pciLevels.size() > 3)
			 {
				 PciEvaluateStr = QStringLiteral("=IF(%1%2>=%3,\"优\",IF(%1%2>=%4,\"良\",IF(%1%2>=%5,\"中\",IF(%1%2>=%6,\"次\",\"差\"))))")
					 .arg(colStr).arg(QString::number(row)).arg(pciLevels.at(0)).arg(pciLevels.at(1)).arg(pciLevels.at(2)).arg(pciLevels.at(3));
			 }
			 break;
		 default:
			 break;
		 } 
	  return PciEvaluateStr;
 }

  QString hnOutExcelMile::getMtdEvaluateStr(QString colStr, int row)
  {
	  /*
	  "=IF(F{0}>={1},\"A\",IF(F{0}>={2},\"B\",IF(F{0}>={3},\"C\",\"D\")))"
	  */ 
	  QStringList levels;
	 
	  switch (Type)
	  {
	  case HnProjectEnums::None:
		  break;
	  case HnProjectEnums::DegreeRoad2018: 

		  levels = QString::fromLocal8Bit(m_roadTypeSetInfo.strPWILevel).split(' ');
		  if (levels.size()>3)
		  {
			  MtdEvaluateStr = QStringLiteral("=IF(%1%2>=%3,\"优\",IF(%1%2>=%4,\"良\",IF(%1%2>=%5,\"中\",IF(%1%2>=%6,\"次\",\"差\"))))")
				  .arg(colStr).arg(QString::number(row)).arg(levels.at(0)).arg(levels.at(1)).arg(levels.at(2)).arg(levels.at(3));
		  }
		
		   
		  break;
	  case HnProjectEnums::CityRoad:
		 
		 levels = QString::fromLocal8Bit(m_roadTypeSetInfo.strMTDLevel).split(' ');
		 if (levels.size()>2)
		 {
			 MtdEvaluateStr = QStringLiteral("=IF(%1%2>=%3,\"A\",IF(%1%2>=%4,\"B\",IF(%1%2>=%5,\"C\",\"D\")))").arg(colStr).arg(QString::number(row)).arg(levels.at(0)).arg(levels.at(1)).arg(levels.at(2));

		 }
		 
		  break;
	  case HnProjectEnums::RuralRoadlowLevel:
		  break;
	  default:
		  break;
	  } 
	  return MtdEvaluateStr;
  }

  void hnOutExcelMile::setRutDisVlaue(double value)
  {
	  m_sRutVal = value;
  }

  void hnOutExcelMile::setRightRutValue(double value)
  {
	  RightRutValue = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum);
  }

  void hnOutExcelMile::setMaxRutValue(double value)
  {
	  RutMaxValue = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum);
  }

  double hnOutExcelMile::getRightRutValue()
  {
	  return RightRutValue;
  }

  void hnOutExcelMile::setLeftRutValue(double value)
  {
	  LeftRutValue = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum);
  }

  _EXCELGPS_ hnOutExcelMile::getStartGpsStr()
  {
	 
	  return this->StartGpsInfo;
  }

  _EXCELGPS_ hnOutExcelMile::getEndGpsStr()
 {
	 return this->EndGpsInfo;
 }

 double hnOutExcelMile::getLeftRutValue()
  {
	  return LeftRutValue;
  }

  void hnOutExcelMile::setjudgeRutValue(double value)
  {
	  judgeRutValue = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum);
  }

  double hnOutExcelMile::getjudgeRutValue()
  {
	  return judgeRutValue;
  }

  double hnOutExcelMile::getMaxRutValue()
  {
	  return qMax(getLeftRutValue(), getRightRutValue());
  }

  void hnOutExcelMile::setLeftPbValue(double value)
  {
	  LeftPbValue = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum);
  }

  
  void hnOutExcelMile::setRightPbValue(double value)
  {
	  RightPbValue = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum);
  }

   double hnOutExcelMile::getLeftPbValue(int length)const
  {
	  if (length ==10)
	  {
		  return LeftPbValue;

	  }
	  else
	  {
		  return -1;
	  }
	 
  }

  double hnOutExcelMile::getRightPbValue(int length) const
  {
	  if (length == 10)
	  {
		  return RightPbValue;
	  }
	  else
	  {
		  return -1;
	  }
	
  }

  QString hnOutExcelMile::getjudgePbValue(int length )
  {
	  if (length == 10)
	  {
		  return judgePbStr;

	  }
	  else
	  {
		  return -1;
	  }
  }

 QString hnOutExcelMile::getPbEvaluateStr()
 {
	 QString pbEvalua;
	 QStringList gradStrs = QString::fromLocal8Bit(m_roadTypeSetInfo.strPBI_KFBZ).split(" ");
	 if (gradStrs.size()>2)
	 {
		 pbEvalua = QString("=IF(%1<%2,\"%5\",IF(%1<%3,\"%6\",IF(%1<%4,\"%7\",\"%8\")))").arg(QString::number(this->getjudgePbiValue()))
			 .arg(gradStrs[0]).arg(gradStrs[1]).arg(gradStrs[2]).arg(QStringLiteral("无跳车")).arg(QStringLiteral("轻度跳车")).arg(QStringLiteral("中度跳车")).arg(QStringLiteral("重度跳车"));
	
	 }
	 return pbEvalua;
  }

  void hnOutExcelMile::setLeftMtdValue(double value)
  {
	  LeftMtdValue = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum);
  }

  void hnOutExcelMile::setRightMtdValue(double value)
  {
	  RightMtdValue = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum);
  }





  void hnOutExcelMile::setCenterMtdValue(double value)
  {
	  this->CenterMtdValue = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum);
  }

  double hnOutExcelMile::getLeftMtdValue()
  {
	  return LeftMtdValue;
  }

 double hnOutExcelMile::getRightMtdValue()
  {
	 if (m_project->get2DProject()->_IsDIRIMTD)
	 {
		 return RightMtdValue;

	  }
	 else
	 {
		 return 0;
	 }
  }

 double hnOutExcelMile::getCenterMtdValue()
  {   
	 return CenterMtdValue;
	  
  }

  double hnOutExcelMile::getPwiValue()
  {
	  double wr = getMpdWrValue();
	  double pwiValue =  100 - m_roadTypeSetInfo.dPWI_a0*pow(wr, m_roadTypeSetInfo.dPWI_a1);
	  pwiValue=	  MyCommonMethods::rountToNDecimalPlaces(pwiValue, m_xrSetting->sheetRoundingOffNum);
	  return pwiValue;
  }

  void hnOutExcelMile::setLeftMpdValue(double value)
  {
	  LeftMpdValue = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum);
  }

  void hnOutExcelMile::setRightMpdValue(double value)
  {
	  RightMpdValue = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum);
  }

  void hnOutExcelMile::setCenterMpdValue(double value)
  {
	  CenterMpdValue = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum);
  }

  

  double hnOutExcelMile::getLeftMpdValue()
  {
	  return LeftMpdValue;
  }

  double hnOutExcelMile::getRightMpdValue()
  {
	  return RightMpdValue;
  }

  double hnOutExcelMile::getCenterMpdValue()
  {
	  return CenterMpdValue;
  }


  double hnOutExcelMile::getjudgePbiValue()
  {
	  return  judgePbiValue;
  }

  QString hnOutExcelMile::getRutExcelStr()
  {
	  if (RutExcelStr.isEmpty())
	  {
		  return "100";
	  }
	  return RutExcelStr;
  }

  QString hnOutExcelMile::getRutMaxExcelStr()
  {
	  return RutMaxExcelStr;
  }

  QString hnOutExcelMile::getRutEvaluateStr(QString colStr, int row)
  {//rut
	   
		  QStringList levels = QString::fromLocal8Bit(m_roadTypeSetInfo.strRDILevel).split(' ');
		  if (levels.size() > 4)
		  { 
			  RutEvaluateStr = QStringLiteral("=IF(%1%2>=%3,\"优\",IF(%1%2>=%4,\"良\",IF(%1%2>=%5,\"中\",IF(%1%2>=%6,\"次\",\"差\"))))")
				  .arg(colStr).arg(QString::number(row)).arg(levels.at(0)).arg(levels.at(1)).arg(levels.at(2)).arg(levels.at(3));
		  }
 
	  return RutEvaluateStr;
   }

   QString hnOutExcelMile::getIriEvaluateStr(QString colStr, int row)
 {
	 QStringList levels = QString::fromLocal8Bit(m_roadTypeSetInfo.strRQILevel).split(' ');
	 switch (m_project->getBaseStandard())
	 {
	 case  HnProjectEnums::CityRoad:
		 if (levels.size() > 3)
		 {
			 IriEvaluateStr = QStringLiteral("=IF(%1%2>=%3,\"A\",IF(%1%2>=%4,\"B\",IF(%1%2>=%5,\"C\",\"D\")))")
				 .arg(colStr).arg(QString::number(row)).arg(levels.at(0)).arg(levels.at(1)).arg(levels.at(2));
		 }

		 break;
	 case HnProjectEnums::DegreeRoad2018:
		 if (levels.size() > 4)
		 {
			 IriEvaluateStr = QStringLiteral("=IF(%1%2>=%3,\"优\",IF(%1%2>=%4,\"良\",IF(%1%2>=%5,\"中\",IF(%1%2>=%6,\"次\",\"差\"))))")
				 .arg(colStr).arg(QString::number(row)).arg(levels.at(0)).arg(levels.at(1)).arg(levels.at(2)).arg(levels.at(3));
		 }
		 break;
	 case HnProjectEnums::RuralRoadlowLevel:
		 if (levels.size() > 4)
		 {
			 IriEvaluateStr = QStringLiteral("=IF(%1%2>=%3,\"优\",IF(%1%2>=%4,\"良\",IF(%1%2>=%5,\"中\",IF(%1%2>=%6,\"次\",\"差\"))))")
				 .arg(colStr).arg(QString::number(row)).arg(levels.at(0)).arg(levels.at(1)).arg(levels.at(2)).arg(levels.at(3));
		 }
		 break;
	 default:
		 break;
	 }
	

	 return IriEvaluateStr;
 }

   QString hnOutExcelMile::getPBIScore()
   { 
	   QStringList levels = QString::fromLocal8Bit(m_roadTypeSetInfo.strPBI_KF).split(' ');
	   if (levels.size()>3)
	   {
		   PBIScoreStr = QString("=IF((100-%1*%4-%2*%5-%3*%6)>0,(100- %1*%4-%2*%5-%3*%6),0)").arg(QString::number(PbiNumbers[1]))
			   .arg(QString::number(PbiNumbers[2])).arg(QString::number(PbiNumbers[3])).arg(levels[1]).arg(levels[2]).arg(levels[3]);
			 
	   }
	   return PBIScoreStr;
   }

 QString hnOutExcelMile::getPbiEvaluateStr(QString colStr, int row)
   {
	 QStringList levels = QString::fromLocal8Bit(m_roadTypeSetInfo.strPBILevel).split(' ');
	 if (levels.size() > 4)
	 {
		 PbEvaluateStr = QStringLiteral("=IF(%1%2>%3,\"优\",IF(%1%2>=%4,\"良\",IF(%1%2>=%5,\"中\",IF(%1%2>=%6,\"次\",\"差\"))))")
			 .arg(colStr).arg(QString::number(row)).arg(levels.at(0)).arg(levels.at(1)).arg(levels.at(2)).arg(levels.at(3)); 
	 }
	 return PbEvaluateStr;
   }

 QString hnOutExcelMile::getPwiValueStr()
 {
	 double wr = getMtdWrValue(); 
	 double a1 = m_roadTypeSetInfo.dPWI_a1; 
	 if (a1 == 0)
	 {
		 //不存在车辙
		 a1 = 1;
	 }
	 PwiValueStr = QStringLiteral("=100-%1*POWER(%2,%3)").arg(QString::number(m_roadTypeSetInfo.dPWI_a0)).arg(QString::number(wr)).arg(QString::number(a1));
	 return PwiValueStr;
	
 }

 double hnOutExcelMile::getMtdWrValue()
 {
	 if (this->getCenterMtdValue() == 0)
	 {
		 return 0;
	 }
	 double wr = 0;
	 //"=IF(F%1-MIN(D%1,E%1)>0, 100*(F%1-MIN(D%1,E%1))/F%1,0)"

	 if (CenterMtdValue != 0)
	 {
		 double minValue = qMin(LeftMtdValue, RightMtdValue);
		 if (CenterMtdValue - minValue > 0)
		 {
			 wr = MyCommonMethods::rountToNDecimalPlaces((CenterMtdValue - minValue) * 100 / CenterMtdValue, m_xrSetting->sheetRoundingOffNum);
		 }
	 }
	 return wr;
 }

 

 double hnOutExcelMile::getRepresentSMtdValue()
 {
	
	 double v1 = getRightMtdValue(); 
	 double v2 = getLeftMtdValue();
	return (v1+ v2) / 2;
	 
 }

 double hnOutExcelMile::getMpdWrValue()
 {
	 if (this->getCenterMpdValue() == 0 )
	 {
		 return 0;
	 }
	 double wr = 0;
	 //"=IF(F%1-MIN(D%1,E%1)>0, 100*(F%1-MIN(D%1,E%1))/F%1,0)"

	 if (CenterMpdValue != 0)
	 {
		 double minValue = qMin(LeftMpdValue, RightMpdValue);
		 if (CenterMpdValue - minValue > 0)
		 {
			 wr = MyCommonMethods::rountToNDecimalPlaces((CenterMpdValue - minValue) * 100 / CenterMpdValue, m_xrSetting->sheetRoundingOffNum);
		 }
	 }
	 return wr;
 }

 

 QString hnOutExcelMile::getPwiEvaluateStr()
 {
	 double wr = getMtdWrValue(); 
	 double pwiValue = 100 - m_roadTypeSetInfo.dPWI_a0*pow(wr, m_roadTypeSetInfo.dPWI_a1);
	 
	 
	 QStringList levels = QString::fromLocal8Bit(m_roadTypeSetInfo.strRDILevel).split(' ');
	 if (levels.size() > 4)
	 {
		 PwiEvaluateStr = QStringLiteral("=IF(%1>=%2,\"优\",IF(%1>=%3,\"良\",IF(%1>=%4,\"中\",IF(%1>=%5,\"次\",\"差\"))))").arg(QString::number(pwiValue)).arg(levels.at(0)).arg(levels.at(1)).arg(levels.at(2)).arg(levels.at(3));
	 }
	 return PwiEvaluateStr;
 }


 QString hnOutExcelMile::getMpdValueStr(QString colStr, int row)
 {
	 MpdValueStr = QStringLiteral("=100-%1*POWER(%2%4,%3)")
		 . arg(QString::number(m_roadTypeSetInfo.dPWI_a0))
		 .arg(colStr)
		 .arg(QString::number(m_roadTypeSetInfo.dPWI_a1))
.arg(QString::number(row));
	 return MpdValueStr;
 }

 QString hnOutExcelMile::getMpdEvaluateStr(QString colStr, int row)
 {
	 QStringList levels = QString::fromLocal8Bit(m_roadTypeSetInfo.strRDILevel).split(' ');
	 if (levels.size() > 4)
	 {
		 MpdEvaluateStr = QStringLiteral("=IF(%1%6>=%2,\"优\",IF(%1%6>=%3,\"良\",IF(%1%6>=%4,\"中\",IF(%1%6>=%5,\"次\",\"差\"))))")
			 .arg(colStr)
			 .arg(levels.at(0))
			 .arg(levels.at(1))
			 .arg(levels.at(2))
			 .arg(levels.at(3))
			.arg(QString::number(row));
	 }
	 return MpdEvaluateStr;
 }

 void hnOutExcelMile::setCurvature(double value)
 {
	 Curvature = value;
 }

 double hnOutExcelMile::getCurvature()
 {
	 return Curvature;
 }

 void hnOutExcelMile::setLongitudianalSlope(double value)
 {
	 LongitudianalSlope = value;
 }

 double hnOutExcelMile::getLongitudianalSlope()
 {
	 return LongitudianalSlope;
 }

 void hnOutExcelMile::setCrossSlope(double value)
 {
	 CrossSlope = value;

 }

 double hnOutExcelMile::getCrossSlope()
 {
	 return CrossSlope;
 }

 hnCommon::hnRoadTypeSetInfo hnOutExcelMile::getRoadTypeSetInfo()
 {
	 return m_roadTypeSetInfo;
 }

 void hnOutExcelMile::setPbiNumbers(int index)
 {
	 this->PbiNumbers[index]++;
 }

 int hnOutExcelMile::getPbiNumber(int index)
 {
	 return this->PbiNumbers[index];
 }

 QString hnOutExcelMile::getMqiValue(int rowCount,QString sciIndex,QString pqiIndex,QString bciIndex,QString tciIndex)
 {
	 QString rowCountStr = QString::number(rowCount);
	  if (hasBPTT)
	  {
		  return QString("0");
	  }
	  else
	  {
		  QString temp = QString("%1").arg(15);
		  QString tem1p = QString("%2%1").arg(15,0,10).arg("F");
		 QString valueStr =   QString("=%5%9*%1+%6%9*%2+%7%9*%3+%8%9*%4").arg(QString::number(m_roadTypeSetInfo.dMQI_WSCI))
			  .arg(QString::number(m_roadTypeSetInfo.dMQI_WPQI)).arg(QString::number(m_roadTypeSetInfo.dMQI_WBCI)).arg(QString::number(m_roadTypeSetInfo.dMQI_WTCI))
			.arg(sciIndex).arg(pqiIndex).arg(bciIndex).arg(tciIndex).arg(rowCount);
		 return valueStr;
	  }
 }


 QString hnOutExcelMile::getPqiValue(int rowCount,QString pciIndex,QString rqiIndex,QString rdiIndex,QString pbiIndex,QString pwiIndex)
 {
	 QString rowCountStr = QString::number(rowCount);
	 QString pqiStr("0"); 
	 if (this->RoadGrad <= 1)
	 { 
		 if (m_xrSetting->roadSnKcShowExcel&& this->RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE)//有刻槽,并且是水泥，pwi不参与计算
		 {
			 //ROUND((0.35*F5+0.3*G5+0.15*IF(EXACT(H5,"-"),0,H5)+0.1*IF(EXACT(I5,"-"),0,I5)+0.1*IF(EXACT(J5,"-"),0,J5))/(0.35+0.3+0.15+0.1+0.1),5)
			 pqiStr = QString("=ROUND((%1*%5%9+%2*%6%9+%3*IF(EXACT(%7%9,\"-\"),0,%7%9)+%4*IF(EXACT(%8%9,\"-\"),0,%8%9))/(%1+%2+%3+%4),5)")
				 . arg(QString::number(m_roadTypeSetInfo.dPQI_WPCI)).
				 arg(QString::number(m_roadTypeSetInfo.dPQI_WRQI))
				 .arg(QString::number(m_roadTypeSetInfo.dPQI_WRDI))
				 .arg(QString::number(m_roadTypeSetInfo.dPQI_WPBI))
				 .arg(pciIndex).arg(rqiIndex).arg(rdiIndex).arg(pbiIndex).arg(rowCountStr);
		 }
		 else
		 {
			 pqiStr = QString("=ROUND((%1*%6%11+%2*%7%11+%3*IF(EXACT(%8%11,\"-\"),0,%8%11)+%4*IF(EXACT(%9%11,\"-\"),0,%9%11)+%5*IF(EXACT(%10%11,\"-\"),0,%10%11))/(%1+%2+%3+%4+%5),5)")
				 .arg(QString::number(m_roadTypeSetInfo.dPQI_WPCI))
				 .arg(QString::number(m_roadTypeSetInfo.dPQI_WRQI))
				 .arg(QString::number(m_roadTypeSetInfo.dPQI_WRDI))
				 .arg(QString::number(m_roadTypeSetInfo.dPQI_WPBI))
				 .arg(QString::number(m_roadTypeSetInfo.dPQI_WPWI)) 
				 .arg(pciIndex).arg(rqiIndex).arg(rdiIndex).arg(pbiIndex).arg(pwiIndex).arg(rowCountStr);
		 }
	 }
	 else
	 { 
		 pqiStr = QString("=ROUND((%1*%3%5+%2*%4%5)/(%1+%2),5)")
			.arg(QString::number(m_roadTypeSetInfo.dPQI_WPCI))
			 .arg(QString::number(m_roadTypeSetInfo.dPQI_WRQI)).arg(pciIndex).arg(rqiIndex).arg(rowCountStr);
	 }
	 return pqiStr;
 }

QString hnOutExcelMile::getPqiEvaluateStr(int rowCnt, QString pqiIndex)
 {
	QString pqiEvaluateStr;
	QStringList pqiJundge = QString::fromLocal8Bit(m_roadTypeSetInfo.strPQILevel).split(" ");
	if (pqiJundge.size()>3)
	{
		pqiEvaluateStr = QString("=IF(%10%11>=%1,%5,IF(%10%11>=%2,%6,IF(%10%11>=%3,%7,IF(%10%11>=%4,%8,%9))))")
			.arg(pqiJundge.at(0))
			.arg(pqiJundge.at(1))
			.arg(pqiJundge.at(2))
			.arg(pqiJundge.at(3))
			.arg(QStringLiteral("\"优\""))
			.arg(QStringLiteral("\"良\""))
			.arg(QStringLiteral("\"中\""))
			.arg(QStringLiteral("\"次\""))
			.arg(QStringLiteral("\"差\""))
			.arg(pqiIndex)
			.arg(QString::number(rowCnt));
	}
	 
	return pqiEvaluateStr;
 }

 void hnOutExcelMile::setBptt(bool has)
 {
	 this->hasBPTT = has;
 }

 void hnOutExcelMile::setTciValue(double value)
 {
	 this->Tci = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum);  
 }

 void hnOutExcelMile::setSciValue(double value)
 {
	 this->Sci = MyCommonMethods::rountToNDecimalPlaces(value, m_xrSetting->sheetRoundingOffNum);
 }

 double hnOutExcelMile::getTciValue()
 {
	 return Tci;

 }

 double hnOutExcelMile::getSciValue()
 {
	 return Sci;
 }

 QString hnOutExcelMile::getTciEvaluate()
 {
    QString  eva = 	 QStringLiteral("=IF(%1>=90,\"优\",IF(%1>=80,\"良\",IF(%1>=70,\"中\",IF(%1>=60,\"次\",\"差\"))))").arg(QString::number( Tci));
  return eva;
 }

  QString hnOutExcelMile::getSciEvaluate()
 {
	 QString  eva = QStringLiteral("=IF(%1>=90,\"优\",IF(%1>=80,\"良\",IF(%1>=70,\"中\",IF(%1>=60,\"次\",\"差\"))))").arg(QString::number(Sci));
	 return eva;
 }

  void hnOutExcelMile::setRoadLength(double length)
  {
	  this->RoadLength = length;
  }

 


  bool hnOutExcelMile::StartCalculate(bool onlyInitSetInfo /*= false */)
  {
	  if (!m_project)
	  {
		  return false;
	  }
	  //if (!onlyInitSetInfo)
	  //{
		 // m_currentMileVec.clear();
		 // //填充 QVector<hnMile> m_currentMileVec;
		 // QVector<hnMile> allMile = m_project->getCurrentMileVector();
		 // int line = 1;//上行 
		 // if (StartMile > EndMile)
		 // {
			//  line = -1;//下行
		 // }
		 // Direction = line;
		 // for (hnMile mile : allMile)
		 // {
			//  if (line > 0)
			//  {
			//	  if (StartMile <= mile.dTrueMile&& mile.dTrueMile < EndMile)
			//	  {
			//		  m_currentMileVec.push_back(mile);
			//	  }
			//  }
			//  else
			//  {
			//	  if (StartMile >= mile.dTrueMile&& mile.dTrueMile > EndMile)
			//	  {
			//		  m_currentMileVec.push_back(mile);
			//	  }
			//  }
		 // }
	  //}
	 
	  //获取规范计算参数 
	  hnApp::hnDataManager::getDataManager()->getRoadTypeSetInfo(Type, RoadDegreestr, RoadSurface, m_roadTypeSetInfo);

	  return true;
  }

bool hnOutExcelMile::calculateDrScore(double width,QVector<hnCommon::hnRoadDiseaseInfo>& diss)
{
	
	double sDmi = 0;
	double eDmi = 0;
	sDmi = StartDmi;
	eDmi = EndDmi;

	for (auto dis : diss)
	{


		 if ((dis.dDmiStart<sDmi&&dis.dDmiEnd<= sDmi)||
			 (dis.dDmiStart>=eDmi&&dis.dDmiEnd>eDmi))
		 {
			 continue;
		 }
		 else
		 {
			 m_roadDisVec.push_back(dis);
		 }
	}
	//m_project->getDB()->m_diseaseTable.readRoadDiseaseData(m_currentMileVec, m_roadDisVec, line, m_project->getRoadSpace());
	//m_project->getDB()->m_diseaseTable.readStreetData(m_currentMileVec, m_streetDisVec, Direction, m_project->getRoadSpace());
	double roadArea = width * this->getRoadLength();
	
	//计算Dr
	double sumArea = 0;
	for (int i = 0 ; i <m_roadDisVec.size();++i)
	{
		hnCommon::hnRoadDiseaseInfo & dis = m_roadDisVec[i];
		  
		double length = dis.dDmiEnd - dis.dDmiStart;
		//病害部分在区间内
		if (dis.dDmiStart<=sDmi&&dis.dDmiEnd<=eDmi)
		{
		
			sumArea += (dis.dArea *dis.diseaseWeight*((dis.dDmiEnd - sDmi) / length));
			dis.dArea = (dis.dArea *dis.diseaseWeight*((dis.dDmiEnd - sDmi) / length));
		}
		 
		else	if (dis.dDmiStart >= sDmi&&dis.dDmiEnd >= eDmi)
		{ 
			sumArea += (dis.dArea *dis.diseaseWeight*((eDmi-dis.dDmiStart) / length));
			dis.dArea = (dis.dArea *dis.diseaseWeight*((eDmi - dis.dDmiStart) / length));
		}
		//病害完全在区间内
		else if (dis.dDmiStart>=sDmi&&dis.dDmiEnd<=eDmi)
		{
			sumArea += dis.dArea *dis.diseaseWeight;
			dis.dArea = dis.dArea *dis.diseaseWeight;
		}
	    //病害整个在区间呢
		else	if (dis.dDmiStart<=sDmi&&dis.dDmiEnd>=eDmi)
		{
			sumArea += (dis.dArea *dis.diseaseWeight*((eDmi-sDmi) / length));
			dis.dArea = (dis.dArea *dis.diseaseWeight*((eDmi - sDmi) / length));
		}
	}
	if (sumArea != 0)
	{
		int a = 0;
	}
	double value = (100 * sumArea / roadArea);
	setDrScore(value);
	switch (Type)
	{
	case HnProjectEnums::None:
		break;
	case HnProjectEnums::DegreeRoad2018:
		PCIExcelStr = QStringLiteral("=100-%2*POWER(%1,%3)").arg(QString::number(getDRScore())).arg(QString::number(m_roadTypeSetInfo.dPCI_a0))
			.arg(QString::number(m_roadTypeSetInfo.dPCI_a1));
		break;
	case HnProjectEnums::CityRoad:
	{
		//需要当前区间的道路类型
		QVector<hnDiseaseSetInfo> diseaseSetInfos = hnApp::hnDataManager::getDataManager()->
			getProjectRoadDiseaseNames(m_project, Type, RoadSurface);
		int len = diseaseSetInfos.size();
		
		QMap<hnDiseaseSetInfo, double> diseaseSum;
		for (int disTempIndex = 0; disTempIndex < diseaseSetInfos.size(); ++disTempIndex)
		{
			diseaseSum.insert(diseaseSetInfos.at(disTempIndex), 0);
		}
		for (auto it = diseaseSum.begin();it!=diseaseSum.end();++it)
		{
			
			for (int disIndex = 0; disIndex < m_roadDisVec.size(); ++disIndex)
			{
				if (strcmp(it.key().strDBTableName, m_roadDisVec.at(disIndex).strDiseaseTableName) == 0)
				{
					it.value() += m_roadDisVec.at(disIndex).dArea;
				}
			}
		}
		QVector< hn_RoadDiseaseType> roadDiseaseType;
		for (auto it = diseaseSum.begin(); it != diseaseSum.end(); ++it)
		{
			hn_RoadDiseaseType curType;
			curType.type = it.key();
			QString dismidu = QString::fromLocal8Bit(curType.type.strSHMD);
		
			QString disscore = QString::fromLocal8Bit(curType.type.strDXKF);
			QStringList disSplit = dismidu.split(' ');
			QStringList dissSplit = disscore.split(' ');
			QVector<double> disTmep;
			for (QString temp : disSplit)
			{
				disTmep.push_back(temp.toDouble()*0.01);
			}
			curType._MiduScore.push_back(disTmep);
			QVector<double> disTmep1;
			for (QString temp : dissSplit)
			{
				disTmep1.push_back(temp.toDouble());
			}
			curType._MiduScore.push_back(disTmep1);

			curType.d_sumArea = it.value();
			roadDiseaseType.push_back(curType);
		}

		QVector< double> totalareatmp;
		totalareatmp.resize(len);
		double uij = 0, wij = 0; 
		QVector<double> DPa  { 0, 0, 0, 0, 0 };
		for (int i = 0 ;  i<len ; ++i)
		{
			totalareatmp[i] =roadDiseaseType[i].d_sumArea / roadArea;
		   totalareatmp[i] = ChaZhi(roadDiseaseType[i]._MiduScore, totalareatmp[i]);
		}
		// 类别内的扣分和
		for (int i = 0; i < len; i++)
		{
			DPa[roadDiseaseType[i].type.fEffectType] += totalareatmp[i];
		}
		QVector < QVector<double>> _WeightParm{ { 3.0,-5.5,3.5,0 },{ 3.0,-5.5,3.5,0 } };
		// 每种病害的uij，得到每种的权重曲线的扣分
		for (int i = 0; i < len; i++)
		{
			if (DPa[roadDiseaseType[i].type.fEffectType] > 0)
			{
				uij = totalareatmp[i] / DPa[roadDiseaseType[i].type.fEffectType];
			}
			else
			{
				uij = 0;
			}
			/*
			<水泥路面权函数曲线 Wi="3.0 -5.5 3.5"> 
			<沥青路面权函数曲线 Wi="3.0 -5.5 3.5">  
			*/
			

			wij = _WeightParm[RoadSurface][0];
			for (int k = 1; k < _WeightParm[RoadSurface].size(); k++)
			{
				wij = wij * uij + _WeightParm[RoadSurface][k];
			}

			totalareatmp[i] = totalareatmp[i] * wij;
		}

		for (int i = 0; i < DPa.size(); ++i)
		{
			DPa[i] = 0;
		}

		// 每类病害的扣分
		double DP = 0;
		for (int i = 0; i < len; i++)
		{
			DPa[roadDiseaseType[i].type.fEffectType] += totalareatmp[i];
			DP += totalareatmp[i];
		}

		double DP2 = 0;
		for (int i = 0; i < DPa.size(); ++i)
		{
			if (DP > 0)
			{
				uij = DPa[i] / DP;
			}
			else
			{
				uij = 0;
			}

			wij = _WeightParm[RoadSurface][0];
			for (int k = 1; k < _WeightParm[RoadSurface].size(); k++)
			{
				wij = wij * uij + _WeightParm[RoadSurface][k];
			}

			DPa[i] = DPa[i] * wij;

			DP2 = DP2 + DPa[i];
		}
	     PCIExcelStr = QString::number( 100 - DP2,'f',5); 
	}


		break;
	case HnProjectEnums::RuralRoadlowLevel:
		PCIExcelStr = QStringLiteral("=100-%2*POWER(%1,%3)").arg(QString::number(getDRScore())).arg(QString::number(m_roadTypeSetInfo.dPCI_a0))
			.arg(QString::number(m_roadTypeSetInfo.dPCI_a1));
		break;
	default:
		break;
	}

	return true;
}

bool hnOutExcelMile::calculateRQIScore(bool hasleftValue,bool hasRightValue)
{
	//计算rqi
	if (m_xrSetting->IRIExcelSide == 2)
	{
		if (m_xrSetting->RQIJudgeType == 0)
		{
			if (hasleftValue&& hasRightValue)
			{
				judgeIirValue = (LeftIriValue + RightIriValue) / 2, m_xrSetting->sheetRoundingOffNum;
			}
			else
			{
				MyCommonMethods::rountToNDecimalPlaces((LeftIriValue + RightIriValue) /1, m_xrSetting->sheetRoundingOffNum); ; 
			}
			
		}
		else
		{
			judgeIirValue = qMax(LeftIriValue, RightIriValue);
		}

	}
	else if (m_xrSetting->IRIExcelSide == 0)
	{
		judgeIirValue = LeftIriValue;

	}
	else if (m_xrSetting->IRIExcelSide == 1)
	{
		judgeIirValue = RightIriValue;
	}
	switch (m_project->getBaseStandard())
	{
	case  HnProjectEnums::CityRoad:
		IriExcelStr = QStringLiteral("=IF(%2+%3*%1>0,%2+%3*%1,0)").arg(QString::number(getJudgeIirValue())).arg(QString::number(m_roadTypeSetInfo.dRQI_a0)).arg(QString::number(m_roadTypeSetInfo.dRQI_a1));
		break;
	case  HnProjectEnums::DegreeRoad2018:
		IriExcelStr = QStringLiteral("=ROUND(100/(1+%1*EXP(%2*%3)),5)").arg(QString::number(m_roadTypeSetInfo.dRQI_a0)).arg(QString::number(m_roadTypeSetInfo.dRQI_a1)).arg(QString::number(getJudgeIirValue()));

		break;
	case HnProjectEnums::RuralRoadlowLevel:
		IriExcelStr = QStringLiteral("=ROUND(100/(1+%1*EXP(%2*%3)),5)").arg(QString::number(m_roadTypeSetInfo.dRQI_a0)).arg(QString::number(m_roadTypeSetInfo.dRQI_a1)).arg(QString::number(getJudgeIirValue()));
		break;
	default:
		break;
	}
	return true;
}

bool hnOutExcelMile::calculatePBIScore()
{
	judgePbStr =QStringLiteral("=MAX(%1,%2)").arg(QString::number(LeftPbValue)).arg(QString::number(RightPbValue));
	judgePbiValue = qMax(LeftPbValue, RightPbValue);
	return true;
}

bool hnOutExcelMile::calculateRUTScore()
{
	switch (Type)
	{
	case HnProjectEnums::None:
		break;
	case HnProjectEnums::DegreeRoad2018:
		RutExcelStr = QStringLiteral("=IF(%1<%2,%3-%4*%1,IF(%1<%5,%6-%7*(%1-%2),0))").arg(QString::number(judgeRutValue)).arg(QString::number(m_roadTypeSetInfo.dRDI_RDa)).arg(QString::number(m_roadTypeSetInfo.dRDI_a))
			.arg(QString::number(m_roadTypeSetInfo.dRDI_a0)).arg(QString::number(m_roadTypeSetInfo.dRDI_RDb)).arg(QString::number(m_roadTypeSetInfo.dRDI_b)).arg(QString::number(m_roadTypeSetInfo.dRDI_a1));

		RutMaxExcelStr = QStringLiteral("=IF(%1<%2,%3-%4*%1,IF(%1<%5,%6-%7*(%1-%2),0))").arg(QString::number(RutMaxValue)).arg(QString::number(m_roadTypeSetInfo.dRDI_RDa)).arg(QString::number(m_roadTypeSetInfo.dRDI_a))
			.arg(QString::number(m_roadTypeSetInfo.dRDI_a0)).arg(QString::number(m_roadTypeSetInfo.dRDI_RDb)).arg(QString::number(m_roadTypeSetInfo.dRDI_b)).arg(QString::number(m_roadTypeSetInfo.dRDI_a1));

		break;
	case HnProjectEnums::CityRoad:
		RutExcelStr = QStringLiteral("=IF(%1<%2,%3-%4*%1,IF(%1<%5,%6-%7*(%1-%2),0))").arg(QString::number(judgeRutValue)).arg(QString::number(m_roadTypeSetInfo.dRDI_RDa)).arg(QString::number(m_roadTypeSetInfo.dRDI_a))
			.arg(QString::number(m_roadTypeSetInfo.dRDI_a0)).arg(QString::number(m_roadTypeSetInfo.dRDI_RDb)).arg(QString::number(m_roadTypeSetInfo.dRDI_b)).arg(QString::number(m_roadTypeSetInfo.dRDI_a1));
 
	RutMaxExcelStr = QStringLiteral("=IF(%1<%2,%3-%4*%1,IF(%1<%5,%6-%7*(%1-%2),0))").arg(QString::number(RutMaxValue)).arg(QString::number(m_roadTypeSetInfo.dRDI_RDa)).arg(QString::number(m_roadTypeSetInfo.dRDI_a))
			.arg(QString::number(m_roadTypeSetInfo.dRDI_a0)).arg(QString::number(m_roadTypeSetInfo.dRDI_RDb)).arg(QString::number(m_roadTypeSetInfo.dRDI_b)).arg(QString::number(m_roadTypeSetInfo.dRDI_a1));

		/*
		string.Format("=IF(F{0}<={1},{2}-{3}*F{0},IF(F{0}<={4},{5}-{6}*(F{0}-{1}),0))",
					i + 4, _RDIRD[0][1], _RDIRD[0][0], _RDIa[0], _RDIRD[1][1], _RDIRD[1][0], _RDIa[1]);
		*/


		break;
	case HnProjectEnums::RuralRoadlowLevel:
		RutExcelStr = QStringLiteral("=IF(%1<%2,%3-%4*%1,IF(%1<%5,%6-%7*(%1-%2),0))").arg(QString::number(judgeRutValue)).arg(QString::number(m_roadTypeSetInfo.dRDI_RDa)).arg(QString::number(m_roadTypeSetInfo.dRDI_a))
			.arg(QString::number(m_roadTypeSetInfo.dRDI_a0)).arg(QString::number(m_roadTypeSetInfo.dRDI_RDb)).arg(QString::number(m_roadTypeSetInfo.dRDI_b)).arg(QString::number(m_roadTypeSetInfo.dRDI_a1));

		RutMaxExcelStr = QStringLiteral("=IF(%1<%2,%3-%4*%1,IF(%1<%5,%6-%7*(%1-%2),0))").arg(QString::number(RutMaxValue)).arg(QString::number(m_roadTypeSetInfo.dRDI_RDa)).arg(QString::number(m_roadTypeSetInfo.dRDI_a))
			.arg(QString::number(m_roadTypeSetInfo.dRDI_a0)).arg(QString::number(m_roadTypeSetInfo.dRDI_RDb)).arg(QString::number(m_roadTypeSetInfo.dRDI_b)).arg(QString::number(m_roadTypeSetInfo.dRDI_a1));

		break;
	default:
		break;
	}
	return true;
}

bool hnOutExcelMile::calculatePwiScore()
{
	 
	return true;
}

void hnOutExcelMile::getallEvaluate1(const QString&colStr, int row)
{
 
}
 
 
bool hnOutExcelMile::calcaulateStreetScore(int type, QVector<hnCommon::hnRoadDiseaseInfo>& diss, QVector<hnDiseaseSetInfo>& disSetting)
{
	
	m_streetDisManageMap.clear();
	double sDmi = 0;
	double eDmi = 0;
	sDmi = StartDmi;
	eDmi =EndDmi;
for (auto dis : diss)
	{
		if (dis.ndiseaseType == type)
		{
			if (dis.dDmi >= sDmi&&dis.dDmi < eDmi)
			{
				m_streetDisVec.push_back(dis);
			}
		}

	} 
	QString preDisName;
	
	for (auto dis : disSetting)
	{
		StreetDiseaseManage manamge;
		manamge.StreetDis = dis; 
		QString disName =QString::fromLocal8Bit(  dis.strDiseaseTypeName);
		m_streetDisManageMap.insert(disName, manamge);
		
		 
		preDisName = QString::fromLocal8Bit( manamge.StreetDis.strDiseaseName) ;
	}
	//计算病害得分
	for (auto dis: m_streetDisVec)
	{
	     	QString disName = QString::fromLocal8Bit( dis.strDisName);
			if (m_streetDisManageMap.contains(disName))
			{
			        if (m_streetDisManageMap[disName].StreetDis.dEffectMeasure==0)
			        {
						m_streetDisManageMap[disName].Area += dis.dArea;
			        }
					else if (m_streetDisManageMap[disName].StreetDis.dEffectMeasure == 1)
					{
						m_streetDisManageMap[disName].Count += dis.dArea;
					}
			}
			if (this->Type==  HnProjectEnums::DegreeRoad2018)
			{
				if (type == 2)
				{
					QString  extraStr = QStringLiteral("边坡坍塌.重"); //额外条件 存在的时候 MQI=0
					if (disName == extraStr&& dis.dArea != 0)
					{ 
						this->setBptt(true); 
					}
					extraStr = QStringLiteral("路基构造物损坏.重");
					if (disName == extraStr&& dis.dArea != 0)
					{
						Sci = 0;
						return true;

					}
				}
				if (type==1)
				{
				      if (disName.contains(QStringLiteral("标线缺损")) || disName.contains(QStringLiteral("绿化管理不善")))
				      {
						  //每10m扣1分  不足10m计10m
						  double tempValue = dis.dArea;
						  if (tempValue>10)
						  {
							  dis.dArea = 10;
						  }
						  else
						  {
							     dis.dArea =  std::ceil(tempValue / 10.0) * 10;
						  }
				      }
				}
			}
		
			 
	}
	double tclval = 0; 
	double ttclval = 0;
	int index = 0;
	QMap<QString, StreetDiseaseManage>::iterator dd;
	StreetDiseaseManage preDisTmep;

	for (dd = m_streetDisManageMap.begin()  ; dd !=m_streetDisManageMap.end();++dd,++index)
	{ 
		//TODO 这个地方城镇道路会报错 需要处理
	
		
		StreetDiseaseManage disTemp = dd.value();
		if (type == 1)
		{
			m_streetYXDisManageVec.push_back(disTemp);
		}
		if (type == 2)
		{
			m_streetLjDisManageVec.push_back(disTemp);
		}
		QString disName = QString::fromLocal8Bit(disTemp.StreetDis.strDiseaseName);
		if (index>0)
		{
			QString preDisName = QString::fromLocal8Bit(preDisTmep.StreetDis.strDiseaseName); 
			if (disName != preDisName)
			{
				ttclval = ttclval * 1000 / this->getRoadLength();
				ttclval = ttclval > 100 ? 100 : ttclval;
				tclval += preDisTmep.StreetDis.fWidget * (100-ttclval);
				ttclval = 0;
			}
		}
		
		if (disTemp.StreetDis.dEffectMeasure == 0)
		{
			ttclval = ttclval + disTemp.StreetDis.nDWKF * disTemp.Area;
		}
		else if (disTemp.StreetDis.dEffectMeasure == 1)
		{
			ttclval = ttclval + disTemp.StreetDis.nDWKF *  disTemp.Count;
		}
		 preDisTmep = dd.value();
	}
	ttclval = ttclval * 1000 / getRoadLength();
	ttclval = ttclval > 100 ? 100 : ttclval;
	
	{
		//最后一个如果已经出现过 前面的循环就不会处理最后一个的分值
		QString key =  m_streetDisManageMap.lastKey();
		StreetDiseaseManage lastDis = m_streetDisManageMap[key];
		if (lastDis.StreetDis.dEffectMeasure == 0 )
		{
			ttclval  += lastDis.StreetDis.nDWKF * lastDis.Area;
		}
		else
		{
			ttclval += lastDis.StreetDis.nDWKF * lastDis.Count;
		}
		tclval += lastDis.StreetDis.fWidget *(100 - ttclval);
	}
	 
	if (type==1)
	{ 
		this->setTciValue(tclval);
		 ;
	}
	if (type==2)
	{
		this->setSciValue(tclval);
	}


	return true;
}

 

double hnOutExcelMile::ChaZhi(QVector<QVector<double>> MiduScore, double mval)
{
	double sval = 0;
	int len = MiduScore[0].size();
	for (int i = 1; i < len; i++)
	{
		if (mval < MiduScore[0][i] && mval >= MiduScore[0][i - 1])
		{
			if (i == 1)
			{
				sval = mval * (MiduScore[1][i] - MiduScore[1][i - 1]) / (MiduScore[0][i] - MiduScore[0][i - 1]);
			}
			else
			{
				if (i < len - 1)
				{
					sval = (mval - MiduScore[0][i - 1])
						* (MiduScore[1][i] - MiduScore[1][i - 1])
						/ (MiduScore[0][i] - MiduScore[0][i - 1])
						+ MiduScore[1][i - 1];
				}
				else
				{
					if (MiduScore[0][len - 1] == MiduScore[0][len - 2])
					{
						sval = MiduScore[1][len - 1];
					}
					else
					{
						sval = (mval - MiduScore[0][i - 1])
							* (MiduScore[1][i] - MiduScore[1][i - 1])
							/ (MiduScore[0][i] - MiduScore[0][i - 1])
							+ MiduScore[1][i - 1];
					}
				}
			}
		}
	}
	return sval;
}

void hnOutExcelMile::GetRutDis(QVector<hnCommon::hnRoadDiseaseInfo> &rutDis,int side)
{
	QVector<double > rutThresh;
	if (m_roadTypeSetInfo.dRutThreslodDown !=0)
	{
		rutThresh.push_back(m_roadTypeSetInfo.dRutThreslodDown);
		rutThresh.push_back(m_roadTypeSetInfo.dRutThreslodUp);
	}
	else
	{
		rutThresh.push_back(m_roadTypeSetInfo.dRutThreslodUp);
	}

 
}



