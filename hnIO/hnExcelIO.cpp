#include "hnExcelIO.h"
#include <QDir>
#include <QObject>
#include <QDebug>
#include "QColor"
hnExcelIO::hnExcelIO()
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	m_pExcel = nullptr;
	m_pWorkBooks = nullptr;
	m_pWorkBook = nullptr;
	m_pWorkSheets = nullptr;
	m_pWorkSheet = nullptr;
	usedRange = nullptr;
	// 连接excel 控件
	m_pExcel = new QAxObject("Excel.Application",0);
	// m_pExcel->setControl("Excel.Applicatio");
	//更改 Excel 标题栏：
	//m_pExcel->setProperty("Caption", "ttt");
	// 设置操作excel时不打开excel窗体
	m_pExcel->dynamicCall("SetVisible(bool Visible)", false);

	// 设置不显示任何警告信息
	m_pExcel->setProperty("DisplayAlert",false);
}


hnExcelIO::hnExcelIO(QString strExcelPath, QString strSavePath):hnExcelIO()
{
	if (strExcelPath != NULL)
	{
		this->_strExcelPath = strExcelPath;

	}
	this->_strSavePath= strSavePath;
}

hnExcelIO::~hnExcelIO()
{
}

bool hnExcelIO::OpenExcel()
{
	QFile qfile(_strExcelPath);
	if (qfile.exists())
	{
		// 获取当前工作簿  
		m_pWorkBooks = m_pExcel->querySubObject("WorkBooks");
		// 打开指定工作簿
		m_pWorkBook = m_pWorkBooks->querySubObject("Open(const QString&)", _strExcelPath);
		if (m_pWorkBook)
		{
			qDebug() << "Open Excel Success!";

		}

		// 获取sheets
		m_pWorkSheets = m_pWorkBook->querySubObject("Sheets");
		// 获取某个sheet
		m_pWorkSheet = m_pWorkSheets->querySubObject("Item(int)", 1);
		return true;
	}
	else
	{
		qDebug() << "文件路径不存在！" << endl;
		return false;
	}
}

void hnExcelIO::AddNewExcel()
{
	// 获取当前工作簿
	m_pWorkBooks = m_pExcel->querySubObject("WorkBooks");
	m_pWorkBooks->dynamicCall("Add");
	//获取活动的工作簿
	m_pWorkBook = m_pExcel->querySubObject("ActiveWorkBook");
	m_pWorkSheets = m_pWorkBook->querySubObject("Sheets");
	m_pWorkSheet = m_pWorkSheets->querySubObject("Item(int)", 1);
}


void hnExcelIO::SaveAndClose()
{
	QString dirName = _strSavePath.mid(0, _strSavePath.lastIndexOf('\\'));
	DelDir(dirName);
	QDir dir(dirName);
	if (!dir.exists())
	{
		dir.mkdir(dirName);
	}
	// 保存文件,一定要将路径中的'/'转为'\\',前者只能被Qt识别
	m_pWorkBook->dynamicCall("SaveAs(const QString&)", QDir::toNativeSeparators(_strSavePath));
	// 关闭文件
	m_pWorkBook->dynamicCall("Close()");
	// 关闭excel
	m_pExcel->dynamicCall("Quit()");
	QString str = QString::fromLocal8Bit("完成输出");
	qDebug() << str;
	delete m_pExcel;
	m_pExcel = nullptr;
}

int hnExcelIO::GetRowsCount()
{
	int iRows = 0;
	QAxObject* pRows = m_pWorkSheet->querySubObject("Rows");
	iRows = pRows->property("Count").toInt();
	return  iRows;
}

int hnExcelIO::GetColumnsCount()
{
	int intCount = 0;
	intCount = m_pWorkBooks->property("Count").toInt();
	return  intCount;

}

int hnExcelIO::GetTablesCount()
{
	int icolums = 0;
	QAxObject* pRows = m_pWorkSheet->querySubObject("Columns");
	icolums = pRows->property("Count").toInt();
	return  icolums;
}

QString hnExcelIO::GetCell(int row, int column)
{
	QAxObject* pCell = m_pWorkSheet->querySubObject("Range(int, int)", row, column);
	return  pCell->property("Value").toString();
}

QString hnExcelIO::GetCell(QString number)
{
	QAxObject* pCell = m_pWorkSheet->querySubObject("Range(QString)", number);
	return  pCell->property("Value").toString();
}

void hnExcelIO::SetCell(int row, int column, QString value)
{
	QAxObject* pCell = new QAxObject;
	pCell = m_pWorkSheet->querySubObject("Range(int, int)", row, column);
	pCell->setProperty("Value", value);
}

