#define 地图
#include "hnRoadDataProcess.h"
#include <QDockWidget>
#include <QTreeWidget>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QDesktopServices>
#include <QString>
#include <QThreadPool>
#include <QApplication>
#include <QElapsedTimer>
#include <QDateTime>
#include <QSignalBlocker>
#include <QScopedValueRollback>
#include <QTimer>
#include "..\hnQtRibbonUI\hnRibbonBar.h"
#include "..\hnQtRibbonUI\hnRibbonCategory.h"
#include "..\hnQtRibbonUI\hnRibbonPannel.h"
#include "..\hnQtRibbonUI\hnRibbonToolButton.h"
#include "..\QtAdvancedDocking\DockManager.h"
#include "..\hnApplication\hnApplication.h"
#include "..\hdFramework\hdCommand.h"
#include "..\hdFramework/hdCommand.h"
#include "..\hdFramework\hdTool.h"
#include "..\hd3DScene\ISceneView.h"
#include "..\hdFramework\hdCommandDef.h"
#include "..\hnApplication\hnDataManager.h"
#include "..\hd3DScene\hd3DView.h"
#include "hnwidget3dview.h"
#include "..\hnApplication\hn2DCameraView.h"
#include "..\hnApplication\hn3DCameraView.h"
#include "hnMarkInfoDlg.h"
#include "..\hnDataTable\hnRoadDiseaseTable.h"
#include "..\hnIO\hnExcelIO.h"
#include "..\hnConfigService\HnXRSettings.h"
#include "hnOpenProjectDlg.h"
#include "calculateIrmForm.h"
#include "statusBarWidget.h"
#include "hnOutputExcelDialog.h"
#include "hnOutExcelMileManage.h"
#include "..\hnProject\hn3DProject.h"
#include "..\hnQtCommon\MyCommonMethods.h"
#include "hnMileAdjustDlg.h"
#include "..\hnCommon\hnRoadTypeDef.h"
#include <QCursor>
#include "hnRegionJumpDlg.h"	//里程跳转对话框
#include "../QHotkey/qhotkey.h"
#include "mergeAidcDiseases.h"
#include "../hnCommon/hn3dPointDef.h"
#include "../hnCommon/hn3dDiseaseDef.h"
//#include "hnSetPorjectMileForm.h"
#include "../hnDxfIO/hnOutputXR.h" 
#include "../hnQtCommon/GPSInfo.h"
#include "../hnQtCommon/TempGpsStrs.h"
#include "../hnQtCommon/SynTrigInfo.h" 
#include "../hnQtCommon/DataIdx.h"
#include "../hnQtCommon/MapGPSMile.h"
#include "xlsxdocument.h"
#include "hnXlsxInterface.h"
#include "hnAboutInfoWidgets.h"
#include "..\hnDBManagerDlg.h" 
#include "../HighAccuracySettingForm.h"
#include "../hnQtCommon/HighAccuracyInfo.h"
#include "..\hnCommon\hn2dDiseaseDef.h"
#include "..\HighAccConvertPlane\hnHighAcc2Plane.h"
#include "..\HighAccConvertPlane\HighAccuracyPositioning.h" 
#include "..\hnPavementCreate3d\hnPavementCamReader.h"
#include "..\hnPavementCreate3d\hnPavementImageInfo.h"
#include "hnCalRoadGeometry.h"
#include <QProgressDialog>
#include "../hnApplication/hnDiseaseService.h"
#include "../hnQtCommon/hnCenterToast.h"
#include "../hnQtCommon/BusyLoadingDialog.h"
using namespace hnApp;
using namespace hnPro;

static QProgressDialog * g_pgDlg = nullptr;

hnRoadDataProcess::hnRoadDataProcess(QWidget *parent)
	: hnRibbonMainWindow(parent), m_pRel3dView(NULL), m_pHn3dView(NULL), m_DockManager(NULL), m_2dPixScrollWidget(NULL),
	m_3dPixScrollWidget(NULL), m_diseaseListWidgetDockWidget(NULL), m_diseaseListWidget(NULL), m_pStreetViewWidget(NULL)
	, m_projects(NULL), m_outExcelDialog(nullptr), m_projectConfgDialog(nullptr), m_projectDockWidget(NULL), m_projectWidget(NULL), m_IrmShowWidget(NULL), m_mapWidget(NULL),m_adjustImageWidget(NULL)
{
	m_centerToast = new hnCenterToast(this);
	m_DockManager = new hn::CDockManager(this);

	// 创建工具栏
	this->createAction();

	//初始化工程相关信息
	if (!this->initProject())
	{
		qWarning() << QString::fromLocal8Bit("初始化工程信息失败");
	}

	// 创建视图;
	this->createView();

	//初始化对话框
	this->initDlg();

	// 创建连接
	this->createConnect();

	// 树状视图连接
	this->createTreeConnect();

	//初始化快捷键
	this->initShortCuts();

	//状态栏显示
	this->statusBar()->show();

	//最大化
	this->showMaximized();

	// 读取已有布局，无效，需要实现各个dockpane的存储;
	this->readLayout();

	//设置软件图标
	this->setWindowIcon(QIcon(":/icons/iconsNew/logo_xroe.ico"));

}

hnRoadDataProcess::~hnRoadDataProcess()
{
	hnDataManager::destoryDataManager();
	if (m_DockManager)
	{
		delete m_DockManager;
		m_DockManager = NULL;
	}
}

bool hnRoadDataProcess::progressCallback(float fval, const char* qstrName, bool bCancle)
{
	if (g_pgDlg)
	{
		g_pgDlg->setValue(static_cast<int>(fval * 100));
		g_pgDlg->setLabelText(QString::fromLocal8Bit(qstrName));

		QCoreApplication::processEvents();
		return !g_pgDlg->wasCanceled();
	}
	return true;
}

bool hnRoadDataProcess::initProject()
{
	//加载根目录  工程相关信息
	if (!loadConfigData())
		return false;
	m_projects = hnDataManager::getDataManager();
	return hnDataManager::getDataManager()->initRoadStandardInfo();

}
void hnRoadDataProcess::widgetviewActive(WId hwnd)
{
	// 当前视图窗口句柄;
	HWND active_view = (HWND)hwnd;

	// 判断当前视图是否为活动视图，如果不是则切换;
	if (hnApplication::getApp()->GetActiveView()->GetHWnd() != active_view)
	{
		// 通过窗口句柄得到当前活动视图;
		IHdView* pView = hnApplication::getApp()->getViewByHwnd(active_view);

		// 设置切换活动视图，不清空视图工具;
		if (pView)
		{
			hnApplication::getApp()->SetActiveView(pView);
			pView->Refresh();
		}
	}
}

void hnRoadDataProcess::closeEvent(QCloseEvent * e)
{
	// QWebEngine starts its global shutdown when the last top-level window
	// closes.  Destroy the map page first so Chromium has no live scheduler
	// clients left when ResourceDispatcherHostImpl::OnShutdown() runs.
	if (m_mapWidget)
	{
		m_mapWidget->shutdownWebEngine();
	}

	if (!m_projects->isOpenProject())
	{
		return;
	}
	hnPro::hnProject * lastProject = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	//记录最后工程帧号
	const int frameNum = m_2dPixScrollWidget->getPixWidget()->getButtomFrameNumber();

	m_xrSetting->lastProjectFn = frameNum;
	m_xrSetting->lastProjectName = lastProject->get2DProName();

	m_projects->closeProject();
	this->saveLayout();

	//保存工程信息 供最近工程使用 
	m_xrSetting->writeData();
}

void hnRoadDataProcess::showEvent(QShowEvent *event)
{
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][MainWindowShow]"
		<< "time=" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
		<< "visible=" << isVisible()
		<< "active=" << isActiveWindow()
		<< "minimized=" << isMinimized()
		<< "maximized=" << isMaximized()
		<< "fullScreen=" << isFullScreen()
		<< "geo=" << QString("%1,%2,%3,%4").arg(geometry().x()).arg(geometry().y()).arg(geometry().width()).arg(geometry().height());
	#endif
	this->setAttribute(Qt::WA_Mapped);
	QWidget::showEvent(event);
}

void hnRoadDataProcess::saveLayout()
{
	// 设置配置参数路径;
	//QString strAppDirPath = QCoreApplication::applicationDirPath();
	QString strAppDirPath = MyCommonMethods::GetUserPath();
	QString strLayoutPath = strAppDirPath + "/Layout.ini";
	QFile file(strLayoutPath);
	if (file.open(QIODevice::WriteOnly))
	{
		QDataStream out(&file);
		out << m_DockManager->saveState();
		file.close();
	}
}

void hnRoadDataProcess::readLayout()
{
	QString baseAppDirPath = QCoreApplication::applicationDirPath() + "/Layout.ini";
	QString strAppDirPath = MyCommonMethods::GetUserPath();
	QString strLayoutPath = strAppDirPath + "/Layout.ini";
	if (QFileInfo::exists(strLayoutPath) == false)
	{
		//不存在就复制过去
		QFile::copy(baseAppDirPath, strLayoutPath);
		if (QFileInfo::exists(strLayoutPath) == false)
		{
			return;
		}
	}

	// 读取数据;
	QFile file(strLayoutPath);
	if (file.open(QIODevice::ReadOnly))
	{
		QByteArray arry;
		QDataStream out(&file);
		out >> arry;
		file.close();

		m_DockManager->restoreState(arry);
	}
}

// 创建视图
void hnRoadDataProcess::createView()
{
	// 创建局部点云视图;
	if (!m_pRel3dView)
	{
		m_pRel3dView = new hnWidget3DView(this);
	}

	// 创建Dock
	m_Doc3dOtherViewDock = new hn::CDockWidget(QStringLiteral("局部点云视图"), this);

	// 设置对象停靠属性;
	hn::CDockWidget::DockWidgetFeatures tFeatures = hn::CDockWidget::NoDockWidgetFeatures;
	tFeatures |= hn::CDockWidget::DockWidgetFloatable;
	tFeatures |= hn::CDockWidget::DockWidgetMovable;
	tFeatures |= hn::CDockWidget::DockWidgetClosable;


	m_Doc3dOtherViewDock->setFeatures(tFeatures);
	m_Doc3dOtherViewDock->setWidget(m_pRel3dView);
	m_Doc3dOtherViewDock->setHidden(false);

	// 根据设定的停靠位置，将dockpane添加到widget中进行管理;
	m_DockManager->addDockWidget(hn::RightDockWidgetArea, m_Doc3dOtherViewDock);
	m_pShowPaneMenu->addAction(m_Doc3dOtherViewDock->toggleViewAction());

	// 初始化全局点云对象;
	if (m_pRel3dView != NULL)
	{
		// 初始化视图;
		IHdView* pView = hnApplication::getApp()->new3DView((HWND)m_pRel3dView->winId());

		// 初始化失败退出;
		if (pView == NULL)
		{
			QMessageBox::information(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("初始化视图失败"),
				QString::fromLocal8Bit("确定"));
			exit(0);
		}

		// 视图名称设置;
		pView->SetName("局部点云视图");

		// 设置当前视图工具为"hdFramework.hdToolFly";
		hnApplication::getApp()->SetCurrentTool(COMMAND_3D_CAMERA);
	}

	// 三维绝对视图;
	if (!m_pHn3dView)
	{
		m_pHn3dView = new hnWidget3DView(this);
	}

	// 停靠三维绝对视图;
	m_Doc3dViewDock = new hn::CDockWidget(QStringLiteral("全局点云视图"), this);
	m_Doc3dViewDock->setFeatures(tFeatures);
	m_Doc3dViewDock->setWidget(m_pHn3dView);

	// 根据设定的停靠位置，将dockpane添加到widget中进行管理;
	m_DockManager->addDockWidget(hn::RightDockWidgetArea, m_Doc3dViewDock);
	m_pShowPaneMenu->addAction(m_Doc3dViewDock->toggleViewAction());

	// 根据设定的停靠位置，将dockpane添加到widget中进行管理;
	m_DockManager->addDockWidget(hn::RightDockWidgetArea, m_Doc3dViewDock);

	auto action = m_pHn3dView->toggleViewAction();
	action->setText(QStringLiteral("三维绝对视图"));
	m_pShowPaneMenu->addAction(action);

	// 初始化三维widget对象;
	if (m_pHn3dView != NULL)
	{
		// 初始化视图;
		IHdView* pView = hnApplication::getApp()->new3DView((HWND)m_pHn3dView->winId());

		// 初始化失败退出;
		if (pView == NULL)
		{
			QMessageBox::information(this, QString::fromLocal8Bit("警告"),
				QString::fromLocal8Bit("初始化三维绝对视图失败"),
				QString::fromLocal8Bit("确定"));
			exit(0);
		}

		// 视图名称设置;
		pView->SetName("全局点云视图");

		// 设置当前视图工具为"hdFramework.hdToolFly";
		hnApplication::getApp()->SetCurrentTool(COMMAND_3D_CAMERA);
	}

	// 路面破损影像视图;
	if (!m_2dPixScrollWidget)
	{
		//hnApplication::getApp()->newRaodDamageContinousBrowserPixWidget();
		m_2dPixScrollWidget = hnApplication::getApp()->newRaodDamageContinousBrowserPixWidget();
	}

	// 停靠路面影像视图;
	m_2dPixScrollDocWidget = new hn::CDockWidget(QStringLiteral("路面影像视图"), this);
	m_2dPixScrollDocWidget->setFeatures(tFeatures);
	m_2dPixScrollDocWidget->setWidget(m_2dPixScrollWidget);
	//m_2dPixScrollDocWidget->setMinimumWidth(550);

#if 0
	// 在绘图区上方添加工具按钮
	QToolBar *toolBar = new QToolBar();
	const QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"),
		QIcon(QStringLiteral(":/icons/iconsNew/导入工程.png")));
	QAction *act = new QAction(projectIcon, QStringLiteral("&导入"), this);
	toolBar->addAction(act);
	m_2dPixScrollDocWidget->setToolBar(toolBar);
#endif

	// 根据设定的停靠位置，将dockpane添加到widget中进行管理;
	m_DockManager->addDockWidget(hn::RightDockWidgetArea, m_2dPixScrollDocWidget);
	m_pShowPaneMenu->addAction(m_2dPixScrollDocWidget->toggleViewAction());

	// 根据设定的停靠位置，将dockpane添加到widget中进行管理;
	m_DockManager->addDockWidget(hn::RightDockWidgetArea, m_2dPixScrollDocWidget);
	m_pShowPaneMenu->addAction(m_2dPixScrollDocWidget->toggleViewAction());

	// 点云影像视图;
	if (!m_3dPixScrollWidget)
	{
		m_3dPixScrollWidget = hnApplication::getApp()->new3DImageViewWidget();
	}

	// 停靠点云影像视图;
	m_3dPixScrollDocWidget = new hn::CDockWidget(QStringLiteral("点云影像视图"), this);
	m_3dPixScrollDocWidget->setFeatures(tFeatures);
	m_3dPixScrollDocWidget->setWidget(m_3dPixScrollWidget);
	//m_3dPixScrollDocWidget->setMinimumWidth(550);

	// 根据设定的停靠位置，将dockpane添加到widget中进行管理;
	m_DockManager->addDockWidget(hn::RightDockWidgetArea, m_3dPixScrollDocWidget);
	m_pShowPaneMenu->addAction(m_3dPixScrollDocWidget->toggleViewAction());

	// 根据设定的停靠位置，将dockpane添加到widget中进行管理;
	m_DockManager->addDockWidget(hn::RightDockWidgetArea, m_3dPixScrollDocWidget);
	m_pShowPaneMenu->addAction(m_3dPixScrollDocWidget->toggleViewAction());


	//景观影像视图;
	if (!m_pStreetViewWidget)
	{
		m_pStreetViewWidget = new hnStreetWidget();
	}

	//// 景观影像视图
	m_pDocStreetImageViewDock = new hn::CDockWidget(QStringLiteral("景观影像视图"), this);
	m_pDocStreetImageViewDock->setFeatures(tFeatures);
	m_pDocStreetImageViewDock->setWidget(m_pStreetViewWidget);

	// 根据设定的停靠位置，将dockpane添加到widget中进行管理;
	m_DockManager->addDockWidget(hn::RightDockWidgetArea, m_pDocStreetImageViewDock);
	m_pShowPaneMenu->addAction(m_pDocStreetImageViewDock->toggleViewAction());

	// 根据设定的停靠位置，将dockpane添加到widget中进行管理;
	m_DockManager->addDockWidget(hn::RightDockWidgetArea, m_pDocStreetImageViewDock);
	m_pShowPaneMenu->addAction(m_pDocStreetImageViewDock->toggleViewAction());
	//原始比例图片显示窗口
	m_originalWidget = new hnOriginalScalePixShowWidget(this);
	m_originalDocWidget = new hn::CDockWidget(QString::fromLocal8Bit("原始比例显示视图"), this);
	m_originalDocWidget->setFeatures(tFeatures);
	m_originalDocWidget->setWidget(m_originalWidget);
	m_DockManager->addDockWidget(hn::RightDockWidgetArea, m_originalDocWidget);
	m_pShowPaneMenu->addAction(m_originalDocWidget->toggleViewAction());


	//病害列表窗口
	if (!m_diseaseListWidget)
	{
		m_diseaseListWidget = new hnDiseaseListWidget(this);
		m_diseaseListWidget->setWindowTitle(QStringLiteral("病害管理"));
	}

	m_diseaseListWidgetDockWidget = new hn::CDockWidget(QStringLiteral("病害管理视图"), this);
	m_diseaseListWidgetDockWidget->setFeatures(tFeatures);
	m_diseaseListWidgetDockWidget->setWidget(m_diseaseListWidget);
	// 根据设定的停靠位置，将dockpane添加到widget中进行管理;
	m_DockManager->addDockWidget(hn::RightDockWidgetArea, m_diseaseListWidgetDockWidget);
	m_pShowPaneMenu->addAction(m_diseaseListWidgetDockWidget->toggleViewAction());


	if (!m_IrmShowWidget)
	{
		m_IrmShowWidget = new IrmActualTimeShow(this);
		m_IrmShowWidget->setWindowTitle(QStringLiteral("IRM界面"));
	}

	m_irmChartDockWidget = new hn::CDockWidget(QStringLiteral("IRM实时显示"), this);
	m_irmChartDockWidget->setFeatures(tFeatures);
	m_irmChartDockWidget->setWidget(m_IrmShowWidget);
	m_DockManager->addDockWidget(hn::RightDockWidgetArea, m_irmChartDockWidget);
	m_pShowPaneMenu->addAction(m_irmChartDockWidget->toggleViewAction());
#ifdef 地图

	if (!m_mapWidget)
	{
	m_mapWidget = new  CustomBaiduMapView(this);
	m_mapWidget->setWindowTitle(QStringLiteral("地图显示"));
	}

	m_winMapDockWidget = new hn::CDockWidget(QStringLiteral("地图显示"), this);
	m_winMapDockWidget->setFeatures(tFeatures);
	m_winMapDockWidget->setWidget(m_mapWidget);
	m_DockManager->addDockWidget(hn::RightDockWidgetArea, m_winMapDockWidget);
	m_pShowPaneMenu->addAction(m_winMapDockWidget->toggleViewAction());
#endif // 地图
	// 创建工程管理树状视图;
	m_projectListTreeWidget = new QTreeWidget(this);
	m_projectListTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

	//右键菜单
	m_treeWidgetRightButtonMenu = new QMenu(this);

	// 树状图设置为两列， 第二列记录工程路径并隐藏;
	m_projectListTreeWidget->setColumnCount(2);
	m_projectListTreeWidget->setColumnHidden(1, true);

	m_projectListTreeWidget->setHeaderLabel(QStringLiteral("工程列表"));
	m_projectListTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

	// 创建Dock
	m_pDoctreeViewDock = new hn::CDockWidget(QStringLiteral("工程管理视图"), this);
	m_pDoctreeViewDock->setFeatures(tFeatures);
	m_pDoctreeViewDock->setWidget(m_projectListTreeWidget);
	m_pDoctreeViewDock->setHidden(false);

	// 根据设定的停靠位置，将dockpane添加到widget中进行管理;
	m_DockManager->addDockWidget(hn::LeftDockWidgetArea, m_pDoctreeViewDock);
	m_pShowPaneMenu->addAction(m_pDoctreeViewDock->toggleViewAction());



	if (!m_projectWidget)
	{
		m_projectWidget = new projectView(this);

	}

	m_projectDockWidget = new hn::CDockWidget(QStringLiteral("信息_校桩_打标"), this);
	m_projectDockWidget->setFeatures(tFeatures);
	m_projectWidget->setMinimumWidth(0);
	m_projectWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	m_projectDockWidget->setWidget(m_projectWidget);
	m_projectWidget->setHidden(false);
	m_projectDockWidget->setMinimumWidth(260);
	m_DockManager->addDockWidget(hn::LeftDockWidgetArea, m_projectDockWidget);
	m_pShowPaneMenu->addAction(m_projectDockWidget->toggleViewAction());
	 
	if (!m_adjustImageWidget)
	{
		m_adjustImageWidget = new adjustImageWidget(this);

	}

	m_padjustImageDock = new hn::CDockWidget(QStringLiteral("图片调整"), this);
	m_padjustImageDock->setFeatures(tFeatures);
	m_padjustImageDock->setWidget(m_adjustImageWidget);
	m_adjustImageWidget->setHidden(false);
	m_padjustImageDock->setMinimumWidth(350);
	m_DockManager->addDockWidget(hn::LeftDockWidgetArea, m_padjustImageDock);
	m_pShowPaneMenu->addAction(m_padjustImageDock->toggleViewAction());

}

void hnRoadDataProcess::initDlg()
{
	//m_adjustImageWidget = new adjustImageWidget(this);
	m_regionJumpDlg = new hnRegionJumpDlg(this);
	m_projectConfgDialog = new hnProjectConfig(this);
	m_dxfExportDialog = new hnDxfCaculateDialog(this);
	m_mergeLittleFrameDlg = new hnMergeLittleFrameThresholdDlg(this);
}



// 创建工具栏
void hnRoadDataProcess::createAction()
{
	// 软件名称
	//设置主窗口标题
	setWindowTitle(QStringLiteral("公路二三维一体化数据处理平台"));
	//创建一个hnRibbonBar框架
	hnRibbonBar* ribbon = ribbonBar();
	//设置字体属性
	QFont f = ribbon->font();
	f.setFamily("Microsoft YaHei");
	f.setPixelSize(15);
	ribbon->setFont(f);
	//ribbon->applitionButton()->setText(QStringLiteral("File"));

	// 工程管理
	//用框架创建一个分类的功能栏tab
	hnRibbonCategory* categoryProject = ribbon->addCategoryPage(QStringLiteral("工程管理"));
	//创建功能栏下的工具栏
	createProCategory(categoryProject);
	// 数据处理
	hnRibbonCategory* categoryGD = ribbon->addCategoryPage(QStringLiteral("病害处理"));
	createDataProcessCategory(categoryGD);
	this->createDataBaseMgrCategory(categoryGD);
	//// 数据库管理
	//hnRibbonCategory* dataBaseMgrCategory = ribbon->addCategoryPage(QStringLiteral("数据库管理"));


	// 数据输出
	hnRibbonCategory* categoryView = ribbon->addCategoryPage(QStringLiteral("数据输出"));
	createOutputCategory(categoryView);

	// 点云处理
#if 0	//功能暂未实现 暂不显示
	hnRibbonCategory* categoryTool = ribbon->addCategoryPage(QStringLiteral("点云处理"));
	createCloudCategory(categoryTool);
#endif

	//视图管理
	hnRibbonCategory* categoryOView = ribbon->addCategoryPage(QStringLiteral("视图管理"));
	createViewsCategory(categoryOView);

	// 系统
	hnRibbonCategory* categoryHelp = ribbon->addCategoryPage(QStringLiteral("系统"));
	createSystemCategory(categoryHelp);
	this->m_statusBarWidget = new statusBarWidget();
	this->statusBar()->addWidget(m_statusBarWidget);
}

void hnRoadDataProcess::initShortCuts()
{
	//添加病害
	QHotkey *addDiseaseHotKey = new QHotkey(QKeySequence(Qt::Key_F1 ), true, this);
	connect(addDiseaseHotKey, &QHotkey::activated, this, &hnRoadDataProcess::slot_changeToAddDiseaseMode);

	//删除病害
	QHotkey *deleteDiseaseHotKey = new QHotkey(QKeySequence(Qt::Key_F2 ), true, this);
	connect(deleteDiseaseHotKey, &QHotkey::activated, this, &hnRoadDataProcess::slot_changeToDeleteDiseaseMode);

	//编辑病害
	QHotkey *editDiseaseHotKey = new QHotkey(QKeySequence(Qt::Key_F3 ), true, this);
	connect(editDiseaseHotKey, &QHotkey::activated, this, &hnRoadDataProcess::slot_changeToEditDiseaseMode);

	//移动病害
	QHotkey *moveDiseaseHotKey = new QHotkey(QKeySequence(Qt::Key_M | Qt::ShiftModifier), true, this);
	connect(moveDiseaseHotKey, &QHotkey::activated, this, &hnRoadDataProcess::slot_changeToMoveDiseaseMode);


	//合并病害
	QHotkey *mergeDiseaseHotKey = new QHotkey(QKeySequence(Qt::Key_U | Qt::ShiftModifier), true, this);
	connect(mergeDiseaseHotKey, &QHotkey::activated, this, &hnRoadDataProcess::slot_changeToMergeDiseaseMode);

	//二三维视图矫正
	QHotkey *enterHotKey = new QHotkey(QKeySequence(Qt::Key_C | Qt::ShiftModifier), true, this);

	connect(enterHotKey, &QHotkey::activated, [this]() {

        if (m_2dPixScrollWidget->getPixWidget()->getMode() == hnWorkMode::GET_MILE)
        {
            const double selected2dMile = m_2dPixScrollWidget->getPixWidget()->getEncoderMile();
            const double selected3dMile = m_3dPixScrollWidget->getPixWidget()->getEncoderMile();
            if (selected2dMile < 0.0 || selected3dMile < 0.0)
            {
                QMessageBox::warning(nullptr, QString::fromLocal8Bit("提示"),
                    QString::fromLocal8Bit("请先分别点击二维视图和三维视图中的同一位置，再按Shift+C 进行矫正"),
                    QString::fromLocal8Bit("确定"));
                return;
            }

            double diff = selected2dMile - selected3dMile;
            auto dataManager = hnDataManager::getDataManager();
            if (dataManager->isOpenProject())
            {
                auto currentProject = dataManager->getCurrentProject();
                if (currentProject && currentProject->get3DProject())
                {
                    m_2dPixScrollWidget->getPixWidget()->claerSelectPoint();
                    m_3dPixScrollWidget->getPixWidget()->claerSelectPoint();
                    currentProject->set2d3dMileDiff(diff);
                    m_2dPixScrollWidget->getPixWidget()->refreshSdkViewState();
                    m_3dPixScrollWidget->getPixWidget()->refreshSdkViewState();
                    const double source2dMile = m_2dPixScrollWidget->getPixWidget()->currentBottomEncoderMile();
                    syncContinuousViews(ContinuousViewSyncSource::Road2D, source2dMile);

                    QMessageBox::information(nullptr, QString::fromLocal8Bit("提示"),
						QString::fromLocal8Bit("已经对二三维里程差值进行矫正"),
                        QString::fromLocal8Bit("确定"));
                }
            }
        }
	});
}

