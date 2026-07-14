#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_hnRoadDataProcess.h"
#include <QProgressDialog>
#include <QProgressBar>
#include <QSharedPointer>
#include <QLabel>
#include "..\hnQtRibbonUI\hnRibbonMainWindow.h"
#include "..\hnDataTable\hnDataTable.h"
#include "..\hnDataTable\hnDBSqliteRoadInfo.h"
#include "..\hnProject\hnProject.h"
#include "../hnApplication/hnDataManager.h"
#include <QVector>
#include "../hnApplication/hn2dPixWidget.h"
#include "../hnApplication/hn2dPixScrollWidget.h"
#include "../hnApplication/hn3dPixScrollWidget.h"
#include "../hnApplication/hn3dPixWidget.h"
#include "hnDiseaseListWidget.h"
#include "hnStreetWidget.h"
#include "../hnApplication/hnStreetCameraView.h"
#include "hnImportAidcDiseases.h"
#include "hnOriginalScalePixShowWidget.h"
#include "adjustImageWidget.h"
#include "hnRegionJumpDlg.h"
#include "hnProjectConfig.h"
#include "hnDxfCaculateDialog.h"
#include "hnMergeLittleFrameThresholdDlg.h"
#include "projectView.h"
#include "IrmActualTimeShow.h"
#include "CustomBaiduMapView.h"
using namespace hnApp;

// 前置声明
class QTreeView;
class QTreeWidget;
class QStandardItemModel;
class QStandardItem;
class QTreeWidgetItem;
class QDockWidget;
class QAction;
class hnGL3dView;
class hnRibbonCategory;
class hnRibbonContextCategory;
class hnGeoDimShowWidget;
class hnITSKJShowWidget;
class hnITSGeoDimPlotShowWidget;
class hnKuoxingShowWidget;
class QLineEdit;
class hnCreate2DPic;
class QComboBox;
class QLineEdit;
class QLabel;
class QCheckBox;
class hnCreateImageThread;
class hnWidget3DView;
class hnView;
class HnXRSettings;
class statusBarWidget;
class hnOutputExcelDialog;
class hnOutExcelMileManage;
class hnAboutInfoWidgets;
class hnCenterToast;
namespace hn
{
	class CDockManager;
	class CDockWidget;
}
namespace hnPro
{
	class hnProjectManager;
	class hnProject;

}



class hnRoadDataProcess : public hnRibbonMainWindow
{
	Q_OBJECT

public:
	hnRoadDataProcess(QWidget *parent = Q_NULLPTR);
	~hnRoadDataProcess();
	static bool progressCallback(float fval, const char* qstrName, bool bCancle);
public:
	//单例  全局设置
	HnXRSettings* m_xrSetting;

public slots:
	// 当前活动视图;
	void widgetviewActive(WId hwnd);

protected:
	//程序退出
	void closeEvent(QCloseEvent * e);

	void showEvent(QShowEvent *event);

private:
	// 保存布局；
	void saveLayout();

	// 读取布局;
	void readLayout();

	// 创建视图
	void createView();

	//初始化对话框	本函数是为了初始化保留状态的对话框，对于临时对话框，只需要用的时候创建对象即可
	void initDlg();

	// 创建工具栏
	void createAction();

	//初始化快捷键
	void initShortCuts();

	// 创建连接
	void createConnect();

	// 树状视图连接
	void createTreeConnect();

	// 创建工程管理模块工具栏
	void createProCategory(hnRibbonCategory* page);

	// 创建数据处理模块工具栏
	void createDataProcessCategory(hnRibbonCategory* page);

	// 创建数据库管理工具栏
	void createDataBaseMgrCategory(hnRibbonCategory* page);

	// 创建数据输出模块工具栏
	void createOutputCategory(hnRibbonCategory* page);

	// 创建点云处理模块工具栏
	void createCloudCategory(hnRibbonCategory* page);

	// 创建视图管理工具栏
	void createViewsCategory(hnRibbonCategory* page);
	 
	// 系统
	void createSystemCategory(hnRibbonCategory* page);

	//加载根目录数据库信息
	bool loadConfigData();

 

	//初始化工程相关信息 
	bool initProject();


private slots:
	// 打开工程
	void openProjectSlot();

