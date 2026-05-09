#pragma once

enum PROJECTROADTYPE
{
	LIQING_ROAD_TYPE = 1,//Á¤ÇàÂ·Ãæ
	SHUINI_ROAD_TYPE,
	SHASHI_ROAD_TYPE
};

// 
typedef struct _HN_2D_POINT_
{
	_HN_2D_POINT_()
	{
		dx = 0.0;
		dy = 0.0;
	}
	_HN_2D_POINT_(double x, double y)
	{
		dx = x;
		dy = y;
	}
	double dx;
	double dy;
}hn2dPt;

class hnDiseaseInfoBase
{
public:
	hnDiseaseInfoBase() {
		nDiseaseType = -1;
		dLength = 0.0;
		dWidth = 0.0;
		dArea = 0.0;
		dDepth = 0.0;
		nLevel = 0;
		dBegMileage = 0.0;
		dEndMileage = 0.0;
		nType = 0;
		nGeometry = 0;
		nDiseaseSum = 0;
	}

	virtual string getBlockName() = 0;// {return "-";}
	virtual string getBlockName(int i) = 0;//{return "-";}
	virtual int getDiseaseType() = 0;//{return 0;}
	virtual hnDiseaseInfoBase* clone() = 0;
	int nDiseaseType;
	double dLength;
	double dWidth;
	double dArea;
	double dDepth;
	int nLevel;
	int nGeometry;
	double dBegMileage;
	double dEndMileage;
	vector<hn2dPt> vecPt;
	int nType;
	int nDiseaseSum;
};


//³ÇÕòµÀÂ·
class hnLiqingDiseaseInfo_CityRoad : public hnDiseaseInfoBase
{
public:
	enum LIQINGDISEASETYPE
	{ //³ÇÕòÐÂ²¡º¦

		WL_DISEASE_TYPE,//ÍøÁÑ
		JL_DISEASE_TYPE,//¹êÁÑ
		YB_DISEASE_TYPE,//Óµ°ü
		CZ_DISEASE_TYPE,     // ³µÕÞ
		CX_DISEASE_TYPE,     // ³ÁÏÝ
		FJ_DISEASE_TYPE,//·­½¬
		BL_DISEASE_TYPE,//°þÂä
		KC_DISEASE_TYPE,     // ¿Ó²Û
		KB_DISEASE_TYPE,//¿Ð±ß
		LKC_DISEASE_TYPE,//Â·¿ò²î
		JJ_DISEASE_TYPE,     // ßó½¬
		FY_DISEASE_TYPE,     // ·ºÓÍ
		XL_DISEASE_TYPE,//ÏßÁÑ
	};
	hnLiqingDiseaseInfo_CityRoad()
	{
		nDiseaseType = -1;
		dLength = 0.0;
		dWidth = 0.0;
		dArea = 0.0;
		dDepth = 0.0;
		nLevel = 0;
		dBegMileage = 0.0;
		dEndMileage = 0.0;
		nType = 0;
		nGeometry = 0;
		nDiseaseSum = 13;
	}

	// »ñÈ¡¿éÃû³Æ
	virtual string getBlockName(int i)
	{
		string str = "";
		switch ((LIQINGDISEASETYPE)i)
	  {
		case WL_DISEASE_TYPE:
		{
			str = "ÍøÁÑ";
			break;
		}

		case JL_DISEASE_TYPE:
		{
			str = "¹êÁÑ";
			break;
		}
		case YB_DISEASE_TYPE:
		{
			str = "Óµ°ü";
			break;
		}

		case CZ_DISEASE_TYPE:
		{
			str = "³µÕÞ";
			break;
		}
		case CX_DISEASE_TYPE:
		{
			str = "³ÁÏÝ";
			break;
		}


		case FJ_DISEASE_TYPE:
		{
			str = "·­½¬";
			break;
		}
		case BL_DISEASE_TYPE:
		{
			str = "°þÂä";
			break;
		}
		case KC_DISEASE_TYPE:
		{
			str = "¿Ó²Û";
			break;
		}
		case KB_DISEASE_TYPE:
		{
			str = "¿Ð±ß";
			break;
		}
		case LKC_DISEASE_TYPE:
		{
			str = "Â·¿ò²î";
			break;
		}
		case JJ_DISEASE_TYPE:
		{
			str = "ßó½¬";
			break;
		}
		case FY_DISEASE_TYPE:
		{
			str = "·ºÓÍ";
			break;

		}
		case XL_DISEASE_TYPE:
		{
			str = "ÏßÁÑ";
			break;
		}
		}
		return str;
	}
	virtual string getBlockName()
	{
		return getBlockName(nDiseaseType);
	}
	virtual int getDiseaseType() { return LIQING_ROAD_TYPE; }

