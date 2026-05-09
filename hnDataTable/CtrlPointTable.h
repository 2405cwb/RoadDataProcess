#pragma once

#include "hnDBTable.h"
#include "..\hnCommon\hnRoadStruct.h"
#include "hndatatable_global.h"
#include "hnMile.h"
#include "hnDBDefine.h"

using namespace hnCommon;

class HNDATATABLE_EXPORT CtrlPointTable : public hnDBTable
{
public:
	CtrlPointTable();

public:
	// 获取某类型病害最大id 自增1
	int getMaxID() override;

public:
	//默认读取所有，可在后面附加筛选语句
	bool readData(vector<hnKZDDataInfo>& vecData, char* strQuery = NULL);

public:
	bool writeData(vector<hnKZDDataInfo>& vecData);

	bool writeData(hnKZDDataInfo& inData);

public:
	//删除病害
	bool deleteData(vector<hnKZDDataInfo> &vecData);

	bool deleteData(const hnKZDDataInfo &data);

	//清空所有病害
	bool deleteAllData();

private:
	QString m_tableName;
};