// 创建连接
void hnRoadDataProcess::createConnect()
{
	// 打开工程
	connect(m_openProjectAct, &QAction::triggered, this, &hnRoadDataProcess::openProjectSlot);
	connect(m_lastProjectAct, &QAction::triggered, this, &hnRoadDataProcess::openLastProjectSlot);
	connect(m_checkProAct, &QAction::triggered, this, &hnRoadDataProcess::checkProSlot);
	//connect(m_autoDiseaseMerge, &QAction::triggered, this, &hnRoadDataProcess::slot_mergeAutoDisease);
	connect(m_openCurrentProjectDirAction, &QAction::triggered, this, &hnRoadDataProcess::slot_openCurrentProjectDir);
	connect(m_outSimpleProjectAction, &QAction::triggered, this, &hnRoadDataProcess::slot_outSimpleProject);
	connect(m_gpsMatchingAct, &QAction::triggered, this, &hnRoadDataProcess::slot_gpsMatching);
	connect(m_oShapeViewportAct, &QAction::triggered, this, &hnRoadDataProcess::slot_toStandard2View);
	connect(m_o3ShapeViewportAct, &QAction::triggered, this, &hnRoadDataProcess::slot_toStandard3View);
	connect(m_oDViewportAct, &QAction::triggered, this, &hnRoadDataProcess::slot_toStandard23View);
	connect(m_config, &QAction::triggered, this, &hnRoadDataProcess::slot_openConfigWidget);
	connect(m_AboutInfoAct, &QAction::triggered, this, &hnRoadDataProcess::slot_aboutInfosWidget);


	connect(m_HelperAct, &QAction::triggered, this, &hnRoadDataProcess::slot_oepnCourseDocument);
	connect(m_outExcel, &QAction::triggered, this, &hnRoadDataProcess::slot_outputExcel);
#ifdef DEBUG
	connect(m_allResultDatasAction, &QAction::triggered, this, &hnRoadDataProcess::slot_outAllResultDatas);

#endif // DEBUG

	connect(m_mergeProjectExcelAct, &QAction::triggered, this, &hnRoadDataProcess::slot_outMergeExcel);

	connect(m_projectListTreeWidget, &QTreeWidget::itemDoubleClicked, this, &hnRoadDataProcess::slot_dClickTreeItem);
	connect(m_projectListTreeWidget, &QTreeWidget::itemSelectionChanged, this, &hnRoadDataProcess::slot_selectNodeChange);
	connect(m_projectListTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slot_showContextMenu(QPoint)));
	connect(m_markInfoAct, &QAction::triggered, this, &hnRoadDataProcess::slot_markInfoSlot);
	//connect(m_mileCorrectAct, &QAction::triggered, this, &hnRoadDataProcess::slot_mileCorrectSlot);

	connect(m_calculateAct, &QAction::triggered, this, &hnRoadDataProcess::slot_calculateIrm);
	connect(m_clearIRMAct, &QAction::triggered, this, &hnRoadDataProcess::slot_clearIrm);
	connect(m_ComputeGeoaligAction, &QAction::triggered, this, &hnRoadDataProcess::slot_compute);

	//影像生成
	connect(m_createImageAct, &QAction::triggered, this, &hnRoadDataProcess::createImageSlot);

	//裁切功能
	connect(m_cutImageAct, &QAction::triggered, this, &hnRoadDataProcess::slot_cutImage);

	//列表删除病害后,界面刷新 
	connect(this->m_diseaseListWidget,
		&hnDiseaseListWidget::deleteDisease,
		this->m_2dPixScrollWidget->getPixWidget(),
		&hn2dPixWidget::slot_deleteDisease
	);
	connect(this->m_diseaseListWidget,
		&hnDiseaseListWidget::deleteDisease,
		this->m_3dPixScrollWidget->getPixWidget(),
		&hn3dPixWidget::slot_deleteDisease
	);

	//添加病害后，病害列表刷新
	//connect(this->m_2dPixScrollWidget->getPixWidget(),
	//	&hn2dPixWidget::signal_addDisease,
	//	this->m_diseaseListWidget,
	//	&hnDiseaseListWidget::addDisease);

	//connect(this->m_3dPixScrollWidget->getPixWidget(), &hn3dPixWidget::signal_addDisease,
	//	this->m_diseaseListWidget, QOverload<const hnRoadDiseaseInfo &, bool>::of(&hnDiseaseListWidget::addDisease));

	//connect(this->m_pStreetViewWidget, &hnStreetWidget::signal_addDisease,
	//	this->m_diseaseListWidget, &hnDiseaseListWidget::addDisease);



	// 添加病害后， 所有窗口都刷新
	/*connect(this->m_2dPixScrollWidget->getPixWidget(),
		&hn2dPixWidget::signal_addDisease, this, &hnRoadDataProcess::updatePixWidget);
	connect(this->m_3dPixScrollWidget->getPixWidget(), &hn3dPixWidget::signal_addDisease,
		this, &hnRoadDataProcess::updatePixWidget);
	connect(this->m_pStreetViewWidget, &hnStreetWidget::signal_addDisease,
		this, &hnRoadDataProcess::updatePixWidget);
	*/

	// Sync disease selection to the other view and the list. The source view has
	// already applied the user action and must not rebuild its SDK layer a second time.
	auto syncDiseaseSelectionFromView = [this](const hnRoadDiseaseInfo& disease, bool sourceIs2d)
	{
		if (this->m_isDiseaseSelectionSync)
		{
			return;
		}
		QScopedValueRollback<bool> selectionGuard(this->m_isDiseaseSelectionSync, true);

		if (this->m_diseaseListWidget)
		{
			this->m_diseaseListWidget->slot_selectDisease(disease);
		}
		auto applySelectionToTarget = [&disease](hn2d3dPixBaseWidget* target)
		{
			if (!target)
			{
				return;
			}
			target->setSelectedDisease(disease);
			if (disease.nID >= 0)
			{
				// Existing diseases can be outside the target view's visible mileage range.
				// Center after the selection layer is rebuilt so the visual state is observable.
				QTimer::singleShot(0, target, [target, disease]()
				{
					target->centerSdkDiseaseInView(disease);
				});
			}
		};

		if (!sourceIs2d && this->m_2dPixScrollWidget && this->m_2dPixScrollWidget->getPixWidget())
		{
			applySelectionToTarget(this->m_2dPixScrollWidget->getPixWidget());
		}
		if (sourceIs2d && this->m_3dPixScrollWidget && this->m_3dPixScrollWidget->getPixWidget())
		{
			applySelectionToTarget(this->m_3dPixScrollWidget->getPixWidget());
		}
	};
	connect(this->m_2dPixScrollWidget->getPixWidget(), &hn2dPixWidget::signal_selectDisease,
		this, [syncDiseaseSelectionFromView](const hnRoadDiseaseInfo& disease)
	{
		syncDiseaseSelectionFromView(disease, true);
	});
	connect(this->m_3dPixScrollWidget->getPixWidget(), &hn3dPixWidget::signal_selectDisease,
		this, [syncDiseaseSelectionFromView](const hnRoadDiseaseInfo& disease)
	{
		syncDiseaseSelectionFromView(disease, false);
	});


	/*connect(this->m_diseaseListWidget , &hnDiseaseListWidget::signal_updateView, this , &hnRoadDataProcess::updatePixWidget);*/
	


	//删除病害后，病害列表刷新
	//connect(this->m_3dPixScrollWidget->getPixWidget(), &hn3dPixWidget::signal_deleteDisease,
	//	this->m_diseaseListWidget, QOverload<const hnRoadDiseaseInfo &>::of(&hnDiseaseListWidget::deleteDisease));
	//connect(this->m_2dPixScrollWidget->getPixWidget(),
	//	&hn2dPixWidget::signal_deleteDisease,
	//	this->m_diseaseListWidget,
	//	&hnDiseaseListWidget::deleteDisease);
	/*connect(this->m_pStreetViewWidget, &hnStreetWidget::signal_deleteDisease,
		this->m_diseaseListWidget, &hnDiseaseListWidget::deleteDisease);*/

	//二维视图滚动条变化，更新打标 较桩界面
	connect(this->m_2dPixScrollWidget, &hn2dPixScrollWidget::signal_roadMileAndDmiChanged,
		m_projectWidget, &projectView::slot_updateMileAndDmi);
	//二维视图滚动条变化，更新IRM界面
	connect(this->m_2dPixScrollWidget, &hn2dPixScrollWidget::signal_roadMileAndDmiChanged,
		m_IrmShowWidget, &IrmActualTimeShow::slot_updateIriFormSlots);

	connect(m_projectWidget, &projectView::signal_updateAllWidget, [this]()
	{

		//// 加载当前工程路面影像
		this->m_2dPixScrollWidget->loadRoadPicture();


		// 加载三维影像
		if (hnDataManager::getDataManager()->getCurrentProject()->getProjectType() != PROJECT_2D_TYPE)
		{
			m_3dPixScrollWidget->load3dImage();
		}
		this->m_diseaseListWidget->updateAllDiseases();
	});

	connect(m_projectWidget, &projectView::signal_updateAllWidget, this, &hnRoadDataProcess::updateAllWidget);

	//病害列表发出帧序号改变的信号   路面显示窗口对应跳转
	connect(this->m_diseaseListWidget, &hnDiseaseListWidget::signal_road2dFrameIdxChanged,
		[this](double encoderMile) {
		const double targetMile = qMax(0.0, encoderMile);
		this->m_2dPixScrollWidget->getPixWidget()->scrollBottomToEncoderMile(targetMile);
		syncContinuousViews(ContinuousViewSyncSource::Road2D, targetMile);
	});
	
	connect(this->m_diseaseListWidget, &hnDiseaseListWidget::signal_setDiseaseIsChecked,
		[this](const hnRoadDiseaseInfo& disease) {
		this->m_2dPixScrollWidget->getPixWidget()->setSelectedDisease(disease);
		this->m_3dPixScrollWidget->getPixWidget()->setSelectedDisease(disease);
		QTimer::singleShot(0, this, [this, disease]() {
			this->m_2dPixScrollWidget->getPixWidget()->centerSdkDiseaseInView(disease);
			this->m_3dPixScrollWidget->getPixWidget()->centerSdkDiseaseInView(disease);
		});
		updatePixWidget();
	});
	connect(hnDataManager::getDataManager()->getDiseaseService(), &hnDiseaseService::diseaseDeleted,
		this, [this](const hnRoadDiseaseInfo&) {
		this->m_2dPixScrollWidget->getPixWidget()->refreshSdkDiseaseLayer();
		this->m_3dPixScrollWidget->getPixWidget()->refreshSdkDiseaseLayer();
		updatePixWidget();
	});

	//三维
	connect(m_diseaseListWidget, &hnDiseaseListWidget::signal_road3dFrameIdxChanged, [this](double encoderMile) {
		const double targetMile = qMax(0.0, encoderMile);
		m_3dPixScrollWidget->getPixWidget()->scrollBottomToEncoderMile(targetMile);
		syncContinuousViews(ContinuousViewSyncSource::Road3D, targetMile);
	});

	connect(this->m_2dPixScrollWidget->getPixWidget(), &hn2dPixWidget::signal_statusInfoChanged,
		this->m_statusBarWidget, QOverload<const QString&>::of(&statusBarWidget::updateLabelTextSlot));
	connect(this->m_3dPixScrollWidget->getPixWidget(), &hn3dPixWidget::signal_statusInfoChanged,
		this->m_statusBarWidget, QOverload<const QString&>::of(&statusBarWidget::updateLabelTextSlot));

	// 只有用户操作触发跨视图同步，程序滚动产生的普通bottom 信号只用于状态刷新，避免 2D/3D 来回追赶。
	connect(this->m_2dPixScrollWidget->getPixWidget(), &hn2dPixWidget::signal_sdkUserBottomEncoderMileChanged,
		this, [this](double source2dMile)
	{
		syncContinuousViews(ContinuousViewSyncSource::Road2D, source2dMile);
	});

	connect(this->m_3dPixScrollWidget->getPixWidget(), &hn3dPixWidget::signal_sdkUserBottomEncoderMileChanged,
		this, [this](double source3dMile)
	{
		syncContinuousViews(ContinuousViewSyncSource::Road3D, source3dMile);
	});

	//切换三维浏览模式
	connect(this->m_3dGrayModeAct, &QAction::triggered, this, &hnRoadDataProcess::slot_changeGray3dMode);
	connect(this->m_3dRgbModeAct, &QAction::triggered, this, &hnRoadDataProcess::slot_changeRgb3dMode);

	//清空所有病害
	connect(this->m_backupsDatabaseAct, &QAction::triggered, this, &hnRoadDataProcess::slot_backupsDatabase);
	connect(this->m_clearAllDiseasesAct, &QAction::triggered, this, &hnRoadDataProcess::slot_clearAllDiseases);
	//	connect(this->m_updateAllDiseasesAct, &QAction::triggered, this, &hnRoadDataProcess::slot_updateDiseaseDatabase);
	connect(this->m_updateDatabase, &QAction::triggered, this, &hnRoadDataProcess::slot_updateDatabase);

#pragma region SDKViewMapping
		//二维视图滚动条变化，更新三维视图
    connect(this->m_pStreetViewWidget, &hnStreetWidget::signal_imageIdxChanged,
        this, &hnRoadDataProcess::slot_streetWidgetFrameIdxChanged, Qt::UniqueConnection);
#pragma endregion

	//二维三维发送信号，原始比例更新视图
	connect(m_2dPixScrollWidget->getPixWidget(), &hn2dPixWidget::sig_mousePosImageChanged,
		m_originalWidget, &hnOriginalScalePixShowWidget::slot_updatePix);
	connect(m_3dPixScrollWidget->getPixWidget(), &hn2dPixWidget::sig_mousePosImageChanged,
		m_originalWidget, &hnOriginalScalePixShowWidget::slot_updatePix);

	connect(m_pStreetViewWidget->getLeftPixWidget(), &hnStreetCameraView::sig_mousePosImageChanged,
		m_originalWidget, &hnOriginalScalePixShowWidget::slot_updatePix);
	connect(m_pStreetViewWidget->getRightPixWidget(), &hnStreetCameraView::sig_mousePosImageChanged,
		m_originalWidget, &hnOriginalScalePixShowWidget::slot_updatePix);

	//原始比例窗口大小变化，通知二维三维视图
	connect(m_originalWidget, &hnOriginalScalePixShowWidget::sig_widgetSizeChanged, [=](int w, int h) {
		m_3dPixScrollWidget->getPixWidget()->setOriginalWidgetWidthHeight(w, h);
		m_2dPixScrollWidget->getPixWidget()->setOriginalWidgetWidthHeight(w, h);
		m_pStreetViewWidget->getLeftPixWidget()->setOriginalWidgetWidthHeight(w, h);
		m_pStreetViewWidget->getRightPixWidget()->setOriginalWidgetWidthHeight(w, h);
	});




	//二维 对话框对比度调整  对应视图更新
	connect(m_adjustImageWidget, &adjustImageWidget::signal_2dContrastIntensityChanged, [this](double intensity)
	{
		m_2dPixScrollWidget->getPixWidget()->setContrastIntensity(intensity);
		m_2dPixScrollWidget->getPixWidget()->update();
	});

	//二维 对比度重置 对应视图更新
	connect(m_adjustImageWidget, &adjustImageWidget::signal_2dResetContrastIntensity, [this]() {
		m_2dPixScrollWidget->getPixWidget()->resetContrastIntensity();
		m_2dPixScrollWidget->getPixWidget()->update();
	});

	//三维 对话框对比度调整  对应视图更新
	connect(m_adjustImageWidget, &adjustImageWidget::signal_3dContrastIntensityChanged, [this](double intensity)
	{
		m_3dPixScrollWidget->getPixWidget()->setContrastIntensity(intensity);
		m_3dPixScrollWidget->getPixWidget()->update();
	});

	//三维 对比度重置 对应视图更新
	connect(m_adjustImageWidget, &adjustImageWidget::signal_3dResetContrastIntensity, [this]() {
		m_3dPixScrollWidget->getPixWidget()->resetContrastIntensity();
		m_3dPixScrollWidget->getPixWidget()->update();
	});

	//二维 对话框亮度调整  对应视图更新
	connect(m_adjustImageWidget, &adjustImageWidget::signal_2dBrightnessIntensityChanged, [this](double intensity)
	{
		m_2dPixScrollWidget->getPixWidget()->setBrightness(intensity);
		m_2dPixScrollWidget->getPixWidget()->update();
	});

	//二维 亮度重置 对应视图更新
	connect(m_adjustImageWidget, &adjustImageWidget::signal_2dResetBrightnessIntensity, [this]() {
		m_2dPixScrollWidget->getPixWidget()->resetBrightness();
		m_2dPixScrollWidget->getPixWidget()->update();
	});

	//三维 对话框亮度调整  对应视图更新
	connect(m_adjustImageWidget, &adjustImageWidget::signal_3dBrightnessIntensityChanged, [this](double intensity)
	{
		m_3dPixScrollWidget->getPixWidget()->setBrightness(intensity);
		m_3dPixScrollWidget->getPixWidget()->update();
	});

	//三维 亮度重置 对应视图更新
	connect(m_adjustImageWidget, &adjustImageWidget::signal_3dResetBrightnessIntensity, [this]() {
		m_3dPixScrollWidget->getPixWidget()->resetBrightness();
		m_3dPixScrollWidget->getPixWidget()->update();
	});

	//里程跳转对话框 发送跳转信号，二维视图跳转
	// todo三维跳转
	connect(m_regionJumpDlg, &hnRegionJumpDlg::signal_updateScrollValue,
		[this](double encoderMile) {
		const double targetMile = qMax(0.0, encoderMile);
		m_2dPixScrollWidget->getPixWidget()->scrollBottomToEncoderMile(targetMile);
		syncContinuousViews(ContinuousViewSyncSource::Road2D, targetMile);
	});

	connect(m_regionJumpDlg, &hnRegionJumpDlg::signal_road3dFrameIdxChanged,
		[this](double encoderMile) {
		const double targetMile = qMax(0.0, encoderMile);
		m_3dPixScrollWidget->getPixWidget()->scrollBottomToEncoderMile(targetMile);
		syncContinuousViews(ContinuousViewSyncSource::Road3D, targetMile);
	});

	connect(m_projectWidget, &projectView::signal_jumpToMile, this, &hnRoadDataProcess::slot_jumpToMile);
	connect(m_projectWidget, &projectView::signal_jumpToMark, this, &hnRoadDataProcess::slot_jumpToMark);

	// Old frame-index jump signals are no longer connected; slot_jumpToMile scrolls the SDK view directly.

	connect(this, &hnRoadDataProcess::signal_updateProject, this->m_projectWidget, &projectView::slot_updateProjectSetting);
	//镜像
	connect(m_projectConfgDialog, &hnProjectConfig::signal_Mirrored, this, &hnRoadDataProcess::slot_widgetMirrored);

	//深度计算设置
	connect(m_projectConfgDialog, &hnProjectConfig::signal_isDepthCaculate, this, &hnRoadDataProcess::slot_setDepthCaculate);

#ifdef 地图
	
		connect(this, &hnRoadDataProcess::signal_loadBaiduMap, m_mapWidget, &CustomBaiduMapView::signal_loadBaiDuMap);

#endif // 百度地图
	 

}

// 树状视图连接
void hnRoadDataProcess::createTreeConnect()
{

}

// 创建工程管理模块工具栏
void hnRoadDataProcess::createProCategory(hnRibbonCategory* page)
{
	//
	hnRibbonPannel* projectPanel = page->addPannel(QStringLiteral("工程管理"));

	{// 打开工程

		const QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"),
			QIcon(QStringLiteral(":/icons/iconsNew/导入工程.png")));
		m_openProjectAct = new QAction(projectIcon, QStringLiteral("&导入工程"), this);
		projectPanel->addLargeAction(m_openProjectAct);
	}

	{
		const QIcon openProjectDirIcon = QIcon::fromTheme(QStringLiteral("openProjectDirIcon"),
			QIcon(QStringLiteral(":/icons/iconsNew/打开工程文件夹.png")));
		m_openCurrentProjectDirAction = new QAction(openProjectDirIcon, QStringLiteral("&打开工程文件夹"), this);
		projectPanel->addLargeAction(m_openCurrentProjectDirAction);
	}
	{

		const QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"),
			QIcon(QStringLiteral(":/icons/iconsNew/最近工程.png")));
		m_lastProjectAct = new QAction(projectIcon, QStringLiteral("&最近工程"), this);
		projectPanel->addLargeAction(m_lastProjectAct);
	}
	{
		//导出简易工程
		const QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"),
			QIcon(QStringLiteral(":/icons/icons/识别CP3.png")));
		m_outSimpleProjectAction = new QAction(projectIcon, QStringLiteral("&导出简易工程"), this);
		projectPanel->addLargeAction(m_outSimpleProjectAction);
	}


	hnRibbonPannel* dataHandelPanel = page->addPannel(QStringLiteral("数据处理"));
	{
		//检查数据
		  QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"),
			QIcon(QStringLiteral(":/icons/icons/识别CP3.png")));
		m_checkProAct = new QAction(projectIcon, QStringLiteral("&检查数据"), this);
		dataHandelPanel->addLargeAction(m_checkProAct);

	}
	{

		  QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"),
			QIcon(QStringLiteral(":/icons/icons/识别CP3.png")));
		m_gpsMatchingAct = new QAction(projectIcon, QStringLiteral("&GPS桩号匹配"), this);
		dataHandelPanel->addLargeAction(m_gpsMatchingAct);
	}
	{// irm计算

	  QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"),
			QIcon(QStringLiteral(":/icons/iconsNew/IRM计算.png")));
		m_calculateAct = new QAction(projectIcon, QStringLiteral("&IRM计算"), this);
		dataHandelPanel->addLargeAction(m_calculateAct);
	}
	{
		// irm计算 
		QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"),
			QIcon(QStringLiteral(":/icons/iconsNew/IRM计算.png")));
		m_ComputeGeoaligAction = new QAction(projectIcon, QStringLiteral("&计算路面几何状况"), this);
		dataHandelPanel->addLargeAction(m_ComputeGeoaligAction);
	}
	{// irm计算

		  QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"),
			QIcon(QStringLiteral(":/icons/iconsNew/清空IRM.png")));
		m_clearIRMAct = new QAction(projectIcon, QStringLiteral("&清空IRM"), this);
		dataHandelPanel->addLargeAction(m_clearIRMAct);
	}

	

	hnRibbonPannel* projectChangePanel = page->addPannel(QStringLiteral("工程信息修改"));
	{
		//里程校准
		/*const QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"),
			QIcon(QStringLiteral(":/icons/iconsNew/里程校准.png")));
		m_mileCorrectAct = new QAction(projectIcon, QStringLiteral("&里程校准"), this);
		projectChangePanel->addLargeAction(m_mileCorrectAct);*/

	}
	{

		  QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"),
			QIcon(QStringLiteral(":/icons/iconsNew/采集打标.png")));
		m_markInfoAct = new QAction(projectIcon, QStringLiteral("&打标信息输出"), this);
		projectChangePanel->addLargeAction(m_markInfoAct);
	}
	{
		//二三维里程矫正
		  QIcon mileCorrectIcon = QIcon::fromTheme(QStringLiteral(""),
			QIcon(QStringLiteral(":/icons/iconsNew/里程校正.png")));
		this->m_2d3dMileCorrentAct = new QAction(mileCorrectIcon, QStringLiteral("二三维里程矫正"), this);
		connect(this->m_2d3dMileCorrentAct, &QAction::triggered, this, &hnRoadDataProcess::slot_changeTo23dMileCorrectMode);
		projectChangePanel->addLargeAction(m_2d3dMileCorrentAct);
	}

	{
		//修改工程有效桩号(绘制病害，出表桩号) 
		  QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"),
			QIcon(QStringLiteral(":/icons/icons/识别CP3.png")));
		m_changeProjectOutMileAct = new QAction(projectIcon, QStringLiteral("&多工程桩号设置"), this);
		projectChangePanel->addLargeAction(m_changeProjectOutMileAct);

	}


	hnRibbonPannel* dataStartHandelPanel = page->addPannel(QStringLiteral("数据预处理工具"));
	{
		//影像生成
		  QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"),
			QIcon(QStringLiteral(":/icons/iconsNew/影像生成.png")));
		m_createImageAct = new QAction(projectIcon, QStringLiteral("&影像生成"), this);
		m_createImageAct->setToolTip(QString::fromLocal8Bit("三维影像生成"));
		dataStartHandelPanel->addLargeAction(m_createImageAct);
	}

	//裁切图片
	  QIcon cutImageIcon = QIcon::fromTheme(QStringLiteral("cutImageIcon"),
		QIcon(QStringLiteral(":/icons/iconsNew/影像裁切.png")));
	m_cutImageAct = new QAction(cutImageIcon, QStringLiteral("&影像裁切"), this);
	m_cutImageAct->setToolTip(QString::fromLocal8Bit("二维影像裁切"));
	dataStartHandelPanel->addLargeAction(m_cutImageAct);


	{
		  QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"), QIcon(QStringLiteral(":/Resources/icons/检查数据.png")));
		m_clearProjectAct = new QAction(projectIcon, QStringLiteral("&清除工程"), this);
#if 0	//功能未实现 暂不启用
		projectPanel->addLargeAction(m_clearProjectAct);
#endif	
	}
	////里程跳转
	//const QIcon rigionJumpIcon = QIcon::fromTheme(QStringLiteral("rigionJumpIcon"), QIcon(QStringLiteral(":/icons/iconsNew/里程跳转.png")));
	//m_regionJumpAct = new QAction(rigionJumpIcon, QStringLiteral("&里程跳转"), this);
	//projectPanel->addLargeAction(m_regionJumpAct); 
	//connect(m_regionJumpAct, &QAction::triggered, this, &hnRoadDataProcess::slot_regionJump); 
}

// 创建数据处理模块工具栏
void hnRoadDataProcess::createDataProcessCategory(hnRibbonCategory* page)
{

	hnRibbonPannel* diseaseRibbonPannel = page->addPannel(QStringLiteral("病害管理"));
	//导入二维软件病害
	  QIcon  twoDDiseaseIcon = QIcon::fromTheme(QStringLiteral(""),
		QIcon(QStringLiteral(":/icons/iconsNew/导入病害.png")));
	this->m_input2dDiseaseAct = new QAction(twoDDiseaseIcon, QStringLiteral("导入二维软件病害"), this);
	connect(m_input2dDiseaseAct, &QAction::triggered, this, &hnRoadDataProcess::slot_import2dDiseases);
	diseaseRibbonPannel->addLargeAction(m_input2dDiseaseAct);

	//导出二维软件病害

	  QIcon  twoOutDiseaseIcon = QIcon::fromTheme(QStringLiteral(""),
		QIcon(QStringLiteral(":/icons/iconsNew/导入病害.png")));
	this->m_output2dDiseaseAct = new QAction(twoDDiseaseIcon, QStringLiteral("导出二维软件病害"), this);
	connect(m_output2dDiseaseAct, &QAction::triggered, this, &hnRoadDataProcess::slot_output2dDiseases);
	diseaseRibbonPannel->addLargeAction(m_output2dDiseaseAct);

	//导入自动识别病害
	  QIcon smartDiseaseIcon = QIcon::fromTheme(QStringLiteral(""),
		QIcon(QStringLiteral(":/icons/iconsNew/导入病害.png")));
	this->m_inputSmartDiseaseAct = new QAction(smartDiseaseIcon, QStringLiteral("导入自动识别病害"), this);
	connect(m_inputSmartDiseaseAct, &QAction::triggered, this, &hnRoadDataProcess::slot_importAidcDiseases);
	diseaseRibbonPannel->addLargeAction(m_inputSmartDiseaseAct);


	hnRibbonPannel* diseaseMgrRibbonPannel = page->addPannel(QStringLiteral("病害交互"));

	//添加病害
	  QIcon addDiseaseIcon = QIcon::fromTheme(QStringLiteral(""),
		QIcon(QStringLiteral(":/icons/iconsNew/添加病害.png")));
	this->m_addDiseaseAct = new QAction(addDiseaseIcon, QStringLiteral("添加病害(F1)"), this);
	m_addDiseaseAct->setToolTip(QStringLiteral("F1"));
	connect(m_addDiseaseAct, &QAction::triggered, this, &hnRoadDataProcess::slot_changeToAddDiseaseMode);
	diseaseMgrRibbonPannel->addLargeAction(m_addDiseaseAct);

	//删除病害
	  QIcon deleteDiseaseIcon = QIcon::fromTheme(QStringLiteral(""),
		QIcon(QStringLiteral(":/icons/iconsNew/删除病害.png")));
	this->m_deleteDiseaseAct = new QAction(deleteDiseaseIcon, QStringLiteral("删除病害(F2)"), this);
	m_deleteDiseaseAct->setToolTip(QStringLiteral("F2"));
	connect(m_deleteDiseaseAct, &QAction::triggered, this, &hnRoadDataProcess::slot_changeToDeleteDiseaseMode);
	diseaseMgrRibbonPannel->addLargeAction(m_deleteDiseaseAct);

	//编辑病害
	  QIcon editDiseaseIcon = QIcon::fromTheme(QStringLiteral(""),
		QIcon(QStringLiteral(":/icons/iconsNew/编辑病害.png")));

	this->m_editDiseaseAct = new QAction(editDiseaseIcon, QStringLiteral("编辑病害(F3)"), this);
	m_editDiseaseAct->setToolTip(QStringLiteral("F3"));

	connect(this->m_editDiseaseAct, &QAction::triggered, this, &hnRoadDataProcess::slot_changeToEditDiseaseMode);
	diseaseMgrRibbonPannel->addLargeAction(m_editDiseaseAct);

	//合并病害
	  QIcon combineDiseaseIcon = QIcon::fromTheme(QStringLiteral(""),
		QIcon(QStringLiteral(":/icons/iconsNew/合并病害.png")));
	this->m_combineDiseaseAct = new QAction(combineDiseaseIcon, QStringLiteral("合并病害"), this);
	connect(this->m_combineDiseaseAct, &QAction::triggered, this, &hnRoadDataProcess::slot_changeToMergeDiseaseMode);
	diseaseMgrRibbonPannel->addLargeAction(m_combineDiseaseAct);






	hnRibbonPannel* diseaseSjModeRibbonPannel = page->addPannel(QStringLiteral("设计模式病害处理"));
	//添加控制点
	  QIcon addCtrlPointIcon = QIcon::fromTheme(QStringLiteral(""),
		QIcon(QStringLiteral(":/icons/iconsNew/添加控制点.png")));
	m_addCtrlPointAct = new QAction(addCtrlPointIcon, QStringLiteral("添加控制点"), this);
	connect(m_addCtrlPointAct, &QAction::triggered, this, &hnRoadDataProcess::slot_changeAddCtrlPointMode);
	diseaseSjModeRibbonPannel->addLargeAction(m_addCtrlPointAct);

	// 添加面状病害
	  QIcon addFacetsIcon = QIcon::fromTheme(QStringLiteral(""),
		QIcon(QStringLiteral(":/icons/iconsNew/添加病害.png")));
	m_addFacetsDiseaseAct = new QAction(addFacetsIcon, QStringLiteral("添加面状病害"), this);
	diseaseSjModeRibbonPannel->addLargeAction(m_addFacetsDiseaseAct);
	connect(m_addFacetsDiseaseAct, &QAction::triggered, this, &hnRoadDataProcess::slot_addFacetsDiseaseMode);

	// 添加线状病害
	  QIcon addLineIcon = QIcon::fromTheme(QStringLiteral(""),
		QIcon(QStringLiteral(":/icons/iconsNew/添加病害.png")));
	m_addLineDiseaseAct = new QAction(addFacetsIcon, QStringLiteral("添加线状病害"), this);
	diseaseSjModeRibbonPannel->addLargeAction(m_addLineDiseaseAct);
	connect(m_addLineDiseaseAct, &QAction::triggered, this, &hnRoadDataProcess::slot_addLineDiseaseMode);

	// 导入控制点
	  QIcon importCtrlPointsIcon = QIcon::fromTheme(QStringLiteral(""),
		QIcon(QStringLiteral(":/icons/iconsNew/导入控制点.png")));
	this->m_importCtrlPointsAction = new QAction(importCtrlPointsIcon, QStringLiteral("导入控制点"), this);
	connect(m_importCtrlPointsAction, &QAction::triggered, this, &hnRoadDataProcess::slot_importCtrlPoints);
	diseaseSjModeRibbonPannel->addLargeAction(m_importCtrlPointsAction);

	// 导出控制点
	  QIcon exportCtrlPointsIcon = QIcon::fromTheme(QStringLiteral(""),
		QIcon(QStringLiteral(":/icons/iconsNew/导出控制点.png")));
	this->m_exportCtrlPointsAction = new QAction(exportCtrlPointsIcon, QStringLiteral("导出控制点"), this);
	connect(m_exportCtrlPointsAction, &QAction::triggered, this, &hnRoadDataProcess::slot_exportCtrlPoints);
	diseaseSjModeRibbonPannel->addLargeAction(m_exportCtrlPointsAction);
	{
		////检查数据
		//const QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"),
		//	QIcon(QStringLiteral(":/icons/icons/识别CP3.png")));
		//m_autoDiseaseMerge = new QAction(projectIcon, QStringLiteral("&自动病害拼接"), this);
		//diseaseMgrRibbonPannel->addLargeAction(m_autoDiseaseMerge);
	}




}