	virtual hnDiseaseInfoBase* clone() {
		hnDiseaseInfoBase* newPtr = new hnLiqingDiseaseInfo_CityRoad;
		//ÓÉÓÚÄÚ²¿ÓÐvector,ÐèÒªÊÖ¶¯¿½±´
		newPtr->nDiseaseType = nDiseaseType;
		newPtr->dLength = dLength;
		newPtr->dWidth = dWidth;
		newPtr->dArea = dArea;
		newPtr->dDepth = dDepth;
		newPtr->nLevel = nLevel;
		newPtr->nGeometry = nGeometry;
		newPtr->dBegMileage = dBegMileage;
		newPtr->dEndMileage = dEndMileage;
		newPtr->nType = nType;
		newPtr->nDiseaseSum = nDiseaseSum;
		newPtr->vecPt.assign(vecPt.begin(), vecPt.end());
		return newPtr;
	}
};
//³ÇÕòµÀÂ·
class hnShuiNiDiseaseInfo_CityRoad : public hnDiseaseInfoBase
{
public:
	enum SHUINIISEASETYPE
	{
		//³ÇÕòÐÂ²¡º¦

		BJDL_DISEASE_TYPE,     // °å½Ç¶ÏÁÑ
		BJLF_SN_DISEASE_TYPE,   // ±ß½ÇÁÑ·ì
		JCLFHPSB_SN_DISEASE_TYPE,   // ½»²æÁÑ·ìºÍÆÆËé°å
		JFLSH_DISEASE_TYPE,     // ½Ó·ìÁÏËð»µ
		BJBL_DISEASE_TYPE,     // ±ß½Ç°þÂä
		KD_DISEASE_TYPE,     // ¿Ó¶´
		BMLW_SN_DISEASE_TYPE,   // ±íÃæÁÑÎÆ
		CZBL_SN_DISEASE_TYPE,   // ²ã×´°þÂä
		CT_DISEASE_TYPE,     // ´íÌ¨
		GZ_SN_DISEASE_TYPE,   // ¹°ÕÍ
		JJ_SN_DISEASE_TYPE,   // ßó½¬
		LKC_SN_DISEASE_TYPE,   // Â·¿ò²î
		CX_SN_DISEASE_TYPE,   // ³ÁÏÝ	
		XL_SN_DISEASE_TYPE,   // ÏßÁÑ
	};
	hnShuiNiDiseaseInfo_CityRoad()
	{
		nDiseaseType = -1;
		dLength = 0.0;
		dWidth = 0.0;
		dArea = 0.0;
		dDepth = 0.0;
		nLevel = 0;
		dBegMileage = 0.0;
		dEndMileage = 0.0;
		nType = 0;
		nGeometry = 0;
		nDiseaseSum = 14;
	}
	// »ñÈ¡¿éÃû³Æ
	virtual string getBlockName(int i)
	{
		string str = "";
		switch ((SHUINIISEASETYPE)i)
		{

		case SHUINIISEASETYPE::BJDL_DISEASE_TYPE:
		{
			str = "°å½Ç¶ÏÁÑ";
			break;
		}
		case SHUINIISEASETYPE::BJLF_SN_DISEASE_TYPE:
		{
			str = "±ß½ÇÁÑ·ì";
			break;
		}

		case SHUINIISEASETYPE::JCLFHPSB_SN_DISEASE_TYPE:
		{
			str = "½»²æÁÑ·ìºÍÆÆËé°å";
			break;
		}
		case SHUINIISEASETYPE::JFLSH_DISEASE_TYPE:
		{
			str = "½Ó·ìÁÏËð»µ";
			break;
		}
		case SHUINIISEASETYPE::BJBL_DISEASE_TYPE:
		{
			str = "±ß½Ç°þÂä";
			break;
		}

		case SHUINIISEASETYPE::KD_DISEASE_TYPE:
		{
			str = "¿Ó¶´";
			break;
		}
		case SHUINIISEASETYPE::BMLW_SN_DISEASE_TYPE:
		{
			str = "±íÃæÁÑÎÆ";
			break;
		}
		case SHUINIISEASETYPE::CZBL_SN_DISEASE_TYPE:
		{
			str = "²ã×´°þÂä";
			break;
		}
		case SHUINIISEASETYPE::CT_DISEASE_TYPE:
		{
			str = "´íÌ¨";
			break;
		}

		case SHUINIISEASETYPE::GZ_SN_DISEASE_TYPE:
		{
			str = "¹°ÕÍ";
			break;
		}
		case SHUINIISEASETYPE::JJ_SN_DISEASE_TYPE:
		{
			str = "ßó½¬";
			break;
		}
		case SHUINIISEASETYPE::LKC_SN_DISEASE_TYPE:
		{
			str = "Â·¿ò²î";
			break;
		}
		case SHUINIISEASETYPE::CX_SN_DISEASE_TYPE:
		{
			str = "³ÁÏÝ";
			break;
		}
		case SHUINIISEASETYPE::XL_SN_DISEASE_TYPE:
		{
			str = "ÏßÁÑ";
			break;
		}
		}

		return str;
	}
	virtual string getBlockName()
	{
		return getBlockName(nDiseaseType);
	}
	virtual int getDiseaseType() { return SHUINI_ROAD_TYPE; }
	virtual hnDiseaseInfoBase* clone() {
		hnDiseaseInfoBase* newPtr = new hnShuiNiDiseaseInfo_CityRoad;
		//ÓÉÓÚÄÚ²¿ÓÐvector,ÐèÒªÊÖ¶¯¿½±´
		newPtr->nDiseaseType = nDiseaseType;
		newPtr->dLength = dLength;
		newPtr->dWidth = dWidth;
		newPtr->dArea = dArea;
		newPtr->dDepth = dDepth;
		newPtr->nLevel = nLevel;
		newPtr->nGeometry = nGeometry;
		newPtr->dBegMileage = dBegMileage;
		newPtr->dEndMileage = dEndMileage;
		newPtr->nType = nType;
		newPtr->nDiseaseSum = nDiseaseSum;
		newPtr->vecPt.assign(vecPt.begin(), vecPt.end());
		return newPtr;
	}
};

