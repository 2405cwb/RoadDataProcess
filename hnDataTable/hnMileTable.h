#pragma once
#include "hnDBTable.h"
#include "../hnQtCommon/hnMile.h"
#include "hndatatable_global.h"
class HNDATATABLE_EXPORT hnMileTable :
	public  hnDBTable
{
public:
	hnMileTable();
	~hnMileTable();
	// 读取里程桩信息
	bool readData(QVector<hnMile>& vecData, char* strQuery = NULL);

	// 写入里程桩信息
	bool writeData(QVector<hnMile>& vecData);

	// 写入里程桩信息
	bool writeData_Simple(QVector<hnMile>& vecData);


	// 写入里程桩信息
	bool writeData(hnMile& inData);

	bool clearData();
	// 获取最大id
	virtual int getMaxID();
};