void hnExcelIO::SetCell(QString number, QString value)
{
	QAxObject* pCell = m_pWorkSheet->querySubObject("Range(QString)", number);
	pCell->setProperty("Value", value);
}

void hnExcelIO::SetCellColor(int row, int column, QColor color)
{
	QAxObject* pCell = m_pWorkSheet->querySubObject("Range(int, int)", row, column);
	QAxObject* pInterior = pCell->querySubObject("Interior");
	pInterior->setProperty("Color", color);
}

void hnExcelIO::SetCellColor(QString number, QColor color)
{
	QAxObject* pCell = m_pWorkSheet->querySubObject("Range(QString)", number);
	QAxObject* pInterior = pCell->querySubObject("Interior");
	pInterior->setProperty("Color", color);
}

void hnExcelIO::deleteRows(UINT RowNum)
{
	QAxObject* cell = m_pWorkSheet->querySubObject("Rows(int)", RowNum);//获取选定的行
	if (cell)
	{
		cell->dynamicCall("Delete()"); //修改所选行
	}
}

	
QString hnExcelIO::getSheetName(const int num)
{
	QAxObject* work = m_pWorkSheets->querySubObject("Item(int)",num);

	QString str = work->property("Name").toString();
	return str;
}

void hnExcelIO::removeSheet(const int num)
{
	m_pWorkSheet = m_pWorkSheets->querySubObject("Item(int)", num);
	if (m_pWorkSheet)
	{
		m_pWorkSheet->dynamicCall("Delete()");
		m_pWorkSheet = m_pWorkSheets->querySubObject("Item(int)", 1);
	}
}

void hnExcelIO::appendSheet(const QString& sheetName)
{
	int intCount = m_pWorkSheets->property("Count").toInt();
	QAxObject* pLastSheet = m_pWorkSheets->querySubObject("Item(int)", intCount);
	m_pWorkSheets->querySubObject("Add(QVariant)", pLastSheet->asVariant());
	m_pWorkSheet = m_pWorkSheets->querySubObject("Item(int)", intCount);
	pLastSheet->dynamicCall("Move(QVariant)", m_pWorkSheet->asVariant());
	m_pWorkSheet->setProperty("Name", sheetName);
}

int hnExcelIO::GetUserStartRowIndex()
{
	return userStartRowIndex;

}

int hnExcelIO::GetUserStartColumnIndex()
{
	return userStartColumnIndex;

}

int hnExcelIO::getUserAllRowsCount()
{
	return userAllRowsCount;
}

int hnExcelIO::getUserAllColumnsCount()
{
	return userAllColumnsCount;
}

QVariant hnExcelIO::readAll()
{
	QVariant var;/*
				 QList<QList<QVariant>> varList;*/
	if (this->m_pWorkSheet != NULL && !this->m_pWorkSheet->isNull())
	{
		//关键句，避免了重复打开表单造成的性能损失，读取速度很快
		usedRange = this->m_pWorkSheet->querySubObject("UsedRange");
		if (NULL == usedRange || usedRange->isNull())
		{
			return var;
		}
		QAxObject* pRows = usedRange->querySubObject("Rows");
		QAxObject* pColumns = usedRange->querySubObject("Columns");
		var = usedRange->dynamicCall("Value");
		/*	varList = My_Arithmetic::castVariaQnQt2ListListVariant(var);*/
		userStartRowIndex = usedRange->property("Row").toInt();          //获得开始行
		userStartColumnIndex = usedRange->property("Column").toInt();     //获得开始列
		userAllRowsCount = pRows->property("Count").toInt();     //
		userAllColumnsCount = pColumns->property("Count").toInt();
		delete usedRange;
	}
	return var;
}

QVariant hnExcelIO::readAll(QString para1, QString para2)
{
	QVariantList params;
	params << para1 << para2;  //A1至A5的数据
	QAxObject* cell = this->m_pWorkSheet->querySubObject("Range(QVariant,QVariant)", params);
	QVariant excel_data = cell->dynamicCall("Value2()");
	return excel_data;
}

bool hnExcelIO::DelDir(const QString path)
{
	//QDir dir(_strSavePath.mid(0, _strSavePath.lastIndexOf('\\')));
	if (path.isEmpty())
	{
		return false;
	}
	QDir dir(path);
	if (!dir.exists())
	{
		return true;
	}
	dir.setFilter(QDir::AllEntries| QDir::NoDotAndDotDot);
	
		auto files = dir.entryInfoList();
		for each (auto item in files)
		{
			if (item.isFile())
			{
				item.dir().remove(item.fileName());

			}
			else
			{
				DelDir(item.absoluteFilePath());
			}
		}

		return true;
	

}