//µÈ¼¶¹«Â·2018
class hnLiqingDiseaseInfo : public hnDiseaseInfoBase
{
public:
	enum LIQINGDISEASETYPE
	{
		JL_DISEASE_TYPE = 0, // ¹êÁÑ
		KN_DISEASE_TYPE,     // ¿éÁÑ
		KC_DISEASE_TYPE,     // ¿Ó²Û
		SS_DISEASE_TYPE,     // ËÉÉ¢
		CX_DISEASE_TYPE,     // ³ÁÏÝ
		CZ_DISEASE_TYPE,     // ³µÕÞ
		FY_DISEASE_TYPE,     // ·ºÓÍ
		JJ_DISEASE_TYPE,     // ßó½¬
		BLYB_DISEASE_TYPE,   // ²¨ÀËÓµ°ü
		LF_HX_DISEASE_TYPE,     // ºáÏòÁÑ·ì£¨Çá¶È£©
		LF_HX_YZ_DISEASE_TYPE,     // ºáÏòÁÑ·ì£¨ÖØ¶È£©
		LF_ZX_DISEASE_TYPE,  // ×ÝÏòÁÑ·ì£¨Çá¶È£©
		LF_ZX_YZ_DISEASE_TYPE,  // ×ÝÏòÁÑ·ì£¨ÑÏÖØ£©
		TZXB_DISEASE_TYPE,    // Ìõ×´ÐÞ²¹
		KZXB_DISEASE_TYPE,   // ¿é×´ÐÞ²¹
		XL_DISEASE_TYPE,   // ÏßÁÑ

	};
	hnLiqingDiseaseInfo()
	{
		nDiseaseType = -1;
		dLength = 0.0;
		dWidth = 0.0;
		dArea = 0.0;
		dDepth = 0.0;
		nLevel = 0;
		dBegMileage = 0.0;
		dEndMileage = 0.0;
		nType = 0;
		nGeometry = 0;
		nDiseaseSum = 13;
	}