void hnRoadDataProcess::createDataBaseMgrCategory(hnRibbonCategory * page)
{
	hnRibbonPannel* dataBaseMgrPanel = page->addPannel(QStringLiteral("数据库管理"));
	{
		  QIcon dataBaseMgrIcon = QIcon::fromTheme("projectIcon", QIcon(QStringLiteral(":/icons/iconsNew/打开工程文件夹.png")));
		this->m_backupsDatabaseAct = new QAction(dataBaseMgrIcon, QStringLiteral("&管理数据库"), this);
		m_backupsDatabaseAct->setToolTip(QString::fromLocal8Bit("备份数据库，支持多人同时操作!"));
		dataBaseMgrPanel->addLargeAction(this->m_backupsDatabaseAct);
	}
	{
		  QIcon dataBaseMgrIcon = QIcon::fromTheme("projectIcon", QIcon(QStringLiteral(":/icons/iconsNew/清空病害.png")));
		this->m_clearAllDiseasesAct = new QAction(dataBaseMgrIcon, QStringLiteral("&清空病害"), this);
		m_clearAllDiseasesAct->setToolTip(QString::fromLocal8Bit("清空数据库中的所有病害，请谨慎操作"));
		dataBaseMgrPanel->addLargeAction(this->m_clearAllDiseasesAct);
	}


	/*const QIcon updateBaseMgrIcon = QIcon::fromTheme("projectIcon", QIcon(QStringLiteral(":/icons/iconsNew/编辑病害.png")));
	this->m_clearAllDiseasesAct = new QAction(updateBaseMgrIcon, QStringLiteral("&恢复已删除病害"), this);
	m_clearAllDiseasesAct->setToolTip(QString::fromLocal8Bit("恢复数据库中所有已删除人工病害，请谨慎操作"));
	dataBaseMgrPanel->addLargeAction(this->m_clearAllDiseasesAct);*/


	/*const QIcon  dataBaseMgrIcon1 = QIcon::fromTheme("projectIcon", QIcon(QStringLiteral(":/icons/iconsNew/更新病害.png")));
	this->m_updateAllDiseasesAct = new QAction(dataBaseMgrIcon1,QStringLiteral("&更新病害"), this);
	m_updateAllDiseasesAct->setToolTip(QString::fromLocal8Bit("将对数据路中病害几何信息重新计算，请谨慎操作"));
	dataBaseMgrPanel->addLargeAction(this->m_updateAllDiseasesAct);
*/

	  QIcon dataBaseMgrIcon0 = QIcon::fromTheme("projectIcon", QIcon(QStringLiteral(":/icons/icons/移除工程.png")));
	this->m_updateDatabase = new QAction(dataBaseMgrIcon0, QStringLiteral("&更新数据库"), this);
#if 0
	dataBaseMgrPanel->addLargeAction(this->m_updateDatabase);
#endif 
}

// 创建数据输出模块工具栏
void hnRoadDataProcess::createOutputCategory(hnRibbonCategory* page)
{
	hnRibbonPannel* exportResult = page->addPannel(QStringLiteral("报表输出"));


#ifdef DEBUG
	  QIcon projectDatasIcon = QIcon::fromTheme("projectIcon", QIcon(QStringLiteral(":/icons/iconsNew/输出报表.png")));
	m_allResultDatasAction = new QAction(projectDatasIcon, QStringLiteral("&导出定制报表"), this);
	exportResult->addLargeAction(m_allResultDatasAction);
#endif // DEBUG

	// 输出报表
	  QIcon projectIcon = QIcon::fromTheme("projectIcon", QIcon(QStringLiteral(":/icons/iconsNew/输出报表.png")));
	m_outExcel = new QAction(projectIcon, QStringLiteral("&输出标准报表"), this);
	exportResult->addLargeAction(m_outExcel);

	// 多工程报表合并
	  QIcon mergeProjectIcon = QIcon::fromTheme("projectIcon", QIcon(QStringLiteral(":/icons/iconsNew/输出报表.png")));
	m_mergeProjectExcelAct = new QAction(mergeProjectIcon, QStringLiteral("&多工程合并工具"), this);
	exportResult->addLargeAction(m_mergeProjectExcelAct);

	hnRibbonPannel* exportDxfResult = page->addPannel(QStringLiteral("DXF输出"));
	//导出dxf
	  QIcon exportDxfIcon = QIcon::fromTheme("exportDxfIcon", QIcon(QStringLiteral(":/icons/iconsNew/导出dxf.png")));
	m_exportDxfAction = new QAction(exportDxfIcon, QStringLiteral("&导出病害展布图"), this);
	exportDxfResult->addLargeAction(m_exportDxfAction);
	connect(m_exportDxfAction, &QAction::triggered, this, &hnRoadDataProcess::slot_exportDXf);

	  QIcon exportDiseaseDxfIcon = QIcon::fromTheme("exportDxfIcon", QIcon(QStringLiteral(":/icons/iconsNew/导出dxf.png")));
	m_exportDiseaseDxfAction = new QAction(exportDiseaseDxfIcon, QStringLiteral("&导出病害矢量图"), this);
	exportDxfResult->addLargeAction(m_exportDiseaseDxfAction);
	connect(m_exportDiseaseDxfAction, &QAction::triggered, this, &hnRoadDataProcess::slot_exportDiseaseDXf);

	  QIcon exportHighAccuracyDiseaseDxfIcon = QIcon::fromTheme("exportDxfIcon", QIcon(QStringLiteral(":/icons/iconsNew/导出dxf.png")));
	m_exportHighAccuracyDiseaseDxfAction = new QAction(exportHighAccuracyDiseaseDxfIcon, QStringLiteral("&导出高精度病害矢量图"), this);
	exportDxfResult->addLargeAction(m_exportHighAccuracyDiseaseDxfAction);
	connect(m_exportHighAccuracyDiseaseDxfAction, &QAction::triggered, this, &hnRoadDataProcess::slot_exportHighAccuracyDiseaseDXf);

	hnRibbonPannel* GJOutResult = page->addPannel(QStringLiteral("国检转换输出"));
	  QIcon gjIcon = QIcon::fromTheme("exportDxfIcon", QIcon(QStringLiteral(":/icons/iconsNew/输出报表.png")));
	m_exportGJDatasAction = new QAction(gjIcon, QStringLiteral("&计算国检转换中间数据"), this);
	GJOutResult->addLargeAction(m_exportGJDatasAction);
	connect(m_exportGJDatasAction, &QAction::triggered, this, &hnRoadDataProcess::slot_exportGjDatas);
}

// 创建点云处理模块工具栏
void hnRoadDataProcess::createCloudCategory(hnRibbonCategory* page)
{

}

void hnRoadDataProcess::createViewsCategory(hnRibbonCategory* page)
{
	hnRibbonPannel* widgetMgrPanel = page->addPannel(QStringLiteral("视图管理"));

	{// 视图

		  QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"), QIcon(QStringLiteral(":/icons/iconsNew/二维标准视图.png")));
		m_oShapeViewportAct = new QAction(projectIcon, QStringLiteral("&二维标准视图"), this);
		widgetMgrPanel->addLargeAction(m_oShapeViewportAct);
	}

	{// 视图

		  QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"), QIcon(QStringLiteral(":/icons/iconsNew/二三维标准视图.png")));
		m_o3ShapeViewportAct = new QAction(projectIcon, QStringLiteral("&三维标准视图"), this);
		widgetMgrPanel->addLargeAction(m_o3ShapeViewportAct);
	}
	{// 视图

		  QIcon projectIcon = QIcon::fromTheme(QStringLiteral("projectIcon"), QIcon(QStringLiteral(":/icons/iconsNew/二三维标准视图.png")));
		m_oDViewportAct = new QAction(projectIcon, QStringLiteral("&二三维标准视图"), this);
		widgetMgrPanel->addLargeAction(m_oDViewportAct);
	}

	 

	//三维视图浏览模式
	hnRibbonPannel* image3dModePannel = page->addPannel(QStringLiteral("三维视图浏览模式"));

	//灰度图模式Action
	  QIcon grayModeIcon = QIcon::fromTheme(QStringLiteral("grayModeIcon"), QIcon(QStringLiteral(":/icons/iconsNew/灰度图模式.png")));
	this->m_3dGrayModeAct = new QAction(grayModeIcon, QStringLiteral("&灰度图模式"));
	image3dModePannel->addLargeAction(m_3dGrayModeAct);

	//深度图模式Action
	  QIcon rgbModeIcon = QIcon::fromTheme(QStringLiteral("rgbModeIcon"), QIcon(QStringLiteral(":/icons/iconsNew/深度图模式.png")));
	this->m_3dRgbModeAct = new QAction(rgbModeIcon, QStringLiteral("&深度图模式"));
	image3dModePannel->addLargeAction(m_3dRgbModeAct);


}

// 系统
void hnRoadDataProcess::createSystemCategory(hnRibbonCategory* page)
{
	hnRibbonPannel* sysPannel = page->addPannel(QStringLiteral("系统"));

	// 菜单项，用于管理面板显示隐藏;
	  QIcon showPaneIcon = QIcon::fromTheme("projectIcon", QIcon(QStringLiteral(":/icons/iconsNew/二三维标准视图.png")));
	m_pShowPaneMenu = new QMenu(this);
	//m_pShowPaneMenu->setStyleSheet();
	m_pShowPaneMenu->setTitle(QStringLiteral("显隐面板"));
	m_pShowPaneMenu->setIcon(showPaneIcon);
	m_pShowPaneMenu->menuAction()->setStatusTip(QStringLiteral("显隐面板"));
	sysPannel->addLargeAction(m_pShowPaneMenu->menuAction());

	// 系统设置
	  QIcon configPaneIcon = QIcon::fromTheme("configIcon", QIcon(QStringLiteral(":/icons/iconsNew/软件设置.png")));
	m_config = new QAction(configPaneIcon, QStringLiteral("软件设置"));
	sysPannel->addLargeAction(m_config);

	// 系统设置
	  QIcon HelperPaneIcon = QIcon::fromTheme("configIcon", QIcon(QStringLiteral(":/icons/iconsNew/软件设置.png")));
	m_HelperAct = new QAction(HelperPaneIcon, QStringLiteral("使用说明"));
	sysPannel->addLargeAction(m_HelperAct);

	  QIcon AuoutInfoPaneIcon = QIcon::fromTheme("configIcon", QIcon(QStringLiteral(":/icons/iconsNew/软件设置.png")));
	m_AboutInfoAct = new QAction(AuoutInfoPaneIcon, QStringLiteral("关于信息"));
	sysPannel->addLargeAction(m_AboutInfoAct);


#if 0	//功能未实现，暂不启用
	sysPannel->addLargeAction(m_config);
#endif
}

bool hnRoadDataProcess::loadConfigData()
{
	QString configDbPath = QApplication::applicationDirPath() + "/config";

	QDir baseDirs(configDbPath);
	if (!baseDirs.exists())
	{ 
		QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("程序目录下缺少config文件夹！"),
			QStringLiteral("确定"));
		return false;
	}

	m_xrSetting = HnXRSettings::getInstance();

	QString baseConfigPath = QApplication::applicationDirPath() + "/config/XRSetting.ini";
	QStringList configTxts =  MyCommonMethods::ReadAllLines(baseConfigPath);
	QString configPath = MyCommonMethods::GetUserPath() + "//XRSetting.ini";

	if (!QFile::exists(configPath))
	{
		//如果不存在复制过去

		if (QFile::copy(baseConfigPath, configPath))
		{
			m_xrSetting->SetConfigFilePath(configPath);
			m_xrSetting->Init();
			m_xrSetting->readData();
		}
		else
		{
			QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("关键配置文件XRSetting.ini拷贝失败！"),
				QStringLiteral("确定"));
			return false;
		}
	}
	else
	{
		QStringList newConfigTxts = MyCommonMethods::ReadAllLines(configPath);
		if (configTxts.size() != newConfigTxts.size())
		{
			QFile::remove(configPath);
			if (QFile::copy(baseConfigPath, configPath))
			{
				m_xrSetting->SetConfigFilePath(configPath);
				m_xrSetting->Init();
				m_xrSetting->readData();
			}
		}
		else
		{
			m_xrSetting->SetConfigFilePath(configPath);
			m_xrSetting->Init();
			m_xrSetting->readData();
		}
	
	}
	//记录用户路径
	return true;
}




void hnRoadDataProcess::slot_dClickTreeItem(QTreeWidgetItem *item, int column)
{
	QElapsedTimer totalTimer;
	totalTimer.start();
	 
	QString selectedItemText = item->text(0);
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][TreeProjectOpenStart]"
		<< "time=" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
		<< "item=" << selectedItemText
		<< "column=" << column;
	#endif
	int grade = item->data(1, Qt::UserRole).value<int>();
	if (grade == 1)  //用户点击的是有效节点  二维工程名
	{
		#ifdef _DEBUG
		qDebug().noquote() << "[HN_PERF][TreeProjectOpenEnd]"
			<< "reason=grade1"
			<< "item=" << selectedItemText
			<< "totalMs=" << totalTimer.elapsed();
		#endif
		return;
	}

	QString projectName = selectedItemText;

	//检查工程 人工模式自动化模式类型冲突
	if (!this->checkProjectFrameTypeConflict(projectName))
	{
		#ifdef _DEBUG
		qDebug().noquote() << "[HN_PERF][TreeProjectOpenEnd]"
			<< "reason=frameTypeConflict"
			<< "item=" << selectedItemText
			<< "totalMs=" << totalTimer.elapsed();
		#endif
		return;
	}
	auto curProject = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	BusyLoadingGuard loading(this, QStringLiteral("打开加载具体工程"), QStringLiteral("正在打开工程，请稍后......"));
	if (curProject)
	{
		QElapsedTimer stepTimer;
		hnCommon::hnProjectSetInfo setting = curProject->getCurProSetInfo();
		// 清空所有视图图片
		stepTimer.start();
		this->clearAllWidgetPixs();
		#ifdef _DEBUG
		qDebug().noquote() << "[HN_PERF][TreeProjectOpenStep]" << "step=clearAllWidgetPixs" << "elapsedMs=" << stepTimer.elapsed();
		#endif
		loading.setMessage(QStringLiteral("所有窗口加载图片..."));
		//所有窗口加载图片
		stepTimer.restart();
		this->allWidgetLoadPictures();
		#ifdef _DEBUG
		qDebug().noquote() << "[HN_PERF][TreeProjectOpenStep]" << "step=allWidgetLoadPictures" << "elapsedMs=" << stepTimer.elapsed();
		#endif
		loading.setMessage(QStringLiteral("更新病害列表..."));

		//更新病害列表
		stepTimer.restart();
		this->m_diseaseListWidget->updateAllDiseases();
		#ifdef _DEBUG
		qDebug().noquote() << "[HN_PERF][TreeProjectOpenStep]" << "step=updateAllDiseases" << "elapsedMs=" << stepTimer.elapsed();
		#endif
		loading.setMessage(QStringLiteral("更新树状视图..."));
		//更新树状视图
		stepTimer.restart();
		this->updateTreeWidget();
		#ifdef _DEBUG
		qDebug().noquote() << "[HN_PERF][TreeProjectOpenStep]" << "step=updateTreeWidget" << "elapsedMs=" << stepTimer.elapsed();
		#endif
		QVector<hnCommon::hnMarkInfo> marks;
		if (m_projects->getCurrentProject()->getProjectType() == PROJECT_JD_3D_TYPE)
		{

		}
		else if (m_projects->getCurrentProject()->getProjectType() == PROJECT_TYPE::PROJECT_XD_3D_TYPE)
		{

		}
		else
		{
			marks = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentMarkVector();
		}
		QVector<hnCommon::hnMilePile> datas = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentMilePileVector();
		if (2 == hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType)
		{
			
			slot_addLineDiseaseMode();
		}
		else
		{
			slot_changeToAddDiseaseMode();

		}

		emit signal_updateProject(setting, marks, datas);
		emit signal_loadBaiduMap();
		#ifdef _DEBUG
		qDebug().noquote() << "[HN_PERF][TreeProjectOpenStep]" << "step=emitUpdateSignals" << "totalMsSoFar=" << totalTimer.elapsed();
		#endif
		auto type = curProject->getProjectType();

		//获取二三维里程差值，如果是0，提示用户做差值处理


		if (PROJECT_23D_TYPE == type)
		{
			const double diff2d3d = curProject->get2d3dMileDiff();
			if (0 == diff2d3d)
			{
				QMessageBox::warning(this, QString::fromLocal8Bit("警告"),
					QString::fromLocal8Bit("二三维里程差值没有进行矫正，如果需要，请进行矫正"),
					QString::fromLocal8Bit("确定"));
			}
		}
	}
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][TreeProjectOpenEnd]"
		<< "item=" << selectedItemText
		<< "hasProject=" << (curProject != nullptr)
		<< "totalMs=" << totalTimer.elapsed();
	#endif
}

void hnRoadDataProcess::slot_selectNodeChange()
{
	//QBrush brush(Qt::blue);
}

void hnRoadDataProcess::slot_showContextMenu(const QPoint pos)
{
	//获取鼠标点击位置处的item
	QTreeWidgetItem *item = nullptr;
	item = this->m_projectListTreeWidget->itemAt(pos);

	//没有item的话就返回  右键菜单m_treeWidgetRightButtonMenu
	if (!item)
	{
		return;
	}

}

void hnRoadDataProcess::slot_toStandard2View()
{
	setLayout(PROJECT_2D_TYPE);
}

void hnRoadDataProcess::slot_toStandard23View()
{
	setLayout(PROJECT_23D_TYPE);
}

void hnRoadDataProcess::slot_toStandard3View()
{
	setLayout(PROJECT_JD_3D_TYPE);
}

void hnRoadDataProcess::slot_calculateIrm()
{
	if (!m_projects->isHasProject())
	{
		return;
	}
	//判断磁盘空间

	calculateIrmForm* form = new calculateIrmForm(this);
	connect(form, &calculateIrmForm::error, this, [=](QString msg)
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), msg,
			QString::fromLocal8Bit("确定"));
	}
	);

	connect(form, &calculateIrmForm::calculationFinished, this, [=]()
	{
		QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("完成"),
			QString::fromLocal8Bit("确定"));
	}
	);
	form->exec();

	//添加
}
void hnRoadDataProcess::slot_compute()
{

	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	PROJECT_TYPE type = 	hnDataManager::getDataManager()->getCurrentProject()->getProjectType();
	if (type != PROJECT_TYPE::PROJECT_23D_TYPE&&
		type != PROJECT_TYPE::PROJECT_JD_3D_TYPE&&
		type != PROJECT_TYPE::PROJECT_XD_3D_TYPE)
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("该功能仅支持二三维工程使用!"), QString::fromLocal8Bit("确定"));
		 
		return;
	}
	auto* curProject = hnDataManager::getDataManager()->getCurrentProject();
	hnCalRoadGeometry calRoadGeo; 
	double dBaseRoll = 0.0;
	double dBasePitch = 0.0;
	QString basePath = curProject->get3DProPath();
	QDir dir(basePath);
	dir.cdUp();
	QString parentPath = dir.absolutePath();
	QString sep = QDir::separator();
	QString strPos = parentPath + sep + "POS" + sep + "IE" + sep ;
	QDir posDir(strPos);
	QStringList filters; 
	filters << "*.pos";
	
	posDir.setNameFilters(filters);
	posDir.setFilter(QDir::Files | QDir::NoSymLinks);
	QFileInfoList fileLst =  posDir.entryInfoList();
	if (fileLst.isEmpty())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), strPos + QString::fromLocal8Bit("\nPOS文件不存在，请检查!"), QString::fromLocal8Bit("确定"));

		return;
	}
	QFileInfo file = fileLst.first(); 
	string tempPath = file .absoluteFilePath().toLocal8Bit();
	calRoadGeo.setBaseParam(dBaseRoll, dBasePitch, tempPath. c_str());
	vector<hnRoadGeoParam> vecRoadGeoParam;
	QVector<hnMile> curMiles = 	curProject->getCurrentMileVector();
	double  roadLength = curProject->getCurProSetInfo().dEndEnclMile;
	int dist = 1;
	for (int i = 0; i <= roadLength; i+= dist)
	{
		hnRoadGeoParam roadGeoParam;
		roadGeoParam.dMileage = i;
		vecRoadGeoParam.push_back(roadGeoParam);
	} 
	QProgressDialog pd(QString::fromLocal8Bit("正在读取空间地理信息..."), QString::fromLocal8Bit("取消"), 0, 100, this);
	pd.setWindowModality(Qt::WindowModal);
	pd.show();
	g_pgDlg = &pd;
	calRoadGeo.calRoadGeometeryNew1(vecRoadGeoParam, dist, progressCallback);
	g_pgDlg = nullptr;
	pd.close();
	
	QString outPath;
	if (type== PROJECT_23D_TYPE )
	{
		outPath = QString("%1/Geoalig_%2m.txt").arg(curProject->get2DProPath()).arg(QString::number(dist));
	}
	else
	{
		outPath = QString("%1/Geoalig_%2m.txt").arg(curProject->get3DProPath()).arg(QString::number(dist));
	}
	QFile gps2MileFile(outPath);

	if (!gps2MileFile.open(QIODevice::WriteOnly | QIODevice::Text)) return ;
	QTextStream gps2MileOut(&gps2MileFile);
	int  dmi = 0;
	for (const hnRoadGeoParam& item : vecRoadGeoParam)
	{
		QString line = QString::number(dmi) + "," + QString::number(item.dC) + "," + QString::number(item.dVAngle) + ","
			+ QString::number(item.dHAngle);
			gps2MileOut << line << "\n";
			dmi += 10;
	}
	gps2MileFile.close();
	QMessageBox::information(this, QStringLiteral("提示窗口"), QStringLiteral("几何线型数据计算完毕，可进行报表输出！"),
		QString::fromLocal8Bit("确定"));
}
void hnRoadDataProcess::slot_clearIrm()
{
	if (!m_projects->isHasProject())
	{
		return;
	}
	hnPro::hnProjectManager* manager = hnApp::hnDataManager::getDataManager()->getProjectManager();
	auto projects = manager->getAllBaseProject();
	for (auto project : projects)
	{
		//清空平整度 
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\DAQ0\\IRI_10m.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\DAQ0\\IRI_20m.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\DAQ0\\IRI_100m.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\DAQ0\\IRI_1000m.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\DAQ0\\PavementBump.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\DAQ0\\ReSample250.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\DAQ0\\Speed_10m.txt");

		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\DAQ1\\IRI_10m.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\DAQ1\\IRI_20m.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\DAQ1\\IRI_100m.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\DAQ1\\IRI_1000m.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\DAQ1\\PavementBump.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\DAQ1\\ReSample250.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\DAQ1\\Speed_10m.txt");
		//清空车辙
		//20240905发现内业车辙算法由于精度问题 无法与外业统一  先采用外业车辙结果
		MyCommonMethods::deleteDirectory(project->get2DProject()->getRutResultPath());

		//清空mtd
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\Laser0\\MTD_10m.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\Laser1\\MTD_10m.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\Laser2\\MTD_10m.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\Laser0\\MPD_10m.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\Laser1\\MPD_10m.txt");
		MyCommonMethods::deleteFile(project->get2DProject()->getIRIPath() + "\\Laser2\\MPD_10m.txt");

		//几何线性
		MyCommonMethods::deleteFile(project->get2DProject()->getBasePath() + "\\camera0\\imu.hon.csv");
		MyCommonMethods::deleteFile(project->get2DProject()->getBasePath() + "\\camera0\\imu.hon.CrossSlope");
		MyCommonMethods::deleteFile(project->get2DProject()->getBasePath() + "\\camera0\\imu.hon.Curvature");
		MyCommonMethods::deleteFile(project->get2DProject()->getBasePath() + "\\camera0\\imu.hon.HeightSlope");
	}

	QMessageBox::information(this, QStringLiteral("提示窗口"), QStringLiteral("IRM数据清空完成,请进行下一步计算！"),
		QString::fromLocal8Bit("确定"));
}

void hnRoadDataProcess::slot_changeToAddDiseaseMode()
{

	if (nullptr == hnDataManager::getDataManager()->getCurrentProject())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("当前未打开工程"), QString::fromLocal8Bit("确定"));
		return;
	}

	if (2 == hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType)
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("当前模式为设计模式，请使用添加面状病害或者添加线状病害"), QString::fromLocal8Bit("确定"));
		return;
	}

	m_centerToast->showMessage(QStringLiteral("进入绘制病害模式"), QStringLiteral("左键绘制病害区域,右键退出当前绘制", 2000));

	//路面破损窗口设置为画病害模式
	this->m_2dPixScrollWidget->getPixWidget()->setAddDiseaseMode();

	//三维窗口设置为画病害模式
	this->m_3dPixScrollWidget->getPixWidget()->setAddDiseaseMode();

	//取消画病害
	this->m_3dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();
	this->m_2dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();

	//景观窗口设置为添加病害模式
	this->m_pStreetViewWidget->m_pStreetView->setAddDiseaseMode();
	if (this->m_pStreetViewWidget->m_pStreetViewDouble)
	{
		this->m_pStreetViewWidget->m_pStreetViewDouble->setAddDiseaseMode();

	}

}

void hnRoadDataProcess::slot_changeToDeleteDiseaseMode()
{
	m_centerToast->showMessage(QStringLiteral("进入删除病害模式"), QStringLiteral("左键点击病害删除", 2000));

	//取消画病害
	this->m_3dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();
	this->m_2dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();

	//路面破损窗口设置为删除病害模式
	this->m_2dPixScrollWidget->getPixWidget()->setDeleteDiseaseMode();

	//三维窗口设置为删除病害模式
	this->m_3dPixScrollWidget->getPixWidget()->setDeleteDiseaseMode();

	//景观删除病害模式
	this->m_pStreetViewWidget->m_pStreetView->setDeleteDiseaseMode();
	if (this->m_pStreetViewWidget->m_pStreetViewDouble)
	{
		this->m_pStreetViewWidget->m_pStreetViewDouble->setDeleteDiseaseMode();

	}
}

void hnRoadDataProcess::slot_changeToEditDiseaseMode()
{
	m_centerToast->showMessage(QStringLiteral("进入编辑病害模式"), QStringLiteral("左键点击病害区域,右键退出当前编辑", 2000));
	//取消画病害
	this->m_3dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();
	this->m_2dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();

	//路面破损窗口设置为编辑病害模式
	this->m_2dPixScrollWidget->getPixWidget()->setEditMode();

	//三维窗口设置为编辑病害模式
	this->m_3dPixScrollWidget->getPixWidget()->setEditMode();
}

void hnRoadDataProcess::slot_changeToMoveDiseaseMode()
{
	//取消画病害
	this->m_3dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();
	this->m_2dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();

	//路面破损窗口设置为移动病害模式
	this->m_2dPixScrollWidget->getPixWidget()->setMoveMode();

	//三维窗口设置为移动病害模式
	this->m_3dPixScrollWidget->getPixWidget()->setMoveMode();
}

void hnRoadDataProcess::slot_changeToMergeDiseaseMode()
{
	//取消画病害
    this->m_3dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();
    this->m_2dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();

    //路面破损窗口设置模式
    this->m_2dPixScrollWidget->getPixWidget()->setMode(hnWorkMode::MERGE);

    //三维窗口设置模式
    this->m_3dPixScrollWidget->getPixWidget()->setMode(hnWorkMode::MERGE);
}

void hnRoadDataProcess::slot_changeTo23dMileCorrectMode()
{
    if (!m_projects->isOpenProject())
    {
        return;
    }

	//取消画病害
    this->m_3dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();
    this->m_2dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();
    this->m_2dPixScrollWidget->getPixWidget()->claerSelectPoint();
    this->m_3dPixScrollWidget->getPixWidget()->claerSelectPoint();
    this->m_2dPixScrollWidget->getPixWidget()->refreshSdkViewState();
    this->m_3dPixScrollWidget->getPixWidget()->refreshSdkViewState();

    //路面破损窗口设置模式
    this->m_2dPixScrollWidget->getPixWidget()->setMode(hnWorkMode::GET_MILE);

    //三维窗口设置模式
    this->m_3dPixScrollWidget->getPixWidget()->setMode(hnWorkMode::GET_MILE);

    QMessageBox::information(this, QStringLiteral("提示"),
		QStringLiteral("请依次点击二维视图、三维视图上相同的位置，然后键盘按Shift + C，进行矫正"),
        QString::fromLocal8Bit("确定"));
}

void hnRoadDataProcess::slot_changeAddCtrlPointMode()
{
	//取消画病害
	this->m_3dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();
	this->m_2dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();

	//路面破损窗口设置模式
	this->m_2dPixScrollWidget->getPixWidget()->setMode(hnWorkMode::NO_MODE);

	//三维窗口设置模式
	this->m_3dPixScrollWidget->getPixWidget()->setMode(hnWorkMode::ADD_CTRL_POINT);

	//景观删除病害模式
	this->m_pStreetViewWidget->m_pStreetView->setMode(hnWorkMode::NO_MODE);

	if (this->m_pStreetViewWidget->m_pStreetViewDouble)
	{
		this->m_pStreetViewWidget->m_pStreetViewDouble->setMode(hnWorkMode::NO_MODE);

	}
}


void hnRoadDataProcess::slot_addFacetsDiseaseMode()
{
	if (nullptr == hnDataManager::getDataManager()->getCurrentProject())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("当前未打开工程"), QString::fromLocal8Bit("确定"));
		return;
	}
	if (2 != hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType)
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("当前模式不是设计模式，请使用添加病害"), QString::fromLocal8Bit("确定"));
		return;
	}
	// 路面破损窗口设置为面状病害模式
	this->m_2dPixScrollWidget->getPixWidget()->setAddDiseaseMode();
	this->m_2dPixScrollWidget->getPixWidget()->setDesignFacetsMode();

	//三维窗口设置为画病害模式
	this->m_3dPixScrollWidget->getPixWidget()->setAddDiseaseMode();
	this->m_3dPixScrollWidget->getPixWidget()->setDesignFacetsMode();

	//取消画病害
	this->m_3dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();
	this->m_2dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();

	//景观窗口设置为添加病害模式
	this->m_pStreetViewWidget->m_pStreetView->setAddDiseaseMode();
	if (this->m_pStreetViewWidget->m_pStreetViewDouble)
	this->m_pStreetViewWidget->m_pStreetViewDouble->setAddDiseaseMode();
}

void hnRoadDataProcess::slot_addLineDiseaseMode()
{
	if (nullptr == hnDataManager::getDataManager()->getCurrentProject())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("当前未打开工程"), QString::fromLocal8Bit("确定"));
		return;
	}
	if (2 != hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType)
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("当前模式不是设计模式，请使用添加病害"), QString::fromLocal8Bit("确定"));
		return;
	}

	// 路面破损窗口设置为面状病害模式
	this->m_2dPixScrollWidget->getPixWidget()->setAddDiseaseMode();
	this->m_2dPixScrollWidget->getPixWidget()->setDesignLineMode();

	//三维窗口设置为画病害模式
	this->m_3dPixScrollWidget->getPixWidget()->setAddDiseaseMode();
	this->m_3dPixScrollWidget->getPixWidget()->setDesignLineMode();

	//取消画病害
	this->m_3dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();
	this->m_2dPixScrollWidget->getPixWidget()->slot_cancelDrawDiseases();

	//景观窗口设置为添加病害模式
	this->m_pStreetViewWidget->m_pStreetView->setAddDiseaseMode();
	if (this->m_pStreetViewWidget->m_pStreetViewDouble)
	this->m_pStreetViewWidget->m_pStreetViewDouble->setAddDiseaseMode();
}

