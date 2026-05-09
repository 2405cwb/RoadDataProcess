#pragma once
#include <QPushButton>
#include <QWidget>
#include <QtWidgets/QDialog>
#include "../hnApplication/hnDataManager.h"
#include "hnMile.h"
#include <QTableWidget>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
class hnMarkInfoDlg : public QDialog
{
	Q_OBJECT

public:
	explicit hnMarkInfoDlg(const QVector<hnCommon::hnMarkInfo>& marks, QWidget *parent = nullptr);
	~hnMarkInfoDlg();
protected:
   void	resizeEvent(QResizeEvent * event);
public:  

	//获取所有打标信息
	QVector<hnCommon::hnMarkInfo> getAllMrkVec() { return m_marks; }


	QVector<hnCommon::hnMarkInfo> getNewMarkVec() { return m_newMarks; }

	QVector<int>getDeleteMarkVec() { return m_deleteMarkIndexs; }
 private:

	QTableWidget * tableWidget;

	//确定按钮
	QPushButton* okPtn;

	//取消按钮
	QPushButton* cancelPtn;

	//全部清空
	QPushButton* cleanAllPtn;

	//新增按钮
	QPushButton* addPtn;

	//删除按钮
	QPushButton* deletePtn;

	//输出报表
	QPushButton * outExcelBtn;

	//传入的
	QVector<hnCommon::hnMarkInfo> m_marks;

	QVector<int> m_deleteMarkIndexs;

	//new
	QVector<hnCommon::hnMarkInfo> m_newMarks;
	
	QHBoxLayout * btnBox;

	//用户写入打标桩号
	QLineEdit * mileBox;

	//打标类型选择下拉框
	QComboBox * typeCombox;

	//添加对话框
	QDialog* m_addMarkFrom;

	//打标内容下拉框
	QComboBox * contextCombox;

	//打标内容lienbox
	QLineEdit * contextLineBox;

	//用户点击确定按钮后增加的 mark
	hnCommon::hnMarkInfo  currentMarkInfo;
private slots:
        void   outMarkFile();

		void cleanAllMarks();
};