	// »ñÈ¡¿éÃû³Æ
	virtual string getBlockName(int i)
	{
		string str = "";
		switch ((LIQINGDISEASETYPE)i)
		{
		case JL_DISEASE_TYPE:
		{
			str = "¹êÁÑ";
			break;
		}
		case XL_DISEASE_TYPE:
		{
			str = "ÏßÁÑ"; break;
		}
		case KN_DISEASE_TYPE:
		{
			str = "¿éÁÑ";
			break;
		}
		case KC_DISEASE_TYPE:
		{
			str = "¿Ó²Û";
			break;
		}
		case SS_DISEASE_TYPE:
		{
			str = "ËÉÉ¢";
			break;
		}
		case CX_DISEASE_TYPE:
		{
			str = "³ÁÏÝ";
			break;
		}
		case CZ_DISEASE_TYPE:
		{
			str = "³µÕÞ";
			break;
		}
		case LF_HX_DISEASE_TYPE:
		case LF_ZX_DISEASE_TYPE:
		{
			str = "ÁÑ·ì(Çá¶È)";
			break;
		}
		case LF_HX_YZ_DISEASE_TYPE:
		case LF_ZX_YZ_DISEASE_TYPE:
		{
			str = "ÁÑ·ì(ÖØ¶È)";
			break;
		}
		case FY_DISEASE_TYPE:
		{
			str = "·ºÓÍ";
			break;
		}
		case JJ_DISEASE_TYPE:
		{
			str = "ßó½¬";
			break;
		}
		case BLYB_DISEASE_TYPE:
		{
			str = "²¨ÀËÓµ°ü";
			break;
		}
		case KZXB_DISEASE_TYPE:
		{
			str = "¿é×´ÐÞ²¹";
			break;
		}
		case TZXB_DISEASE_TYPE:
		{
			//str = "ÁÑ·ìÐÞ²¹"; //cwb
			str = "Ìõ×´ÐÞ²¹";
			break;
		}

		}

		return str;
	}
	virtual string getBlockName()
	{
		return getBlockName(nDiseaseType);
	}
	virtual int getDiseaseType() { return LIQING_ROAD_TYPE; }