void hnRoadDataProcess::slot_import2dDiseases()
{
	//异常处理
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("请打开工程"),
			QString::fromLocal8Bit("确定"));
		return;
	}

	//获取框选类型
	int frameType = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType;

	PROJECT_TYPE projectType = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType();



	if (2 == frameType)
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("设计模式不支持病害导入"), QString::fromLocal8Bit("确定"));
		return;
	}

	if (projectType != PROJECT_TYPE::PROJECT_2D_TYPE)
	{
		//二三维工程由于起始存在里程的原因 即 打标和较桩必须减去起始里程 导致二三维中的二维工程的较桩和打标无法一致
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("该功能仅支持导入二维工程病害"), QString::fromLocal8Bit("确定"));
		return;
	}



	//提示用户是否继续
	QString frameTypeQString = frameType == 0 ? QString::fromLocal8Bit("人工模式") : QString::fromLocal8Bit("自动化模式");
	auto reply = QMessageBox::question(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("您即将进行%1二维软件病害的病害导入，"
		"注意！重复使用会导致添加多个一样的病害；此过程不可逆，请做好备份工作，是否继续？").arg(frameTypeQString),
		QString::fromLocal8Bit("是"), QString::fromLocal8Bit("否"));
	if (1 == reply)
	{
		return;
	}

	// 设置标题和默认值
	QString frameTypeTitle = frameType == 0 ? QString::fromLocal8Bit("导入人工模式病害") : QString::fromLocal8Bit("导入自动化模式病害");
	this->m_mergeLittleFrameDlg->setWindowTitle(frameTypeTitle);
	if (frameType == 0)
	{
		this->m_mergeLittleFrameDlg->ui.vThreshold->setValue(300);
		this->m_mergeLittleFrameDlg->ui.hThreshold->setValue(400);
	}
	else if (frameType == 1)
	{
		this->m_mergeLittleFrameDlg->ui.vThreshold->setValue(700);
		this->m_mergeLittleFrameDlg->ui.hThreshold->setValue(700);
	}
	if (this->m_mergeLittleFrameDlg->exec() == QDialog::Rejected)
	{
		return;
	}
	hnApp::hnDataManager::getDataManager()->vMergeLittleFrameThr = this->m_mergeLittleFrameDlg->vDisThreshold();
	hnApp::hnDataManager::getDataManager()->hMergeLittleFrameThr = this->m_mergeLittleFrameDlg->hDisThreshold();
	bool isMerge = this->m_mergeLittleFrameDlg->importMergeFlag();
	bool isMap = this->m_mergeLittleFrameDlg->importMapFlag();

	//定义导出对象
	//hnImportAidcDiseases importDisease(frameType,this);
	hnImportAidcDiseases importDisease(frameType, isMerge, isMap, this);

	//执行导出操作
	importDisease.import2dDisease();

	//更新所有视图
	this->updateAllWidget();
}

void hnRoadDataProcess::slot_output2dDiseases()
{
	//异常处理
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("请打开工程"),
			QString::fromLocal8Bit("确定"));
		return;
	}


	hnPro::hnProject* curProjet = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	QString standard = HnProjectEnums::roadTypeEnumToQString(curProjet->getBaseStandard());
	//获取框选类型
	int frameType = curProjet->getCurProSetInfo().nDrawType;
	PROJECT_TYPE projectType = curProjet->getProjectType();

	if (2 == frameType || frameType == 0)
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("该模式不支持病害导出"), QString::fromLocal8Bit("确定"));
		return;
	}

	if (projectType != PROJECT_TYPE::PROJECT_2D_TYPE && projectType != PROJECT_TYPE::PROJECT_23D_TYPE)
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("该功能仅支持导入二维工程病害"), QString::fromLocal8Bit("确定"));
		return;
	}

	//获取所有病害

	QVector<hnRoadDiseaseInfo> allDiseaseInfos;

	QVector<hnMile> miles = curProjet->getCurrentMileVector();

	if (frameType == 1)
	{
	 
		allDiseaseInfos = hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllRoadDiseases();

	}

}

void hnRoadDataProcess::slot_importAidcDiseases()
{
	//异常处理
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("请打开工程"),
			QString::fromLocal8Bit("确定"));
		return;
	}

	//获取框选类型
	int frameType = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType;
	if (2 == frameType)
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("设计模式不支持病害导入"), QString::fromLocal8Bit("确定"));
		return;
	}



	//提示用户是否继续
	QString frameTypeQString = frameType == 0 ? QString::fromLocal8Bit("人工模式") : QString::fromLocal8Bit("自动化模式");
	auto reply = QMessageBox::question(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("您即将进行%1病害的病害导入，"
		"此过程不可逆，请做好备份工作，是否继续？").arg(frameTypeQString),
		QString::fromLocal8Bit("是"), QString::fromLocal8Bit("否"));
	if (1 == reply)
	{
		return;
	}

	// 设置标题和默认值
	QString frameTypeTitle = frameType == 0 ? QString::fromLocal8Bit("导入人工模式病害") : QString::fromLocal8Bit("导入自动化模式病害");
	this->m_mergeLittleFrameDlg->setWindowTitle(frameTypeTitle);
	if (frameType == 0)
	{
		this->m_mergeLittleFrameDlg->ui.vThreshold->setValue(300);
		this->m_mergeLittleFrameDlg->ui.hThreshold->setValue(400);
	}
	else if (frameType == 1)
	{
		this->m_mergeLittleFrameDlg->ui.vThreshold->setValue(700);
		this->m_mergeLittleFrameDlg->ui.hThreshold->setValue(700);
	}
	if (this->m_mergeLittleFrameDlg->exec() == QDialog::Rejected)
	{
		return;
	}
	hnApp::hnDataManager::getDataManager()->vMergeLittleFrameThr = this->m_mergeLittleFrameDlg->vDisThreshold();
	hnApp::hnDataManager::getDataManager()->hMergeLittleFrameThr = this->m_mergeLittleFrameDlg->hDisThreshold();
	bool isMerge = this->m_mergeLittleFrameDlg->importMergeFlag();
	bool isMap = this->m_mergeLittleFrameDlg->importMapFlag();

	//定义导出对象
	//hnImportAidcDiseases importDisease(frameType,this);
	hnImportAidcDiseases importDisease(frameType, isMerge, isMap, this);

	//执行导出操作
	importDisease.import();

	//更新所有视图
	this->updateAllWidget();
}

void hnRoadDataProcess::slot_changeGray3dMode()
{
	this->m_3dPixScrollWidget->getPixWidget()->setGrayMode();
	this->m_3dPixScrollWidget->laodPicRetainScrollBarValue();
	this->m_3dPixScrollWidget->getPixWidget()->update();
}

void hnRoadDataProcess::slot_changeRgb3dMode()
{
	this->m_3dPixScrollWidget->getPixWidget()->setRgbMode();
	this->m_3dPixScrollWidget->laodPicRetainScrollBarValue();
	this->m_3dPixScrollWidget->getPixWidget()->update();
}

void hnRoadDataProcess::slot_cutImage()
{
	QProcess process;

	//获取裁切软件绝对路径
	QString cutImageSoftName = QApplication::applicationDirPath() + "/cutSoft/PreProcess.exe";
	QStringList arguments;

	//启动程序  非阻塞的方式
	process.startDetached(cutImageSoftName, arguments);
}

 

bool hnRoadDataProcess::UTCT2GPST(const DATE_TIME_INFO& stTime, int& nGpsWeek, double& dGpsSeconds, double dGPSSubUTC /*= 0*/)
{
	int dayofw(0), dayofy(0), yr(0), ttlday(0), m(0), weekno(0);
	const  int  dinmth[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	//  Convert day, month and year to day of year 
	if (stTime.month == 1)
	{
		dayofy = stTime.day;
	}
	else
	{
		dayofy = 0;
		for (m = 1; m <= (stTime.month - 1); m++)
		{
			dayofy += dinmth[m];
			if (m == 2)
			{
				if (stTime.year % 4 == 0 && stTime.year % 100 != 0 || stTime.year % 400 == 0)
				{
					dayofy += 1;
				}
			}
		}
		dayofy += stTime.day;
	}
	//  Convert day of year and year into week number and day of week 
	ttlday = 360;
	for (yr = 1981; yr <= (stTime.year - 1); yr++)
	{
		ttlday += 365;
		if (yr % 4 == 0 && yr % 100 != 0 || yr % 400 == 0)
		{
			ttlday += 1;
		}
	}
	ttlday += dayofy;
	weekno = ttlday / 7;
	dayofw = ttlday - 7 * weekno;

	nGpsWeek = weekno;
	dGpsSeconds = dayofw * 86400.0 + stTime.hour * 3600.0 + stTime.minute * 60.0 + stTime.second + stTime.milliseconds / 1000000.;

	dGpsSeconds += dGPSSubUTC;
	if (dGpsSeconds > 7 * 24 * 3600)
	{
		dGpsSeconds -= 7 * 24 * 3600;
		nGpsWeek += 1;
	}

	return true;
}

void hnRoadDataProcess::slot_backupsDatabase()
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject()) {
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("请先打开工程!"),
			QString::fromLocal8Bit("确定"));
		return;

	}
	hnDBManagerDlg dlg;
	dlg.exec();
}

void hnRoadDataProcess::slot_clearAllDiseases()
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("请先打开工程!"),
			QString::fromLocal8Bit("确定"));
		return;
	}

	int ret = QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("是否删除所有病害与控制点？删除后病害无法恢复！")
		, QString::fromLocal8Bit("是"), QString::fromLocal8Bit("否"));

	if (0 == ret)
	{
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteAllDiseases();
		hnApp::hnDataManager::getDataManager()->getCurrentProject()->getDB()->getCtrlPointTable()->deleteAllData();
		//更新所有视图
		//this->updateAllWidget();
	}
	else
	{
		return;
	}

}

void hnRoadDataProcess::slot_updateDatabase()
{
	if (!m_projects->isOpenProject())
	{
		return;
	}
	m_projects->getCurrentProject()->updataMarkDatabase();
	m_projects->getCurrentProject()->updataMilePileDatabase();
}

void hnRoadDataProcess::slot_updateDiseaseDatabase()
{

}

bool hnRoadDataProcess::canUseContinuousViewSync() const
{
	const auto dataManager = hnDataManager::getDataManager();
	if (!dataManager || !dataManager->isOpenProject())
	{
		return false;
	}

	auto project = dataManager->getCurrentProject();
	if (!project || !project->get2DProject() || !project->get3DProject())
	{
		return false;
	}

	return m_2dPixScrollWidget && m_2dPixScrollWidget->getPixWidget()
		&& m_3dPixScrollWidget && m_3dPixScrollWidget->getPixWidget();
}

void hnRoadDataProcess::syncStreetViewBy2dEncoderMile(double encoderMile)
{
	if (!hnDataManager::getDataManager()->isOpenProject() || !m_pStreetViewWidget)
	{
		return;
	}

	auto project = hnDataManager::getDataManager()->getCurrentProject();
	if (!project || !project->get2DProject())
	{
		return;
	}

	// 景观控件现有接口收的是“行驶距离整数”，内部再按 StreetDis/StreetDis2 算图片序号。
    {
        QSignalBlocker blocker(m_pStreetViewWidget);
        m_pStreetViewWidget->updateViewImage(qMax(0, qRound(encoderMile)));
    }

    const QString streetImagePath = m_pStreetViewWidget->currentStreetImagePath();
    m_2dPixScrollWidget->getPixWidget()->setCurrentStreetPictureNameForStatus(streetImagePath);
    if (m_3dPixScrollWidget && m_3dPixScrollWidget->getPixWidget())
    {
        m_3dPixScrollWidget->getPixWidget()->setCurrentStreetPictureNameForStatus(streetImagePath);
    }
}

void hnRoadDataProcess::syncContinuousViews(ContinuousViewSyncSource source, double sourceEncoderMile)
{
	if (m_isProgrammaticViewSync)
	{
		return;
	}

	const auto dataManager = hnDataManager::getDataManager();
	if (!dataManager || !dataManager->isOpenProject())
	{
		return;
	}

	auto project = dataManager->getCurrentProject();
	if (!project)
	{
		return;
	}

	const bool has2dView = project->get2DProject()
		&& m_2dPixScrollWidget && m_2dPixScrollWidget->getPixWidget();
	const bool has3dView = project->get3DProject()
		&& m_3dPixScrollWidget && m_3dPixScrollWidget->getPixWidget();
	const bool canSync23d = has2dView && has3dView;
	const double sourceMile = qMax(0.0, sourceEncoderMile);
	const double syncToleranceMeters = 0.01;
	auto scrollBottomIfNeeded = [syncToleranceMeters](hn2d3dPixBaseWidget* pixWidget, double targetMile)
	{
		if (!pixWidget)
		{
			return;
		}

		const double clampedTargetMile = qMax(0.0, targetMile);
		if (qAbs(pixWidget->currentBottomEncoderMile() - clampedTargetMile) >= syncToleranceMeters)
		{
			pixWidget->scrollBottomToEncoderMile(clampedTargetMile);
		}
		pixWidget->refreshSdkViewState();
	};

	QScopedValueRollback<bool> syncingGuard(m_isProgrammaticViewSync, true);

	if (source == ContinuousViewSyncSource::Road2D)
	{
		if (canSync23d)
		{
			const double diff2d3d = project->get2d3dMileDiff();
			const double target3dMile = qMax(0.0, sourceMile - diff2d3d);
			scrollBottomIfNeeded(m_3dPixScrollWidget->getPixWidget(), target3dMile);
		}

		if (has2dView)
		{
			syncStreetViewBy2dEncoderMile(sourceMile);
			m_2dPixScrollWidget->getPixWidget()->refreshSdkViewState();
		}
		return;
	}

	if (source == ContinuousViewSyncSource::Road3D)
	{
		if (canSync23d)
		{
			const double diff2d3d = project->get2d3dMileDiff();
			const double target2dMile = qMax(0.0, sourceMile + diff2d3d);
			scrollBottomIfNeeded(m_2dPixScrollWidget->getPixWidget(), target2dMile);
			syncStreetViewBy2dEncoderMile(target2dMile);
			m_3dPixScrollWidget->getPixWidget()->refreshSdkViewState();
		}
		else if (has3dView)
		{
			m_3dPixScrollWidget->getPixWidget()->refreshSdkViewState();
		}
		return;
	}

	if (source == ContinuousViewSyncSource::Street)
	{
		if (has2dView)
		{
			scrollBottomIfNeeded(m_2dPixScrollWidget->getPixWidget(), sourceMile);
		}

		if (canSync23d)
		{
			const double diff2d3d = project->get2d3dMileDiff();
			const double target3dMile = qMax(0.0, sourceMile - diff2d3d);
			scrollBottomIfNeeded(m_3dPixScrollWidget->getPixWidget(), target3dMile);
		}

		const QString streetImagePath = m_pStreetViewWidget ? m_pStreetViewWidget->currentStreetImagePath() : QString();
		if (has2dView)
		{
			m_2dPixScrollWidget->getPixWidget()->setCurrentStreetPictureNameForStatus(streetImagePath);
			m_2dPixScrollWidget->getPixWidget()->refreshSdkViewState();
		}
		if (has3dView)
		{
			m_3dPixScrollWidget->getPixWidget()->setCurrentStreetPictureNameForStatus(streetImagePath);
			m_3dPixScrollWidget->getPixWidget()->refreshSdkViewState();
		}
	}
}
void hnRoadDataProcess::slot_streetWidgetFrameIdxChanged(int streetFrameIdx)
{
    if (m_isProgrammaticViewSync)
    {
        return;
    }

    if (!hnDataManager::getDataManager()->isOpenProject())
    {
        return;
    }
    if (!hnDataManager::getDataManager()->getCurrentProject()->get2DProject())
    {
        return;
    }

    // 景观信号传出来的是行驶距离，不是纯图片下标，直接作为 2D 编码器里程使用。
    syncContinuousViews(ContinuousViewSyncSource::Street, qMax(0, streetFrameIdx));
}

void hnRoadDataProcess::slot_setDepthCaculate(bool isCaculate)
{
	this->m_3dPixScrollWidget->getPixWidget()->setOpenDepthCaculate(isCaculate);
	this->m_2dPixScrollWidget->getPixWidget()->setOpenDepthCaculate(isCaculate);
}

void hnRoadDataProcess::slot_widgetMirrored(bool isH2dMirrored, bool isV2dMirrored, bool isH3dMirrored, bool isV3dMirrored)
{
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	if (hnDataManager::getDataManager()->getCurrentProject()->get2DProject())
	{
		hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->setIsHMirrored(isH2dMirrored);
		hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->setIsVMirrored(isV2dMirrored);
		this->m_2dPixScrollWidget->getPixWidget()->setHMirrored(isH2dMirrored);
		this->m_2dPixScrollWidget->getPixWidget()->setVMirrored(isV2dMirrored);
		this->m_2dPixScrollWidget->loadRoadPicture();
		this->m_2dPixScrollWidget->getPixWidget()->update();
	}

	if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->setIsHMirrored(isH3dMirrored);
		hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->setIsVMirrored(isV3dMirrored);
		this->m_3dPixScrollWidget->getPixWidget()->setHMirrored(isH3dMirrored);
		this->m_3dPixScrollWidget->getPixWidget()->setVMirrored(isV3dMirrored);
		this->m_3dPixScrollWidget->load3dImage();
		this->m_3dPixScrollWidget->getPixWidget()->update();
	}
}

void hnRoadDataProcess::slot_exportDXf()
{
	m_dxfExportDialog->exec();
}

void hnRoadDataProcess::slot_exportDiseaseDXf()
{
	//获取所有病害
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	int type = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType;
	if (type != 0 &&
		type != 2
		)
	{
		QMessageBox::critical(this, QStringLiteral("警告"), QStringLiteral("仅支持导出人工模式病害与设计模式病害"));
		return;
	}
	if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType() != PROJECT_TYPE::PROJECT_23D_TYPE)
	{
		QMessageBox::critical(this, QStringLiteral("警告"), QStringLiteral("仅支持二三维工程"));
		return;
	}

	QVector<hnRoadDiseaseInfo> allDiseaseInfos;

	//加载病害

	hnPro::hnProject* project = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	QVector<hnMile> miles = project->getCurrentMileVector();
	auto setting = project->getCurProSetInfo();
	QString standard = HnProjectEnums::roadTypeEnumToQString(project->getBaseStandard());
	if (type == 2)
	{ 
		//设计模式病害
	allDiseaseInfos=	hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllDesignDiseases();

	}
	else
	{
		allDiseaseInfos = hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllRoadDiseases();
	}


	//弹出用户选择窗口
	//弹出对话框 让用户设置输出位置
	QString projectPath = QFileDialog::getExistingDirectory(this, QStringLiteral("请选择输出路径"));
	if (projectPath.isEmpty())
	{
		return;
	}
	QString outName = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2DProName() + QStringLiteral("病害.dxf");
	QString outFilePath = projectPath + "/" + outName;

	std::vector<	hnCommon::hn3dDiseaseDef<hnCommon::hn3dPointD>*> disVector;
	std::vector<	hnCommon::hn3dDiseaseLineDef<hnCommon::hn3dPointD>*> disLineVector;
	//设计模式病害导出
	if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType == 2)
	{
		for (auto diseaseInfo : allDiseaseInfos)
		{

			//处理病害 获得病害深度 
			if (diseaseInfo.vec3dRect.size() > 0)
			{
				bool isLine = diseaseInfo.nDrawType == 3 ? true : false;

				if (diseaseInfo.vec3dRect.size() <= 0)
				{
			QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("该功能不支持纯三维工程！"));
					return;
				}
				auto rect3d = diseaseInfo.vec3dRect.at(0);
				hnCommon::hn3dPointD pt3d;

				if (false == hnDataManager::getDataManager()->getDisease3DPoint(rect3d.p0, pt3d))
				{
			QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("该功能不支持纯三维工程！"));
					return;
				}

				QVector<hnCommon::hn3dPointD> points;
				if (isLine)
				{

					for (int i = 0; i < diseaseInfo.vec3dRect.size(); ++i)
					{
						hnCommon::hn3dPointD pt3dTemp;
						hnDataManager::getDataManager()->getDisease3DPoint(diseaseInfo.vec3dRect[i].p0, pt3dTemp);
						points.push_back(pt3dTemp);

					}
				}
				else
				{
					hnCommon::hn3dPointD pt3d01, pt3d02, pt3d03;
					hnDataManager::getDataManager()->getDisease3DPoint(rect3d.p1, pt3d01);
					hnDataManager::getDataManager()->getDisease3DPoint(rect3d.p2, pt3d02);
					hnDataManager::getDataManager()->getDisease3DPoint(rect3d.p3, pt3d03);
					points.push_back(pt3d);
					points.push_back(pt3d01);
					points.push_back(pt3d02);
					points.push_back(pt3d03);
				}

				hnCommon::hn3dDiseaseLineDef<hnCommon::hn3dPointD>* hn3dLineDis = new hnCommon::hn3dDiseaseLineDef<hnCommon::hn3dPointD>
					(points.toStdVector(), diseaseInfo.strDisName, diseaseInfo.dMileage, isLine);
				disLineVector.push_back(hn3dLineDis);
			}

		}

		QByteArray temp = outFilePath.toLocal8Bit();
		if (OutputDiseaseLine3dDxf(temp.constData(), disLineVector, type))
		{
			QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("处理完成"));

		}
		else
		{
			QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("处理失败,请检查是否生成!"));

		}

	}
	else
	{
		for (auto diseaseInfo : allDiseaseInfos)
		{
			//处理病害 获得病害深度

			if (diseaseInfo.vec3dRect.size() > 0)
			{
				auto rect3d = diseaseInfo.vec3dRect.at(0);
				hnCommon::hn3dPointD pt3d00, pt3d01, pt3d02, pt3d03;

				if (false == hnDataManager::getDataManager()->getDisease3DPoint(rect3d.p0, pt3d00))
				{
			QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("该功能不支持纯三维工程！"));
					return;
				}
				else
				{
					hnDataManager::getDataManager()->getDisease3DPoint(rect3d.p1, pt3d01);
					hnDataManager::getDataManager()->getDisease3DPoint(rect3d.p2, pt3d02);
					hnDataManager::getDataManager()->getDisease3DPoint(rect3d.p3, pt3d03);
				}
				hnCommon::hn3dDiseaseDef<hnCommon::hn3dPointD>* hn3dDis = new hnCommon::hn3dDiseaseDef<hnCommon::hn3dPointD>(pt3d00, pt3d01, pt3d02, pt3d03, diseaseInfo.strDisName, diseaseInfo.dMileage);
				disVector.push_back(hn3dDis);
			}
		}
		QByteArray temp1 = outFilePath.toLocal8Bit();
		if (OutputDisease3dDxf(temp1.constData(), disVector, type))
		{
			QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("处理完成"));

		}
		else
		{
			QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("处理失败,请检查是否生成!"));

		}
	}
	//析构 
	for (hnCommon::hn3dDiseaseDef<hnCommon::hn3dPointD>* dis : disVector)
	{
		delete dis;
	}
	for (hnCommon::hn3dDiseaseLineDef<hnCommon::hn3dPointD>* dis : disLineVector)
	{
		delete dis;
	}
}



void hnRoadDataProcess::slot_exportHighAccuracyDiseaseDXf()
{
	//获取所有病害
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	HighAccuracySettingForm form;
	auto rc = form.exec();

	if (rc == QDialog::Accepted)
	{
		POS_CONVERT_INFO config = form.getConfigInfo();
		int type = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType;
		if (type != 0
			)
		{
			QMessageBox::critical(this, QStringLiteral("警告"), QStringLiteral("仅支持导出人工模式病害"));
			return;
		}
		if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType() != PROJECT_TYPE::PROJECT_23D_TYPE
			&& hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType() != PROJECT_TYPE::PROJECT_2D_TYPE)
		{
			QMessageBox::critical(this, QStringLiteral("警告"), QStringLiteral("仅支持二三维及二维工程"));
			return;
		}

		QVector<hnRoadDiseaseInfo> allDiseaseInfos;
		//加载病害

		hnPro::hnProject* project = hnApp::hnDataManager::getDataManager()->getCurrentProject();

	 
		QString  highGpsFilePath = project->get2DProject()->getBasePath() + "\\HighGps2Mile.txt";
		if (!QFile::exists(highGpsFilePath))
		{ 
			QMessageBox::critical(this, QStringLiteral("警告"), QStringLiteral("非高精度模块或未进行gps桩号匹配，无法导出高精度病害dxf!"));
			return;
		}
		else
		{
			project->get2DProject()->initGpsInfos(); 
		} 
		QVector<hnMile> miles = project->getCurrentMileVector();
		auto setting = project->getCurProSetInfo();
		QString standard = HnProjectEnums::roadTypeEnumToQString(project->getBaseStandard());


		if (PROJECT_TYPE::PROJECT_23D_TYPE == hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType() ||
			PROJECT_TYPE::PROJECT_2D_TYPE == hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType())
		{
	
			allDiseaseInfos = 	hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllRoadDiseases(); 
		}
		else
		{
			QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("该功能不支持纯三维工程！"));
			return;
		}

		//弹出对话框 让用户设置输出位置
		QString projectPath = QFileDialog::getExistingDirectory(this, QStringLiteral("请选择输出路径"));
		if (projectPath.isEmpty())
		{
			return;
		}
		QString outName = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2DProName() + QStringLiteral("高精度病害.dxf");
		QString outFilePath = projectPath + "/" + outName;
		std::unique_ptr<HighAccuracyPositioning>m_highAccuracy =
			std::make_unique<  HighAccuracyPositioning>(hnApp::hnDataManager::getDataManager()->getCurrentProject());
		std::vector<	hnCommon::hn2dDiseaseDef<hnCommon::hn2dGpsPoint>*> disVector;
		hnHighAcc2Plane plane;
		plane.initialParam(&config);
		for (auto diseaseInfo : allDiseaseInfos)
		{
			//处理病害 获得病害深度

			if (diseaseInfo.vec2dRect.size() > 0)
			{
				auto rect2d = diseaseInfo.vec2dRect.at(0);
				hnCommon::hn2dGpsPoint pt2d00, pt2d01, pt2d02, pt2d03;
				hnCommon::hn2dGpsPoint ptResult0, ptResult1, ptResult2, ptResult3;

				 
			m_highAccuracy->getHighAccPosition(true, m_xrSetting->equipType, project->enclToTrueMile(rect2d.p0.m_dmi), rect2d.p0.x, rect2d.p0.y, pt2d00.x, pt2d00.y, pt2d00.z);
				m_highAccuracy->getHighAccPosition(true, m_xrSetting->equipType, project->enclToTrueMile(rect2d.p1.m_dmi), rect2d.p1.x, rect2d.p1.y, pt2d01.x, pt2d01.y, pt2d01.z);
				m_highAccuracy->getHighAccPosition(true, m_xrSetting->equipType, project->enclToTrueMile(rect2d.p2.m_dmi), rect2d.p2.x, rect2d.p2.y, pt2d02.x, pt2d02.y, pt2d02.z);
				m_highAccuracy->getHighAccPosition(true, m_xrSetting->equipType, project->enclToTrueMile(rect2d.p3.m_dmi), rect2d.p3.x, rect2d.p3.y, pt2d03.x, pt2d03.y, pt2d03.z);

			
				plane.convertBLHToProjection(pt2d00.x, pt2d00.y, pt2d00.z, ptResult0.x, ptResult0.y, ptResult0.z);
				plane.convertBLHToProjection(pt2d01.x, pt2d01.y, pt2d01.z, ptResult1.x, ptResult1.y, ptResult1.z);
				plane.convertBLHToProjection(pt2d02.x, pt2d02.y, pt2d02.z, ptResult2.x, ptResult2.y, ptResult2.z);
				plane.convertBLHToProjection(pt2d03.x, pt2d03.y, pt2d03.z, ptResult3.x, ptResult3.y, ptResult3.z);




				hnCommon::hn2dDiseaseDef<hnCommon::hn2dGpsPoint>* hn2dDis =
					new hnCommon::hn2dDiseaseDef<hnCommon::hn2dGpsPoint>(ptResult0, ptResult1, ptResult2, ptResult3, diseaseInfo.strDisName, diseaseInfo.dMileage);
 
				


			/*	m_highAccuracy->getHighAccPosition(false, m_xrSetting->equipType, project->enclToTrueMile(rect2d.p0.m_dmi), rect2d.p0.x, rect2d.p0.y, pt2d00.x, pt2d00.y, pt2d00.z);
				m_highAccuracy->getHighAccPosition(false, m_xrSetting->equipType, project->enclToTrueMile(rect2d.p1.m_dmi), rect2d.p1.x, rect2d.p1.y, pt2d01.x, pt2d01.y, pt2d01.z);
				m_highAccuracy->getHighAccPosition(false, m_xrSetting->equipType, project->enclToTrueMile(rect2d.p2.m_dmi), rect2d.p2.x, rect2d.p2.y, pt2d02.x, pt2d02.y, pt2d02.z);
				m_highAccuracy->getHighAccPosition(false, m_xrSetting->equipType, project->enclToTrueMile(rect2d.p3.m_dmi), rect2d.p3.x, rect2d.p3.y, pt2d03.x, pt2d03.y, pt2d03.z);
 


				hnCommon::hn2dDiseaseDef<hnCommon::hn2dGpsPoint>* hn2dDis =
					new hnCommon::hn2dDiseaseDef<hnCommon::hn2dGpsPoint>(pt2d00, pt2d01, pt2d02, pt2d03, diseaseInfo.strDisName, diseaseInfo.dMileage);
*/

				disVector.push_back(hn2dDis);
			}


		}
		QByteArray temp1 = outFilePath.toLocal8Bit();
		if (OutputDisease2dGpsDxf(temp1.constData(), disVector, type))
		{
			QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("处理完成"));

		}
		else
		{
			QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("处理失败,请检查是否生成!"));

		}

		//析构 
		for (hnCommon::hn2dDiseaseDef<hnCommon::hn2dGpsPoint>* dis : disVector)
		{
			delete dis;
		}
	}
	else
	{
		return;
	}
}

