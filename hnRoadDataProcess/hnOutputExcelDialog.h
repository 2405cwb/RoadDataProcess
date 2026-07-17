#pragma once
#include <QDialog>
#include "ui_hnOutputExcelDialog.h"
#include "../hnApplication/hnDataManager.h"
#include <vector>
#include <QCloseEvent>
#include "hnOutExcelMileManage.h" 
#include "xlsxdocument.h"
#include "../ReportConfig.h"
#include "hnOutExcelManage.h"
#include <QMap>
QXLSX_USE_NAMESPACE

using namespace hnApp;
class HnXRSettings;
class QTableWidget;
class hnOutputExcelDialog : public QDialog
{
	Q_OBJECT

public:
	hnOutputExcelDialog(QWidget *parent = Q_NULLPTR);
	static bool validateProjectCompatibility(const std::vector<hnPro::hnProject*>& projects, QString& errorMessage);

protected:
	void closeEvent(QCloseEvent * event) override
	{
		event->ignore();
		hide();
	};
public:
		double getStartMile() { return m_startMile; }

		double getEndMile() { return m_endMile; }

private:
	Ui::hnOutputExcelDialog ui;

	//初始化出表设置界面
	void setSettingFrom();
	void setupMultiProjectWidthTable(const std::vector<hnPro::hnProject*>& projects);
	bool projectRoadWidthsFromUi(QMap<hnPro::hnProject*, double>& widths, int& invalidRow) const;


	//初始化单表出表
	void setSingleProjectFrom();

	//初始化表格选择
	void setModelChooseForm();

	void addRadioButtonToModelGroup(int& index, const QString& text, QWidget* widget, QButtonGroup*btnGroup, QGridLayout*layout);

	//获得用户选中的所有excel报表 及其分段
	void getUserSelectExcelName(QGridLayout* grid, QMap<int,QVector<double>>&  selectMessage);

	private slots:
	void onTableChange(int index);

	//当用户选定模表后启动 设置路面报表选择
	void onModelClicked(QAbstractButton * radBtn);

	//用户点击确定按钮
	void onOkButton();

	//保存用户界面配置到文件
	void	saveConfig();

	//根据配置文件 设置界面
	void readSetting();

	//根据用户设置 设置参数
	void setSetting();

	void onCannelButton();

	//全选按钮
	void onSelectAllExcel(bool value);

	//恢复默认桩号按钮  单工程出表
	void onReturnMile();

	void onReturnMileOk();

	void connectMethods();

	bool changeSingleMileAndStandard(double sMile, double eMile, HnProjectEnums::StandardParmTypeEnum type);
   
private:
	 

	//当前出表遵照的工程规范
	HnProjectEnums::StandardParmTypeEnum m_nowStandard;

	PROJECT_TYPE  m_nowProjectType;

	//当前出表遵照的绘制模式  0 人工模式 1 自动化模式 2设计模式
	hnCommon::ROAD_WORK_TYPE m_nowDrawType;

	//用于检测 道路标准是否发生了变化  变化了需要出表刷新界面
	HnProjectEnums::StandardParmTypeEnum preStandard;
	PROJECT_TYPE  preNowProjectType;
	//单表出表起点桩号
	double m_dSMile;

	//单表出表终点桩号
	double m_dEMile;

	hnPro::hnProjectManager* manager;

	//单例  全局设置
	HnXRSettings* m_xrSetting; 

	//是否是单表模式
	bool  isSingleProject;
	QTableWidget* m_projectWidthTable = nullptr;
	std::vector<hnPro::hnProject*> m_reportProjects;

	//道路标准->模块名称->表名
	QMap < HnProjectEnums::StandardParmTypeEnum,QMap<QString,QStringList>> allExcelModel1;
 
	//读取json配置文件
	void readExcelConfigData();

	//用户选择模式 buttonGroup
	QButtonGroup *p_modelGroup;

	//报表选择gridLayout
	QGridLayout * p_excelLayout;
	 
	 
	//读取用户配置 设置进度 启动输出报表
	void startOutExcelManager(const QString& excelDir, const QString&selectModelTxt, hnPro::hnProject*curProject, 
		int & progressValue,
		QProgressDialog* process);

	void startOutExcelManager_Street(const QString& excelDir,
		const QString&selectModelTxt, std::vector<hnPro::hnProject*>& allProject, 
		const QMap<int, QVector<double>>&streetSelect,
		int & progressValue,
		QProgressDialog* process);

	  


private:
	//输出报表根文件夹名称
	QString m_strOutExcelPath;

	//当前出表工程
	hnPro::hnProject*   m_currentPorject;

	//当前出表工程道路规范
	HnProjectEnums::StandardParmTypeEnum m_standard;

	//当前出表起始桩号
	double m_startMile;

	//当前出表终点桩号
	double m_endMile;

	//当前出表文件夹
	//QString saveExcelDir;

	//小数位数
	//int m_decimalDigits;

	//当前项目的配置文件地址
	QString m_configPath;

	AppConfig m_ExcelSelectConfig;

	//成员变量保存界面控件和ReportItem的映射
	QMap<ReportItem*, QPair<QCheckBox*, QComboBox*>> m_ItemWidgetMap;

	//用户选中报表个数
	int m_UserSelectCount = 0 ;

	//出表进度条
	QProgressDialog* m_progressDialog;

	//进度条当前值
	int m_progressValue;
};