	virtual hnDiseaseInfoBase* clone() {
		hnDiseaseInfoBase* newPtr = new hnLiqingDiseaseInfo;
		//ÓÉÓÚÄÚ²¿ÓÐvector,ÐèÒªÊÖ¶¯¿½±´
		newPtr->nDiseaseType = nDiseaseType;
		newPtr->dLength = dLength;
		newPtr->dWidth = dWidth;
		newPtr->dArea = dArea;
		newPtr->dDepth = dDepth;
		newPtr->nLevel = nLevel;
		newPtr->nGeometry = nGeometry;
		newPtr->dBegMileage = dBegMileage;
		newPtr->dEndMileage = dEndMileage;
		newPtr->nType = nType;
		newPtr->nDiseaseSum = nDiseaseSum;
		newPtr->vecPt.assign(vecPt.begin(), vecPt.end());
		return newPtr;
	}
};
//µÈ¼¶¹«Â·2018
class hnShuiNiDiseaseInfo : public hnDiseaseInfoBase
{
public:
	enum SHUINIISEASETYPE
	{
		PSB_DISEASE_TYPE = 0, // ÆÆËé°å
		BJDL_DISEASE_TYPE,     // °å½Ç¶ÏÁÑ
		CT_DISEASE_TYPE,     // ´íÌ¨
		GQ_DISEASE_TYPE,     // ¹°Æð
		BJBL_DISEASE_TYPE,     // ±ß½Ç°þÂä
		JFLSH_DISEASE_TYPE,     // ½Ó·ìÁÏËð»µ
		KD_DISEASE_TYPE,     // ¿Ó¶´
		JN_DISEASE_TYPE,     // ßóÄà
		LG_DISEASE_TYPE,   // Â¶¹Ç
		CZ_SN_DISEASE_TYPE,    // ³µÕÞ
		LF_HX_DISEASE_TYPE,     // ºáÏòÁÑ·ì£¨Çá¶È£©
		LF_HX_YZ_DISEASE_TYPE,     // ºáÏòÁÑ·ì£¨ÖØ¶È£©
		LF_ZX_DISEASE_TYPE,  // ×ÝÏòÁÑ·ì£¨Çá¶È£©
		LF_ZX_YZ_DISEASE_TYPE,  // ×ÝÏòÁÑ·ì£¨ÑÏÖØ£©
		TZXB_SN_DISEASE_TYPE,    // Ìõ×´ÐÞ²¹
		KZXB_SN_DISEASE_TYPE,   // ¿é×´ÐÞ²¹
		XL_SN_DISEASE_TYPE,   // ÏßÁÑ
		LF_SN_DISEASE_TYPE,   // ÁÑ·ì
		LF_SN_YZ_DISEASE_TYPE,   // ÁÑ·ì ÑÏÖØ

	};
	hnShuiNiDiseaseInfo()
	{
		nDiseaseType = -1;
		dLength = 0.0;
		dWidth = 0.0;
		dArea = 0.0;
		dDepth = 0.0;
		nLevel = 0;
		dBegMileage = 0.0;
		dEndMileage = 0.0;
		nType = 0;
		nGeometry = 0;
		nDiseaseSum = 14;
	}
	// »ñÈ¡¿éÃû³Æ
	virtual string getBlockName(int i)
	{
		string str = "";
		switch ((SHUINIISEASETYPE)i)
		{
		case  SHUINIISEASETYPE::XL_SN_DISEASE_TYPE:
		{
			str = "ÏßÁÑ";
			break;
		}
		case SHUINIISEASETYPE::PSB_DISEASE_TYPE:
		{

			str = "ÆÆËé°å";
			break;
		}
		case SHUINIISEASETYPE::BJDL_DISEASE_TYPE:
		{
			str = "°å½Ç¶ÏÁÑ";
			break;
		}
		case SHUINIISEASETYPE::CT_DISEASE_TYPE:
		{
			str = "´íÌ¨";
			break;
		}
		case SHUINIISEASETYPE::GQ_DISEASE_TYPE:
		{
			str = "¹°Æð";
			break;
		}
		case SHUINIISEASETYPE::BJBL_DISEASE_TYPE:
		{
			str = "±ß½Ç°þÂä";
			break;
		}
		case SHUINIISEASETYPE::JFLSH_DISEASE_TYPE:
		{
			str = "½Ó·ìÁÏËð»µ";
			break;
		}
		case  SHUINIISEASETYPE::LF_SN_DISEASE_TYPE:
		case SHUINIISEASETYPE::LF_HX_DISEASE_TYPE:
		case SHUINIISEASETYPE::LF_ZX_DISEASE_TYPE:
		{
			str = "ÁÑ·ì(Çá¶È)";
			break;
		}
		case  SHUINIISEASETYPE::LF_SN_YZ_DISEASE_TYPE:
		case SHUINIISEASETYPE::LF_HX_YZ_DISEASE_TYPE:
		case SHUINIISEASETYPE::LF_ZX_YZ_DISEASE_TYPE:
		{
			str = "ÁÑ·ì(ÖØ¶È)";
			break;
		}
		case SHUINIISEASETYPE::KD_DISEASE_TYPE:
		{
			str = "¿Ó¶´";
			break;
		}
		case SHUINIISEASETYPE::JN_DISEASE_TYPE:
		{
			str = "ßóÄà";
			break;
		}
		case SHUINIISEASETYPE::LG_DISEASE_TYPE:
		{
			str = "Â¶¹Ç";
			break;
		}
		case SHUINIISEASETYPE::KZXB_SN_DISEASE_TYPE:
		{
			str = "¿é×´ÐÞ²¹";
			break;
		}
		case SHUINIISEASETYPE::TZXB_SN_DISEASE_TYPE:
		{
			//str = "ÁÑ·ìÐÞ²¹";
			str = "Ìõ×´ÐÞ²¹";
			break;
		}
		case SHUINIISEASETYPE::CZ_SN_DISEASE_TYPE:
		{
			str = "³µÕÞ";
			break;
		}

		}

		return str;
	}
	virtual string getBlockName()
	{
		return getBlockName(nDiseaseType);
	}
	virtual int getDiseaseType() { return SHUINI_ROAD_TYPE; }
	virtual hnDiseaseInfoBase* clone() {
		hnDiseaseInfoBase* newPtr = new hnShuiNiDiseaseInfo;
		//ÓÉÓÚÄÚ²¿ÓÐvector,ÐèÒªÊÖ¶¯¿½±´
		newPtr->nDiseaseType = nDiseaseType;
		newPtr->dLength = dLength;
		newPtr->dWidth = dWidth;
		newPtr->dArea = dArea;
		newPtr->dDepth = dDepth;
		newPtr->nLevel = nLevel;
		newPtr->nGeometry = nGeometry;
		newPtr->dBegMileage = dBegMileage;
		newPtr->dEndMileage = dEndMileage;
		newPtr->nType = nType;
		newPtr->nDiseaseSum = nDiseaseSum;
		newPtr->vecPt.assign(vecPt.begin(), vecPt.end());
		return newPtr;
	}
};

// Çø¼ä²¡º¦ÐÅÏ¢
typedef struct _GRID_DISEASE_INFO_
{
	_GRID_DISEASE_INFO_()
	{
		strName = "";
		dBegMileage = 0.0;
		dEndMileage = 0.0;
		strBegMile = "";
		strEndMile = "";
		dRoadWidth = 0.0;
		nRoadTotalNum = 2;
	}
	~_GRID_DISEASE_INFO_()
	{
	}
	string strName;
	string strBegMile;
	string strEndMile;
	double dBegMileage;
	double dEndMileage;
	double dRoadWidth;
	int nRoadTotalNum;
	vector<hnDiseaseInfoBase*> vecDiseaseInfo;	//Çø¼äµÄ²¡º¦ÐÅÏ¢

}hnGridDiseaseInfo;