void hnRoadDataProcess::slot_exportGjDatas()
{
	//导出国检转换中间数据 
	if (!m_projects->isOpenProject())
	{
		return;
	}
	//弹出界面
	if (!m_projects->isHasProject())
	{
		QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("当前没有可用的工程"),
			QStringLiteral("确定"));
		return;
	}
	auto reply = QMessageBox::question(this, QStringLiteral("提示"), QStringLiteral("出总表时间较长,是否继续"), QMessageBox::Yes | QMessageBox::No);

	if (reply != QMessageBox::Yes)
		return;
	hnPro::hnProjectManager* manager = hnApp::hnDataManager::getDataManager()->getProjectManager();
	auto allPorject = manager->getAllBaseProject();
	if (allPorject.size() < 1)
	{
		return;
	}


	auto defaultStarndar = HnProjectEnums::roadTypeQStringToEnum(allPorject.at(0)->getCurProSetInfo().strRoadStandard);
	auto defaultDrawType = allPorject.at(0)->getCurProSetInfo().nDrawType;
	QString defaultStanardStr = HnProjectEnums::roadTypeEnumToQString_ForExcel(defaultStarndar);
	QString defaultDrawTypeStr = QString::fromLocal8Bit(hnCommon::workTypeToQString((ROAD_WORK_TYPE)defaultDrawType));

	double sMile = 0, eMile = 0;
	m_xrSetting->outMileWithMark = true;
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QApplication::processEvents();
	bool outSucceed = true;
	QVector<int> splitValueVec = { 10,100,1000 };


	QString excelDir;



	QString segment = "10,100,1000";
	int excelIndexs[] = { 0,1,2,3,4,5,7,8,9,10,11,12 };
	//计算总任务数量
	int totalTasks = 3 * 11 * allPorject.size();
	int currentTask = 0;

	//创建进度对话框
	QProgressDialog progress(QStringLiteral("处理工程数据..."), QStringLiteral("取消"), 0, totalTasks);
	progress.setWindowTitle(QStringLiteral("导出所有数据"));
	progress.setWindowModality(Qt::WindowModal);  //模态
	progress.setMinimumDuration(0);   //立即显示

	QString selectModelTxt = QStringLiteral("单项指标出表");
	m_xrSetting->outExcelFormatDmi = false;
	m_xrSetting->outRoadUnitMark = true;
	m_xrSetting->diseaseExcelOutPicture = false;

	for each (auto curProject in allPorject)
	{

		QString projectBasePath = curProject->get2DProPath();
		if (projectBasePath.isEmpty())
		{
			currentTask++;
			progress.setValue(currentTask);
			QApplication::processEvents();
			continue;
		}
		excelDir = curProject->get2DProPath() + QStringLiteral("/结果表格/") + defaultStanardStr + "/" + defaultDrawTypeStr + "/";
		QDir outDir(excelDir);
		if (!outDir.exists())
		{
			outDir.mkpath(excelDir);
		}
		sMile = curProject->getCurProSetInfo().dBegMile;
		eMile = curProject->getCurProSetInfo().dEndMile;

		QStringList segmentList = segment.split(',');

		for each (QString  splitValueStr in segmentList)
		{
			for (int index : excelIndexs)
			{

				double splitValue = splitValueStr.toDouble();

				switch (defaultStarndar)
				{

				case HnProjectEnums::DegreeRoad2018:
				{
					switch (defaultDrawType)
					{
					case hnCommon::ROAD_WORK_LARGE_RECT:
						hnOutExcelManage::outBigRectExcel2018(excelDir, selectModelTxt, index, splitValue, curProject, sMile, eMile);

						break;
					case hnCommon::ROAD_WORK_SMALL_RECT:
						hnOutExcelManage::outExcelSmallRectDegreeRoad2018(excelDir, selectModelTxt, index, splitValue, curProject, sMile, eMile);

						break;
					case hnCommon::DESIGN:
						hnOutExcelManage::outDesignExcel2018(excelDir, selectModelTxt, index, splitValue, curProject, sMile, eMile);

						break;
					default:
						break;
					}
				}

				case HnProjectEnums::CityRoad:
					switch (defaultDrawType)
					{
					case hnCommon::ROAD_WORK_LARGE_RECT:
						hnOutExcelManage::outExcelCityRoad(excelDir, selectModelTxt, index, splitValue, curProject, sMile, eMile);

						break;
					case hnCommon::ROAD_WORK_SMALL_RECT:
						hnOutExcelManage::outExcelSmallRectCityRoad(excelDir, selectModelTxt, index, splitValue, curProject, sMile, eMile);

						break;
					case hnCommon::DESIGN:
						hnOutExcelManage::outExcelDesignCityRoad(excelDir, selectModelTxt, index, splitValue, curProject, sMile, eMile);

						break;
					default:
						break;
					}

					break;
				case HnProjectEnums::RuralRoadlowLevel:
				{
					switch (defaultDrawType)
					{
					case hnCommon::ROAD_WORK_LARGE_RECT:
						hnOutExcelManage::outBigRectExcelRuralRoadlowLevelRoad(excelDir, selectModelTxt, index, splitValue, curProject, sMile, eMile);

						break;
					case hnCommon::ROAD_WORK_SMALL_RECT:
						hnOutExcelManage::ouSmallRectlRuralRoadlowLevelRoad(excelDir, selectModelTxt, index, splitValue, curProject, sMile, eMile);

						break;
					case hnCommon::DESIGN:
						hnOutExcelManage::outDesignExcel2018(excelDir, selectModelTxt, index, splitValue, curProject, sMile, eMile);

						break;
					default:
						break;
					}
					break;
				}

				default:
					break;
				}
				currentTask++;
				progress.setValue(currentTask);
				QApplication::processEvents();

			}

		}


		{
			auto marks = curProject->getCurrentMarkVector();
			QString markFilePath = curProject->get2DProject()->getFullRoadTypeMarkFilePath();
			QFile markFile(markFilePath);
			if (markFile.exists())
			{
				QString newFilePath = excelDir + "RoadTypeInfo.txt";
				QFile fileTemp1(newFilePath);
				if (!fileTemp1.exists())
				{
					markFile.copy(newFilePath);
				}
			}
		}

	}
	QApplication::restoreOverrideCursor();

	QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("导出完毕"));
}

void hnRoadDataProcess::slot_importCtrlPoints()
{
	if (false == hnDataManager::getDataManager()->isOpenProject())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("请先打开工程"));
		return;
	}

	if (nullptr == hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("请先打开工程"));
		return;
	}

	// 提示用户选择文件
	QString fileName = QFileDialog::getOpenFileName(this);
	if (true == fileName.isEmpty())
	{
		return;
	}
	QFile file(fileName);
	file.open(QIODevice::ReadOnly);
	while (false == file.atEnd())
	{
		hnKZDDataInfo ctrlPoint;
		QString line = file.readLine();
		QStringList list = line.split(',');
		if (7 != list.size())
		{
			continue;
		}
		const int id = hnDataManager::getDataManager()->getCurrentProject()->getDB()->getCtrlPointTable()->getMaxID();
		ctrlPoint.nID = id;
		QString name = QString::fromLocal8Bit("控制点_%1").arg(id);
		strcpy(ctrlPoint.strKzdName, name.toLocal8Bit().data());
		const double trueMile = list.at(1).toDouble();
		const double encoderMile = m_3dPixScrollWidget->getPixWidget()->trueMileToEncoderMile(trueMile);
		ctrlPoint.dMileage = encoderMile;
		ctrlPoint.dGpsTimer = list.at(2).toDouble();

		// 获取图像名称
		QString pixName = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageByMile(encoderMile);
		strcpy(ctrlPoint.strImageName, pixName.toLocal8Bit().data());
		// 获取x坐标
		ctrlPoint.nLocX = list.at(6).toInt();
		// 获取y坐标
		const double roadHeight = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageHeightScale()*
			hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelHeight();

		double singlePixMile = fmod(encoderMile, roadHeight);
		singlePixMile = roadHeight - singlePixMile;
		const double yScale = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageHeightScale();
		const int y = singlePixMile / yScale;
		ctrlPoint.nLocY = y;

		ctrlPoint.dX = list.at(3).toDouble();
		ctrlPoint.dY = list.at(4).toDouble();
		ctrlPoint.dZ = list.at(5).toDouble();
		hnDataManager::getDataManager()->getCurrentProject()->getDB()->getCtrlPointTable()->writeData(ctrlPoint);

	}

	QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("导入完成"));

}

void hnRoadDataProcess::slot_exportCtrlPoints()
{
	if (false == hnDataManager::getDataManager()->isOpenProject())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("请先打开工程"));
		return;
	}

	if (nullptr == hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("请先打开三维工程"));
		return;
	}

	//提示用户选择文件夹
	QString filePath = QFileDialog::getExistingDirectory(this);
	if (true == filePath.isEmpty())
	{
		return;
	}
	QString fileName = filePath + QString::fromLocal8Bit("/二三维自用控制点信息.txt");

	QFile file(fileName);

	if (false == file.open(QIODevice::WriteOnly))
	{
		return;
	}
	QTextStream stream(&file);

	// 获取所有控制点
	std::vector<hnKZDDataInfo> ctrlPoints;
	hnDataManager::getDataManager()->getCurrentProject()->getDB()->getCtrlPointTable()->readData(ctrlPoints);

	for (const hnKZDDataInfo ctrlPoint : qAsConst(ctrlPoints))
	{
		double trueMile = m_3dPixScrollWidget->getPixWidget()->encoderMileToTrueMile(ctrlPoint.dMileage);
		QString lineData = QString("%1,%2,%3,%4,%5,%6,%7")
			.arg(QString::fromLocal8Bit(ctrlPoint.strKzdName))
			.arg(trueMile, 0, 'f', 8)
			.arg(ctrlPoint.dGpsTimer, 0, 'f', 8)
			.arg(ctrlPoint.dX, 0, 'f', 8)
			.arg(ctrlPoint.dY, 0, 'f', 8)
			.arg(ctrlPoint.dZ, 0, 'f', 8)
			.arg(ctrlPoint.nLocX);
		stream << lineData << endl;
	}

	QString outFileName = filePath + QString::fromLocal8Bit("/控制点信息.txt");

	QFile outFile(outFileName);

	if (false == outFile.open(QIODevice::WriteOnly))
	{
		return;
	}
	QTextStream outStream(&outFile);
	for (const hnKZDDataInfo ctrlPoint : qAsConst(ctrlPoints))
	{
		double trueMile = m_3dPixScrollWidget->getPixWidget()->encoderMileToTrueMile(ctrlPoint.dMileage);
		QString lineData = QString("%1,%2,%3,%4,%5,%6")
			.arg(QString::fromLocal8Bit(ctrlPoint.strKzdName))
			.arg(trueMile, 0, 'f', 8)
			.arg(ctrlPoint.dGpsTimer, 0, 'f', 8)
			.arg(ctrlPoint.dX, 0, 'f', 8)
			.arg(ctrlPoint.dY, 0, 'f', 8)
			.arg(ctrlPoint.dZ, 0, 'f', 8);
		outStream << lineData << endl;
	}
	file.close();
	outFile.close();
	QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("导出完成"));
}

void hnRoadDataProcess::slot_mergeAutoDisease()
{
	//	//异常处理
	//	if (!hnDataManager::getDataManager()->isOpenProject())
	//	{
	//		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("请打开工程"),
	//			QString::fromLocal8Bit("确定"));
	//		return;
	//	}
	//	//获取框选类型
	//	int frameType = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType;
	//	if (2 == frameType)
	//	{
	//		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("设计模式不支持病害拼接"), QString::fromLocal8Bit("确定"));
	//		return;
	//	}
	//
	//	//提示用户是否继续
	//	QString frameTypeQString = frameType == 0 ? QString::fromLocal8Bit("人工模式") : QString::fromLocal8Bit("自动化模式");
	//	auto reply = QMessageBox::question(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("您即将进行%1病害的病害拼接，"
	//		"此过程不可逆，请做好备份工作，是否继续？").arg(frameTypeQString),
	//		QString::fromLocal8Bit("是"), QString::fromLocal8Bit("否"));
	//	if (1 == reply)
	//	{
	//		return;
	//	} 
	//	//定义导出对象
	//	hnImportAidcDiseases importDisease(frameType, this);
	//
	//
	//	//将自动识别的病害转换成本程序的病害类型
	//	QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> diseases; 
	//
	//	QVector<hnRoadDiseaseInfo> allDiseaseInfos;
	//	// 二三维病害
	//	hnPro::hnProject* project = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	//	QVector<hnMile> miles = project->getCurrentMileVector();
	//	hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurDB()->getDiseaseTable()->readRoadDiseaseData(miles, allDiseaseInfos, project->getCurProSetInfo().nLineType, project->getRoadSpace());
	//
	//	for (auto dis: allDiseaseInfos)
	//	{
	//
	//		QString disName = QString::fromLocal8Bit( dis.strDiseaseTableName);
	//		if (diseases.keys().contains(disName))
	//		{
	//			diseases[disName].push_back(dis);
	//		}
	//		else
	//		{
	//			std::vector<hnCommon::hnRoadDiseaseInfo> singleTableResultDiseases;
	//			singleTableResultDiseases.push_back(dis);
	//			diseases.insert(disName, singleTableResultDiseases);
	//		}
	//
	//	} 
	//	if (diseases.isEmpty())
	//	{
	//		return;
	//	}
	//
	//#if 1
	//	//合并纵向裂缝、修补病害。
	//	const double roadWidth = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRoadLength;
	//	const int pixHeight = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelY;
	//	bool isVMirror = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getIsVMirrored();
	//	mergeAidcDiseases merge(roadWidth, pixHeight, isVMirror, this);
	//	diseases = merge.mergeDiseases(diseases);
	//#endif 
	//	//批量写入数据库
	//	importDisease.writeDb(diseases);
	//
	//	//提示用户完成
	//	QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("拼接病害完成"));
}

void hnRoadDataProcess::updateAllWidget()
{
	if (this->m_2dPixScrollWidget && this->m_2dPixScrollWidget->getPixWidget())
	{
		this->m_2dPixScrollWidget->getPixWidget()->refreshSdkDiseaseLayer();
		this->m_2dPixScrollWidget->update();
	}
	if (this->m_3dPixScrollWidget && this->m_3dPixScrollWidget->getPixWidget())
	{
		this->m_3dPixScrollWidget->getPixWidget()->refreshSdkDiseaseLayer();
		this->m_3dPixScrollWidget->update();
	}
	this->m_diseaseListWidget->updateAllDiseases();
}
void hnRoadDataProcess::updatePixWidget()
{
	this->m_2dPixScrollWidget->update();
	this->m_3dPixScrollWidget->update();
}

void hnRoadDataProcess::updateTreeWidget()
{
	//清空树状视图
	this->m_projectListTreeWidget->clear();


	hnPro::hnProjectManager* projects = m_projects->getProjectManager();

	//添加节点到树上
	QMap<QString, QVector<hnProjectDataInfo>> projectDatas;
	for each (hnProjectDataInfo data in m_projectDataInfos)
	{
		QString temp(QString::fromLocal8Bit(data.strProJectName));
		if (projectDatas.keys().contains(temp))
		{
			projectDatas[temp].append(data);
		}
		else
		{
			QVector<hnProjectDataInfo> vDatas;
			vDatas.push_back(data);
			projectDatas.insert(temp, vDatas);
		}
	}

	for (QMap<QString, QVector<hnProjectDataInfo>>::iterator
		it = projectDatas.begin(); it != projectDatas.end(); ++it)
	{
		QString key = it.key();
		QVector<hnProjectDataInfo> vec = it.value();
		QTreeWidgetItem  * projectNameItem = new QTreeWidgetItem(m_projectListTreeWidget);
		projectNameItem->setText(0, key);
		projectNameItem->setData(1, Qt::UserRole, 1);

		//判断是不是当前工程，如果是当前工程  就加粗
		QString currentProjectName;
		if (hnApp::hnDataManager::getDataManager()->isOpenProject())
		{
			currentProjectName = hnDataManager::getDataManager()->getCurrentProject()->getProjectName();
			if (currentProjectName == key)
			{
				QFont font = projectNameItem->font(0);
				font.setBold(true);
				projectNameItem->setFont(0, font);
			}
		}
		QTreeWidgetItem  * projectTitle2dItem = new QTreeWidgetItem(projectNameItem);
		projectTitle2dItem->setText(0, QStringLiteral("二维工程"));
		projectTitle2dItem->setData(1, Qt::UserRole, 1);

		QTreeWidgetItem  * projectTitle3dItem = new QTreeWidgetItem(projectNameItem);
		projectTitle3dItem->setText(0, QStringLiteral("三维工程"));
		projectTitle3dItem->setData(1, Qt::UserRole, 1);

		for (int i = 0; i < vec.count(); ++i)
		{

			auto projectInfo = vec[i];

			if (projectInfo.proSetInfo.nWorkType == PROJECT_TYPE::PROJECT_2D_TYPE)
			{
				//添加二维工程
				QTreeWidgetItem  * item2d = new QTreeWidgetItem(projectTitle2dItem);

				//如果是当前二维工程,就加粗处理
				QString projectName2d(QString::fromLocal8Bit(projectInfo.str2DProName));
				if (hnApp::hnDataManager::getDataManager()->isOpenProject())
				{
					if (projectName2d == hnDataManager::getDataManager()->getCurrentProject()->get2DProName())
					{
						QFont font = item2d->font(0);
						font.setBold(true);
						item2d->setFont(0, font);
					}
				}

				item2d->setText(0, projectName2d);
				item2d->setData(1, Qt::UserRole, 0);
			}
			else if (projectInfo.proSetInfo.nWorkType == PROJECT_TYPE::PROJECT_23D_TYPE)
			{
				//添加二维工程
				QTreeWidgetItem  * item2d = new QTreeWidgetItem(projectTitle2dItem);

				//如果是当前二维工程,就加粗处理
				QString projectName2d(QString::fromLocal8Bit(projectInfo.str2DProName));
				if (hnApp::hnDataManager::getDataManager()->isOpenProject())
				{
					if (projectName2d == hnDataManager::getDataManager()->getCurrentProject()->get2DProName())
					{
						QFont font = item2d->font(0);
						font.setBold(true);
						item2d->setFont(0, font);
					}
				}

				item2d->setText(0, projectName2d);
				item2d->setData(1, Qt::UserRole, 0);

				//添加三维工程  
				QTreeWidgetItem  * item3d = new QTreeWidgetItem(projectTitle3dItem);

				//如果是当前三维工程，就加粗处理
				QString projectName3d(QString::fromLocal8Bit(projectInfo.str3DProName));
				if (hnApp::hnDataManager::getDataManager()->isOpenProject())
				{
					if (projectName3d == hnDataManager::getDataManager()->getCurrentProject()->get3DProName())
					{
						QFont font = item3d->font(0);
						font.setBold(true);
						item3d->setFont(0, font);
					}
				}

				item3d->setText(0, projectName3d);
				item3d->setData(1, Qt::UserRole, 1);
				item3d->setFlags(item3d->flags()&~Qt::ItemIsEnabled);
			}
			else
			{
				//单独三维工程
				//添加三维工程  
				QTreeWidgetItem  * item3d = new QTreeWidgetItem(projectTitle3dItem);

				//如果是当前三维工程，就加粗处理
				QString projectName3d(QString::fromLocal8Bit(projectInfo.str3DProName));
				if (hnApp::hnDataManager::getDataManager()->isOpenProject())
				{
					if (projectName3d == hnDataManager::getDataManager()->getCurrentProject()->get3DProName())
					{
						QFont font = item3d->font(0);
						font.setBold(true);
						item3d->setFont(0, font);
					}
				}

				item3d->setText(0, projectName3d);
				item3d->setData(1, Qt::UserRole, 2);
				item3d->setFlags(item3d->flags()&~Qt::ItemIsEnabled);
			}


		}
		//如果是当前工程，展开所有节点
		const int maxProjectSize = 5;
		if (currentProjectName == key || vec.size() < maxProjectSize)
		{
			projectNameItem->setExpanded(true);
			projectTitle2dItem->setExpanded(true);
			projectTitle3dItem->setExpanded(true);
		}
	}
}

void hnRoadDataProcess::allWidgetLoadPictures()
{
	QElapsedTimer totalTimer;
	QElapsedTimer stepTimer;
	totalTimer.start();
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][AllWidgetLoadStart]";
	#endif
	// 加载当前工程数据
	hnPro::hnProject* curProject = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	if (curProject)
	{
		#ifdef _DEBUG
		qDebug().noquote() << "[HN_PERF][AllWidgetLoadProject]"
			<< "projectType=" << static_cast<int>(curProject->getProjectType())
			<< "projectName=" << curProject->getProjectName();
		#endif
		if (curProject->getProjectType() == PROJECT_TYPE::PROJECT_23D_TYPE)
		{
			// 加载当前工程路面影像
			stepTimer.start();
			this->m_2dPixScrollWidget->loadRoadPicture();
			#ifdef _DEBUG
			qDebug().noquote() << "[HN_PERF][AllWidgetLoadStep]" << "step=2dRoadPicture" << "elapsedMs=" << stepTimer.elapsed();
			#endif

			// 加载景观影像
			if (m_pStreetViewWidget)
			{
				stepTimer.restart();
				m_pStreetViewWidget->initView();
				#ifdef _DEBUG
				qDebug().noquote() << "[HN_PERF][AllWidgetLoadStep]" << "step=streetView" << "elapsedMs=" << stepTimer.elapsed();
				#endif
			}
			// 加载三维影像
			if (hnDataManager::getDataManager()->getCurrentProject()->getProjectType() != PROJECT_2D_TYPE)
			{
				stepTimer.restart();
				m_3dPixScrollWidget->load3dImage();
				#ifdef _DEBUG
				qDebug().noquote() << "[HN_PERF][AllWidgetLoadStep]" << "step=3dImage" << "elapsedMs=" << stepTimer.elapsed();
				#endif
			}
		}
		else if (curProject->getProjectType() == PROJECT_TYPE::PROJECT_2D_TYPE)
		{
			// 加载当前工程路面影像
			stepTimer.start();
			this->m_2dPixScrollWidget->loadRoadPicture();
			#ifdef _DEBUG
			qDebug().noquote() << "[HN_PERF][AllWidgetLoadStep]" << "step=2dRoadPicture" << "elapsedMs=" << stepTimer.elapsed();
			#endif

			// 加载景观影像
			if (m_pStreetViewWidget)
			{
				stepTimer.restart();
				m_pStreetViewWidget->initView();
				#ifdef _DEBUG
				qDebug().noquote() << "[HN_PERF][AllWidgetLoadStep]" << "step=streetView" << "elapsedMs=" << stepTimer.elapsed();
				#endif
			}
		}
		else
		{
			// 加载三维影像
			if (hnDataManager::getDataManager()->getCurrentProject()->getProjectType() != PROJECT_2D_TYPE)
			{
				stepTimer.start();
				m_3dPixScrollWidget->load3dImage();
				#ifdef _DEBUG
				qDebug().noquote() << "[HN_PERF][AllWidgetLoadStep]" << "step=3dImage" << "elapsedMs=" << stepTimer.elapsed();
				#endif
			}
		}

	}
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][AllWidgetLoadEnd]" << "totalMs=" << totalTimer.elapsed();
	#endif
}

void hnRoadDataProcess::clearCurrentProjectUiState()
{
	if (m_2dPixScrollWidget && m_2dPixScrollWidget->getPixWidget())
	{
		m_2dPixScrollWidget->getPixWidget()->clearPix();
		m_2dPixScrollWidget->getPixWidget()->clearSdkView();
	}
	if (m_3dPixScrollWidget && m_3dPixScrollWidget->getPixWidget())
	{
		m_3dPixScrollWidget->getPixWidget()->clearPix();
		m_3dPixScrollWidget->getPixWidget()->clearSdkView();
	}
	if (m_pStreetViewWidget)
	{
		m_pStreetViewWidget->clearPix();
	}
	if (m_diseaseListWidget)
	{
		m_diseaseListWidget->clearDiseases();
	}
	if (m_projectWidget)
	{
		m_projectWidget->clearProjectInfo();
	}
	if (m_projectListTreeWidget)
	{
		m_projectListTreeWidget->clear();
	}
	if (m_mapWidget)
	{
		m_mapWidget->clearMapData();
	}
	if (m_statusBarWidget)
	{
		m_statusBarWidget->updateLabelTextSlot(QString());
	}
}
void hnRoadDataProcess::clearAllWidgetPixs()
{
	m_2dPixScrollWidget->getPixWidget()->clearPix();
	m_3dPixScrollWidget->getPixWidget()->clearPix();
	m_pStreetViewWidget->clearPix();
}

void hnRoadDataProcess::setLayout(PROJECT_TYPE projectType)
{
	QString strAppDirPath = QCoreApplication::applicationDirPath();
	QString strLayoutPath = "";
	switch (projectType)
	{
	case hnCommon::PROJECT_2D_TYPE:
		strLayoutPath = strAppDirPath + "/Layout2D.ini";
		break;
	case hnCommon::PROJECT_XD_3D_TYPE:
		strLayoutPath = strAppDirPath + "/Layout3D.ini";
		break;
	case hnCommon::PROJECT_JD_3D_TYPE:
		strLayoutPath = strAppDirPath + "/Layout3D.ini";
		break;
	case hnCommon::PROJECT_23D_TYPE:
		strLayoutPath = strAppDirPath + "/Layout23D.ini";
		break;
	default:
		break;
	}

	if (QFileInfo::exists(strLayoutPath) == false)
	{
		return;
	}

	// 读取数据;
	QFile file(strLayoutPath);
	if (file.open(QIODevice::ReadOnly))
	{
		QByteArray arry;
		QDataStream out(&file);
		out >> arry;
		file.close();

		m_DockManager->restoreState(arry);
	}


}

bool hnRoadDataProcess::checkProjectFrameTypeConflict(const QString & projectName)
{
	//设置当前工程
	hnApp::hnDataManager::getDataManager()->setCurrentProject(projectName);

	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return false;
	}

	auto projectInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
	int drawType = projectInfo.nDrawType;
	QStringList standards = hnApp::hnDataManager::getDataManager()->getRoadStandardNames();
	QString standard = QString::fromLocal8Bit(projectInfo.strRoadStandard);
	for (int i = standards.size() - 1; i >= 0; --i)
	{
		if (standards[i] == standard)
		{
			standards.removeAt(i);
		}
	}
	QVector<int> types;
	types << 0 << 1 << 2 << 3;

	//设计模式下，工程的绘制类型和病害的绘制类型不同，工程只有2，病害有2 3
	if (2 == drawType)
	{
		types.removeAll(2);
		types.removeAll(3);
	}
	else
	{
		types.removeAll(drawType);
	}

	for (int drawType : qAsConst(types))
	{
		for (QString stand : standards)
		{
			if (true == hnApp::hnDataManager::getDataManager()->getCurrentProject()->getDB()->getDiseaseTable()->checkDiseaseExist(stand, drawType))
			{
				if (true == this->handleConflict(stand, drawType))
				{
					return true;
				}
				else
				{
					return false;
				}
			}

		}

	}

	return true;
}

bool hnRoadDataProcess::handleConflict(QString standard, int drawType)
{
	QString strDrawType;

	if (0 == drawType)
	{
		strDrawType = QString::fromLocal8Bit("人工模式");
	}
	else if (1 == drawType)
	{
		strDrawType = QString::fromLocal8Bit("自动化模式");
	}
	else if (2 == drawType || 3 == drawType)
	{
		strDrawType = QString::fromLocal8Bit("设计模式");
	}

	const int result = QMessageBox::question(this, QString::fromLocal8Bit("提示")
		, QString::fromLocal8Bit("您当前的绘制方式与数据库冲突,是否删除所有%1%2病害?").arg(standard).arg(strDrawType),
		QString::fromLocal8Bit("是"), QString::fromLocal8Bit("否"));

	if (0 == result)
	{
	 

		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteAllTargetTypeDisease(standard, drawType);
		return true;
	}
	else
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("病害绘制方式与数据库冲突，不允许打开工程，如需切换绘制方式，请重新打开工程"),
			QString::fromLocal8Bit("确定"));
		hnApp::hnDataManager::getDataManager()->closeCurrentProject();
		clearCurrentProjectUiState();
		return false;
	}
}





void hnRoadDataProcess::slot_jumpToMile(double mile)
{
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}

	double encoderMile = hnDataManager::getDataManager()->getCurrentProject()->trueMileToEncl(mile);
	encoderMile = qMax(0.0, encoderMile);

	auto projectType = hnDataManager::getDataManager()->getCurrentProject()->getProjectType();
	if (PROJECT_23D_TYPE == projectType || PROJECT_2D_TYPE == projectType)
	{
		m_2dPixScrollWidget->getPixWidget()->scrollBottomToEncoderMile(encoderMile);
		syncContinuousViews(ContinuousViewSyncSource::Road2D, encoderMile);
	}
	else
	{
		m_3dPixScrollWidget->getPixWidget()->scrollBottomToEncoderMile(encoderMile);
	}
}

void hnRoadDataProcess::slot_jumpToMark(int markId, double trueMile, double tableEncoderMile)
{
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}

	auto project = hnDataManager::getDataManager()->getCurrentProject();
	if (!project)
	{
		return;
	}

	// 表格第一列显示的是真实桩号；编码器里程必须和真实桩号能互相校验，避免旧库 EnclMile=0 时误跳到工程开头。
	auto encoderMatchesTrueMile = [project, trueMile](double encoderMile)
	{
		if (encoderMile < 0.0)
		{
			return false;
		}
		if (trueMile <= 0.0)
		{
			return true;
		}
		const double checkedTrueMile = project->enclToTrueMile(encoderMile);
		return qAbs(checkedTrueMile - trueMile) <= 5.0;
	};

	double encoderMile = -1.0;
	if (encoderMatchesTrueMile(tableEncoderMile))
	{
		encoderMile = tableEncoderMile;
	}

	const QVector<hnCommon::hnMarkInfo> marks = project->getCurrentMarkVector();
	for (const hnCommon::hnMarkInfo& mark : marks)
	{
		if (mark.nID != markId)
		{
			continue;
		}
		if (trueMile > 0.0 && qAbs(mark.dTrueMile - trueMile) > 0.01)
		{
			continue;
		}

		if (encoderMile < 0.0 && encoderMatchesTrueMile(mark.dEnclMile))
		{
			encoderMile = mark.dEnclMile;
		}
		if (encoderMile < 0.0 && mark.dTrueMile > 0.0)
		{
			encoderMile = project->trueMileToEncl(mark.dTrueMile);
		}
		break;
	}

	if (encoderMile < 0.0)
	{
		encoderMile = project->trueMileToEncl(trueMile);
	}
	encoderMile = qMax(0.0, encoderMile);

	#ifdef _DEBUG
	qDebug().noquote() << "[HN_MARK_JUMP]"
		<< "markId=" << markId
		<< "trueMile=" << trueMile
		<< "tableEncoderMile=" << tableEncoderMile
		<< "targetEncoderMile=" << encoderMile;
	#endif

	const auto projectType = project->getProjectType();
	QScopedValueRollback<bool> syncingGuard(m_isProgrammaticViewSync, true);
	if (PROJECT_23D_TYPE == projectType || PROJECT_2D_TYPE == projectType)
	{
		if (m_2dPixScrollWidget && m_2dPixScrollWidget->getPixWidget())
		{
			m_2dPixScrollWidget->getPixWidget()->centerOnEncoderMile(encoderMile);
			m_2dPixScrollWidget->getPixWidget()->refreshSdkDiseaseLayer();
		}

		if (m_3dPixScrollWidget && m_3dPixScrollWidget->getPixWidget() && project->get3DProject())
		{
			const double diff2d3d = project->get2d3dMileDiff();
			m_3dPixScrollWidget->getPixWidget()->centerOnEncoderMile(qMax(0.0, encoderMile - diff2d3d));
			m_3dPixScrollWidget->getPixWidget()->refreshSdkDiseaseLayer();
		}

		syncStreetViewBy2dEncoderMile(encoderMile);
	}
	else if (m_3dPixScrollWidget && m_3dPixScrollWidget->getPixWidget())
	{
		m_3dPixScrollWidget->getPixWidget()->centerOnEncoderMile(encoderMile);
		m_3dPixScrollWidget->getPixWidget()->refreshSdkDiseaseLayer();
	}
}
void hnRoadDataProcess::openProjectSlot()
{
	if (m_outExcelDialog != nullptr)
	{
		delete m_outExcelDialog;
		m_outExcelDialog = nullptr;
	}

	//获取用户选择的文件夹
	QString projectPath = QFileDialog::getExistingDirectory(this, QStringLiteral("请选择工程路径,二维,三维,二三维工程不支持混合导入处理!"), m_xrSetting->DefaultPath);
	if (projectPath.isEmpty())
	{
		return;
	}

	m_projectListTreeWidget->clear();

	m_xrSetting->DefaultPath = projectPath;
	m_xrSetting->writeData();
	hnCommon::PROJECT_TYPE  nWorkType = PROJECT_2D_TYPE; 
	//获取所有工程
	m_projectDataInfos.clear();
	if (!m_projects->getAllProject(projectPath, m_projectDataInfos, nWorkType))
	{
		QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("未解析任何到符合格式的项目数据，请检查"),
			QStringLiteral("确定"));
		return;
	} 
	//progressDialog.reset();
	//打开工程对话框
	auto roadTypes = hnDataManager::getDataManager()->getRoadStandardNames();
	hnOpenProjectDlg openProjectDlg(nWorkType, roadTypes, m_projectDataInfos, this);
	const int rc = openProjectDlg.exec();

	if (rc == QDialog::Accepted)
	{
		m_projectDataInfos = openProjectDlg.getProjectSettingInfo();
	}
	else
	{
		return;
	}
	 
	BusyLoadingGuard loading(this, QStringLiteral("打开工程"), QStringLiteral("正在打开工程，请稍后......"));
	
	clearCurrentProjectUiState();
	if (m_projects->isHasProject())
	{
		m_projects->closeProject();
	}

	//根据各个模块标准设置其病害表名称
	m_projects->setProjectDiseaseVector(m_projectDataInfos);
	//初始化工程
	loading.setMessage(QStringLiteral("正在初始化工程..."));
	bool initok =  m_projects->initProject(m_projectDataInfos);
	if (initok)
	{
	
		loading.setMessage(QStringLiteral("正在加载所有视图的图片数据..."));
		// 加载所有视图的图片数据
		this->allWidgetLoadPictures(); 

		loading.setMessage(QStringLiteral("正在更新树状视图..."));

		//更新树状视图
		this->updateTreeWidget(); 
		loading.setMessage(QStringLiteral("正在更新更新所有界面..."));

		//更新所有界面
		this->updateAllWidget();
		
	}
 
}