	//最近工程
	void openLastProjectSlot();

	//打开当前工程文件夹
	void slot_openCurrentProjectDir();

	//导出简易工程
	void slot_outSimpleProject();

	//gps桩号匹配
	void slot_gpsMatching();

	// 检查数据
	void checkProSlot();
	 
	// 影像生成
	void createImageSlot();

	// 里程校准
	void slot_mileCorrectSlot();

	// 采集打标
	void slot_markInfoSlot();

	// 清除工程
	void slot_clearProjectSlot();

	//病害里程跳转
	void slot_regionJump();

	//打开设置界面
	void slot_openConfigWidget();

	//使用说明
	void slot_oepnCourseDocument();

	//关于信息
	void slot_aboutInfosWidget();

	//输出报表 
	void slot_outputExcel();


	//输出结果数据
	void slot_outAllResultDatas();

	//输出多工程合并报表
	void slot_outMergeExcel();

	//双击树状工程菜单
	void slot_dClickTreeItem(QTreeWidgetItem *item, int column);

	void slot_selectNodeChange();

	//右键树菜单
	void slot_showContextMenu(const QPoint pos);

	//视图控制
	void slot_toStandard2View();

	void slot_toStandard23View();

	void slot_toStandard3View();
	//计算irm
	void slot_calculateIrm();

	//清空irm
	void slot_clearIrm();


	void slot_compute();

	//槽函数 切换为添加病害模式
	void slot_changeToAddDiseaseMode();

	//槽函数 切换为删除病害模式
	void slot_changeToDeleteDiseaseMode();

	//槽函数  切换为编辑病害模式
	void slot_changeToEditDiseaseMode();

	//槽函数 切换为移动病害模式
	void slot_changeToMoveDiseaseMode();

	//槽函数  切换为合并病害模式
	void slot_changeToMergeDiseaseMode();

	//槽函数 切换为二三维里程矫正模式
	void slot_changeTo23dMileCorrectMode();

	// 槽函数 切换为添加控制点模式
	void slot_changeAddCtrlPointMode();

	// 切换为添加面状病害模式
	void slot_addFacetsDiseaseMode();

	// 切换为添加线状病害模式
	void slot_addLineDiseaseMode();

	//导入二维软件绘制识别病害
	void slot_import2dDiseases();

	//导出为二维软件病害
	void slot_output2dDiseases();

	//导入自动识别病害
	void slot_importAidcDiseases();

	//槽函数 切换灰度图浏览模式
	void slot_changeGray3dMode();

	//槽函数 切换深度图浏览模式
	void slot_changeRgb3dMode();

	//槽函数 裁切功能
	void slot_cutImage();

	bool UTCT2GPST(const DATE_TIME_INFO& stTime, int& nGpsWeek,
		double& dGpsSeconds, double dGPSSubUTC = 0);

	void slot_backupsDatabase();

	//槽函数 清空所有病害
	void slot_clearAllDiseases();

	//更新配置数据库
	void slot_updateDatabase();

	//更新病害数据库
	void slot_updateDiseaseDatabase();

	//槽函数  景观帧序号变化时
	void slot_streetWidgetFrameIdxChanged(int streetFrameIdx);


	//是否进行深度计算
	void slot_setDepthCaculate(bool isCaculate);

	//视图镜像处理
	void slot_widgetMirrored(bool isH2dMirrored, bool isV2dMirrored, bool isH3dMirrored, bool isV3dMirrored);

	//导出DXF
	void slot_exportDXf();

	//导出病害DXF
	void slot_exportDiseaseDXf();


	//导出高精度病害DXF
	void slot_exportHighAccuracyDiseaseDXf();

	//导出国检转换中间数据
	void slot_exportGjDatas();

	// 导入控制点
	void slot_importCtrlPoints();

	// 导出控制点
	void slot_exportCtrlPoints();
	
	//拼接病害
	void slot_mergeAutoDisease();
	 
	//界面跳转到指定桩号
	void slot_jumpToMile(double mile);
	void slot_jumpToMark(int markId, double trueMile, double encoderMile);
private:
	//更新所有视图
	void updateAllWidget();

