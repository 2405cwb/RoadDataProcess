#pragma once
#include "hnapplication_global.h"
#include "hnBrowsePixWidget.h"
#include "hnWorkMode.h"
#include "hnFrameMode.h"
#include "drawDiseases.h"
#include "rectAlgorithm.h"
#include "hnAdjustImage.h"
#include "depthCaculate.h"
#include "mergeDisease.h"
#include <QKeyEvent>
#include "addDiseaseDialog.h"
#include "hnSdkDiseaseGraphicsLayer.h"
#include "../hnProject/hn3DProject.h"
#include <QMetaObject>
#include <QList>
#include <QHash>
#include <QByteArray>
#include "lineAlgorithm.h"
#include "../SDK/include/tools/GridSelectionTool.h"
#include <qmath.h>
using namespace hnApp;

class TiledGraphicsView;
struct TiledViewAnchor;
class TunnelViewerController;
class SdkDiseaseOverlayWidget;
class QTimer;

class HNAPPLICATION_EXPORT hn2d3dPixBaseWidget:
	public hnBrowsePixWidget,
	public hnWorkMode,
	public hnFrameMode,
	public drawDiseases,
	public rectAlgorithm,
	public hnAdjustImage,
	public depthCaculate,
	public mergeDisease,
	public lineAlgorithm
{
	Q_OBJECT
	friend class SdkDiseaseOverlayWidget;
public:
	hn2d3dPixBaseWidget(QWidget *parent = Q_NULLPTR);
	~hn2d3dPixBaseWidget();
	void setSingleFrameNavigationEnabled(bool enabled);
	bool stepSingleFrame(int visualDelta);

protected:
	enum WidgetType
	{
		WIDGET_2D,
		WIDGET_3D
	};

	struct SdkStatusContext
	{
		bool valid = false;
		QString imageName;
		int imageIndex = -1;
		QPoint viewportPoint;
		QPointF scenePoint;
		QPoint singleImagePoint;
		QPoint routeImagePoint;
		double encoderMile = 0.0;
	};

	// SDK 大框跨图绘制时的单图矩形片段，坐标为对应原始图片内的像素坐标。
	struct SdkSingleImageRect
	{
		QString pixName;
		QRect singleRect;

		bool isValid() const
		{
			return !pixName.isEmpty() && singleRect.isValid() && !singleRect.isNull();
		}
	};


	// SDK 小框正式病害的渲染模式：稀疏格子逐格显示，密集区域显示外接矩形。
	enum SdkLittleFrameRenderMode
	{
		SdkLittleFrameRenderSparseCells,
		SdkLittleFrameRenderDenseRect
	};

	// SDK 小框渲染缓存项，保存轻量 scene 路径及其显示模式。
	struct SdkLittleFrameRenderCacheEntry
	{
		QPainterPath path;
		QRectF sceneBounds;
		SdkLittleFrameRenderMode mode = SdkLittleFrameRenderSparseCells;
		int rectCount = 0;
	};

signals:
	//信号 数据库添加病害
	//void signal_addDisease(hnRoadDiseaseInfo disease,bool modify);

	//用户选中病害
	void signal_selectDisease(const hnRoadDiseaseInfo& disease);
	  
signals:
	//状态栏信息改变
	void signal_statusInfoChanged(QString &statusInfo);

	// SDK 显示层的中心里程变化了，主窗口用它做 2D/3D 的连续联动。
	void signal_sdkCenterEncoderMileChanged(double encoderMile);

	void signal_sdkBottomEncoderMileChanged(double encoderMile);

	// 只有用户主动浏览 SDK 视图时才发出该信号；2D/3D 跨视图联动只监听这一类用户信号。
	void signal_sdkUserBottomEncoderMileChanged(double encoderMile);

 
protected:
	void keyPressEvent(QKeyEvent *event) override;
	
	void mousePressEvent(QMouseEvent *event) override;

	void resizeEvent(QResizeEvent* event) override;

	bool eventFilter(QObject* watched, QEvent* event) override;
 
	int diseaseImagePixels(const QImage &image,int screenPixels) const;
public slots:
	//取消画病害
	void slot_cancelDrawDiseases();

	void slot_deleteDisease(const hnRoadDiseaseInfo &disease);
	void slot_moveMouse(bool up,bool is2D) override;
public:
	// 设置单张图对应的实际里程；SDK 单图模式用它在像素 Y 和连续编码器里程之间换算。
	void setImageDistanceMeters(double meters);

	// Adjust the current SDK-rendered image brightness, range -100 to 100.
	void setSdkImageBrightness(int value);

	// 获取当前 SDK 视图中心点对应的连续编码器里程，2D/3D 联动以该值作为同步基准。
	double currentCenterEncoderMile() const;

	// 获取当前 SDK 视图底部锚点对应的连续编码器里程，用于状态栏、滚动条和跨视图同步。
	double currentBottomEncoderMile() const;

	// 按连续编码器里程滚动 SDK 视图中心，属于程序定位入口。
	void scrollToEncoderMile(double encoderMile);

	// 按连续编码器里程滚动 SDK 视图底部锚点，保持“当前底部里程”语义。
	void scrollBottomToEncoderMile(double encoderMile);

	// 把指定连续编码器里程移动到 SDK 视图中心，常用于打标、病害或列表跳转。
	void centerOnEncoderMile(double encoderMile);

	// 根据 SDK 当前可见区域刷新旧浏览状态、状态栏、打标和病害显示。
	void refreshSdkViewState();

	// 主动重建 SDK 正式病害图层；病害增删改后应调用该入口而不是等待滚动触发。
	void refreshSdkDiseaseLayer();

	// 设置状态栏显示用的当前景观图片名称，避免景观图与路面图状态信息脱节。
	void setCurrentStreetPictureNameForStatus(const QString& pictureName);

	// 兼容旧接口：按病害 ID 设置选中项；SDK 内部会尽量转换为表名+ID 的唯一 key。
	void setSelectedDiseaseId(int diseaseId) override;

	// 设置当前选中的完整病害对象，并同步 SDK 图层高亮 key。
	void setSelectedDisease(const hnRoadDiseaseInfo& disease);

	// 按病害 ID 在 SDK 视图中居中显示；旧列表只传 ID 时使用。
	bool centerSdkDiseaseInView(int diseaseId);

	// 按完整病害对象在 SDK 视图中居中显示，优先使用表名+ID 定位唯一病害。
	bool centerSdkDiseaseInView(const hnRoadDiseaseInfo& disease);

	// 清空 SDK 图像、病害、临时绘制、打标和相关状态，用于卸载工程或切换项目。
	void clearSdkView();

protected:
	//计算线状病害属性
	virtual hnRoadDiseaseInfo caculateLineDiseaseInfo(QVector<pixImagePoint> lineDiseasePoints, hnDiseaseSetInfo diseaseSetInfo) = 0;

	// 把旧项目已经排好序的图片列表交给 SDK 显示，当前项目按纵向拼接来处理。
	void loadSdkVerticalImageSequence(const QStringList& pixNames);

	// 确保 SDK 的 QGraphicsView 和控制器已经创建；只创建一次，后续复用。
	void ensureSdkImageView();

	// 确保旧 QWidget overlay 的兼容对象存在；SDK 模式下正式病害不再由该 overlay 绘制。
	void ensureSdkDiseaseOverlay();

	// 根据当前 SDK/旧视图状态显示或隐藏旧 overlay，避免它遮挡 SDK scene。
	void updateSdkDiseaseOverlay();

	// 旧 overlay 的 paintEvent 入口；SDK 模式下保留空实现用于兼容对象生命周期。
	void paintSdkDiseaseOverlay(QPaintEvent* event);

	// 根据 SDK viewport 坐标刷新状态栏，鼠标移动时优先走该入口。
	bool refreshStatusInfoFromSdkViewportPoint(const QPoint& viewportPoint);

	// Refresh the 1:1 preview from source-image scene pixels.
	void updateSdkInspectionViews(const QPoint& viewportPoint);

	// 根据 SDK 底部或中心 anchor 刷新状态栏，滚轮/键盘/滚动条变化时使用。
	void refreshStatusInfoFromSdkAnchor(const TiledViewAnchor& anchor);

	// 优先使用当前鼠标点刷新状态栏；鼠标不在视图内时退回 anchor。
	void refreshStatusInfoFromSdkCurrentMouseOrAnchor(const TiledViewAnchor& anchor);

	// 获取状态栏中的景观图片文件名；没有外部同步值时按真实桩号回查工程。
	QString currentStreetPictureNameForStatus(double fallbackTrueMile) const;

	// 旧 widget 点生成 SDK 状态文字的兼容虚函数；SDK 路径优先使用 SdkStatusContext。
	virtual QString sdkStatusInfoFromWidgetPoint(const QPoint& widgetPoint);

	// 子类根据 SDK 命中的真实图片、单图坐标和里程生成状态栏文字。
	virtual QString sdkStatusInfoFromContext(const SdkStatusContext& context);

	// 从 SDK viewport 点构造状态上下文：图片名、单图坐标、连续拼接坐标和编码器里程。
	bool sdkStatusContextFromViewportPoint(const QPoint& viewportPoint, SdkStatusContext& context) const;

	// 从 SDK anchor 构造状态上下文，用于无鼠标点的滚动刷新。
	bool sdkStatusContextFromAnchor(const TiledViewAnchor& anchor, SdkStatusContext& context) const;

	// 把 SDK 内部图片名解析回工程图片名，并可返回图片帧序号。
	bool resolveSdkImageName(const QString& sdkImageName, QString& projectImageName, int* frameIdx = nullptr) const;

	// 把 SDK viewport 坐标转换成旧 widget 坐标，供少量旧逻辑兼容使用。
	QPoint sdkViewportPointToLegacyWidgetPoint(const QPoint& viewportPoint);

	// 判断当前 SDK 鼠标事件是否需要转交旧 widget 逻辑处理。
	bool shouldForwardSdkMouseToLegacy() const;

	// 将 SDK 捕获到的鼠标事件转发给旧绘制/选择逻辑；迁移未完成的路径使用。
	bool forwardSdkMouseEventToLegacy(QMouseEvent* mouseEvent);

	// 将 SDK 捕获到的滚轮事件转发给旧逻辑；非 SDK 或兼容路径使用。
	bool forwardSdkWheelEventToLegacy(QWheelEvent* wheelEvent);

	// 判断当前 widget 是否已经启用 SDK 图像视图。
	bool hasSdkImageView() const;

	// 重置 SDK 病害绘制状态；可选清空选中项或退出添加模式。
	void resetSdkDiseaseDrawingState(bool clearSelection = false, bool exitAddMode = false);

	// SDK 浏览或缩放后刷新临时病害预览、正式病害图层和状态栏。
	void refreshSdkDrawingAfterBrowse(const QPoint& viewportPoint);

	// 普通小框绘制中滚轮前记录鼠标对应的单图点，滚轮后让鼠标跟随该点，避免视图移动被误判为绘制轨迹。
	void rememberLittleFrameWheelAnchor(const QPoint& viewportPoint);
	void restoreLittleFrameCursorAfterWheelBrowse();
	bool isSdkPlainLittleFrameDrawing() const;

	// 把一个编码器里程换成 SDK 的 sceneY，换算只依赖单张图高度和单张图里程。
	double encoderMileToSdkSceneY(double encoderMile) const;

	// 把 SDK sceneY 换成编码器里程，滚动条不参与这个计算。
	double sdkSceneYToEncoderMile(double sceneY) const;

	// 获取 SDK 当前可见区域覆盖的编码器里程范围，病害和打标刷新都依赖该范围。
	bool currentSdkVisibleEncoderMileRange(double& beginMile, double& endMile) const;

	// 将工程编码器里程转换成当前 SDK 视图使用的里程；3D 子类可叠加二三维差值。
	virtual double projectEncoderMileToSdkViewEncoderMile(double projectEncoderMile) const;

	// 把 SDK 命中的单图像素点转换成连续编码器里程。
	double sdkPixPointToEncoderMile(const pixImagePoint& point) const;

	// 把 SDK anchor 转换成连续编码器里程。
	double sdkAnchorToEncoderMile(const TiledViewAnchor& anchor) const;

	// 按中心里程滚动 SDK；内部带程序滚动保护，避免触发反向联动。
	void setSdkCenterEncoderMile(double encoderMile);

	// 按底部里程滚动 SDK；用于保持 2D/3D 联动的底部锚点语义。
	void setSdkBottomEncoderMile(double encoderMile);

	// SDK 滚动后同步旧浏览状态字段，供未迁移的状态、校验和入库逻辑继续读取。
	void syncLegacyBrowseStateFromEncoderMile(double encoderMile);

	// 把 SDK scene 坐标转换为图片名和单图像素点。
	bool sdkScenePointToPixPoint(const QPointF& scenePos, pixImagePoint& point) const;

	// 把 SDK scene 坐标转换为可绘制病害点，并校验点在图片范围内。
	bool sdkScenePointToDiseasePoint(const QPointF& scenePos, pixImagePoint& point) const;

	// 把 SDK 单图点转换成旧“大图坐标”，仅给旧算法兼容使用。
	QPoint sdkPixPointToBigImagePoint(const pixImagePoint& point) const;

	// 把 SDK 单图点转换成 SDK scene 坐标，正式病害显示优先使用该方向。
	QPointF sdkPixPointToScenePoint(const pixImagePoint& point) const;

	// 把两个 SDK 单图点转换成旧“大图矩形”，仅用于旧小框算法兼容。
	QRect sdkPixRectToBigImageRect(const pixImagePoint& first, const pixImagePoint& second) const;

	// 将 SDK 大框或设计面选区按图片边界切分为多个单图矩形。
	bool currentSdkBigFrameSingleRects(QVector<SdkSingleImageRect>& rects) const;

	// SDK 鼠标按下事件处理入口，负责开始病害、追加点、右键提交或取消。
	bool handleSdkDiseaseMousePress(QMouseEvent* mouseEvent);

	// SDK 鼠标移动事件处理入口，负责刷新临时框、轨迹线和状态栏。
	bool handleSdkDiseaseMouseMove(QMouseEvent* mouseEvent);

	// SDK 鼠标释放事件处理入口，拉框类病害在这里提交。
	bool handleSdkDiseaseMouseRelease(QMouseEvent* mouseEvent);

	// SDK 删除模式下的小框专用入口：左键删单格，右键删整条病害。
	bool handleSdkLittleFrameDeleteMousePress(QMouseEvent* mouseEvent, const pixImagePoint& point);

	// 从指定 SDK 单图点开始一笔病害绘制。
	void beginSdkDiseaseAt(const pixImagePoint& point);

	// 结束 SDK 大框/设计面绘制，弹出病害选择并入库。
	bool finishSdkBigFrameDisease();

	// 结束 SDK 小框绘制，弹出病害选择并入库。
	bool finishSdkLittleFrameDisease();

	// 结束 SDK 小框 B 折线绘制，补齐中间格子后入库。
	bool finishSdkLineLittleFrameDisease();

	// 为 SDK 小框折线轨迹补充跨帧或长距离移动中的中间采样点。
	void addInterpolatedLittleFramePointsForSdkLine(QVector<pixImagePoint>& points) const;

	// 根据当前 SDK 鼠标轨迹或拉框范围刷新临时小框集合。
	void updateSdkLittleFramePreview();

	// 提交小框病害前，根据当前模式一次性生成最终小框集合；绘制阶段不实时铺满小格。
	void prepareSdkLittleFrameSelectionsForCommit();

	// 把当前临时绘制状态同步到 SDK scene 临时图层。
	void refreshSdkTemporaryDiseaseItems();

	// 向 SDK scene 添加临时矩形预览。
	void addSdkTemporaryRect(const QRectF& rect, const QColor& color, int width, Qt::PenStyle style);

	// 向 SDK scene 添加临时折线/轨迹预览。
	void addSdkTemporaryLinePath(const QVector<pixImagePoint>& points, const QColor& color, int width, Qt::PenStyle style, const pixImagePoint* tailPoint = nullptr);

	// 把旧大图矩形集合转换成 SDK scene 临时矩形并显示。
	void addSdkTemporaryBigImageRects(const QVector<QRect>& rects, const QColor& color, int width, Qt::PenStyle style);

	// 从病害服务按当前可见里程范围重建 SDK 正式病害 item。
	void refreshSdkDiseaseItems();

	// Coalesce scroll-driven refreshes; explicit editing and jumps still call refresh directly.
	void scheduleSdkDiseaseRefresh();
	QByteArray sdkDiseaseRenderFingerprint(const hnRoadDiseaseInfo& disease);

	// 向 SDK 正式病害图层添加单个病害 item。
	void addSdkDiseaseItem(const hnRoadDiseaseInfo& disease);

	// 确保指定病害在 SDK 图层存在并触发 scene/viewport 刷新。
	void ensureSdkDiseaseItemVisible(const hnRoadDiseaseInfo& disease);

	// 刷新 SDK 中的路面材质、标准、等级等工程打标显示。
	void refreshSdkMaterialMarks();

	// 把工程打标类型编号转换成状态/标签可读名称。
	QString sdkRoadMarkTypeName(int markType) const;

	// 根据打标内容生成稳定颜色，材质切换边界可被一眼识别。
	QColor sdkRoadMarkColor(const hnCommon::hnMarkInfo& mark) const;

	// 判断某类打标是否会影响病害类型选择和跨材质绘制校验。
	bool isRoadAttributeMarkType(int markType) const;

	// 计算当前临时病害覆盖的编码器里程范围，用于跨材质/标准校验。
	bool temporaryDiseaseEncoderMileRange(int frameType, double& beginMile, double& endMile) const;

	// 判断当前临时病害是否跨越不同路面属性；跨材质或标准时禁止绘制。
	bool isTmpDiseaseRoadMarkRangeValid(int frameType, QString* invalidReason = nullptr) const;

	// 在 SDK scene 中按 viewport 点命中正式病害并同步选中状态。
	bool selectSdkDiseaseAtViewportPoint(const QPoint& viewportPoint);

	// 合并模式直接使用 SDK scene 命中病害，避免回退到已失效的旧 widget 坐标命中。
	bool handleSdkMergeMousePress(QMouseEvent* mouseEvent);

	// 在当前可见 SDK 病害中查找鼠标命中的小框病害和格子下标。
	bool findSdkLittleFrameDiseaseAtPoint(const pixImagePoint& point, hnRoadDiseaseInfo& disease, int& hitIndex);


	// 删除模式下更新鼠标所在小格的临时高亮，离开病害时清除高亮。
	void updateSdkLittleFrameDeleteHover(const QPoint& viewportPoint);

	// 生成小框渲染缓存 key；病害几何或显示参数变化时 key 必须随之变化。
	QString sdkLittleFrameRenderCacheKey(const hnRoadDiseaseInfo& disease) const;

	// 读取、写入和清空小框 scene 路径缓存，避免滚动时重复构造大量路径。
	bool cachedSdkLittleFrameScenePath(const hnRoadDiseaseInfo& disease, QPainterPath& path) const;
	void cacheSdkLittleFrameScenePath(const hnRoadDiseaseInfo& disease, const QPainterPath& path, SdkLittleFrameRenderMode mode);
	void clearSdkLittleFrameRenderCache();

	// 生成 SDK 病害唯一 key；应用层负责组合表名和 ID，SDK 图层只接收不透明 key。
	QString sdkDiseaseKey(const hnRoadDiseaseInfo& disease) const;

	// 判断病害对象是否与指定 SDK key 指向同一条业务病害。
	bool isSameSdkDisease(const hnRoadDiseaseInfo& disease, const QString& diseaseKey) const;

	// 把旧大图矩形转换成 SDK scene path，供尚未完全迁移的旧几何使用。
	QPainterPath sdkPathFromBigImageRect(const QRect& rect) const;

	// 把旧大图点序列转换成 SDK scene path，线状病害兼容路径使用。
	QPainterPath sdkPathFromBigImagePoints(const QVector<QPoint>& points) const;

	// 子类把数据库中的 2D/3D 病害几何转换成 SDK scene path。
	virtual QPainterPath sdkDiseaseScenePath(const hnRoadDiseaseInfo& disease);

	// 把旧大图坐标点转换成 SDK scene 坐标。
	QPointF bigImagePointToSdkScenePoint(const QPoint& point) const;

	// 判断当前病害是否应该在 SDK 可见范围内渲染。
	bool shouldRenderSdkDisease(const hnRoadDiseaseInfo& disease) const;

	// 根据材质名称生成 SDK 打标颜色。
	QColor sdkMaterialMarkColor(const QString& materialName) const;

	// 生成 SDK 病害标签文字；选中病害显示更详细内容。
	QString sdkDiseaseLabel(const hnRoadDiseaseInfo& disease, bool selected) const;

	// 清空最近一次 SDK 新增病害缓存。
	void clearLastSdkAddedDisease();

	// 记录刚刚成功入库的 SDK 病害，提交后用于自动选中和居中。
	void rememberLastSdkAddedDisease(const hnRoadDiseaseInfo& disease);

	// 取出并清空最近一次 SDK 新增病害缓存。
	bool takeLastSdkAddedDisease(hnRoadDiseaseInfo& disease);

	// 子类返回病害对应的旧大图矩形集合，主要用于兼容调试和旧路径兜底。
	virtual QVector<QRect> sdkDiseaseBigImageRects(const hnRoadDiseaseInfo& disease) = 0;

	// 子类提交大框/设计面病害，内部继续复用原有入库和校验逻辑。
	virtual bool sdkCommitBigFrameDisease() = 0;

	// 子类提交小框病害，内部继续复用原有入库和校验逻辑。
	virtual bool sdkCommitLittleFrameDisease() = 0;

	// 子类按 SDK 单图点创建小框格子集合，用于预览和入库几何生成。
	virtual QVector<QRect> sdkCreateLittleFrameRects(const QVector<pixImagePoint>& pixImagePoints) = 0;

	// 子类用 SDK 单图点命中数据库小框下标；删除模式不再使用旧大图鼠标坐标。
	virtual int sdkLittleFrameHitIndex(const hnRoadDiseaseInfo& disease, const pixImagePoint& point) = 0;

	// 子类把数据库中的指定小框格子转换为 SDK scene 矩形，用于删除悬停高亮。
	virtual bool sdkLittleFrameCellSceneRect(const hnRoadDiseaseInfo& disease, int hitIndex, QRectF& sceneRect) = 0;

	// 子类复用各自的小框更新逻辑，统一完成重算面积、更新数据库和刷新信号。
	virtual void updateSdkLittleFrameDiseaseAfterCellDelete(hnRoadDiseaseInfo& disease) = 0;

protected:
	// 获取病害类型
	QStringList getDiseaseTypes();
	// 获取病害设置信息
	QVector<hnDiseaseSetInfo> getDiseaseSetInfos();

protected:
	// 添加线状病害
	bool lineDiseaseAddDisease();
	void appendLittleFrameDrawingPoint(const pixImagePoint& point);
	bool hasLittleFramePointInPix(const QString& pixName) const;
protected:
	//绘制线状病害
	void drawLineDiseases(const vector<hnRoadDiseaseInfo> &diseases, QImage &image);

	//绘制临时线状病害
	void drawTmpLineDiseases(QImage &image);

	//2025.11.3 新增绘制最后一个点与鼠标连线（虚线）
	void drawTempDashLine(QImage &image);

	// 创建线状病害的点数组
	virtual QVector<QPoint> createBrokenLinePoints(hnRoadDiseaseInfo &disease) = 0;

	//输入一个针对于一个大image的点，输出hnmile
	virtual hnMile getHnMileFromPoint(const QPoint &allImagePoint) = 0;

	// SDK 绘制直接按图片名和单图像素获取道路上下文，禁止再绕行旧临时大图。
	virtual bool sdkHnMileFromPoint(const pixImagePoint& point, hnMile& mile) const = 0;

	// 业务层可限制 SDK 绘制点；面阵默认允许，线阵二维视图按有效区域拒绝或钳制横坐标。
	virtual bool adjustSdkDiseasePointToValidArea(pixImagePoint& point, bool clampToArea) const;
	virtual bool validateDiseaseGeometryWithinValidArea(const hnRoadDiseaseInfo& disease) const;

	//线状病害计算里程
	double  calculateLineDiseaseCenterMile(QVector<pixImagePoint> lineDiseasePoints);
	double  calculateLineDiseaseBeginMile(QVector<pixImagePoint> lineDiseasePoints);
	double  calculateLineDiseaseEndMile(QVector<pixImagePoint> lineDiseasePoints);

	// 根据大image上的点 ，得到编码器里程
	virtual double caculateEncoderMileByBigImagePoint(const QPoint &bigImagePoint) = 0;

	//创造线病害点数组的里程map 从小到大排列
	QMap<int, double> createLineDiseaseMileMap(const QVector<pixImagePoint> &lineDiseasePoints);

	// 判断临时病害是否有效  
	virtual bool isTmpDiseaseRoadTypeValid(const int &frameType) = 0;

	//判断临时病害面积是否有效  
	virtual bool isTmpDiseaseAreaValid(const hnCommon::hnRoadDiseaseInfo& disease) = 0;


protected:
	// 判断当前是否处于小框自动化绘制状态，供滚轮翻页和自动续画逻辑判断。
	bool isDrawingLittleFrameDisease() const;

	// 程序自动移动鼠标后，子类 mouseMoveEvent 调用该方法跳过一次误触发的绘制移动。
	bool ignoreMouseMoveAfterAutoCursorMove(QMouseEvent* event);

	// 绘制小框过程中滚轮翻页后，延迟把鼠标移动到适合继续绘制的位置。
	void scheduleMoveCursorToBestContinuePointAfterBrowse(bool up, bool is2D);

	// 执行鼠标复位，并计算复位后的 SDK/旧图像病害锚点。
	bool moveCursorToBestContinuePointAfterBrowse(bool up, bool is2D);

	// 返回当前可见图像区域；SDK 图像铺满时默认就是 widget 区域。
	virtual QRect visibleImageWidgetRect() const;

	// 子类把病害单图点转换成当前 widget 坐标，滚轮翻页续画时用于寻找鼠标复位点。
	virtual bool diseasePointToWidgetPointAfterBrowse(
		const pixImagePoint& point,
		bool up,
		QPoint& widgetPoint)  = 0;

	// 把当前 widget 点转换成病害单图点；旧视图和 SDK 兼容绘制路径共用。
	bool widgetPointToDiseasePoint(const QPoint& widgetPoint, pixImagePoint& point);

	// 滚轮翻页后把续画锚点写回小框临时点、起止点和 SDK 预览状态。
	void syncLittleFrameContinueAnchor(const QPoint& targetWidgetPoint);

	// 拉框模式跨页或提交前，把当前 SDK 单图格子选区固化到 committed 集合。
	struct LittleFrameSingleRectSelection;

	void commitCurrentLittleRectDrawSelection();

	// 把已固化的单图格子临时转换回旧大图矩形，供旧小框算法继续做预览和面积计算。
	// 返回当前 SDK 小框单图格子集合，合并已固化格子和当前预览格子；SDK 显示只使用这个集合。
	QVector<LittleFrameSingleRectSelection> currentLittleFrameSelectionsForSdk() const;

	// 仅在提交入库边界把 SDK 单图格子转换成旧数据库需要的大图 QRect；绘制预览禁止依赖该结果。
	void buildCurrentLittleFrameStorageRects(QVector<QRect>& rects) const;

	// Deprecated: 旧大图小框桥接接口，只允许非 SDK 路径或历史兼容代码使用。
	void appendCommittedLittleRectDrawSelection(QVector<QRect>& rects);

	// 旧兜底：从旧大图矩形反算当前小框单图格子；SDK 新绘制路径不应优先依赖它。
	void rebuildCurrentLittleFrameSingleSelections(const QVector<QRect>& bigRects);

	// SDK 拉框模式：直接根据起止单图点生成当前小框单图格子，避免旧大图坐标漂移。
	void rebuildCurrentLittleFrameSingleSelectionsFromRect(const pixImagePoint& first, const pixImagePoint& second);

	// SDK 轨迹/B 折线模式：根据单图轨迹点生成当前小框单图格子，并补齐中间采样格。
	void rebuildCurrentLittleFrameSingleSelectionsFromPoints(const QVector<pixImagePoint>& points);

	// 根据单个 SDK 单图点计算其所在的 0.1m 小框格子。
	QRect littleFrameSingleCellForPoint(const pixImagePoint& point);

	// 根据指定 SDK 图片构造物理网格规格，保证 2D/3D 小框按实际比例对齐。
	PhysicalGridSpec sdkLittleFrameGridSpecForImage(const QString& pixName) const;

	// 将 SDK 网格工具返回的格子转换为应用层单图小框选择记录。
	bool littleFrameSelectionFromGridCell(const GridCell& cell, LittleFrameSingleRectSelection& selection) const;

	// 把一个单图格子加入当前 SDK 小框选区，自动过滤空格子和重复格子。
	void appendCurrentLittleFrameSingleSelection(const QString& pixName, const QRect& singleRect);

	// SDK 大框/设计面提交前直接由起止单图点生成旧大图矩形，并拦截高度或宽度为 0 的异常框。
	bool currentSdkBigFrameRect(QRect& rect) const;

	// SDK 大框/设计面提交前直接取 scene 四角命中的单图点，正式入库坐标优先使用该结果。
	bool currentSdkBigFrameCornerPoints(QVector<pixImagePoint>& points) const;


	// 清空小框绘制过程中的临时格子、已固化格子和旧大图矩形缓存。
	void clearLittleRectDrawSelection();

	// 重置小框绘制状态，包括起止点、轨迹点和临时预览数据。
	void resetLittleFrameDrawState();

	// 根据小框单图点重建旧大图点集合，供旧 crossOver/crossLineOver 算法兼容使用。
	void rebuildLittleFrameBigImagePoints();


protected:
	struct LittleFrameSingleRectSelection
	{
		QString pixName;
		QRect singleRect;
		int row = -1;
		int column = -1;

		bool operator==(const LittleFrameSingleRectSelection& other) const
		{
			if (row >= 0 && column >= 0 && other.row >= 0 && other.column >= 0)
			{
				return pixName == other.pixName && row == other.row && column == other.column;
			}
			return pixName == other.pixName && singleRect == other.singleRect;
		}
	};

	virtual QRect bigImageRectToSingleImageRect(const QRect& bigImageRect, QString* imageName) = 0;
	virtual QRect singleImageRectToBigImageRect(const QRect& singleImageRect, const QString& imageName) = 0;

	// 程序自动 setPos 后，下一次 mouseMove 不参与绘制
	bool m_ignoreNextMouseMoveAfterAutoCursorMove = false;
	pixImagePoint m_littleFrameWheelAnchorPoint;
	bool m_hasLittleFrameWheelAnchorPoint = false;
	// 记录滚轮发生时的视口位置。跨不同 DPI 屏幕恢复鼠标时，横向位置必须保持不变。
	QPoint m_littleFrameWheelViewportPoint;
	// 取消小框病害选择后，必须等下一次左键按下才允许重新追加小框轨迹。
	bool m_waitLittleFrameLeftPressAfterCancel = false;
	bool m_hasLastSdkStatusViewportPoint = false;
	QPoint m_lastSdkStatusViewportPoint;
	QVector<LittleFrameSingleRectSelection> m_committedLittleFrameDiseaseRects;
	QVector<LittleFrameSingleRectSelection> m_currentLittleFrameSingleSelections;
	mutable QHash<QString, SdkLittleFrameRenderCacheEntry> m_sdkLittleFrameRenderCache;
	mutable QStringList m_sdkLittleFrameRenderCacheOrder;
	QString m_currentSdkLittleFrameHoverKey;
	int m_currentSdkLittleFrameHoverIndex = -1;
protected:
	// 添加线状病害
	void addLineDisease(const QPoint &widgetPoint);

	// 通用的删除病害的方法
	void commonDeleteDisease(const QPoint &mousePoint,hnFrameMode::FrameMode frameMode) ;

	// 通用的编辑病害的方法
	void commonEditDisease(const QPoint &mousePoint, hnFrameMode::FrameMode frameMode);

	// 线状病害合并
	void mergeLineDisease(const QPoint &widgetPoint);

	//判断一个点是不是在线状病害附近
	bool isNearbyLineDisease(const QPoint &bigImagePoint, const hnRoadDiseaseInfo&disease);

	//判断屏幕上一个点是不是在一个病害的内部
	virtual bool isInDisease(const QPoint & screenPoint, const hnRoadDiseaseInfo& disease, const hnFrameMode::FrameMode &frameMode) = 0;

	//大自动化模式通用编辑病害的通用接口
	virtual void editDisease(hnRoadDiseaseInfo &disease, const QPoint &mousePoint) = 0;

	// 计算线状病害的总长度
	double caculateLineDiseaseLenth(QVector<pixImagePoint> lineDiseasePoints,WidgetType widgetType);

protected:

	//自动纠正三维视图的x坐标
	void autoCorrectXIn3dView(int &x);


	QPoint currentMousePos;
protected:
	// pixImagePoint 转 bigImagePoint
	QPoint pixImagePointToBigImagePoint(const pixImagePoint &point);
	void clearVisibleLittleFrameRectCache();
protected:
	//线状病害的临时点，这个点用于存储用户点击的点
	QVector<pixImagePoint> m_tmpLineDiseasePoints;

	// 绘制的线状病害的临时点，用于绘制
	QVector<pixImagePoint> m_tmpPaintLineDiseasePoints;

	//2025.11.3 画临时虚线（最后一个鼠标左键点击点与鼠标位置点）
	QVector<pixImagePoint> m_tempPoints;

	//绘制线状病害 记录最后一次用户按下后移动的点 
	QVector<pixImagePoint> m_tmpLastPaintLineDiseasePoints; 
	// 绘制线条的宽度，矩形的边框宽度也适用
	int m_lineWidth;
	// 字体大小
	int m_fontSize;

	QVector<QRect> m_cachedVisibleLittleFrameRects;
	QStringList m_cachedVisibleLittleFramePixNames;
protected:
	WidgetType m_widgetType;

	TiledGraphicsView* m_sdkImageView = nullptr;
	TunnelViewerController* m_sdkImageController = nullptr;
	bool m_singleFrameNavigationEnabled = false;
	QWidget* m_sdkDiseaseOverlay = nullptr;
	hnSdkDiseaseGraphicsLayer m_sdkDiseaseGraphicsLayer;
	QTimer* m_sdkDiseaseRefreshTimer = nullptr;
	QHash<QString, QByteArray> m_sdkDiseaseRenderFingerprints;

	// 单张图代表多少米。默认 0 表示还没配置，配置前不做里程换算。
	double m_imageDistanceMeters = 0.0;
	bool m_isProgrammaticSdkScroll = false;
    QString m_currentStreetPictureNameForStatus;
	QString m_selectedSdkDiseaseKey;
	hnRoadDiseaseInfo m_lastSdkAddedDisease;
	bool m_hasLastSdkAddedDisease = false;
	int m_programmaticSdkScrollTicket = 0;

	// 鼠标右键移动删除病害，是否按下标志
	bool m_isRightDeleteMouseDown;

	// 右键拖拽删除病害时只记录按下时自动化模式位置，鼠标滑动的位置不处理，不然会卡
	QPoint m_RightDeleteMousePoint;

	int m_pendingRightClcikDeleteSerial = 0;
	QPoint m_pendingRightClickDeletePoint = QPoint(-1,-1);
	
	//重新计算病害尺寸或 重新计算病后尺寸后写入数据库  
	//save为false为适配以前版本，为自动化模式外接矩形计算长度和宽度,不写入数据库
	//save为true,当用户修改了病害面积计算方式的时候重新计算病害尺寸并且写入数据库
	void reCalculateDiseaseSizeAndSave(hnCommon::hnRoadDiseaseInfo&  disease,bool save);


	//重新计算老版本的自动化模式尺寸，以前的版本没有 长度和宽度
	void reCalculateOldDiseaseSizeAndSave(const QVector<QRect>& diseaseRects,hnCommon::hnRoadDiseaseInfo&  disease);


	//计算病害长度和宽度，根据自动化模式的最大外接矩形
	void CalculateDiseaseSize(const QVector<QRect>&diseaseRects, hnCommon::hnRoadDiseaseInfo&  disease);

	// 删除单个小框后，按剩余小框重新计算面积、数量、长度和宽度，保证界面与数据库一致。
	void recalculateLittleFrameDiseaseAfterCellDelete(hnCommon::hnRoadDiseaseInfo& disease);
 	 
private:
		//根据最大外接矩形设置病害尺寸
		void setLittleDiseaseSize(const QVector<QRect>& diseaseRects, hnRoadDiseaseInfo& disease);
protected:
	//是否结束左键连续点击添加病害 
	bool m_isEndAddPoint;

	//bool m_diseaseCacheValid;
	//double m_cacheBeginEncoderMile;
	//double m_cacheEndEncoderMile;
	//int m_cacheFrameMode;
};