void hnRoadDataProcess::openLastProjectSlot()
{
	QElapsedTimer totalTimer;
	QElapsedTimer stepTimer;
	totalTimer.start();
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][RecentProjectStart]"
		<< "time=" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
		<< "defaultPath=" << m_xrSetting->DefaultPath
		<< "lastProject=" << m_xrSetting->lastProjectName
		<< "lastFrame=" << m_xrSetting->lastProjectFn;
	#endif
	 
	//获取用户选择的文件夹
	QString projectPath = m_xrSetting->DefaultPath;
	if (projectPath.isEmpty())
	{
		#ifdef _DEBUG
		qDebug().noquote() << "[HN_PERF][RecentProjectEnd]" << "reason=emptyDefaultPath" << "totalMs=" << totalTimer.elapsed();
		#endif
		QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("未找到最近工程"),
			QStringLiteral("确定"));
		return;
	}
	m_projectListTreeWidget->clear();
	hnCommon::PROJECT_TYPE  nWorkType = PROJECT_2D_TYPE;


	//获取所有工程
	m_projectDataInfos.clear();
	stepTimer.start();
	if (!m_projects->getAllProject(projectPath, m_projectDataInfos, nWorkType))
	{
		#ifdef _DEBUG
		qDebug().noquote() << "[HN_PERF][RecentProjectEnd]"
			<< "reason=getAllProjectFailed"
			<< "stepMs=" << stepTimer.elapsed()
			<< "totalMs=" << totalTimer.elapsed();
		#endif
		QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("未找到最近工程"),
			QStringLiteral("确定"));
		return;
	}
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][RecentProjectStep]"
		<< "step=getAllProject"
		<< "elapsedMs=" << stepTimer.elapsed()
		<< "projectCount=" << m_projectDataInfos.size();
	#endif

	BusyLoadingGuard loading(this, QStringLiteral("打开工程"), QStringLiteral("正在打开工程，请稍后......"));
 
	stepTimer.restart();
	clearCurrentProjectUiState();
	if (m_projects->isHasProject())
	{
		m_projects->closeProject();
	}
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][RecentProjectStep]" << "step=closeAndClear" << "elapsedMs=" << stepTimer.elapsed();
	#endif
	 

	//根据各个模块标准设置其病害表名称
	stepTimer.restart();
	m_projects->setProjectDiseaseVector(m_projectDataInfos);
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][RecentProjectStep]" << "step=setProjectDiseaseVector" << "elapsedMs=" << stepTimer.elapsed();
	#endif

	loading.setMessage(QStringLiteral("正在初始化工程..."));
	//初始化工程
	stepTimer.restart();
	m_projects->initProject(m_projectDataInfos);
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][RecentProjectStep]" << "step=initProject" << "elapsedMs=" << stepTimer.elapsed();
	#endif

	 
	loading.setMessage(QStringLiteral("正在加载所有视图的图片数据..."));
	// 加载所有视图的图片数据
	stepTimer.restart();
	this->allWidgetLoadPictures();
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][RecentProjectStep]" << "step=allWidgetLoadPictures" << "elapsedMs=" << stepTimer.elapsed();
	#endif

	loading.setMessage(QStringLiteral("正在更新树状视图..."));

	//更新树状视图
	stepTimer.restart();
	this->updateTreeWidget();
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][RecentProjectStep]" << "step=updateTreeWidget" << "elapsedMs=" << stepTimer.elapsed();
	#endif
	QTreeWidgetItem  * item = new QTreeWidgetItem(m_projectListTreeWidget);
	item->setText(0, m_xrSetting->lastProjectName);
	item->setData(1, Qt::UserRole, 0);
	//选中最后工程
	stepTimer.restart();
	slot_dClickTreeItem(item, 0);
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][RecentProjectStep]" << "step=slot_dClickTreeItem" << "elapsedMs=" << stepTimer.elapsed();
	#endif

	// Jump to the frame remembered by the last-project setting.
	int frameNum2d = m_xrSetting->lastProjectFn;

	stepTimer.restart();
	double roadImageDistance = 2.0;
	if (hnDataManager::getDataManager()->getCurrentProject()->get2DProject())
	{
		roadImageDistance = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->_RoadImgDis;
	}
	const double targetEncoderMile = qMax(0, frameNum2d - 1) * roadImageDistance;
	m_2dPixScrollWidget->getPixWidget()->scrollBottomToEncoderMile(targetEncoderMile);
    syncContinuousViews(ContinuousViewSyncSource::Road2D, targetEncoderMile);
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][RecentProjectStep]" << "step=scrollSdkToLastFrame" << "elapsedMs=" << stepTimer.elapsed();
	#endif
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][RecentProjectEnd]"
		<< "totalMs=" << totalTimer.elapsed()
		<< "targetFrame=" << frameNum2d;
	#endif
	 
}

void hnRoadDataProcess::slot_openCurrentProjectDir()
{
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("请先打开工程"),
			QString::fromLocal8Bit("确定"));
		return;
	}

	QString projectAbsulotelyPath = hnDataManager::getDataManager()->getCurrentProject()->getAbsulotelyPath();

	if (projectAbsulotelyPath == "")
	{
		return;
	}

	// 打开文件夹
	QString url = "file:///" + projectAbsulotelyPath;

	QDesktopServices::openUrl(QUrl::fromLocalFile(url));
}

void hnRoadDataProcess::slot_outSimpleProject()
{
	//选择输出目录
	//获取用户选择的文件夹
	QString outProjectPath = QFileDialog::getExistingDirectory(this, QStringLiteral("请选择工程路径"));
	if (outProjectPath.isEmpty())
	{
		return;
	}

	QList<QString> out2dFilePath
	{
		R"(\RUT\camera0\orirut.txt)",
		R"(\RUT\camera1\orirut.txt)",
		R"(\IRIMTD\DAQ0\IRI_10m.txt)",
		R"(\IRIMTD\DAQ0\resample.txt)",
		R"(\IRIMTD\DAQ0\Setting.ini)",
		R"(\IRIMTD\DAQ0\Coeff.dat)",
		R"(\IRIMTD\DAQ0\ReSample250.txt)",
		R"(\IRIMTD\DAQ0\Speed_10m.txt)",
		R"(\IRIMTD\DAQ1\IRI_10m.txt)",
		R"(\IRIMTD\DAQ1\resample.txt)",
		R"(\IRIMTD\DAQ1\Setting.ini)",
		R"(\IRIMTD\DAQ1\Coeff.dat)",
		R"(\IRIMTD\DAQ1\ReSample250.txt)",
		R"(\IRIMTD\DAQ1\Speed_10m.txt)",
		R"(\IRIMTD\Laser0\MTD_10m.txt)",
		R"(\IRIMTD\Laser0\Setting.ini)",
		R"(\IRIMTD\Laser1\MTD_10m.txt)",
		R"(\IRIMTD\Laser1\Setting.ini)",
		R"(\IRIMTD\Laser2\MTD_10m.txt)",
		R"(\IRIMTD\Laser2\Setting.ini)",
		R"(\IRIMTD\Laser0\MPD_10m.txt)",
		R"(\IRIMTD\Laser1\MPD_10m.txt)",
		R"(\IRIMTD\Laser2\MPD_10m.txt)",

		R"(\RoadImg\Camera0\Road2Mile.txt)",
		R"(\RoadImg\SYN\gps.txt)",
		R"(\RoadImg\SYN\trigger.txt)",
		R"(\StreetImg\Camera0\Street2Mile.txt)",
		R"(\StreetImg\SYN\gps.txt)",
		R"(\StreetImg\SYN\trigger.txt)",
		R"(\StreetImg\Camera1\Street2Mile.txt)",

		R"(\DegreeInfo.txt)",
		R"(\Dmi2Mile.txt)",
		R"(\GPS2Mile.txt)",
		R"(\GPSInfo.txt)",
		R"(\GPSTime2Dmi.txt)",
		R"(\ProjectInfo.txt)",
		R"(\RoadTypeInfo.txt)",
		R"(\Setting.ini)",
		R"(\2d3dDiffSetting.ini)",

		R"(\MileStoneCaliInfo.txt)"
	};

	QList<QString> out3dFilePath
	{
		R"(\PointCloud\1\Mms-Cam-1.cam)",
		R"(\Image\pavement-cam-1.idx)",
		R"(\3dProjectConfig.ini)",
		R"(\mirroredSetting.ini)",
		R"(\Mms-Para.config)",
		R"(\Mms-Para.db)",

	};

	hnPro::hnProjectManager* manager = hnApp::hnDataManager::getDataManager()->getProjectManager();
	auto allPorject = manager->getAllBaseProject();
	for (auto project : allPorject)
	{
		auto type = project->getProjectType();
		QString outPath = outProjectPath + "\\" + project->getProjectName();
		QDir outBaseDir(outPath);
		QString project2dBasePath;
		QString outBaseProjectDir;
		if (!outBaseDir.exists())
		{
			if (type == hnCommon::PROJECT_2D_TYPE)
			{
				project2dBasePath = outPath;
				outBaseProjectDir = outPath;
			}
			else
			{
				outBaseDir.mkdir(outPath);
				QString timeDirName = project->getAbsulotelyPath().split("/").last();
				outBaseProjectDir = outPath + "\\" + timeDirName;
				if (!outBaseDir.exists(outBaseProjectDir))
				{
					outBaseDir.mkdir(outBaseProjectDir);
				}

#pragma region 复制工程信息

				QString settingInfoFilePath = outBaseProjectDir + "\\ProjectInfo.xml";
				QFile toolFile;
				toolFile.copy(project->getAbsulotelyPath() + "\\ProjectInfo.xml", settingInfoFilePath);
				project2dBasePath = outBaseProjectDir + "\\" + project->get2DProName();
#pragma endregion
			}
		}





#pragma region 复制必要的二维数据

		project->getCurDB()->m_mileInfoTable.writeData_Simple(project->getCurrentMileVector());

		if (!outBaseDir.exists(project2dBasePath))
		{
			outBaseDir.mkdir(project2dBasePath);
		}
		for (QString& file2dPath : out2dFilePath)
		{

			QString realOriOut2dPath = project->get2DProPath() + file2dPath;
			QString realOut2dPath = project2dBasePath + file2dPath;
			QFile tempFile(realOriOut2dPath);
			QFileInfo tempFileInfo(realOut2dPath);
			QString tempFolderPath = tempFileInfo.absolutePath();
			QDir tempDir(tempFolderPath);
			if (!tempDir.exists())
			{
				bool mkResult = tempDir.mkpath(tempFolderPath);
			}
			if (tempFile.exists())
			{
				tempFile.copy(realOut2dPath);
			}
		}
		//复制5张图像
		QStringList roadPicturePaths = project->get2DProject()->getRoadPicturePath();
		for (int picIdx = 0; picIdx <= 5; picIdx++)
		{
			if (picIdx >= roadPicturePaths.size())
			{
				continue;
			}
			QString curFilePath = roadPicturePaths[picIdx];
			QFile curFile(curFilePath);
			//构建输出路径
			QString basePicPath = project2dBasePath + R"(\RoadImg\Camera0\Image_0000\)";

			QDir tempDir(basePicPath);
			if (!tempDir.exists())
			{
				bool mkResult = tempDir.mkpath(basePicPath);
			}

			if (curFile.exists())
			{
				QFileInfo curFileInfo(curFile);
				QString outPicPath = basePicPath + curFileInfo.fileName();
				curFile.copy(outPicPath);
			}
		}
#pragma endregion

#pragma region 复制成果数据
		QString resultDbPath = project->getAbsulotelyPath() + QStringLiteral("\\成果数据\\");
		QString outResultDbPath = outBaseProjectDir + QStringLiteral("\\成果数据\\");
		QDir resultDir(resultDbPath);
		if (resultDir.exists())
		{
			if (!resultDir.exists(outResultDbPath))
			{
				resultDir.mkpath(outResultDbPath);
			}

			QFileInfoList entries = resultDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
			for (const QFileInfo & entity : entries)
			{
				QString sourceEntryPath = entity.absoluteFilePath();
				if (entity.isDir())
				{
					QString outEntryDirPath = outResultDbPath + entity.fileName();
					if (!resultDir.exists(outEntryDirPath))
					{
						resultDir.mkdir(outEntryDirPath);
					}
					QDir oriEntryDirPathDir(outEntryDirPath);
					QDir orientryDirPathDir(sourceEntryPath);
					QFileInfoList outEntrys = orientryDirPathDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
					for (const QFileInfo & outEntity : outEntrys)
					{
						QString outFilePath = outEntryDirPath + "\\" + outEntity.fileName();
						QFile outEntityFile(outEntity.absoluteFilePath());
						outEntityFile.copy(outFilePath);
					}

				}


			}
		}
#pragma endregion

		switch (type)
		{
		case hnCommon::PROJECT_2D_TYPE:
			break;
		case hnCommon::PROJECT_XD_3D_TYPE:
		case hnCommon::PROJECT_JD_3D_TYPE:
		case hnCommon::PROJECT_23D_TYPE:
		{
#pragma region 复制必要的三维数据
			QString outPosBaseDir = outPath + "\\POS";
			if (!outBaseDir.exists(outPosBaseDir))
			{
				outBaseDir.mkdir(outPosBaseDir);
			}
			QString project3dBasePath = outBaseProjectDir + "\\" + project->get3DProName();
			for (QString& file3dPath : out3dFilePath)
			{

				QString realOriOut3dPath = project->get3DProPath() + "\\" + project->get3DProName() + file3dPath;
				QString realOut3dPath = project3dBasePath + file3dPath;
				QFile tempFile(realOriOut3dPath);
				QFileInfo tempFileInfo(realOut3dPath);
				QString tempFolderPath = tempFileInfo.absolutePath();
				QDir tempDir(tempFolderPath);
				if (!tempDir.exists())
				{
					tempDir.mkpath(tempFolderPath);
				}
				if (tempFile.exists())
				{
					tempFile.copy(realOut3dPath);
				}
			}


#pragma endregion



#pragma region 复制必要的三维数据

#pragma endregion

			break;
		}
		default:
			break;
		}
	}


	QMessageBox::about(this, QStringLiteral("提示"), QStringLiteral("输出完毕!"));
}

void hnRoadDataProcess::slot_gpsMatching()
{
	if (!m_projects->isHasProject())
	{
		return;
	}

	hnPro::hnProjectManager* manager = hnApp::hnDataManager::getDataManager()->getProjectManager();
	auto allPorject = manager->getAllBaseProject();
	QMessageBox msgBox;
	msgBox.setWindowTitle(QStringLiteral("设备选择"));
	msgBox.setText(QStringLiteral("二三维设备采集选择【是】, 模块化设备采集选择【否】"));
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);
	int ret = msgBox.exec();
	if (ret == QMessageBox::Yes)
	{
		m_xrSetting->equipType = 1;
	}
	else if (ret == QMessageBox::No)
	{
		m_xrSetting->equipType = 0;
	}
	m_xrSetting->writeData();
	for (auto project : allPorject)
	{
		auto type = project->getProjectType();
		switch (type)
		{
		case hnCommon::PROJECT_XD_3D_TYPE:
			break;
		case hnCommon::PROJECT_JD_3D_TYPE:
			break;
		case hnCommon::PROJECT_2D_TYPE:
		case hnCommon::PROJECT_23D_TYPE:
		{
			QString projectBase2dPath = project->get2DProPath();

			QApplication::setOverrideCursor(Qt::WaitCursor);
			MappingGPS2Mile(project, projectBase2dPath);

			break;
		}
		default:
			break;
		}
	}

	QApplication::restoreOverrideCursor();

}

void hnRoadDataProcess::MappingGPS2Mile(hnPro::hnProject* project, QString baseProjectPath)
{
	bool IsIRIMap = false;
	bool IsRoadMap = false;
	bool IsRutMap = false;
	bool IsStreetMap = false;

	IsRoadMap = GetRoadGPSTime2Dmi(project, "Road");

	if (!IsRoadMap)
	{
		IsRoadMap = GetRoadGPSTime2Dmi(project, "Street");
	}
	if (IsRoadMap)
	{
		bool ok = false;
		QStringList subpath = { "\\GPSModel\\gps.txt","\\gps.dat", "\\RoadImg\\SYN\\gps.txt", "\\StreetImg\\SYN\\gps.txt",
			"\\IRIMTD\\SYN0\\gps.txt","\\IRIMTD\\SYN1\\gps.txt","\\camera0\\gps.txt","\\camera1\\gps.txt" };
		QString gpsfile;

		for (int i = 0; i < subpath.size(); i++)
		{
			gpsfile = project->get2DProPath() + subpath.at(i);
			QFile gpsFile(gpsfile);
			if (gpsFile.exists() && !ok)
			{
				ok = GetGPSMileMapping(project, gpsfile);
				if (!ok)
				{
					continue;
				}
				break;
			}

		}
		if (ok)
		{
			QString  highGpsFilePath = baseProjectPath + "/GPSModel/gps.txt";
			if (QFile::exists(highGpsFilePath))
			{

				HighAccuracyPositioning  position(project);
				position.writeHighAccPicture();
			}
			project->get2DProject()->initGpsInfos();
		}
		if (!ok)
		{
			QMessageBox msgBox;
			msgBox.setWindowTitle(QStringLiteral("错误"));
			msgBox.setText(project->get2DProName() + QStringLiteral("所有gps文件有效gps信息不足5条,gps计算失败!"));
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setDefaultButton(QMessageBox::Ok);
			int ret = msgBox.exec();
		}
	}
	else
	{
		QMessageBox msgBox;
		msgBox.setWindowTitle(QStringLiteral("错误"));
		msgBox.setText(QStringLiteral("生成 GPSTime2Dmi.txt GPS时间和里程映射文件 失败！"));
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		int ret = msgBox.exec();
	}


}

