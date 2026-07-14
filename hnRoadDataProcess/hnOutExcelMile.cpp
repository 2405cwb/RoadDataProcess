#pragma  once
#include "hnOutExcelMile.h"
#include "DrPciCalculator.h"
#include "../hnApplication/hnDataManager.h"
#include "../hnCommon/hnTypeDefs.h"
#include "../hnQtCommon/MyCommonMethods.h"
hnOutExcelMile::hnOutExcelMile(hnPro::hnProject * project, HnProjectEnums::StandardParmTypeEnum roadStandard):m_project(project),Type(roadStandard) 
{
	
	PCIExcelStr ="";
	DRScore = 0;
	SurveyWidth = 0;
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
	SurveyWidth = 0;
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
	 return DRScore;
 }

 double hnOutExcelMile::getDRExcelScore()
 {
	 return MyCommonMethods::rountToNDecimalPlaces(DRScore, m_xrSetting->sheetRoundingOffNum_Dr);
 }

 void hnOutExcelMile::setDrScore(double value)
 { 
	 DRScore = value;
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

 


void hnOutExcelMile::reportCalculationError(const QString& message)
{
	QString projectName = m_project ? m_project->get2DProName() : QStringLiteral("未知工程");
	QString fullMessage = projectName + QStringLiteral("\r\n") + message.trimmed();
	if (m_xrSetting && !fullMessage.trimmed().isEmpty() &&
		!m_xrSetting->ExcelErrorMessageList.contains(fullMessage))
	{
		m_xrSetting->ExcelErrorMessageList.append(fullMessage);
	}
}

bool hnOutExcelMile::StartCalculate(bool onlyInitSetInfo /*= false */)
{
	Q_UNUSED(onlyInitSetInfo);
	if (!m_project)
	{
		return false;
	}

	m_roadTypeSetInfo = hnRoadTypeSetInfo();
	const hnCommon::ROAD_WORK_TYPE drawType = m_project->getBaseDrawType();
	const bool found = hnApp::hnDataManager::getDataManager()->getRoadTypeSetInfo(
		Type, RoadDegreestr, RoadSurface, drawType, m_roadTypeSetInfo);
	if (!found)
	{
		reportCalculationError(QStringLiteral("找不到道路计算参数，已停止该类型分段的 DR/PCI 计算：规范=%1，等级=%2，路面=%3，作业模式=%4")
			.arg(HnProjectEnums::roadTypeEnumToQString(Type))
			.arg(RoadDegreestr)
			.arg(RoadSurfaceStr)
			.arg(QString::fromLocal8Bit(hnCommon::workTypeToQString(drawType))));
		return false;
	}

	if ((Type == HnProjectEnums::DegreeRoad2018 || Type == HnProjectEnums::RuralRoadlowLevel) &&
		(!hnDrPci::isFinite(m_roadTypeSetInfo.dPCI_a0) || !hnDrPci::isFinite(m_roadTypeSetInfo.dPCI_a1) ||
		m_roadTypeSetInfo.dPCI_a0 <= 0.0 || m_roadTypeSetInfo.dPCI_a1 <= 0.0))
	{
		reportCalculationError(QStringLiteral("道路计算参数中的 PCI 系数无效，已停止该类型分段计算：规范=%1，等级=%2，路面=%3")
			.arg(HnProjectEnums::roadTypeEnumToQString(Type)).arg(RoadDegreestr).arg(RoadSurfaceStr));
		return false;
	}
	return true;
}

bool hnOutExcelMile::calculateDrScore(double fallbackWidth, QVector<hnCommon::hnRoadDiseaseInfo>& diss)
{
	m_roadDisVec.clear();
	setDrScore(0.0);
	PCIExcelStr.clear();

	const double segmentLength = getRoadLength();
	double surveyWidth = SurveyWidth;
	if (!hnDrPci::isFinite(surveyWidth) || surveyWidth <= hnDrPci::kEpsilon)
	{
		surveyWidth = fallbackWidth;
		reportCalculationError(QStringLiteral("未获得分段实际检测宽度，DR 调查面积已回退使用工程默认宽度。"));
	}
	const double surveyArea = surveyWidth * segmentLength;
	if (!hnDrPci::isFinite(surveyWidth) || surveyWidth <= hnDrPci::kEpsilon ||
		!hnDrPci::isFinite(segmentLength) || segmentLength <= hnDrPci::kEpsilon ||
		!hnDrPci::isFinite(surveyArea) || surveyArea <= hnDrPci::kEpsilon)
	{
		reportCalculationError(QStringLiteral("分段调查面积无效，已停止 DR/PCI 计算：起点=%1，终点=%2，长度=%3，宽度=%4")
			.arg(StartMile).arg(EndMile).arg(segmentLength).arg(surveyWidth));
		return false;
	}

	double sumArea = 0.0;
	bool hasInvalidDisease = false;
	for (int i = 0; i < diss.size(); ++i)
	{
		const hnCommon::hnRoadDiseaseInfo& source = diss[i];
		double clippedArea = 0.0;
		if (!hnDrPci::clippedWeightedArea(source.dDmiStart, source.dDmiEnd,
			source.dArea, source.diseaseWeight, StartDmi, EndDmi, clippedArea))
		{
			hasInvalidDisease = true;
			continue;
		}
		if (clippedArea <= hnDrPci::kEpsilon)
		{
			continue;
		}
		hnCommon::hnRoadDiseaseInfo clipped = source;
		clipped.dArea = clippedArea;
		m_roadDisVec.push_back(clipped);
		sumArea += clippedArea;
	}
	if (hasInvalidDisease)
	{
		reportCalculationError(QStringLiteral("部分病害的里程、面积或权重无效，已跳过这些病害并继续计算。"));
	}

	double dr = 0.0;
	if (!hnDrPci::calculateDr(sumArea, surveyArea, dr))
	{
		reportCalculationError(QStringLiteral("DR 计算结果无效，已停止该分段 PCI 计算：起点=%1，终点=%2")
			.arg(StartMile).arg(EndMile));
		return false;
	}
	setDrScore(dr); // Keep full precision; rounding happens only when writing report cells.

	switch (Type)
	{
	case HnProjectEnums::None:
		return true;
	case HnProjectEnums::DegreeRoad2018:
	case HnProjectEnums::RuralRoadlowLevel:
	{
		double pci = 0.0;
		if (!hnDrPci::calculatePowerPci(getDRScore(), m_roadTypeSetInfo.dPCI_a0,
			m_roadTypeSetInfo.dPCI_a1, pci))
		{
			reportCalculationError(QStringLiteral("PCI 系数或 DR 值无效，已停止该分段 PCI 计算。"));
			return false;
		}
		PCIExcelStr = QStringLiteral("=100-%2*POWER(%1,%3)")
			.arg(QString::number(getDRScore(), 'g', 15))
			.arg(QString::number(m_roadTypeSetInfo.dPCI_a0, 'g', 15))
			.arg(QString::number(m_roadTypeSetInfo.dPCI_a1, 'g', 15));
		return true;
	}
	case HnProjectEnums::CityRoad:
	{
		// Compatibility field only: City-road DR is the total weighted damaged-area rate;
		// CJJ PCI below is calculated from disease-density deductions, not directly from DR.
		QVector<hnDiseaseSetInfo> diseaseSetInfos = hnApp::hnDataManager::getDataManager()->
			getProjectRoadDiseaseNames(m_project, Type, RoadSurface);
		QMap<hnDiseaseSetInfo, double> diseaseSum;
		for (int i = 0; i < diseaseSetInfos.size(); ++i)
			diseaseSum.insert(diseaseSetInfos[i], 0.0);
		for (auto it = diseaseSum.begin(); it != diseaseSum.end(); ++it)
		{
			for (int i = 0; i < m_roadDisVec.size(); ++i)
			{
				if (strcmp(it.key().strDBTableName, m_roadDisVec[i].strDiseaseTableName) == 0)
					it.value() += m_roadDisVec[i].dArea;
			}
		}

		QVector<hn_RoadDiseaseType> roadDiseaseType;
		for (auto it = diseaseSum.begin(); it != diseaseSum.end(); ++it)
		{
			hn_RoadDiseaseType current;
			current.type = it.key();
			const QStringList densityText = QString::fromLocal8Bit(current.type.strSHMD)
				.split(' ', QString::SkipEmptyParts);
			const QStringList deductionText = QString::fromLocal8Bit(current.type.strDXKF)
				.split(' ', QString::SkipEmptyParts);
			if (densityText.isEmpty() || densityText.size() != deductionText.size())
			{
				reportCalculationError(QStringLiteral("城镇道路病害密度/扣分配置不完整：%1")
					.arg(QString::fromLocal8Bit(current.type.strDiseaseName)));
				return false;
			}
			QVector<double> density;
			QVector<double> deduction;
			for (int i = 0; i < densityText.size(); ++i)
			{
				density.push_back(densityText[i].toDouble() * 0.01);
				deduction.push_back(deductionText[i].toDouble());
			}
			current._MiduScore.push_back(density);
			current._MiduScore.push_back(deduction);
			current.d_sumArea = it.value();
			roadDiseaseType.push_back(current);
		}

		QVector<double> deductions(roadDiseaseType.size(), 0.0);
		QVector<double> categoryDeduction(5, 0.0);
		for (int i = 0; i < roadDiseaseType.size(); ++i)
		{
			const int effectType = roadDiseaseType[i].type.fEffectType;
			if (effectType < 0 || effectType >= categoryDeduction.size())
			{
				reportCalculationError(QStringLiteral("城镇道路病害影响分类超出有效范围，已停止 PCI 计算。"));
				return false;
			}
			deductions[i] = ChaZhi(roadDiseaseType[i]._MiduScore,
				roadDiseaseType[i].d_sumArea / surveyArea);
			categoryDeduction[effectType] += deductions[i];
		}

		const QVector<QVector<double>> weightParameters{
			{ 3.0, -5.5, 3.5, 0.0 }, { 3.0, -5.5, 3.5, 0.0 }
		};
		const int surfaceIndex = static_cast<int>(RoadSurface);
		if (surfaceIndex < 0 || surfaceIndex >= weightParameters.size())
		{
			reportCalculationError(QStringLiteral("城镇道路 PCI 不支持当前路面类型。"));
			return false;
		}

		for (int i = 0; i < deductions.size(); ++i)
		{
			const int effectType = roadDiseaseType[i].type.fEffectType;
			const double uij = categoryDeduction[effectType] > 0.0
				? deductions[i] / categoryDeduction[effectType] : 0.0;
			deductions[i] *= hnDrPci::cityWeight(uij);
		}

		categoryDeduction.fill(0.0);
		for (int i = 0; i < deductions.size(); ++i)
			categoryDeduction[roadDiseaseType[i].type.fEffectType] += deductions[i];

		double cityPci = 0.0;
		if (!hnDrPci::calculateCityPciFromCategories(categoryDeduction, cityPci))
		{
			reportCalculationError(QStringLiteral("城镇道路综合扣分无效，已停止 PCI 计算。"));
			return false;
		}
		PCIExcelStr = QString::number(cityPci, 'f', 5);
		return true;
	}
	default:
		return false;
	}
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
	if (MiduScore.size() != 2)
	{
		return 0.0;
	}
	double result = 0.0;
	if (!hnDrPci::interpolateClamped(MiduScore[0], MiduScore[1], mval, result))
	{
		return 0.0;
	}
	return result;
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



