#include "stdafx.h"
#include "hnRoadConfig.h"
#include "stdafx.h"
#include <atlstr.h>
#include "../hdCommon/hdCommon.h"
namespace hnCommon {

	hnRoadConfig::hnRoadConfig()
	{
		readData();
	}


	hnRoadConfig::hnRoadConfig(const hnRoadConfig&)
	{

	}

	hnRoadConfig* hnRoadConfig::_hnRoadConfig = nullptr;

	hnRoadConfig::~hnRoadConfig()
	{
		if (!_hnRoadConfig)
		{
			delete _hnRoadConfig;
			_hnRoadConfig = nullptr;
		}
	}

	hnRoadConfig* hnRoadConfig::getInstance()
	{
		if (!_hnRoadConfig)
		{
			_hnRoadConfig = new hnRoadConfig();
			return _hnRoadConfig;
		}
	}

	void hnRoadConfig::readData()
	{
		//string s = getCurrentDir();
		//s += "hnRoadConfig.ini";
		//auto str = IniFiles::string2LPCTSTR(s);
		//hnCommon::IniFiles inisetting(str);
		//delete str;
		//str = NULL;
		//
		//this->ImageWidth = inisetting.ReadInteger(_T("ImageInfo"), _T("ImageWidth"), 0);
		//this->ImageHeight = inisetting.ReadInteger(_T("ImageInfo"), _T("ImageHeight"), 0);
		//this->RealWidth = inisetting.ReadDouble(_T("ImageInfo"), _T("RealWidth"), 0);
		//this->RealHeight = inisetting.ReadInteger(_T("ImageInfo"), _T("RealHeight"), 0);
		//this->DetectWidth = inisetting.ReadDouble(_T("ImageInfo"), _T("DetectWidth"), 0);

		//this->WidthScale = RealWidth * 1.0 / ImageWidth;
		//this->HeightScale = RealHeight * 1.0 / ImageHeight;

		//PartWidthNum = (int)(RealWidth * 10);
		//PartHeightNum = (int)(RealHeight * 10);
		////这里考虑如何取整
		//PartImgWidth =  (ImageWidth * 1.0 / PartWidthNum);
		//PartImgHeight = (ImageHeight * 1.0 / PartHeightNum);
		
	}

	void hnRoadConfig::writeData()
	{
		/*string s = getCurrentDir();
		s += "hnRoadConfig.ini";
		auto str = IniFiles::string2LPCTSTR(s);
		hnCommon::IniFiles inisetting(str);
		delete str;
		str = NULL;

		inisetting.WriteInteger(_T("ImageInfo"), _T("ImageWidth"),0);
		inisetting.WriteInteger(_T("ImageInfo"), _T("ImageHeight"), 0);
		inisetting.WriteDouble(_T("ImageInfo"), _T("RealWidth"), 0);
		inisetting.WriteInteger(_T("ImageInfo"), _T("RealHeight"), 0);
		inisetting.WriteDouble(_T("ImageInfo"), _T("DetectWidth"), 0);*/
	}
}