bool hnRoadDataProcess::GetRoadGPSTime2Dmi(hnPro::hnProject* project, QString ImgSource)
{
	QString gpsModelFile = project->get2DProPath() + "/GPSModel/gps.txt";
	bool needSub1s = false;

	if (QFile::exists(gpsModelFile)) {
		needSub1s = true;
		if (m_xrSetting->equipType == 1) {
			needSub1s = false;
		}
	}
	else {
		needSub1s = false;
	}

	QString fpath = QString("%1/%2Img/SYN/trigger.txt").arg(project->get2DProPath()).arg(ImgSource);
	if (!QFile::exists(fpath)) return false;

	QFile file(fpath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

	QList<QString> syntrigstrs;
	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine();
		syntrigstrs.append(line);
	}
	file.close();

	QList<QString> tempStrs;
	QSet<QString> filterSet = { "G", "g", "�" };

	QList<QString> filteredList;
	for (const QString &item : syntrigstrs)
	{
		if (!(filterSet.contains(item)
			&& (item.startsWith("%CDA")
				|| item.startsWith("%BDA")
				|| item.startsWith("%XRC"))))
		{
			filteredList.push_back(item);
		}
	}
	syntrigstrs = filteredList;

	QString number, preNumber;
	qint64 timeLong = 0, preTime = 0;
	int errorRow = 0;

	for (int i = 0; i < syntrigstrs.size() - 1; ++i) {
		try {
			errorRow++;
			QString nowStr = syntrigstrs[i];
			number = nowStr.split(',')[1];
			QString resultStr = syntrigstrs[i + 1];
			preNumber = resultStr.split(',')[1];
			qint64 lTemp0 = nowStr.split(',')[2].toLongLong();
			qint64 lTemp1 = resultStr.split(',')[2].toLongLong();
			if (number == preNumber) {
				continue;
			}
			else {
				tempStrs.append(nowStr);
			}
		}
		catch (...) {
			continue;
		}
	}
	tempStrs.append(syntrigstrs.last());

	QList<QString> tempStrs1;
	if (tempStrs.size() >= 3) {
		tempStrs1.append(tempStrs[0]);
		for (int i = 1; i < tempStrs.size() - 1; ++i) {
			QString pre = tempStrs[i - 1];
			QString now = tempStrs[i];
			QString last = tempStrs[i + 1];
			tempStrs1.append(now);
		}
		tempStrs1.append(tempStrs.last());
	}
	errorRow = 0;
	syntrigstrs = tempStrs1;
	if (syntrigstrs.size() < 1) return false;
	QString gPath = QString("%1/%2Img/SYN/gps.txt").arg(project->get2DProPath()).arg(ImgSource);

#pragma region 20250709 修复同步版60未进位导致的bug
	int addTime = 0;
	QFile tempGpsFile(gPath);
	if (tempGpsFile.exists())
	{
		QStringList gStrs = MyCommonMethods::ReadAllLines(gPath);
		if (gStrs.size() > 0)
		{
			QString ansLines = gStrs[gStrs.size() - 2];
			if (ansLines.contains("%ANS"))
			{
				QStringList oneDatas = ansLines.split(',');
				if (oneDatas.size() >= 10)
				{
					int findIndex = oneDatas.indexOf("S");
					QString timeStr = "";
					if (findIndex == 8)
					{
						timeStr = oneDatas[6];
					}
					else if (findIndex == 10)
					{
						timeStr = oneDatas[9];
					}

					if (timeStr.length() == 6)
					{
						bool ok;
						//解析小时部分
						int hour = timeStr.mid(0, 2).toInt(&ok);
						if (!ok)
						{
							hour = 0;
						}
						//解析分钟部分
						int min = timeStr.mid(2, 2).toInt(&ok);
						if (!ok)
						{
							min = 0;
						}
						int mm = timeStr.mid(4, 2).toInt(&ok);
						if (!ok)
						{
							mm = 0;
						}
						if (min == 60)
						{
							addTime = 60 * 61;
						}

						if (mm == 60)
						{
							addTime = 61;
						}
					}
				}
			}
		}
	}
#pragma endregion


	TempGpsStrs tempGpsStrs;
	QString ymd;
	if (QFile::exists(gPath)) {
		QFile gFile(gPath);
		if (!gFile.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

		QList<QString> gStrs;
		QTextStream gIn(&gFile);
		while (!gIn.atEnd()) {
			QString line = gIn.readLine();
			gStrs.append(line);
		}
		gFile.close();
		QString oTimeStr;
		//QString oTimeStr = gStrs.filter([](const QString& t) { return t.contains("%ANS"); }).value(0);

		for (const QString& timeStr : gStrs)
		{
			if (timeStr.contains("%ANS"))
			{
				oTimeStr = timeStr;
				break;
			}
		}

		QDateTime oTime;
		if (!oTimeStr.isEmpty() && needSub1s) {
			QStringList splitOtime = oTimeStr.split(',');
			QString oTimeS;
			if (splitOtime.size() < 26) {
				oTimeS = splitOtime[5] + splitOtime[6];
			}
			else {
				oTimeS = splitOtime[8] + splitOtime[9];
			}
			qint64 longTime = oTimeS.toLongLong();
			oTime = QDateTime::fromString(oTimeS, "yyyyMMddHHmmss");
			ymd = oTime.toString("yyyyMMdd");
			tempGpsStrs = TempGpsStrs(gStrs, oTime, ymd);
		}
	}
	else {
		return false;
	}

	bool IsFirst = true;
	int tidx;
	int dmival = 0;
	int linecnt = 0;
	float startdmi0 = 0;
	float dmival0 = 0;
	QString curtime = "000000000";
	QString curIndex = "000000000";
	QString ttstr;
	QStringList strs;
	QList<SynTrigInfo> syntriglist;
	SynTrigInfo lastinfo;
	double firstDmi = 0;

	for (const QString& linestr : syntrigstrs) {
		errorRow++;
		++linecnt;
		if (linestr.startsWith("%CDA") || linestr.startsWith("%BDA")) {
			try {
				tidx = linestr.lastIndexOf('%');
				tidx = tidx > 0 ? tidx : 0;
				ttstr = linestr.mid(tidx);
				ttstr = ttstr.replace(':', '9');
				ttstr = ttstr.replace('o', '9');
				strs = ttstr.split(',');
			}
			catch (...) {
				continue;
			}
			if (strs.size() == 7) {
				try {
					SynTrigInfo tinfo(strs[1], strs[2], strs[3], strs[5]);
					if (IsFirst && tinfo._trigdmi > 0)
					{
						syntriglist.append(tinfo);
						IsFirst = false;
					}
					else if (tinfo._trigdmi != lastinfo._trigdmi)
					{
						syntriglist.append(tinfo);
					}
					lastinfo = tinfo;

				}
				catch (...) {
					continue;
				}
			}
		}
		else if (linestr.startsWith("%XRC")) {
			if (linestr.length() < 33) continue;
			strs = linestr.split(',');
			if (strs.size() < 3) continue;
			if (strs[2].length() == 9) {
				curtime = strs[2];
				curIndex = strs[1];
				QString nowStr = curtime;

				if (addTime != 0)
				{
					QDate currentDate = QDate::currentDate();
					QTime timePart = QTime::fromString(nowStr, "HHmmsszzz");
					QDateTime nowDataTime(currentDate, timePart);
					if (nowDataTime.isValid())
					{
						QDateTime realNowDateTime = nowDataTime.addSecs(addTime);
						if (realNowDateTime.isValid())
						{
							curtime = realNowDateTime.toString("HHmmsszzz");
						}
						else
						{
							curtime = nowStr;
						}
					}
				}
				else
				{

				}


				if (tempGpsStrs.gpsStrs.size() > 0) {
					for (const QDateTime& time : tempGpsStrs.NeedSub1sList) {
						nowStr = ymd + curtime;
						QDateTime now = QDateTime::fromString(nowStr, "yyyyMMddHHmmssfff");
						if (now.time().second() == time.time().second()) {
							qint64 lNow = curtime.toLongLong();
							qint64 realNow = lNow - 1000;
							curtime = QString::number(realNow).rightJustified(9, '0');
						}
					}
				}
			}
			else {
				continue;
			}
			if (strs[3].length() == 8) {
				try {
					dmival = strs[3].toInt(nullptr, 16);
					dmival0 = static_cast<float>(dmival * 0.001);
				}
				catch (...) {
					continue;
				}
			}
			else {
				continue;
			}
			if (strs[1].length() == 8) {
				try {
					if (strs[1].toInt(nullptr, 16) == 0 && linecnt == 1) {
						startdmi0 = dmival0;
					}
				}
				catch (const QException& ex) {
					qDebug() << "gps桩号匹配功能错误:文件" << fpath << "解析时在第" << errorRow << "行出现错误，请提交专业人员检查";
					continue;
				}
			}
			else {
				continue;
			}
			dmival0 = std::round(dmival0 - startdmi0);  //帧号取证 防止1999的情况
			try {
				if (syntriglist.size() > 0) {
					// if (qAbs(syntriglist.last()._trigdmi.toDouble() - dmival0) > 2000) {
					//     continue;
					// }
				}
			}
			catch (const QException& ex) {
				qDebug() << "gps桩号匹配功能错误:文件" << fpath << "解析时在第*" << errorRow << "*行出现错误，请提交专业人员检查";
				continue;
			}
			SynTrigInfo tinfo(curIndex, project->get2DProject()->_DataDate, curtime, QString::number(dmival0));
			syntriglist.append(tinfo);
		}
	}

	int triglen = syntriglist.size();
	if (triglen < 3) return false;

	for (int i = triglen - 2; i >= 0; --i) {
		if (syntriglist[i]._trigdmi == syntriglist[i + 1]._trigdmi) {
			syntriglist.removeAt(i + 1);
		}
	}
	triglen = syntriglist.size();

	try {
		SynTrigInfo tempTrigInfo;
		QList<QString> gpstrigstrs;
		for (int i = 0; i < syntriglist[0]._trigdmi; i += 2) {
			tempTrigInfo = SynTrigInfo(syntriglist[0], syntriglist[1], i);
			gpstrigstrs.append(tempTrigInfo.toString());
		}
		double otherDmi = 0;
		int times = 0;
		for (int i = 1; i < triglen; ++i)
		{
			QString synValueString = syntriglist[i - 1].toString();
			gpstrigstrs.append(syntriglist[i - 1].toString());
			for (double j = syntriglist[i - 1]._trigdmi + 2; j < syntriglist[i]._trigdmi; j += 2)
			{

				int indexSub = qAbs(syntriglist[i - 1]._FrameIndex - syntriglist[i]._FrameIndex) * 2;
				bool needAdd = indexSub == (syntriglist[i]._trigdmi - syntriglist[i - 1]._trigdmi);
				if (needAdd) {
					times++;
					tempTrigInfo = SynTrigInfo(syntriglist[i - 1], syntriglist[i], j);
					synValueString = tempTrigInfo.toString();
					gpstrigstrs.append(tempTrigInfo.toString());
				}

			}
		}
		double sumStopLength = 0;
		for (int i = 1; i < gpstrigstrs.size(); ++i) {
			double preDmi = gpstrigstrs[i - 1].split(' ')[1].toDouble();
			double nowDmi = gpstrigstrs[i].split(' ')[1].toDouble();
			otherDmi = (nowDmi - preDmi - 2);
			if (otherDmi != 0) {
				sumStopLength += otherDmi;
				for (int d = i; d < gpstrigstrs.size(); d++) {
					double newDmi = gpstrigstrs[d].split(' ')[1].toDouble() - otherDmi;
					QString newStr = gpstrigstrs[d].split(' ')[0] + " " + QString::number(newDmi);
					gpstrigstrs[d] = newStr;
				}
			}
		}
		triglen--;
		QString lastMsg = syntriglist[triglen]._trigtime + " " + QString::number(syntriglist[triglen]._trigdmi - sumStopLength);
		gpstrigstrs.append(lastMsg);
		double endEnclMile = project->getCurProSetInfo().dEndEnclMile;
		for (double i = syntriglist[triglen]._trigdmi + 2; i <= endEnclMile; i += 2) {
			try {
				tempTrigInfo = SynTrigInfo(syntriglist[triglen - 1], syntriglist[triglen], i);
				gpstrigstrs.append(tempTrigInfo.toString());
			}
			catch (...) {
				throw;
			}
		}

		if (QFile::exists(gPath)) {
			QFile gFile(gPath);
			if (!gFile.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

			QList<QString> gStrs;
			QTextStream gIn(&gFile);
			while (!gIn.atEnd()) {
				QString line = gIn.readLine();
				gStrs.append(line);
			}
			gFile.close();

			GPSInfo tgpsinfoFirst;
			for (const QString& line : gStrs)
			{
				GPSInfo tgpsinfo(line);
				if (tgpsinfo._IsOK)
				{
					tgpsinfoFirst = tgpsinfo;
					break;
				}
			}
			if (!tgpsinfoFirst._IsOK) {
				return false;
			}
			if (gpstrigstrs.size() > 0) {
				QDateTime dt;
				QString time = gpstrigstrs[0].split(' ').first();
				dt = dt.addSecs(time.mid(0, 2).toInt() * 3600 + time.mid(2, 2).toInt() * 60 + time.mid(4, 2).toInt());
				dt = dt.addMSecs(time.mid(6, 3).toInt());
				qint64 diff = dt.toMSecsSinceEpoch() - tgpsinfoFirst._utctime.toMSecsSinceEpoch();

				if (diff > 600000) { // 10 minutes in milliseconds
					return false;
				}
			}
			else {
				return false;
			}
		}

		QFile outFile(QString("%1/GPSTime2Dmi.txt").arg(project->get2DProPath()));
		if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

		QTextStream out(&outFile);
		for (const QString& line : gpstrigstrs) {
			out << line << "\n";
		}
		outFile.close();

	}
	catch (...) {
		return false;
	}

	return true;
}

bool hnRoadDataProcess::GetGPSMileMapping(hnPro::hnProject* project, QString gps_fname)
{
	int i = 0, j = 0;
	int hour = 0;
	int minute = 0;
	int second = 0;
	int msSecond = 0;
	QDate validDate = QDate::currentDate();
	QList<GPSInfo> GPSInfoList;
	QList<QString> GPSInfoStrs;
	QFile gpsFile(gps_fname);
	if (!gpsFile.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
	double elevation = 0;
	QTextStream gpsIn(&gpsFile);
	int tempIdx = 0;
	while (!gpsIn.atEnd())
	{
		tempIdx++;

		QString line = gpsIn.readLine();
		GPSInfo tgpsinfo(line);
		if (tgpsinfo._IsOK)
		{
			if (tgpsinfo._elevation != 0)
			{
				elevation = tgpsinfo._elevation;
			}

			if (!GPSInfoList.isEmpty())
			{
				if (tgpsinfo._utctime != GPSInfoList.last()._utctime)
				{
					GPSInfoList.append(tgpsinfo);
					GPSInfoStrs.append(tgpsinfo);
				}
				if (tgpsinfo._utctime == GPSInfoList.last()._utctime)
				{
					if (GPSInfoList.last()._elevation == 0)
					{
						GPSInfoList.last()._elevation = elevation;
					}

				}

			}
			else
			{
				GPSInfoList.append(tgpsinfo);
				GPSInfoStrs.append(tgpsinfo);
			}
		}

	}
	gpsFile.close();

	int gps_len = GPSInfoList.size();
	if (gps_len < 5) {
		return false;
	}
	else {
		QFile gpsInfoFile(QString("%1/GPSInfo.txt").arg(project->get2DProPath()));
		if (!gpsInfoFile.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
		QTextStream gpsInfoOut(&gpsInfoFile);
		for (const QString& line : GPSInfoStrs) {
			gpsInfoOut << line << "\n";
		}
		gpsInfoFile.close();
	}

	for (i = 1; i < gps_len; ++i) {

		QDateTime nowValid(validDate, GPSInfoList[i]._utctime.time());
		QDateTime nextValid(validDate, GPSInfoList[i - 1]._utctime.time());

		qint64 t2 = nextValid.secsTo(nowValid);

		if (t2 < -72000000) {
			break;
		}
	}
	for (; i < gps_len; ++i) {
		GPSInfoList[i]._utctime = GPSInfoList[i]._utctime.addDays(1);
	}

	QList<QString> GPSMileStrList;
	QList<MapGPSMile> GPSMileList;
	QString utc_dmi_fname = QString("%1/GPSTime2Dmi.txt").arg(project->get2DProPath());
	QFile utc_dmi_file(utc_dmi_fname);
	if (!utc_dmi_file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

	QTextStream utc_dmi_in(&utc_dmi_file);
	while (!utc_dmi_in.atEnd()) {
		QString line = utc_dmi_in.readLine();
		QStringList strs = line.split(' ');
		MapGPSMile tmap;
		tmap._gpsinfo = GPSInfo();
		tmap._gpsinfo._utctime = QDateTime();

		try {
			hour = strs[0].mid(0, 2).toInt();
			minute = strs[0].mid(2, 2).toInt();
			second = strs[0].mid(4, 2).toInt();
			msSecond = strs[0].mid(6, 3).toInt();
		}
		catch (...) {
			continue;
		}

		QTime time(hour, minute, second, msSecond);
		QString temp1 = time.toString("HHmmsszzz");
		tmap._gpsinfo._utctime.setTime(time);
		tmap._dmi = strs[1].toFloat();
		tmap._mile = project->enclToTrueMile(tmap._dmi);
		tmap.time = tmap._gpsinfo;
		GPSMileList.append(tmap);
	}
	utc_dmi_file.close();
	int utc_dmi_len = GPSMileList.size();



	// 跨天的时候，日期要加1
	for (i = 1; i < utc_dmi_len; ++i)
	{

		QDateTime nowValid(validDate, GPSMileList[i]._gpsinfo._utctime.time());
		QDateTime nextValid(validDate, GPSMileList[i - 1]._gpsinfo._utctime.time());
		qint64 t3 = 0;
		if (nowValid.isValid())
		{
			t3 = nextValid.msecsTo(nowValid);

		}


		MyCommonMethods::printDateTime(nowValid);
		qDebug() << "_____________________________";
		MyCommonMethods::printDateTime(nextValid);

		if (t3 < -72000000)
		{
			break;
		}
		else if (t3 < 0)
		{
			if (t3 > -1000)
			{
				GPSMileList[i]._gpsinfo._utctime = GPSMileList[i]._gpsinfo._utctime.addSecs(1);
			}
			else if (t3 > -10000 && t3 < -9000)
			{
				GPSMileList[i]._gpsinfo._utctime = GPSMileList[i]._gpsinfo._utctime.addSecs(10);
			}
		}
	}
	for (; i < utc_dmi_len; ++i)
	{
		GPSMileList[i]._gpsinfo._utctime = GPSMileList[i]._gpsinfo._utctime.addDays(1);
	}

	bool isfind = false;
	QList<DataIdx> SynIdx;
	for (i = 0; i < utc_dmi_len; ++i)
	{
		isfind = false;
		for (j = 1; j < gps_len; ++j)
		{

			QString curTime = GPSMileList[i]._gpsinfo._utctime.time().toString("HHmmsszzz");

			if (GPSInfoList[j - 1]._utctime <= GPSMileList[i]._gpsinfo._utctime &&
				GPSInfoList[j]._utctime > GPSMileList[i]._gpsinfo._utctime)
			{
				QString findTime0 = QString(GPSInfoList[j - 1]);
				QString findTime = QString(GPSInfoList[j]);
				GPSMileList[i]._gpsinfo = GPSInfo(GPSInfoList[j - 1], GPSInfoList[j], GPSMileList[i]._gpsinfo._utctime);
				GPSMileStrList.append(GPSMileList[i]);
				isfind = true;
				break;
			}
		}
		if (!isfind)
		{
			if (GPSInfoList[0]._utctime > GPSMileList[i]._gpsinfo._utctime)
			{
				GPSMileList[i]._gpsinfo = GPSInfo(GPSInfoList[0], GPSInfoList[1], GPSMileList[i]._gpsinfo._utctime);
				GPSMileStrList.append(GPSMileList[i]);
			}
			else if (GPSInfoList.last()._utctime <= GPSMileList[i]._gpsinfo._utctime)
			{
				GPSMileList[i]._gpsinfo = GPSInfo(GPSInfoList[GPSInfoList.size() - 2], GPSInfoList.last(), GPSMileList[i]._gpsinfo._utctime);
				GPSMileStrList.append(GPSMileList[i]);
			}
		}
	}

	if (GPSMileStrList.size() == GPSMileList.size())
	{
		QFile gps2MileFile(QString("%1/GPS2Mile.txt").arg(project->get2DProPath()));
		if (!gps2MileFile.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
		QTextStream gps2MileOut(&gps2MileFile);
		for (const QString& line : GPSMileStrList)
		{
			gps2MileOut << line << "\n";
		}
		gps2MileFile.close();
	}
	else {

		return false;
	}
	return true;
}

void hnRoadDataProcess::writeStreetDiseaseMsgToExcel(int disType, hnProject* project, Document& xlsx)
{
	QVector<hnCommon::hnRoadDiseaseInfo> diss; 

	diss = hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllStreetDiseases();

	 
	for (int d = diss.size() - 1; d >= 0; d--)
	{
		if (diss[d].ndiseaseType != disType)
		{
			diss.removeAt(d);
		}
	}

	QVector<hnDiseaseSetInfo> diseaseSetInfos = hnApp::hnDataManager::getDataManager()->
		getCurrentProjectStreetDiseases(project->getBaseStandard(), disType);

	for (int i = 0; i < diss.size(); ++i)
	{
		hnDiseaseSetInfo  setInfo;
		auto dis = diss.at(i);
		QString disName = QString::fromLocal8Bit(dis.strDisName);

		for (int setInfoIndex = 0; setInfoIndex < diseaseSetInfos.size(); ++setInfoIndex)
		{
			QString name = QString::fromLocal8Bit(diseaseSetInfos.at(setInfoIndex).strDiseaseTypeName);
			if (name == disName)
			{
				setInfo = diseaseSetInfos.at(setInfoIndex);
			}
		}

		if (dis.ndiseaseType != disType)
		{
			continue;
		}
		int rowCount = i + 2;
		int  startTrueMile = qRound(project->enclToTrueMile(dis.dDmi));
		//开始桩号	
		int endTrueMile = qRound(project->enclToTrueMile(dis.dDmi));

		xlsx.write(QString("A%1").arg(rowCount), startTrueMile);
		xlsx.write(QString("B%1").arg(rowCount), endTrueMile);
		xlsx.write(QString("C%1").arg(rowCount), disName);
		QString level = dis.nLevel == 0 ? QStringLiteral("无") : dis.nLevel == 1 ? QStringLiteral("轻") : dis.nLevel == 2 ? QStringLiteral("中") : dis.nLevel == 3 ? QStringLiteral("重") : "";
		xlsx.write(QString("D%1").arg(rowCount), level);
		xlsx.write(QString("E%1").arg(rowCount), setInfo.dEffectMeasure);
		xlsx.write(QString("F%1").arg(rowCount), setInfo.nDWKF); //扣分值
		QString judgeType = setInfo.dEffectMeasure == 0 ? QStringLiteral("长度") : setInfo.dEffectMeasure == 1 ? QStringLiteral("个数") : "";
		xlsx.write(QString("G%1").arg(rowCount), judgeType); //病害单位
		xlsx.write(QString("H%1").arg(rowCount), dis.diseaseWeight);
		xlsx.write(QString("I%1").arg(rowCount), dis.dArea);
	}
}

// 检查数据
void hnRoadDataProcess::checkProSlot()
{


	//弹出界面
	if (!m_projects->isOpenProject())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QStringLiteral("请确保至少打开一个工程!"),
			QString::fromLocal8Bit("确定"));
		return;
	}

	hnPro::hnProjectManager* manager = hnApp::hnDataManager::getDataManager()->getProjectManager();
	std::vector<hnPro::hnProject*> allPorject = manager->getAllBaseProject();
	if (allPorject.size() > 0)
	{
		//创建数据检查结果文件txt
		hnPro::hnProject * firstPro = allPorject.at(0);
		QString fPath;
		if (firstPro->getProjectType() == PROJECT_TYPE::PROJECT_JD_3D_TYPE || firstPro->getProjectType() == PROJECT_TYPE::PROJECT_XD_3D_TYPE)
		{
			fPath = firstPro->get3DProPath() + QStringLiteral("/检查结果.txt");;
		}
		else
		{
			fPath = firstPro->get2DProPath() + QStringLiteral("/检查结果.txt");
		}
		QFile fFile(fPath);

		if (fFile.exists())
		{
			fFile.remove();
		}
		if (!fFile.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			qDebug() << QStringLiteral("无法打开文件");
			return;
		}
		QTextStream out(&fFile);
		for (hnPro::hnProject* curProject : allPorject)
		{

			//区分二三维项目类型 分别处理
			if (curProject->getProjectType() == PROJECT_TYPE::PROJECT_JD_3D_TYPE || curProject->getProjectType() == PROJECT_TYPE::PROJECT_XD_3D_TYPE)
			{
				//纯三维项目数据检查 
			}
			else
			{
				QString curProjectName = curProject->get2DProName();
				//二三维项目数据检查
				//检查桩号是否合法 工程数据的完整性
				hnCommon::hnProjectSetInfo setInfo = curProject->getCurProSetInfo();
				if (setInfo.nLineType == 1)
				{
					if (setInfo.dBegMile >= setInfo.dEndMile)
					{
						QString msg = QStringLiteral("不合法数据,请检查!\n工程名称:%6道路名称:%5道路编号:%4,起点桩号:%1,终点桩号:%2,行车方向:%3").arg(QString::number(setInfo.dBegMile, 'f', 2)).
							arg(QString::number(setInfo.dEndMile, 'f', 2)).arg(setInfo.nLineType == 1 ? QStringLiteral("上行") : QStringLiteral("下行")).arg(QString::fromLocal8Bit(setInfo.strNumber)).arg(QString::fromLocal8Bit(setInfo.strRoadName)).arg(curProjectName);
						out << msg;
						QMessageBox::critical(this, QString::fromLocal8Bit("错误"), msg,
							QString::fromLocal8Bit("确定"));
					}
				}
				else
				{
					if (setInfo.dBegMile <= setInfo.dEndMile)
					{
						QString msg = QStringLiteral("不合法数据,请检查!\n工程名称:%6道路名称:%5道路编号:%4,起点桩号:%1,终点桩号:%2,行车方向:%3").arg(QString::number(setInfo.dBegMile, 'f', 2)).
							arg(QString::number(setInfo.dEndMile, 'f', 2)).arg(setInfo.nLineType == 1 ? QStringLiteral("上行") : QStringLiteral("下行")).arg(QString::fromLocal8Bit(setInfo.strNumber)).arg(QString::fromLocal8Bit(setInfo.strRoadName)).arg(curProjectName);
						out << msg;
						QMessageBox::critical(this, QString::fromLocal8Bit("错误"), msg,
							QString::fromLocal8Bit("确定"));
					}
				}
				QString settingFile = curProject->get2DProPath() + QStringLiteral("/Setting.ini");
				if (!QFile::exists(settingFile))
				{
					QString msg = QStringLiteral("不合法数据,请检查!\n工程名称:%3道路名称:%1道路编号:%2,缺少工程配置文件Setting.ini,请从其他同配置工程拷贝同名文件到路径下!")
						.arg(QString::fromLocal8Bit(setInfo.strNumber)).arg(QString::fromLocal8Bit(setInfo.strRoadName)).arg(curProjectName);
					out << msg;
					QMessageBox::critical(this, QString::fromLocal8Bit("错误"), msg,
						QString::fromLocal8Bit("确定"));
				}
				//检查路面数据的完整性
				if (curProject->get2DProject()->_IsRoad)
				{
					QString roadPath = curProject->get2DProject()->getBasePath() + QStringLiteral("\\RoadImg");
					QDir roadDir(roadPath);
					if (!roadDir.exists())
					{
						QString msg = QStringLiteral("不合法数据,请检查!\n工程名称:%3道路名称:%1道路编号:%2,缺少路面文件夹!")
							.arg(QString::fromLocal8Bit(setInfo.strNumber)).arg(QString::fromLocal8Bit(setInfo.strRoadName)).arg(curProjectName);
						out << msg;
						QMessageBox::critical(this, QString::fromLocal8Bit("错误"), msg,
							QString::fromLocal8Bit("确定"));
						return;
					}
					auto roadPicData = curProject->get2DProject()->getRoadPicturePath();
					if (roadPicData.size() <= 0)
					{
						QString msg = QStringLiteral("不合法数据,请检查!\n工程名称:%3道路名称:%1道路编号:%2,缺少路面图像!")
							.arg(QString::fromLocal8Bit(setInfo.strNumber)).arg(QString::fromLocal8Bit(setInfo.strRoadName)).arg(curProjectName);
						out << msg;
						QMessageBox::critical(this, QString::fromLocal8Bit("错误"), msg,
							QString::fromLocal8Bit("确定"));
						return;
					}
					else
					{
						double dmi = 0;
						if (curProject->getProjectType() == PROJECT_TYPE::PROJECT_2D_TYPE)
						{
							dmi = setInfo.dLength;
						}
						else
						{
							dmi = setInfo.dEndEnclMile;
						}

						int imgnum = roadPicData.size() * setInfo.dRoadLength;
						if (imgnum < dmi - setInfo.dRoadLength * 5)
						{
							int temp = dmi / setInfo.dRoadLength - imgnum / setInfo.dRoadLength;
							QString msg = QStringLiteral("工程名称:%5道路名称:%1道路编号:%2,路面图像缺少图像%3张,设置拍照距离%4!")
								.arg(QString::fromLocal8Bit(setInfo.strNumber)).arg(QString::fromLocal8Bit(setInfo.strRoadName)).arg(QString::number(temp)).arg(QString::number(setInfo.dRoadLength)).arg(curProjectName);
							out << msg;
							QMessageBox::critical(this, QString::fromLocal8Bit("错误"), msg,
								QString::fromLocal8Bit("确定"));
						}
						else if (imgnum > dmi + setInfo.dRoadLength * 5)
						{
							int temp = imgnum / setInfo.dRoadLength - dmi / setInfo.dRoadLength;
							QString msg = QStringLiteral("工程名称:%5道路名称:%1道路编号:%2,路面图像多采图像%3张,设置拍照距离%4!")
								.arg(QString::fromLocal8Bit(setInfo.strNumber)).arg(QString::fromLocal8Bit(setInfo.strRoadName)).arg(QString::number(temp)).arg(QString::number(setInfo.dRoadLength)).arg(curProjectName);
							out << msg;
							QMessageBox::information(this, QString::fromLocal8Bit("信息"), msg,
								QString::fromLocal8Bit("确定"));
						}
					}
				}
				//TODO 检查数据 CWB 处理中
				//检查道路标准与道路等级的匹配情况等等
				//检查车辙数据的完整性  




				//检查路面数据的完整性
				if (curProject->get2DProject()->_IsStreet)
				{
					QString roadPath = curProject->get2DProject()->getBasePath() + QStringLiteral("\\StreetImg");
					QDir roadDir(roadPath);
					if (!roadDir.exists())
					{
						QString msg = QStringLiteral("不合法数据,请检查!\n工程名称:%3道路名称:%1道路编号:%2,缺少景观文件夹!")
							.arg(QString::fromLocal8Bit(setInfo.strNumber)).arg(QString::fromLocal8Bit(setInfo.strRoadName)).arg(curProjectName);
						out << msg;
						QMessageBox::critical(this, QString::fromLocal8Bit("错误"), msg,
							QString::fromLocal8Bit("确定"));
						return;
					}

					auto roadPciData = curProject->get2DProject()->getLeftStreetPicturePath();
					if (roadPciData.size() <= 0)
					{
						QString msg = QStringLiteral("不合法数据,请检查!\n工程名称:%3道路名称:%1道路编号:%2,缺少左侧景观图像!")
							.arg(QString::fromLocal8Bit(setInfo.strNumber)).arg(QString::fromLocal8Bit(setInfo.strRoadName)).arg(curProjectName);
						out << msg;
						QMessageBox::critical(this, QString::fromLocal8Bit("错误"), msg,
							QString::fromLocal8Bit("确定"));

					}
					else
					{
						double dmi = 0;
						if (curProject->getProjectType() == PROJECT_TYPE::PROJECT_2D_TYPE)
						{
							dmi = setInfo.dLength;
						}
						else
						{
							dmi = setInfo.dEndEnclMile;
						}

						int roadLength = curProject->get2DProject()->_StreetImgDis;
						int imgnum = roadPciData.size() * roadLength;
						if (imgnum < dmi - roadLength * 5)
						{
							int temp = dmi / roadLength - imgnum / roadLength;
							QString msg = QStringLiteral("工程名称:%5道路名称:%1道路编号:%2,左侧景观图像缺少图像%3张,设置拍照距离%4!")
								.arg(QString::fromLocal8Bit(setInfo.strNumber)).arg(QString::fromLocal8Bit(setInfo.strRoadName)).arg(QString::number(temp)).arg(QString::number(roadLength)).arg(curProjectName);
							out << msg;
							QMessageBox::critical(this, QString::fromLocal8Bit("错误"), msg,
								QString::fromLocal8Bit("确定"));
						}
						else if (imgnum > dmi + roadLength * 5)
						{
							int temp = imgnum / roadLength - dmi / roadLength;
							QString msg = QStringLiteral("工程名称:%5道路名称:%1道路编号:%2,左侧景观图像多采图像%3张,设置拍照距离%4!")
								.arg(QString::fromLocal8Bit(setInfo.strNumber)).arg(QString::fromLocal8Bit(setInfo.strRoadName)).arg(QString::number(temp)).arg(QString::number(roadLength)).arg(curProjectName);
							out << msg;
							QMessageBox::information(this, QString::fromLocal8Bit("提示"), msg,
								QString::fromLocal8Bit("确定"));
						}
					}
				}
				if (curProject->get2DProject()->_IsDStreet)
				{
					QString roadPath = curProject->get2DProject()->getBasePath() + QStringLiteral("\\StreetImg");
					QDir roadDir(roadPath);
					if (!roadDir.exists())
					{
						QString msg = QStringLiteral("不合法数据,请检查!\n工程名称:%3道路名称:%1道路编号:%2,缺少景观文件夹!")
							.arg(QString::fromLocal8Bit(setInfo.strNumber)).arg(QString::fromLocal8Bit(setInfo.strRoadName)).arg(curProjectName);
						out << msg;
						QMessageBox::critical(this, QString::fromLocal8Bit("错误"), msg,
							QString::fromLocal8Bit("确定"));
						return;
					}

					auto roadPciData = curProject->get2DProject()->getRightStreetPicturePath();
					if (roadPciData.size() <= 0)
					{
						QString msg = QStringLiteral("不合法数据,请检查!\n工程名称:%3道路名称:%1道路编号:%2,缺少右侧景观图像!")
							.arg(QString::fromLocal8Bit(setInfo.strNumber)).arg(QString::fromLocal8Bit(setInfo.strRoadName)).arg(curProjectName);
						out << msg;
						QMessageBox::critical(this, QString::fromLocal8Bit("错误"), msg,
							QString::fromLocal8Bit("确定"));
					}
					else
					{
						double dmi = 0;
						if (curProject->getProjectType() == PROJECT_TYPE::PROJECT_2D_TYPE)
						{
							dmi = setInfo.dLength;
						}
						else
						{
							dmi = setInfo.dEndEnclMile;
						}


						int roadLength = curProject->get2DProject()->_StreetImgDis;
						int imgnum = roadPciData.size() * roadLength;
						if (imgnum < dmi - roadLength * 5)
						{
							int temp = dmi / roadLength - imgnum / roadLength;
							QString msg = QStringLiteral("工程名称:%5道路名称:%1道路编号:%2,右侧景观图像缺少图像%3张,设置拍照距离%4!")
								.arg(QString::fromLocal8Bit(setInfo.strNumber)).arg(QString::fromLocal8Bit(setInfo.strRoadName)).arg(QString::number(temp)).arg(QString::number(roadLength)).arg(curProjectName);
							out << msg;
							QMessageBox::critical(this, QString::fromLocal8Bit("错误"), msg,
								QString::fromLocal8Bit("确定"));
						}
						else if (imgnum > dmi + roadLength * 5)
						{
							int temp = imgnum / roadLength - dmi / roadLength;
							QString msg = QStringLiteral("工程名称:%5道路名称:%1道路编号:%2,右侧景观图像多采图像%3张,设置拍照距离%4!")
								.arg(QString::fromLocal8Bit(setInfo.strNumber)).arg(QString::fromLocal8Bit(setInfo.strRoadName)).arg(QString::number(temp)).arg(QString::number(roadLength)).arg(curProjectName);
							out << msg;
							QMessageBox::information(this, QString::fromLocal8Bit("警告"), msg,
								QString::fromLocal8Bit("确定"));
						}
					}
				}

			}
		}
		fFile.close();
		QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("数据检查结束"));

	}
}

// 影像生成
void hnRoadDataProcess::createImageSlot()
{
	QProcess process;

	//获取影像生成软件绝对路径
	QString cutImageSoftName = QApplication::applicationDirPath() + "/createImageTool/hnCreate2DImage.exe";
	QStringList arguments;

	//启动程序  非阻塞的方式
	process.startDetached(cutImageSoftName, arguments);
}

// 里程校准
void hnRoadDataProcess::slot_mileCorrectSlot()
{
	//弹出界面
	if (!m_projects->isOpenProject())
	{
		return;
	}
	QVector<hnCommon::hnMilePile> datas = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentMilePileVector();
	hnMileAdjustDlg*  dlg = new  hnMileAdjustDlg(datas, this);
	auto result = dlg->exec();
	if (result == QDialog::Accepted)
	{
		QVector<hnCommon::hnMilePile> nowPileInfos = dlg->getAllMileVec();
		//更新marks ，xml里面  mark.txt 里面 数据库 里面

		m_projects->getCurrentProject()->changeMilePile(nowPileInfos);
		//更新视图
		// 加载当前工程数据
		if (hnApp::hnDataManager::getDataManager()->getCurrentProject())
		{
			// 加载当前工程路面影像
			this->m_2dPixScrollWidget->loadRoadPicture();

			// 加载当前工程景观影像
			//const QString streetImagePath =
			//	hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2DProPath() + "/StreetImg/Camera0/Image_0000";
			//this->m_streetContinousBrowsePixWidget->loadPix(streetImagePath);
			QVector<hnCommon::hnMarkInfo> marks = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentMarkVector();
			emit signal_updateProject(hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo(), marks, nowPileInfos);

			// 加载三维影像
			if (hnDataManager::getDataManager()->getCurrentProject()->getProjectType() != PROJECT_2D_TYPE)
			{
				m_3dPixScrollWidget->load3dImage();
			}
			this->m_diseaseListWidget->updateAllDiseases();
		}
		//重新计算 qvector<hnMile>
	}
	else if (result == QDialog::Rejected)
	{
		//
	}
}

// 采集打标
void hnRoadDataProcess::slot_markInfoSlot()
{
	//弹出界面
	if (!m_projects->isOpenProject())
	{
		return;
	}
	if (m_projects->getCurrentProject()->getProjectType() == PROJECT_JD_3D_TYPE)
	{
		return;
	}
	else if (m_projects->getCurrentProject()->getProjectType() == PROJECT_TYPE::PROJECT_XD_3D_TYPE)
	{
		return;
	}
	QVector<hnCommon::hnMarkInfo> marks = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentMarkVector();
	hnMarkInfoDlg* dlg = new hnMarkInfoDlg(marks, this);
	auto result = dlg->exec();
	if (result == QDialog::Accepted)
	{
		QVector<hnCommon::hnMarkInfo> nowMarkInfos = dlg->getNewMarkVec();
		QVector<int> nowDeleteMarkInfoIndexs = dlg->getDeleteMarkVec();
		//更新marks ，xml里面  mark.txt 里面 数据库 里面
		bool needUpdate = m_projects->getCurrentProject()->changeMark(nowMarkInfos, nowDeleteMarkInfoIndexs);

		//更新视图
		// 加载当前工程数据
		if (hnApp::hnDataManager::getDataManager()->getCurrentProject())
		{
			QVector<hnCommon::hnMilePile> datas = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentMilePileVector();

			emit signal_updateProject(hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo(),
				hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentMarkVector(), datas);
			if (needUpdate)
			{
				//// 加载当前工程路面影像
				this->m_2dPixScrollWidget->loadRoadPicture();


				// 加载三维影像
				if (hnDataManager::getDataManager()->getCurrentProject()->getProjectType() != PROJECT_2D_TYPE)
				{
					m_3dPixScrollWidget->load3dImage();
				}
				this->m_diseaseListWidget->updateAllDiseases();
			}
		}

			this->updateAllWidget();
	}
	else if (result == QDialog::Rejected)
	{
		//
	}
}

// 清除工程
void hnRoadDataProcess::slot_clearProjectSlot()
{
	clearCurrentProjectUiState();
	m_projectDataInfos.clear();
	if (m_projects && m_projects->isHasProject())
	{
		m_projects->closeProject();
	}
}
void hnRoadDataProcess::slot_regionJump()
{
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}

	//弹出对话框
	m_regionJumpDlg->show();

}
//软件设置
void hnRoadDataProcess::slot_openConfigWidget()
{
	m_projectConfgDialog->exec();
}

void hnRoadDataProcess::slot_oepnCourseDocument()
{
	QProcess process;
	//获取影像生成软件绝对路径
	QString cutImageSoftName = QApplication::applicationDirPath() + QStringLiteral("\\二三维一体化内业处理平台使用手册.docx");
	QStringList arguments;
	QUrl url = QUrl::fromLocalFile(cutImageSoftName);
	QDesktopServices::openUrl(url);
	//启动程序  非阻塞的方式
//	process.startDetached(cutImageSoftName, arguments);
}

void hnRoadDataProcess::slot_aboutInfosWidget()
{
	hnAboutInfoWidgets*  aboutWidget = new hnAboutInfoWidgets(this);
	aboutWidget->exec();
}

void hnRoadDataProcess::slot_outMergeExcel()
{
	QProcess process;


	QString cutImageSoftName = QApplication::applicationDirPath() + "/MergeExcelSoftWare/MergeExcel.exe";
	QStringList arguments;

	//启动程序  非阻塞的方式
	process.startDetached(cutImageSoftName, arguments);


}

void hnRoadDataProcess::slot_outputExcel()
{
	//弹出界面
	if (!m_projects->isOpenProject())
	{
		return;
	}
	//弹出界面
	if (!m_projects->isHasProject())
	{
		QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("当前没有可用的工程"),
			QStringLiteral("确定"));
		return;
	}
	hnPro::hnProjectManager* manager = hnApp::hnDataManager::getDataManager()->getProjectManager();
	auto allPorject = manager->getAllBaseProject();
	auto defaultStarndar = HnProjectEnums::roadTypeQStringToEnum(allPorject.at(0)->getCurProSetInfo().strRoadStandard);
	auto defaultDrawType = allPorject.at(0)->getCurProSetInfo().nDrawType;
	for each (auto project in allPorject)
	{
		auto starndar = HnProjectEnums::roadTypeQStringToEnum(project->getCurProSetInfo().strRoadStandard);
		auto drawType = project->getCurProSetInfo().nDrawType;
		if (starndar != defaultStarndar || drawType != defaultDrawType)
		{
			QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("多模式出表需要保证所有工程的[道路标准及绘制方式]一致,请检查数据或尝试单独导入工程出表！"),
				QStringLiteral("确定"));
			return;
		}

	}
	m_xrSetting->outExcel = false;
	if (m_outExcelDialog == nullptr)
	{
		m_outExcelDialog = new hnOutputExcelDialog(this);
	}
	auto resultForm = m_outExcelDialog->exec();

	if (m_xrSetting->outExcel)
	{
		if (!m_xrSetting->outMileWithMark)
		{
			for (auto project : allPorject)
			{
				if (project->getProjectType() == PROJECT_TYPE::PROJECT_23D_TYPE ||
					project->getProjectType() == PROJECT_TYPE::PROJECT_2D_TYPE)
				{

					QDir outDir(m_xrSetting->OutPath);
					if (outDir.exists())
					{
						QFileInfoList files = outDir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot);
						for (auto curFile : files)
						{
							if (curFile.isDir() && curFile.fileName().contains(project->get2DProName()))
							{
								QString temp1 = curFile.fileName();
								QString temp2 = project->get2DProName();

								QString markFilePath = project->get2DProject()->getMarkFilePath();
								QFile markFile(markFilePath);
								if (markFile.exists())
								{
									QString temp3 = curFile.filePath();
									QString newFilePath = curFile.filePath() + "\\markInfo.txt";
									markFile.copy(newFilePath);
									continue;
								}

							}
						}

					}

				}

			}
		}
	}
	//如果用户选择了不根据打标分段 则输出每个工程的达标文件  