	void updatePixWidget();

	// 判断当前工程和视图是否已经具备连续里程联动条件。
	bool canUseContinuousViewSync() const;

	// 2D 联动景观时只认连续编码器里程，不再拿半张滚动条 value 推。
	void syncStreetViewBy2dEncoderMile(double encoderMile);

	enum class ContinuousViewSyncSource
	{
		Road2D,
		Road3D,
		Street
	};

	// 统一处理 2D、3D、景观图之间的连续里程同步，避免滚轮/键盘/滚动条各自重复计算。
	void syncContinuousViews(ContinuousViewSyncSource source, double sourceEncoderMile);

	//更新树状视图
	void updateTreeWidget();

	//所有视图重新加载图片
	void allWidgetLoadPictures();

	// 清空所有视图图片
	void clearAllWidgetPixs();
	void clearCurrentProjectUiState();

	//设置布局
	void setLayout(PROJECT_TYPE projectType);

	//检查工程人工模式自动化模式冲突，做限制处理，不允许人工模式自动化模式同时存在 冲突返回false 不冲突或者解决完冲突 返回true
	bool checkProjectFrameTypeConflict(const QString& projectName);

	// 处理冲突
	bool handleConflict(QString standard, int drawType);

	void MappingGPS2Mile(hnPro::hnProject* project, QString baseProjectPath);
	
	bool GetRoadGPSTime2Dmi(hnPro::hnProject* project, QString type);

	bool GetGPSMileMapping(hnPro::hnProject* project, QString gpsFilePath);


	void writeStreetDiseaseMsgToExcel(int disType, hnPro::hnProject* project, Document& xlsx);
signals:

	void signal_loadBaiduMap();

	//通知工程界面更新
	void signal_updateProject(hnCommon::hnProjectSetInfo, QVector<hnCommon::hnMarkInfo>, QVector<hnCommon::hnMilePile>);

private:
	// 打开工程
	QAction* m_openProjectAct;

	//最近工程
	QAction * m_lastProjectAct;

	// 打开当前工程文件夹
	QAction* m_openCurrentProjectDirAction;

	//导出简易工程
	QAction * m_outSimpleProjectAction;

	QAction * m_gpsMatchingAct;

	// 检查数据
	QAction* m_checkProAct;

	//修改工程有效桩号(绘制病害，出表桩号)
	QAction* m_changeProjectOutMileAct;

	// 影像生成
	QAction* m_createImageAct;

	//裁切图片
	QAction* m_cutImageAct;

	// 里程校准
	//QAction* m_mileCorrectAct;

	// 采集打标
	QAction* m_markInfoAct;

	// 清除工程
	QAction* m_clearProjectAct;

	//病害拼接
	QAction * m_autoDiseaseMerge;

	// 里程跳转
	QAction* m_regionJumpAct;

	// 标准二维视图
	QAction* m_oShapeViewportAct;

	// 标准三维视图
	QAction* m_o3ShapeViewportAct;

	// 标准二三维视图
	QAction* m_oDViewportAct;

	//平整度 车辙等计算
	QAction* m_calculateAct;

	//清除IRM计算结果按钮
	QAction* m_clearIRMAct;

	//添加病害模式
	QAction* m_addDiseaseAct;

	//删除病害模式
	QAction* m_deleteDiseaseAct;

	//编辑病害模式
	QAction* m_editDiseaseAct;

	// 合并病害模式
	QAction* m_combineDiseaseAct;

	// 添加控制点模式
	QAction *m_addCtrlPointAct;

	// 添加面状病害
	QAction *m_addFacetsDiseaseAct;

	// 添加线状病害
	QAction *m_addLineDiseaseAct;

	//二三维里程矫正模式
	QAction* m_2d3dMileCorrentAct;

	//三维灰度图模式
	QAction* m_3dGrayModeAct;

	//三维深度图模式
	QAction* m_3dRgbModeAct;

 

	//备份数据库
	QAction * m_backupsDatabaseAct;

	//清空病害
	QAction* m_clearAllDiseasesAct;

	//更新病害
	QAction* m_updateAllDiseasesAct;

	//多车道工程合并
	QAction * m_mergeProjectExcelAct;

