#ifndef _HN_DISEASE_SETINFO_TABLE_H_
#define _HN_DISEASE_SETINFO_TABLE_H_
#include "hnDBTable.h"

#include "..\hnCommon\hnRoadStruct.h"
#include "hndatatable_global.h"

using namespace hnCommon;

//namespace hnDataTable
//{
	class HNDATATABLE_EXPORT hnDiseaseSetInfoTable : public hnDBTable
	{
	public:
		hnDiseaseSetInfoTable();
		virtual ~hnDiseaseSetInfoTable();

	public:
		// 读取病害设置信息
		bool readData(vector<hnDiseaseSetInfo>& vecData, char* strQuery = NULL);

		// 写入病害设置信息
		bool writeData(vector<hnDiseaseSetInfo>& vecData);

		// 写入病害设置信息
		bool writeData(hnDiseaseSetInfo& inData);

		// 获取最大id
		virtual int getMaxID();
	};
//}

#endif // _HN_RING_TABLE_H_