#if 0
	//需要的参数是
	int  splitValue = 10; //分割区间大小  10 100 1000  n 

	double sMile = 0; //起始桩号 
	double  eMile = 1000; //终点桩号 

	//根据用户输入进行区间分段
	//注意条件  遇到路面标准切换需要生成新表
	hnRoadTypeSetInfo roadSetting;
	auto projects = m_projects->getPorjectManager()->getAllBaseProject();
	for (int i = 0; i < projects.size(); ++i)
	{
		hnPro::hnProject* project = projects[i];
		auto standard = project->getBaseStandard();
		auto level = QString::fromLocal8Bit(project->getCurProSetInfo().strRoadLevel);
		auto surface = project->getBaseSurface();
		bool getOk = m_projects->getRoadTypeSetInfo(
			project->getCurProSetInfo(),
			roadSetting);
		if (getOk)
		{

		}

	}
	hnRoadDiseaseTable roadDisease;
	//vector<hnRoadDiseaseInfo>& vecData, char* strQuery/* = NULL*/)
	vector<hnRoadDiseaseInfo> vecData;
	roadDisease.readAllData(vecData);
	hnExcelIO * excel = new hnExcelIO("D:\\TFS\\24-hnRoadDataProcess\\hnRoadDataProcess\\bin\\Debug-X64\\报表模板\\CPMS路面病害调查表.xlsx",
		"D:\\TFS\\24-hnRoadDataProcess\\hnRoadDataProcess\\bin\\Debug-X64\\输出报表\\CPMS路面病害调查表Copy.xlsx");
	if (excel->OpenExcel())
	{
		excel->SaveAndClose();
		qDebug() << "复制成功" << endl;
	}
#endif
					}

void hnRoadDataProcess::slot_outAllResultDatas()
{
	//弹出界面
	if (!m_projects->isOpenProject())
	{
		return;
	}
	//弹出界面
	if (!m_projects->isHasProject())
	{
		QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("当前没有可用的工程"),
			QStringLiteral("确定"));
		return;
	}

	auto reply = QMessageBox::question(this, QStringLiteral("提示"), QStringLiteral("出总表时间较长,是否继续"), QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::Yes)
	{

	}
	else
	{
		return;
	}
	hnPro::hnProjectManager* manager = hnApp::hnDataManager::getDataManager()->getProjectManager();
	auto allPorject = manager->getAllBaseProject();
	if (allPorject.size() < 1)
	{
		return;
	}
	auto defaultStarndar = HnProjectEnums::roadTypeQStringToEnum(allPorject.at(0)->getCurProSetInfo().strRoadStandard);
	auto defaultDrawType = allPorject.at(0)->getCurProSetInfo().nDrawType;
	QString modelStandard = "";
	QString modelDrawType = "";
	switch (defaultStarndar)
	{
	case HnProjectEnums::None:
		modelStandard = QStringLiteral("等级公路2018");
		break;
	case HnProjectEnums::DegreeRoad2018:
		modelStandard = QStringLiteral("等级公路2018");
		break;
	case HnProjectEnums::CityRoad:
		modelStandard = QStringLiteral("城镇道路2024");
		break;
	case HnProjectEnums::RuralRoadlowLevel:
		modelStandard = QStringLiteral("低等级农村公路2024");
		break;
	default:
		break;
	}
	switch (defaultDrawType)
	{
	case  0:
		break;
		modelDrawType = QStringLiteral("人工模式");
	case  1:
		modelDrawType = QStringLiteral("自动化模式");
		break;
	case 2:
		modelDrawType = QStringLiteral("设计模式");
		break;
	default:
		break;
	}

	for each (auto project in allPorject)
	{
		auto starndar = HnProjectEnums::roadTypeQStringToEnum(project->getCurProSetInfo().strRoadStandard);
		auto drawType = project->getCurProSetInfo().nDrawType;
		if (starndar != defaultStarndar || drawType != defaultDrawType)
		{
			QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("多模式出表需要保证所有工程的[道路标准及绘制方式]一致,请检查数据或尝试单独导入工程出表！"),
				QStringLiteral("确定"));
			return;
		}

	}

	double sMile = 0, eMile = 0;
	m_xrSetting->outMileWithMark = true;
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QApplication::processEvents();
	bool outSucceed = true;
	QVector<int> splitValueVec = { 10,100,1000 };

	//计算总任务数量
	int totalTasks = allPorject.size() * splitValueVec.size();
	int currentTask = 0;
	//创建进度对话框
	QProgressDialog progress(QStringLiteral("处理工程数据..."), QStringLiteral("取消"), 0, totalTasks);
	progress.setWindowModality(Qt::WindowModal);  //模态
	progress.setMinimumDuration(0);   //立即显示
	for each (auto project in allPorject)
	{
		/*QElapsedTimer timer;
		timer.start();
		qint64 elapsed = 0;*/
		sMile = project->getCurProSetInfo().dBegMile;
		eMile = project->getCurProSetInfo().dEndMile;
		for (int splitValue : splitValueVec)
		{
			//跟新进度
			currentTask++;
			progress.setValue(currentTask);
			progress.setLabelText(QStringLiteral("处理	%1	(%2m)...").arg(project->get2DProName()).arg(splitValue));

			if (progress.wasCanceled())
			{
				return;
			}
			QString xlsxAllDataTemplatePath = QApplication::applicationDirPath() + QStringLiteral("\\报表模板\\定制出表模板\\") + modelStandard + "\\" + modelDrawType + "\\";

			QString xlsxAllDataTemplateFilePath = xlsxAllDataTemplatePath + "allInfo.xlsx";
			QString xlsxAllDiseaseTemplateFilePath = xlsxAllDataTemplatePath + QStringLiteral("AllDisease.xlsx");
			QFile file(xlsxAllDataTemplateFilePath);
			if (!file.exists())
			{
				QApplication::restoreOverrideCursor();

				QMessageBox::about(this, QStringLiteral("提示"), QStringLiteral("找不到模板文件!"));


				return;
			}
			//加载表格模板
			Document xlsx(xlsxAllDataTemplateFilePath);

			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			setEquip.STREET = true;
			setEquip.RUT = true;
			setEquip.IRI = true;
			setEquip.JUMP = true;
			setEquip.MPD = true;
			setEquip.JHXX = true;
			setEquip.SMTD = true;
			if (m_xrSetting->outSpeedAndMarkExcel)
			setEquip.SPEED = true;
			setEquip.GPS = true;
			m_outExcelManage = QSharedPointer<hnOutExcelMileManage>
				(new hnOutExcelMileManage(defaultStarndar, project, sMile, eMile, splitValue, setEquip));
			if (!m_outExcelManage->getDataComplete())
			{
				QApplication::restoreOverrideCursor();
				QMessageBox::about(this, QStringLiteral("错误"), project->get2DProName() + QStringLiteral("_存在指标未进行计算，请根据提示进行IRM计算!"));
				return;
			}
#pragma region 写入指标数据
			if (!xlsx.selectSheet("score"))
			{
				return;
			}
			QVector<hnOutExcelMile> allMiles = m_outExcelManage->getRoadMessageVec();
			QVector<hnOutExcelMile> allMiles_10M = m_outExcelManage->getRoadMessage_10m_Vec();
			for (int i = 0; i < allMiles.size(); ++i)
			{
				int colCnt = 1;
				auto curMile = allMiles.at(i);
				auto curMile_10M = allMiles_10M.at(i);
				xlsx.write(i + 2, colCnt++, curMile.getStartMile());
				xlsx.write(i + 2, colCnt++, curMile.getEndMile());
				xlsx.write(i + 2, colCnt++, curMile.RoadDegreestr);
				xlsx.write(i + 2, colCnt++, curMile.RoadSurfaceStr);
				xlsx.write(i + 2, colCnt++, curMile.getMqiValue(i + 2, "O", "F", "M", "N"));
				xlsx.write(i + 2, colCnt++, curMile.getPqiValue(i + 2, "G", "H", "I", "J", "L"));//pqi
				xlsx.write(i + 2, colCnt++, curMile.getPCIExcelStr());
				xlsx.write(i + 2, colCnt++, curMile.getIriExcelStr());
				xlsx.write(i + 2, colCnt++, curMile.getRutExcelStr());
				xlsx.write(i + 2, colCnt++, curMile.getPBIScore());
				xlsx.write(i + 2, colCnt++, curMile.getCenterMtdValue()); //mtd  
				xlsx.write(i + 2, colCnt++, curMile.getPwiValueStr()); //磨耗

				xlsx.write(i + 2, colCnt++, 100); //bci 桥隧
				xlsx.write(i + 2, colCnt++, curMile.getTciValue()); //tci
				xlsx.write(i + 2, colCnt++, curMile.getSciValue());
				xlsx.write(i + 2, colCnt++, 100);//sri
				xlsx.write(i + 2, colCnt++, 100);//pssi 
				xlsx.write(i + 2, colCnt++, curMile.getDRExcelScore()); //tci
				xlsx.write(i + 2, colCnt++, curMile.getLeftIriValue());
				xlsx.write(i + 2, colCnt++, curMile.getRightIriValue());
				xlsx.write(i + 2, colCnt++, curMile.getJudgeIirValue());

				xlsx.write(i + 2, colCnt++, curMile.getLeftRutValue());
				xlsx.write(i + 2, colCnt++, curMile.getRightRutValue());
				xlsx.write(i + 2, colCnt++, curMile.getjudgeRutValue());

				if (splitValue != 10)
				{
					xlsx.write(i + 2, colCnt++, "");//跳车纵断面高差
					xlsx.write(i + 2, colCnt++, "");
					xlsx.write(i + 2, colCnt++, "");
					xlsx.write(i + 2, colCnt++, curMile_10M.getPbEvaluateStr());


				}
				else
				{
					xlsx.write(i + 2, colCnt++, curMile_10M.getLeftPbValue(10));//跳车纵断面高差
					xlsx.write(i + 2, colCnt++, curMile_10M.getRightPbValue(10));
					xlsx.write(i + 2, colCnt++, curMile_10M.getjudgePbValue(10));
					xlsx.write(i + 2, colCnt++, curMile_10M.getPbEvaluateStr());

				}


				xlsx.write(i + 2, colCnt++, curMile.getPbiNumber(1));//跳车
				xlsx.write(i + 2, colCnt++, curMile.getPbiNumber(2));
				xlsx.write(i + 2, colCnt++, curMile.getPbiNumber(3));

				xlsx.write(i + 2, colCnt++, curMile.getLeftMtdValue());//smtd
				xlsx.write(i + 2, colCnt++, curMile.getRightMtdValue());
				xlsx.write(i + 2, colCnt++, curMile.getRepresentSMtdValue()); //代表smtd

				xlsx.write(i + 2, colCnt++, curMile.getLeftMpdValue()); //mpd 磨耗
				xlsx.write(i + 2, colCnt++, curMile.getRightMpdValue());
				xlsx.write(i + 2, colCnt++, curMile.getCenterMpdValue());
				xlsx.write(i + 2, colCnt++, curMile.getMpdWrValue());
				xlsx.write(i + 2, colCnt++, curMile.getMpdValueStr("AL", i + 2));



				xlsx.write(i + 2, colCnt++, curMile.getLeftMtdValue());//mtd 磨耗
				xlsx.write(i + 2, colCnt++, curMile.getRightMtdValue());
				xlsx.write(i + 2, colCnt++, curMile.getCenterMtdValue());
				xlsx.write(i + 2, colCnt++, curMile.getMtdWrValue());
				xlsx.write(i + 2, colCnt++, curMile.getPwiValueStr()); //磨耗

				xlsx.write(i + 2, colCnt++, curMile.getPqiEvaluateStr(i + 2, "F"));
				xlsx.write(i + 2, colCnt++, curMile.getPciEvaluateStr("G", i + 2));
				xlsx.write(i + 2, colCnt++, curMile.getIriEvaluateStr("H", i + 2));
				xlsx.write(i + 2, colCnt++, curMile.getRutEvaluateStr("I", i + 2));
				xlsx.write(i + 2, colCnt++, curMile.getPbiEvaluateStr("J", i + 2));
				xlsx.write(i + 2, colCnt++, curMile.getMtdEvaluateStr("K", i + 2)); //城镇道路mt
				xlsx.write(i + 2, colCnt++, curMile.getMtdEvaluateStr("L", i + 2));
				xlsx.write(i + 2, colCnt++, curMile.getMpdEvaluateStr("AM", i + 2));
				xlsx.write(i + 2, colCnt++, QStringLiteral("优"));//BCI评价
				xlsx.write(i + 2, colCnt++, curMile.getTciEvaluate());
				xlsx.write(i + 2, colCnt++, curMile.getSciEvaluate());
				_EXCELGPS_ sGps = curMile.getStartGpsStr();
				_EXCELGPS_ eGps = curMile.getEndGpsStr();

				xlsx.write(i + 2, colCnt++, sGps._longitude);//经度
				xlsx.write(i + 2, colCnt++, sGps._latitude);
				xlsx.write(i + 2, colCnt++, sGps._elevation);
				xlsx.write(i + 2, colCnt++, eGps._longitude);//经度
				xlsx.write(i + 2, colCnt++, eGps._latitude);
				xlsx.write(i + 2, colCnt++, eGps._elevation);

				xlsx.write(i + 2, colCnt++, curMile.getSpeed());//速度
				xlsx.write(i + 2, colCnt++, curMile.getUnitStr());//速度 
			}
#pragma endregion

#pragma region 写入工程数据
			if (!xlsx.selectSheet("project_info"))
			{
				return;
			}

			int rowIndex = 2;
			int colIndex = 1;

			//获取工程信息
			auto projectInfo = project->getCurProSetInfo();;
			//省
			QString province = QString::fromLocal8Bit(projectInfo.strProvince);
			xlsx.write(rowIndex, colIndex++, province);
			//市
			QString city = QString::fromLocal8Bit(projectInfo.strCity);

			xlsx.write(rowIndex, colIndex++, city);
			//县
			QString county = QString::fromLocal8Bit(projectInfo.strCounty);

			xlsx.write(rowIndex, colIndex++, county);
			//道路编号
			QString roadNum = QString::fromLocal8Bit(projectInfo.strNumber);

			xlsx.write(rowIndex, colIndex++, roadNum);
			//道路名称
			QString roadName = QString::fromLocal8Bit(projectInfo.strRoadName);

			xlsx.write(rowIndex, colIndex++, roadName);
			//起点桩号 
			xlsx.write(rowIndex, colIndex++, m_outExcelManage->getStartMile());
			//行车方向
			QString direction = projectInfo.nLineType == 1 ? QString::fromLocal8Bit("上行") : QString::fromLocal8Bit("下行");

			xlsx.write(rowIndex, colIndex++, direction);
			//公路等级
			QString roadLevel = QString::fromLocal8Bit(projectInfo.strRoadLevel);
			xlsx.write(rowIndex, colIndex++, roadLevel);
			//车道
			QString lane = QString::fromLocal8Bit(projectInfo.strRoadNO);
			xlsx.write(rowIndex, colIndex++, lane);
			//采集日期  
			xlsx.write(rowIndex, colIndex++, projectInfo.strDate);
			//工程开始时刻 
			xlsx.write(rowIndex, colIndex++, projectInfo.strTimer);
			//检测员
			QString detectPeople = QString::fromLocal8Bit(projectInfo.strSurveyor);
			xlsx.write(rowIndex, colIndex++, detectPeople);
			//检测天气
			QString wheather = QString::fromLocal8Bit(projectInfo.strWeather);

			xlsx.write(rowIndex, colIndex++, wheather);
			//路面材质
			QString roadType = QString::fromLocal8Bit(projectInfo.getRSurfaceType().data());

			xlsx.write(rowIndex, colIndex++, roadType);
			//终点桩号

			xlsx.write(rowIndex, colIndex++, m_outExcelManage->getEndMile());
			//检测里程（km）

			xlsx.write(rowIndex, colIndex++, qAbs(m_outExcelManage->getStartMile() - m_outExcelManage->getEndMile()) * 0.001);
			QString standard = HnProjectEnums::roadTypeEnumToQString(project->getBaseStandard());

			xlsx.write(rowIndex, colIndex++, standard);

			xlsx.write(rowIndex, colIndex++, projectInfo.nDrawType);
			xlsx.write(rowIndex, colIndex++, project->getProjectType());
			xlsx.write(rowIndex, colIndex++, projectInfo.dRoadWidth);
#pragma endregion

#pragma region 写入路面病害数据
			if (!xlsx.selectSheet("road_Dis"))
			{
				return;
			}
			//获取病害
			QVector<hnCommon::hnRoadDiseaseInfo> diss;

			diss = hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllRoadDiseases(); 
			int rowCount = 2;
			QVector<hnMile> curMiles = project->getCurrentMileVector();
			if (projectInfo.nDrawType == 0)
			{
				for (int i = 0; i < diss.size(); ++i, ++rowCount)
				{
					auto dis = diss.at(i);
					int  startTrueMile = qRound(project->enclToTrueMile(dis.dMileage));
					//开始桩号	
					xlsx.write(QString("A%1").arg(rowCount), startTrueMile);
					//车道	 
					QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
					xlsx.write(QString("B%1").arg(rowCount), RoadNum);
					//病害类型
					QString disName = QString::fromLocal8Bit(dis.strDisName);
					QStringList disSplit = disName.split('.');
					if (disSplit.size() > 1)
					{
						xlsx.write(QString("C%1").arg(rowCount), disSplit.at(0));
						//病害程度 
						xlsx.write(QString("D%1").arg(rowCount), disSplit.at(1));
					}
					else
					{
						xlsx.write(QString("C%1").arg(rowCount), disSplit.at(0));
						//病害程度 
						xlsx.write(QString("D%1").arg(rowCount), QStringLiteral("无"));
					}
					//病害框长度（m）
					xlsx.write(QString("E%1").arg(rowCount), dis.dLength);
					//病害框宽度（m）
					xlsx.write(QString("F%1").arg(rowCount), dis.dWidth);
					//病害中心位置（距路面图像右边距离）（m）
					//单位是米
					double distance = 0.0;
					if (dis.vec2dRect.size() != 0)
					{
						auto hn2drect = dis.vec2dRect.at(0);
						const double widthScale = project->getCurProSetInfo().dRadioX;
						const double roadWidth = project->getCurProSetInfo().dRoadWidth;
						//	distance = roadWidth - ((hn2drect.p0.x + hn2drect.p1.x) / 2.0) * widthScale;
						distance = (((hn2drect.p1.x - hn2drect.p0.x) / 2.0) + hn2drect.p0.x) * widthScale;
						distance = roadWidth - distance;
					}

					xlsx.write(QString("G%1").arg(rowCount), distance);
					//病害面积(m2)
					xlsx.write(QString("H%1").arg(rowCount), dis.dArea);
					//病害计算长度（m）
					xlsx.write(QString("I%1").arg(rowCount), dis.dRealLen);
					//病害计算宽度（m）
					xlsx.write(QString("J%1").arg(rowCount), dis.dReaWidth);
					//病害深度 （mm）
					xlsx.write(QString("K%1").arg(rowCount), dis.dDepth * 1000);
					hnMile curMile;
					double disCurMile = project->enclToTrueMile(dis.dMileage);
					double mileLenght = 100000;
					//根据桩号找到匹配图片
					for (int i = 0; i < curMiles.size(); ++i)
					{

						double dCurMile = curMiles.at(i).dTrueMile;
						double curLength = qAbs(disCurMile - dCurMile);
						if (curLength < mileLenght)
						{
							mileLenght = curLength;
							curMile = curMiles.at(i);
						}
					}
					int findIndex = curMile.picturePath.lastIndexOf("/");
					QString subName = curMile.picturePath.mid(findIndex + 1);

					//路面图像名称
					xlsx.write(QString("L%1").arg(rowCount), subName);

					findIndex = curMile.picturePath.indexOf("RoadImg");
					subName = "/" + curMile.picturePath.mid(findIndex);
					//路面图像相对路径
					xlsx.write(QString("M%1").arg(rowCount), subName);
					//路面材质
					QString roadType = dis.nRSurfaceType == 0 ? QStringLiteral("沥青") :
						dis.nRSurfaceType == 1 ? QStringLiteral("水泥") : QString("");
					xlsx.write(QString("N%1").arg(rowCount), roadType);
					double lat = 0.0;
					double lon = 0.0;
					double height = 0.0;
					if (project->getProjectType() == PROJECT_23D_TYPE || project->getProjectType() == PROJECT_TYPE::PROJECT_JD_3D_TYPE
						|| project->getProjectType() == PROJECT_TYPE::PROJECT_XD_3D_TYPE)
					{
						double centerX = 0;
						double centerY = 0;
						double centerZ = 0;
						double mile = 0;
						if (dis.vec3dRect.size() > 0)
						{
							if (dis.nDrawType == 3)
							{
								centerX = dis.vec3dRect[0].p0.x;
								centerY = dis.vec3dRect[0].p0.y;
								centerZ = dis.vec3dRect[0].p0.z;
								mile = dis.vec3dRect[0].p0.bottomEncoderMile;
							}
							else
							{
								centerX = (dis.vec3dRect[0].p0.x + dis.vec3dRect[0].p1.x + dis.vec3dRect[0].p2.x + dis.vec3dRect[0].p3.x) / 4;
								centerY = (dis.vec3dRect[0].p0.y + dis.vec3dRect[0].p1.y + dis.vec3dRect[0].p2.y + dis.vec3dRect[0].p3.y) / 4;
								centerZ = (dis.vec3dRect[0].p0.z + dis.vec3dRect[0].p1.z + dis.vec3dRect[0].p2.z + dis.vec3dRect[0].p3.z) / 4;
								mile = (dis.vec3dRect[0].p0.bottomEncoderMile + dis.vec3dRect[0].p1.bottomEncoderMile + dis.vec3dRect[0].p2.bottomEncoderMile + dis.vec3dRect[0].p3.bottomEncoderMile) / 4;
							}
							hnCommon::hn3dPointWithMileI pt(centerX, centerY, centerZ, mile);
							hnDataManager::getDataManager()->getDiseaseLoction(pt, lat, lon, height);
						}
					}
					xlsx.write(QString("O%1").arg(rowCount), lon);
					xlsx.write(QString("P%1").arg(rowCount), lat);
					xlsx.write(QString("Q%1").arg(rowCount), height);
					//备注 
					QString mark = dis.strRemark;
					xlsx.write(QString("R%1").arg(rowCount), mark);
				}
			}
			else if (projectInfo.nDrawType == 1)
			{
				for (int i = 0; i < diss.size(); ++i, ++rowCount)
				{
					auto dis = diss.at(i);
					int  startTrueMile = qRound(project->enclToTrueMile(dis.dMileage));

					//开始桩号	

					xlsx.write(QString("A%1").arg(rowCount), startTrueMile);
					//车道	 
					QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
					xlsx.write(QString("B%1").arg(rowCount), RoadNum);
					//病害类型  
					QString disName = QString::fromLocal8Bit(dis.strDisName);
					QStringList disSplit = disName.split('.');
					if (disSplit.size() > 1)
					{
						xlsx.write(QString("C%1").arg(rowCount), disSplit.at(0));
						//病害程度  
						xlsx.write(QString("D%1").arg(rowCount), disSplit.at(1));
					}
					else
					{
						xlsx.write(QString("C%1").arg(rowCount), disSplit.at(0));
						//病害程度  
						xlsx.write(QString("D%1").arg(rowCount), QStringLiteral("无"));
					}

					//病害面积(m2) 
					xlsx.write(QString("H%1").arg(rowCount), dis.dArea);

					//具体位置_距右侧标线位置(m) 
					//算出自动化模式小方格横向的中点，然后再用道路宽度减去中点
					double distance = 0.0;
					if (dis.vec2dRect.size() != 0)
					{
						QMap<double, double> centerMap;
						const double widthScale = project->getCurProSetInfo().dRadioX;
						const double roadWidth = project->getCurProSetInfo().dRoadWidth;
						for (auto hn2drect : qAsConst(dis.vec2dRect))
						{
							double rectCenter = (hn2drect.p0.x + hn2drect.p1.x) / 2.0;
							centerMap.insert(rectCenter, rectCenter);
						}
						distance = roadWidth - (((centerMap.first() + centerMap.last()) / 2.0)* widthScale);
						//distance = roadWidth - distance;
					}

					xlsx.write(QString("G%1").arg(rowCount), distance);



					hnMile curMile;
					double disCurMile = project->enclToTrueMile(dis.dMileage);
					double mileLenght = 100000;
					//根据桩号找到匹配图片
					for (int i = 0; i < curMiles.size(); ++i)
					{
						double dCurMile = curMiles.at(i).dTrueMile;
						double curLength = qAbs(disCurMile - dCurMile);
						if (curLength < mileLenght)
						{
							mileLenght = curLength;
							curMile = curMiles.at(i);
						}
					}
					//路面图像名称 
					int findIndex = curMile.picturePath.lastIndexOf("/");
					QString subName = curMile.picturePath.mid(findIndex + 1);

					//路面图像名称
					xlsx.write(QString("L%1").arg(rowCount), subName);

					findIndex = curMile.picturePath.indexOf("RoadImg");
					subName = "/" + curMile.picturePath.mid(findIndex);
					//路面图像相对路径
					xlsx.write(QString("M%1").arg(rowCount), subName);

					//路面材质 
					QString roadType = dis.nRSurfaceType == 0 ? QStringLiteral("沥青") :
						dis.nRSurfaceType == 1 ? QStringLiteral("水泥") : QString("");
					xlsx.write(QString("N%1").arg(rowCount), roadType);
					double lat = 0.0;
					double lon = 0.0;
					double height = 0.0;
					if (project->getProjectType() == PROJECT_23D_TYPE || project->getProjectType() == PROJECT_TYPE::PROJECT_JD_3D_TYPE
						|| project->getProjectType() == PROJECT_TYPE::PROJECT_XD_3D_TYPE)
					{
						if (dis.vec3dRect.size() > 0)
						{

							double centerX = (dis.vec3dRect[0].p0.x + dis.vec3dRect[0].p1.x + dis.vec3dRect[0].p2.x + dis.vec3dRect[0].p3.x) / 4;
							double centerY = (dis.vec3dRect[0].p0.y + dis.vec3dRect[0].p1.y + dis.vec3dRect[0].p2.y + dis.vec3dRect[0].p3.y) / 4;
							double centerZ = (dis.vec3dRect[0].p0.z + dis.vec3dRect[0].p1.z + dis.vec3dRect[0].p2.z + dis.vec3dRect[0].p3.z) / 4;
							double mile = (dis.vec3dRect[0].p0.bottomEncoderMile + dis.vec3dRect[0].p1.bottomEncoderMile + dis.vec3dRect[0].p2.bottomEncoderMile + dis.vec3dRect[0].p3.bottomEncoderMile) / 4;
							hnCommon::hn3dPointWithMileI pt(centerX, centerY, centerZ, mile);
							hnDataManager::getDataManager()->getDiseaseLoction(pt, lat, lon, height, project);
						}
					}
					xlsx.write(QString("O%1").arg(rowCount), lon);
					xlsx.write(QString("P%1").arg(rowCount), lat);
					xlsx.write(QString("Q%1").arg(rowCount), height);
				}
			}
#pragma endregion 
#pragma region 写入景观病害数据

			if (!xlsx.selectSheet("street_YXDis"))
			{
				return;
			}
			writeStreetDiseaseMsgToExcel(1, project, xlsx);
			if (!xlsx.selectSheet("street_LJDis"))
			{
				return;
			}
			writeStreetDiseaseMsgToExcel(2, project, xlsx);
#pragma endregion   

			QString saveExcelName = project->getAbsulotelyPath() + QStringLiteral("\\成果数据\\") + project->get2DProName() + QStringLiteral("\\%1_hnResult_23dDatas_%2m.xlsx").arg(project->get2DProName()).arg(splitValue);


			if (!xlsx.saveAs(saveExcelName))
			{
				outSucceed = false;
				QApplication::restoreOverrideCursor();
				QMessageBox::about(this, QStringLiteral("错误"), saveExcelName + "\n" + QStringLiteral("存储路径文件被打开,无法写入!"));
				return;
			}
		}
	}


	QApplication::restoreOverrideCursor();
	if (outSucceed)
	{
		QMessageBox::about(this, QStringLiteral("提示"), QStringLiteral("输出完毕!"));
	}
	//打开出表软件

}