	//手动更新数据库
	QAction* m_updateDatabase;

	//导入二维软件病害
	QAction* m_input2dDiseaseAct;

	//导出二维软件病害
	QAction* m_output2dDiseaseAct;

	//导入自动识别病害
	QAction* m_inputSmartDiseaseAct;


	// 导出dxf
	QAction *m_exportDxfAction;

   //导出病害dxf
	QAction *m_exportDiseaseDxfAction;

	//导出高精度病害定位dxf
	QAction *m_exportHighAccuracyDiseaseDxfAction;

	QAction * m_ComputeGeoaligAction;

	// 导入控制点
	QAction *m_importCtrlPointsAction;

	// 导出控制点
	QAction *m_exportCtrlPointsAction;

	QAction *m_allResultDatasAction;

	//导出国检转换数据
	QAction * m_exportGJDatasAction;

	//高精度定位
private:
	//工程设置
	QAction* m_config;

	QAction* m_HelperAct;
	QAction* m_outExcel;
	statusBarWidget *m_statusBarWidget;

	QAction * m_AboutInfoAct;
	
private:
	// 用于控制面板显示隐藏;
	QMenu* m_pShowPaneMenu;

private:
	// dock面板管理器;
	hn::CDockManager* m_DockManager;

	//病害信息视图
	hnDiseaseListWidget *m_diseaseListWidget;
	hn::CDockWidget* m_diseaseListWidgetDockWidget;

	//IRM效果图
	IrmActualTimeShow* m_IrmShowWidget;
	hn::CDockWidget* m_irmChartDockWidget;

	//地图显示
	CustomBaiduMapView * m_mapWidget;
	hn::CDockWidget* m_winMapDockWidget;


	projectView * m_projectWidget;
	hn::CDockWidget * m_projectDockWidget;





	// 三维相对点云视图
	hnWidget3DView* m_pRel3dView;
	hn::CDockWidget *m_Doc3dOtherViewDock;

	// 三维绝对视图
	hnWidget3DView* m_pHn3dView;
	hn::CDockWidget *m_Doc3dViewDock;

	// 路面影像视图
	hn2dPixScrollWidget* m_2dPixScrollWidget;
	hn::CDockWidget*  m_2dPixScrollDocWidget;

	// 点云影像视图
	hn3dPixScrollWidget* m_3dPixScrollWidget;
	hn::CDockWidget* m_3dPixScrollDocWidget;

	// 景观影像视图
	hnStreetWidget *m_pStreetViewWidget;
	hn::CDockWidget* m_pDocStreetImageViewDock;

	//调整图片视图
	adjustImageWidget *m_adjustImageWidget;
	hn::CDockWidget* m_padjustImageDock;



	// 原始比例显示窗口
	hnOriginalScalePixShowWidget *m_originalWidget;
	hn::CDockWidget *m_originalDocWidget;

	// 工程列表树状视图;
	QTreeWidget* m_projectListTreeWidget;
	hn::CDockWidget *m_pDoctreeViewDock;
	QMenu *m_treeWidgetRightButtonMenu;		//工程树状图右键菜单

	//对话框
private:


private:
	//里程跳转对话框
	hnRegionJumpDlg *m_regionJumpDlg;
	//导出dxf对话框
	hnDxfCaculateDialog *m_dxfExportDialog;
	// 自动化模式病害合并对话框
	hnMergeLittleFrameThresholdDlg *m_mergeLittleFrameDlg;

private:
	hnDataManager* m_projects;
	std::vector<hnProjectDataInfo> m_projectDataInfos;
	hnOutputExcelDialog* m_outExcelDialog;
	//软件设置对话框
	hnProjectConfig * m_projectConfgDialog;

	//数据输出生成管理
	QSharedPointer<hnOutExcelMileManage> m_outExcelManage;

	
private:
	Ui::hnRoadDataProcessClass ui;

	hnCenterToast * m_centerToast = nullptr;

	// 程序主动同步视图时置 true，避免 2D/3D 的联动信号互相打回去。
	bool m_isProgrammaticViewSync = false;
	// Guards programmatic disease selection propagation between 2D, 3D and the list.
	bool m_isDiseaseSelectionSync = false;

};
