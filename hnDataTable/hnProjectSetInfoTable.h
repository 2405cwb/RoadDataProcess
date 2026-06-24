#ifndef _HN_PROJECT_SETINFO_TABLE_H_
#define _HN_PROJECT_SETINFO_TABLE_H_
#include "hnDBTable.h"
#include "..\hnCommon\hnRoadStruct.h"
#include "hndatatable_global.h"

using namespace hnCommon;

//namespace hnDataTable
//{
	class HNDATATABLE_EXPORT hnProjectSetInfoTable : public hnDBTable
	{
	public:
		hnProjectSetInfoTable();
		virtual ~hnProjectSetInfoTable();

	public:
		// 读取工程设置信息
		bool readData(vector<hnProjectSetInfo>& vecData, char* strQuery = NULL);

		// 读取工程设置信息
		bool readData(hnProjectSetInfo& retData, char* strQuery = NULL);

		// 写入工程设置信息
		bool writeData(vector<hnProjectSetInfo>& vecData);

		// 写入工程设置信息
		bool writeData(hnProjectSetInfo& inData);

		bool clearData();
		// 获取最大id
		virtual int getMaxID();
	};
//}

#endif // _HN_RING_TABLE_H_
