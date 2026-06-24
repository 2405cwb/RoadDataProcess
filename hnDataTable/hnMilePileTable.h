#ifndef _HN_MILEAGE_PILE_TABLE_H_
#define _HN_MILEAGE_PILE_TABLE_H_
#include "hnDBTable.h"
#include "..\hnCommon\hnRoadStruct.h"
#include "hndatatable_global.h"

using namespace hnCommon;

//namespace hnDataTable
//{
	class HNDATATABLE_EXPORT hnMilePileTable : public hnDBTable
	{
	public:
		hnMilePileTable();
		virtual ~hnMilePileTable();

	public:
		// 读取里程桩信息
		bool readData(vector<hnMilePile>& vecData, char* strQuery = NULL);

		// 写入里程桩信息
		bool writeData(vector<hnMilePile>& vecData);

		// 写入里程桩信息
		bool writeData(hnMilePile& inData);
		bool clearData();

		// 获取最大id
		virtual int getMaxID();
	};
//}

#endif // _HN_RING_TABLE_H_
