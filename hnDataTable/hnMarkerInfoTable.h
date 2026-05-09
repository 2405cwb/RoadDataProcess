#ifndef _HN_MARKER_INFO_TABLE_H_
#define _HN_MARKER_INFO_TABLE_H_
#include "hnDBTable.h"
#include "..\hnCommon\hnRoadStruct.h"
#include "hndatatable_global.h"

using namespace hnCommon;

//namespace hnDataTable
//{
	class HNDATATABLE_EXPORT hnMarkerInfoTable : public hnDBTable
	{
	public:
		hnMarkerInfoTable();
		virtual ~hnMarkerInfoTable();

	public:
		// 读取打标信息
		bool readData(vector<hnMarkInfo>& vecData, char* strQuery = NULL);

		// 写入打标信息
		bool writeData(vector<hnMarkInfo>& vecData, bool(*pProgress)(float fVal, const char* qstrName, bool bCancle) = NULL);

		bool updateData(vector<hnMarkInfo>& vecData,  bool(*pProgress)(float fVal, const char* qstrName, bool bCancle) = NULL);

		//删除打标信息
		bool deleteData(const vector<int>& vecIdxData);

		bool clearData();
		// 获取最大id
		virtual int getMaxID();
	};
//}

#endif // _HN_MARKER_INFO_TABLE_H_
