#ifndef _HN_ROADTYPE_SETINFO_TABLE_H_
#define _HN_ROADTYPE_SETINFO_TABLE_H_
#include "hnDBTable.h"
#include "..\hnCommon\hnRoadStruct.h"
#include "hndatatable_global.h"

using namespace hnCommon;

//namespace hnDataTable
//{
	class HNDATATABLE_EXPORT hnRoadTypeSetInfoTable : public hnDBTable
	{
	public:
		hnRoadTypeSetInfoTable();
		virtual ~hnRoadTypeSetInfoTable();

	public:
		// 读取病害设置信息
		bool readData(vector<hnRoadTypeSetInfo>& vecData, char* strQuery = NULL);

		// 写入病害设置信息
		bool writeData(vector<hnRoadTypeSetInfo>& vecData);

		// 写入病害设置信息
		bool writeData(hnRoadTypeSetInfo& inData);

		// 获取最大id
		virtual int getMaxID();
	};
//}

#endif // _HN_RING_TABLE_H_
