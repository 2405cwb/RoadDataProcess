#ifndef TILEDGRAPHICSVIEW_H
#define TILEDGRAPHICSVIEW_H

#include <QCoreApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QElapsedTimer>
#include <QTimer>
#include <QHash>
#include <QMap>
#include <QCache>
#include <QSet>
#include <QSharedPointer>
#include <QThreadPool>
#include <vector>
#include "TunnelSectionItem.h"
#include "TunnelGlobal.h"
#include "DefectManager.h"
#include "../include/ImageDisplayAdjustments.h"
#include "../include/ImageCacheManager.h"
#include "../include/ImageCoordinateMapper.h"
#include "../include/ImageSource.h"
#include "../include/ImageLayoutManager.h"
#include <opencv2/opencv.hpp>

using namespace std;

// 前置声明：减少头文件之间的直接依赖，缩短工程编译时间。
class AbstractTool;
class DefectDrawTool;
class CurtainCompareItem;
class ISequenceFrameSource;
class ImageSequenceModel;
class SequenceChunkItem;
struct SequenceFrameDescriptor;

/**
 * @brief 当前视口底部中心点对应的“图片 + 像素”锚点。
 *
 * 主要用于业务程序把 SDK 场景坐标转换为实际图片位置，例如：
 * 1. 双视图同步；
 * 2. 根据图片像素计算里程；
 * 3. 记录用户当前浏览位置并在之后恢复。
 */
struct TiledViewAnchor
{
    bool valid = false;          ///< 当前锚点是否有效。
    QString imageName;           ///< 锚点所在图片名称。
    int imageIndex = -1;         ///< 锚点所在图片序号。
    QPointF scenePos;            ///< 锚点对应的全局 Scene 坐标。
    QPointF imagePixelPos;       ///< 锚点在原始图片内部的像素坐标。
};

/**
 * @brief 隧道/道路连续图像浏览视图，是 SDK 的核心显示控件。
 *
 * 该类主要负责：
 * 1. 显示数据库瓦片或虚拟图片序列（普通单图、Pack 均属于虚拟序列）；
 * 2. 根据视口和缩放级别按需调度缩略图/高清图；
 * 3. 维护图片解码缓存、后台解码线程和预加载；
 * 4. 处理缩放、滚动、键盘导航、鼠标平移等浏览交互；
 * 5. 管理病害、轨枕、拱脚线等业务图元；
 * 6. 提供区域导出、Mat 获取、卷帘对比、双视图同步等能力。
 *
 * 设计原则：
 * 普通单图和 Pack 只在 ISequenceFrameSource 的“图片读取方式”上不同；
 * 图片进入虚拟序列以后，共用相同的布局、缩略图/高清图、缓存、滚动和绘制流程。
 */
class TiledGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    /**
     * @brief 视图渲染后端。
     */
    enum RenderBackend
    {
        RenderBackend_Raster,    ///< Qt 默认栅格渲染，兼容性和稳定性最好。
        RenderBackend_OpenGL     ///< 使用 OpenGL viewport 渲染。
    };

    /**
     * @brief 初始化 Qt 高 DPI 支持。
     *
     * 如果工程需要高 DPI 缩放，应在创建 QApplication 之前调用本函数。
     */
    static void init()
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
        // Qt 要求该属性必须在 QApplication 创建之前设置。
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    }

    explicit TiledGraphicsView(QWidget* parent = nullptr);
    ~TiledGraphicsView();

    // =====================================================================
    // 坐标转换与视口定位
    // =====================================================================

    /**
     * @brief 将指定图片内部的像素坐标转换为全局 Scene 坐标。
     * @param imageName 图片名称。
     * @param localX 图片内部 X 像素坐标。
     * @param localY 图片内部 Y 像素坐标。
     * @return 对应的 Scene 坐标；找不到图片时返回 QPointF(0, 0)。
     *
     * 注意：该接口为了兼容旧代码可能使用图片名称别名/缓存匹配。
     * 如果业务要求必须精确匹配图片名称，优先使用 tryMapToGlobalScene()。
     */
    QPointF mapToGlobalScene(const QString& imageName, qreal localX, qreal localY);

    /**
     * @brief 使用严格图片名匹配，把图片像素坐标转换为 Scene 坐标。
     * @param imageName 要定位的图片名称。
     * @param localX 图片内部 X 像素坐标。
     * @param localY 图片内部 Y 像素坐标。
     * @param scenePos [out] 转换成功后的 Scene 坐标。
     * @return 成功返回 true；未找到精确图片时返回 false。
     */
    bool tryMapToGlobalScene(const QString& imageName, qreal localX, qreal localY, QPointF& scenePos);

    /**
     * @brief 判断当前数据库图层中是否存在名称完全匹配的图片。
     */
    bool hasExactImageLayer(const QString& imageName) const;

    /**
     * @brief 在数据库图层中按图片名称精确查找，并把局部像素转换为 Scene 坐标。
     */
    bool tryMapToExactImageLayer(const QString& imageName, qreal localX, qreal localY, QPointF& scenePos);

    /**
     * @brief 将全局 Scene 坐标反算为“图片名称 + 图片内部像素坐标”。
     * @param pt Scene 坐标。
     * @param imageName [out] 命中的图片名称。
     * @param localX [out] 图片内部 X 像素。
     * @param localY [out] 图片内部 Y 像素。
     * @return 转换成功返回 true。
     */
    bool GlobalSceneToMap(QPointF pt, QString& imageName, int& localX, int& localY);

    /* 新统一坐标接口：同时覆盖数据库瓦片和虚拟序列。 */
    bool imageCoordinateToScene(const ImageCoordinate& coordinate, QPointF& scenePos) const;
    bool sceneToImageCoordinate(const QPointF& scenePos, ImageCoordinate& coordinate) const;

    /**
     * @brief 清空“图片名称 -> TunnelSectionItem”快速查找缓存。
     *
     * 在图层被整体重建、删除或重新加载之后调用，避免继续使用旧 Item 指针。
     */
    void clearImageItemLookupCache();

    /**
     * @brief 将视图中心跳转到指定 Scene 位置。
     * @param scenePos 目标 Scene 坐标。
     * @param targetScale 目标缩放比例；<= 0 时保持当前缩放。
     */
    void focusOnPosition(const QPointF& scenePos, double targetScale = -1.0);

    /** @brief 获取当前视口中心对应的 Scene 坐标。 */
    QPointF currentCenterScenePos() const;

    /**
     * @brief 获取当前视口中心的 Scene Y 坐标。
     * @note 主要用于纵向拼接图像的里程映射。
     */
    double currentCenterSceneY() const;

    /** @brief 获取当前视口底部中心点对应的 Scene 坐标。 */
    QPointF currentBottomCenterScenePos() const;

    /**
     * @brief 获取当前视口底部中心点对应的图片/像素锚点。
     * @return 锚点无效时 valid 为 false。
     */
    TiledViewAnchor currentBottomAnchor() const;

    /**
     * @brief 滚动到指定 Scene Y，尽量保持当前横向位置不变。
     */
    void scrollToSceneY(double sceneY);

    /**
     * @brief 按图片序号滚动到指定像素 Y。
     * @param imageIndex 图片序号。
     * @param pixelY 图片内部 Y 像素。
     * @param anchorBottom true：让该像素位于视口底部；false：按实现使用普通定位方式。
     */
    void scrollToImagePixel(int imageIndex, double pixelY, bool anchorBottom = true);

    /**
     * @brief 双视图同步专用：按图片序号轻量同步浏览位置。
     *
     * 本函数只调整位置，不立即执行一整套重型瓦片调度；
     * 后续加载由滚动条防抖/可视区域调度统一处理。
     */
    void synchronizeToImagePixel(int imageIndex, double pixelY, bool anchorBottom = true);

    /**
     * @brief 双视图同步专用：按图片名称轻量同步浏览位置。
     */
    void synchronizeToImagePixel(QString qstrImageName, double pixelY, bool anchorBottom = true);

    /**
     * @brief 按图片名称和图片内部像素 Y 定位。
     *
     * 相比依赖图片序号，图片名定位在两套数据序号不完全一致时更加稳定。
     */
    void scrollToImagePixel(const QString& imageName, double pixelY, bool anchorBottom = true);

    // =====================================================================
    // 数据源加载
    // =====================================================================

    /**
     * @brief 添加一个数据库瓦片图层。
     * @param source 数据库瓦片数据源，View 接管其对应显示 Item 的创建和调度。
     */
    void addLayer(AbstractTileSource* source);

    /**
     * @brief 添加带业务里程信息的数据库瓦片图层。
     *
     * 该重载主要服务于 TunnelViewer 的卷帘对比功能；
     * 其他软件仍可继续调用 addLayer(AbstractTileSource*)。
     */
    void addLayer(AbstractTileSource* source, const DbImageInfo& imageInfo);

    /**
     * @brief 加载虚拟图片序列。
     * @param source 图片帧数据源。普通单图和当前 Pack 都实现 ISequenceFrameSource。
     * @param options 缩略图尺寸、缓存大小、解码线程数、预加载范围等参数。
     * @param orientation 图片拼接方向。
     * @param horizontalMirror 是否水平镜像。
     * @param verticalMirror 是否垂直镜像。
     * @return 加载成功返回 true。
     *
     * 该函数只建立序列模型和 Scene 布局，不会一次性完整解码所有图片。
     * 真正图片解码由 updateVisibleTiles() 根据当前视口按需触发。
     */
    bool loadVirtualSequence(const QSharedPointer<ISequenceFrameSource>& source,
        const SequenceLoadOptions& options, LayoutOrientation orientation,
        bool horizontalMirror, bool verticalMirror);

    /*
     * 新通用命名入口。IImageSource 与旧 ISequenceFrameSource 是同一接口，
     * 因而只是 API 收口，不会复制解码流程或改变现有显示结果。
     */
    bool loadImageSource(const QSharedPointer<IImageSource>& source,
        const SequenceLoadOptions& options, LayoutOrientation orientation,
        bool horizontalMirror = false, bool verticalMirror = false)
    {
        return loadVirtualSequence(source, options, orientation, horizontalMirror, verticalMirror);
    }

    /** @brief 获取当前内容模式：数据库瓦片或虚拟图片序列。 */
    ContentMode contentMode() const { return m_contentMode; }

    /** @brief 获取当前已加载图片数量。 */
    int imageCount() const;

    /**
     * @brief 获取虚拟序列中指定源序号对应的图片描述信息。
     */
    SequenceFrameDescriptor imageDescriptor(int sourceIndex) const;

    /**
     * @brief 获取当前视口实际相交的图片文件名。
     * @return 按当前视觉顺序返回图片名称列表。
     *
     * 本函数只查询几何关系，不会触发缩略图或高清图解码。
     */
    QStringList visibleImageFileNames() const;

    /**
     * @brief 获取当前数据库图层的数据库路径列表。
     * @param vecItemsDBPath [out] DB 文件路径。
     * @return 始终返回 true。
     * @note 兼容老 TunnelViewer 接口；虚拟图片序列模式下通常为空。
     */
    bool getAllItemDBPath(std::vector<QString>& vecItemsDBPath)
    {
        vecItemsDBPath.clear();
        for (TunnelSectionItem* item : m_items) {
            if (item && item->getSource())
                vecItemsDBPath.push_back(item->getSource()->getDbPath());
        }
        return true;
    }

    // =====================================================================
    // 虚拟序列绘制辅助
    // =====================================================================

    /**
     * @brief SequenceChunkItem 绘制时获取指定图片。
     * @param sourceIndex 图片在数据源中的序号。
     * @param highResolution true 请求高清图；false 请求缩略图。
     * @return 缓存中可直接使用的图片；如果目标图尚未完成解码，函数会触发后台请求。
     */
    QImage sequenceImageForPaint(int sourceIndex, bool highResolution);

    /** @brief 当前序列数据源是否支持缩略图快速浏览。 */
    bool sequenceUsesThumbnailPreview() const;

    /** @brief 根据数据源、浏览状态和 LOD 判断当前是否应显示高清图。 */
    bool sequenceShouldUseHighResolution(qreal lod) const;

    /** @brief 获取虚拟序列是否水平镜像。 */
    bool sequenceHorizontalMirror() const { return m_sequenceHorizontalMirror; }

    /** @brief 获取虚拟序列是否垂直镜像。 */
    bool sequenceVerticalMirror() const { return m_sequenceVerticalMirror; }

    // =====================================================================
    // 浏览/绘制模式与专项功能
    // =====================================================================

    /**
     * @brief 设置当前视图工作模式。
     * @param mode 浏览、绘制、导出框或移动模式。
     */
    void setViewMode(ViewMode mode);

    /**
     * @brief 清空当前显示内容、图元和相关缓存，并恢复可重新加载状态。
     */
    void clear();

    /**
     * @brief 启用/关闭 DXF 测量点拾取功能。
     * @note TunnelViewer 专项功能，默认关闭。
     */
    void setDxfMeasureEnabled(bool enabled);

    /** @brief 返回 DXF 测量模式是否启用。 */
    bool isDxfMeasureEnabled() const { return m_dxfMeasureEnabled; }

    /**
     * @brief 设置隧道定位类图元是否允许移动。
     */
    void setTunnelLocationMoveEnabled(bool enabled);

    /**
     * @brief 设置卷帘对比所使用的对比图像列表。
     * @return 配置成功返回 true。
     */
    bool setCurtainCompareImages(const QList<DbImageInfo>& compareImages);

    /**
     * @brief 启用或关闭卷帘对比。
     * @return 切换成功返回 true。
     */
    bool setCurtainCompareEnabled(bool enabled);

    /** @brief 返回卷帘对比当前是否启用。 */
    bool isCurtainCompareEnabled() const { return m_curtainEnabled; }

    /** @brief 设置卷帘分割方向（垂直/水平）。 */
    void setCurtainOrientation(CurtainOrientation orientation);

    /** @brief 获取当前卷帘分割方向。 */
    CurtainOrientation curtainOrientation() const { return m_curtainOrientation; }

    /**
     * @brief 设置卷帘分割线位置。
     * @param ratio 归一化位置比例，通常位于 0~1 之间。
     */
    void setCurtainPosition(qreal ratio);

    /** @brief 获取当前卷帘分割线归一化位置。 */
    qreal curtainPosition() const;

    /**
     * @brief 清空卷帘对比资源并关闭相关显示状态。
     */
    void clearCurtainCompare();

    /** @brief 获取当前视图使用的 QGraphicsScene。 */
    QGraphicsScene* scene() const { return m_scene; }

    /**
     * @brief 启用/关闭 SDK 内置键盘导航。
     */
    void setKeyboardNavigationEnabled(bool enable) { m_enableKeyNav = enable; }

    /**
     * @brief 设置虚拟序列是否使用“单帧步进”导航。
     *
     * 开启后，在普通单图/Pack 模式下，滚轮及主轴方向导航键可一次精确移动一张图片；
     * 数据库瓦片模式仍保持原有连续滚动方式。
     */
    void setSingleFrameNavigationEnabled(bool enable) { m_singleFrameNavigationEnabled = enable; }

    /** @brief 返回是否启用了单帧导航。 */
    bool singleFrameNavigationEnabled() const { return m_singleFrameNavigationEnabled; }

    /**
     * @brief 按视觉顺序前进或后退一帧。
     * @param visualDelta 正值向后，负值向前。
     * @return 无内容或已经到达首尾时返回 false。
     */
    bool stepSingleFrame(int visualDelta);

    /**
     * @brief 切换视图渲染后端。
     */
    void setRenderBackend(RenderBackend backend);

    /** @brief 获取当前渲染后端。 */
    RenderBackend renderBackend() const { return m_renderBackend; }

    /**
     * @brief 设置图片序列拼接方向。
     */
    void setLayoutOrientation(LayoutOrientation orientation)
    {
        m_orientation = orientation;
    }

    /**
     * @brief 设置键盘/滚动导航使用的基础移动速度。
     */
    void set_scrollSpeed(int speed)
    {
        m_scrollSpeed = speed;
    }

    /** @brief 设置当前 View 的业务名称，用于多视图场景区分。 */
    void setViewName(QString strName);

    /** @brief 获取当前 View 的业务名称。 */
    QString getViewName();

    /**
     * @brief 启动病害/业务图元绘制，并指定要绘制的几何形状。
     */
    void startDrawingDefect(DrawShape shapeType);

    /**
     * @brief 更新当前绘图工具使用的几何类型。
     */
    void setDrawingGeometry(DrawShape shapeType);

    /**
     * @brief 绘图工具内部回调：把完成的几何图形转发为 sigGeometryDrawn 信号。
     */
    void emitGeometryDrawn(DrawShape shapeType, const QPainterPath& path)
    {
        // 老信号保持不变；新业务同时获得本次绘图上下文。
        emit sigGeometryDrawn(shapeType, path);
        emit sigAnnotationGeometryDrawn(m_drawContext, shapeType, path);
    }

    /* 外部自定义工具可在开始绘制前设置业务上下文；不会改变旧绘图接口。 */
    void setAnnotationDrawContext(const AnnotationDrawContext& context) { m_drawContext = context; }
    AnnotationDrawContext annotationDrawContext() const { return m_drawContext; }
    void clearAnnotationDrawContext() { m_drawContext = AnnotationDrawContext(); }

    /**
     * @brief 设置 LOD 加载等级。
     * @param level 1~5；1 偏保守、占用资源更少，5 加载更积极。
     */
    void setLodLevel(int level);

    /**
     * @brief 根据当前视口重新调度需要显示的瓦片/图片。
     *
     * 虚拟序列模式下，该函数会根据滚动状态和缩放比例决定加载缩略图还是高清图，
     * 并触发可视区域及预加载区域的后台解码任务。
     */
    void updateVisibleTiles();

    /** @brief 撤销当前绘制工具最后加入的一个绘制点。 */
    void undoLastDrawPoint();

    /** @brief 取消当前尚未完成的绘制操作。 */
    void cancelCurrentDrawing();

    /** @brief 获取当前图片/序列的基准图片高度。 */
    double getImageHeight();

    /** @brief 获取当前图片/序列的基准图片宽度。 */
    double getImageWidth();

    // =====================================================================
    // 各类业务图元管理器
    // =====================================================================

    // ---------------------------------------------------------------------
    // 通用 Annotation Manager Registry（新接口）
    // ---------------------------------------------------------------------

    /*
     * 注册一个标注图层管理器。manager==nullptr 时由 View 创建并持有。
     * 已存在相同 layerKey 时直接返回已有 Manager，不重复创建。
     */
    AnnotationManager* registerAnnotationManager(const QString& layerKey, AnnotationManager* manager = nullptr);

    /* 按通用 layerKey 获取 Manager；不存在时返回 nullptr。 */
    AnnotationManager* annotationManager(const QString& layerKey) const;

    /* 返回所有已经注册的通用图层 key。 */
    QStringList annotationLayerKeys() const;

    /* 清空指定标注图层，但不影响图片和其他标注层。 */
    bool clearAnnotationLayer(const QString& layerKey);

    /* 清空所有已注册标注层。 */
    void clearAnnotations();

    /* 仅清空图像/序列内容，不清空已注册标注。 */
    void clearImages();

    /* 设置外部自定义交互工具；旧 startDrawingDefect() 仍切回内置 DefectDrawTool。 */
    void setActiveTool(AbstractTool* tool, bool takeOwnership = false);
    AbstractTool* activeTool() const { return m_currentTool; }

    /* 外部自定义 Item Factory 的 View 级便捷入口。 */
    bool registerAnnotationItemFactory(const QString& layerKey, const QString& typeKey, const AnnotationItemCreator& creator);
    bool unregisterAnnotationItemFactory(const QString& layerKey, const QString& typeKey);

    /* 图层运行状态；只影响当前 View，不写入历史业务数据。 */
    bool setAnnotationLayerState(const QString& layerKey, const AnnotationLayerState& state);
    AnnotationLayerState annotationLayerState(const QString& layerKey) const;
    bool setAnnotationLayerVisible(const QString& layerKey, bool visible);
    bool setAnnotationLayerLocked(const QString& layerKey, bool locked);
    bool setAnnotationLayerSelectable(const QString& layerKey, bool selectable);
    bool setAnnotationLayerExportable(const QString& layerKey, bool exportable);

    /** @brief 获取病害图元管理器（旧接口继续保留）。 */
    DefectManager* defectManager() const { return m_defectManager; }

    /** @brief 获取 CP3 图元管理器。 */
    DefectManager* cp3Manager() const { return m_vecCp3Manager; }

    /** @brief 获取站台图元管理器。 */
    DefectManager* platformManager() const { return m_vecPlatformManager; }

    /** @brief 获取链相关图元管理器。 */
    DefectManager* chainManager() const { return m_vecChainManager; }

    /**
     * @brief 获取轨枕图元管理器。
     * @return 轨枕管理器；视图尚未初始化时可能返回 nullptr。
     */
    DefectManager* sleeperManager() const;

    /** @brief 获取隧道定位图元管理器。 */
    DefectManager* tunnelLocManager() const { return m_vecTunnelLocManager; }

    /** @brief 获取环信息图元管理器。 */
    DefectManager* ringInfoManager() const { return m_vecRingInfoManager; }

    /** @brief 获取断面图元管理器。 */
    DefectManager* sectionManager() const { return m_vecSectionManager; }

    /** @brief 获取自动环/自动识别环图元管理器。 */
    DefectManager* reAutoRingManager() const { return m_vecReAutoRingManager; }

    /** @brief 获取拱脚线/用户线管理器。 */
    DefectManager* architraveLineManager() const { return m_vecArchitraveLineManager; }

    /** @brief 获取当前视图工作模式。 */
    ViewMode viewMode() const { return m_currentMode; }

    /**
     * @brief 高亮指定业务图元。
     * @param uuid 图元唯一编号。
     * @param ele 图元类型。
     */
    void setHighLightElement(int uuid, ElementType ele);

    /* 新通用高亮接口；旧 setHighLightElement() 继续保持历史 ElementType 行为。 */
    bool setHighLightAnnotation(const QString& layerKey, int uuid, qreal margin = 1.85);

    // =====================================================================
    // 区域预览、导出与 OpenCV 数据获取
    // =====================================================================

    /**
     * @brief 导出指定 Scene 区域。
     * @param sceneRect 要导出的 Scene 区域。
     * @param quality 导出质量：缩略或高清。
     * @param drawDefects 是否同时绘制病害/业务图元。
     * @return 导出的 QPixmap。
     */
    QPixmap exportRegionData(const QRectF& sceneRect,
        ExportQuality quality = Export_HighRes,
        bool drawDefects = true);

    /**
     * @brief 非阻塞生成指定区域的 1:1 检查预览图。
     *
     * 虚拟序列模式下只使用当前已解码缓存；如果缺少高清图，
     * 会把高清解码任务放到后台，而不会在 UI 线程同步读取大图造成卡顿。
     */
    QPixmap previewRegionData(const QRectF& sceneRect);

    /**
     * @brief 按 Word/报告导出用途生成指定区域图片。
     */
    QPixmap exportRegionDataToWord(const QRectF& sceneRect,
        ExportQuality quality = Export_HighRes,
        bool drawDefects = true);

    /**
     * @brief 导出指定区域为 OpenCV 灰度 Mat。
     */
    cv::Mat exportRegionGrayMat(const QRectF& sceneRect,
        ExportQuality quality = Export_HighRes,
        bool drawDefects = true);

    /**
     * @brief 导出指定区域为 OpenCV Mat。
     * @param returnBgr true 时返回 OpenCV 常用 BGR 顺序；false 时按内部实现返回其他顺序。
     */
    cv::Mat exportRegionMat(const QRectF& sceneRect,
        ExportQuality quality = Export_HighRes,
        bool drawDefects = true,
        bool returnBgr = true);

    /**
     * @brief 根据图片名称获取该图片对应的 OpenCV Mat。
     */
    cv::Mat getMatByImageName(QString qstrImageName);

    /**
     * @brief 获取当前数据库瓦片模式下的底层 TunnelSectionItem 列表。
     */
    QList<TunnelSectionItem*> getTunnelSectionItem() { return m_items; }

public slots:
    /**
     * @brief 重置缩放，使全部当前内容适配到视口范围内。
     */
    void resetToFit();

    /**
     * @brief 更新 HUD/统计信息显示。
     */
    void updateHUD();

    /**
     * @brief 设置统一图像显示参数。
     *
     * 亮度、对比度、锐化同时作用于数据库瓦片和虚拟图片序列。
     */
    void setImageAdjustments(const ImageDisplayAdjustments& adjustments);

    /**
     * @brief 兼容旧接口：只修改亮度，保留当前对比度和锐化参数。
     */
    void setImageBrightness(int value);

    /**
     * @brief 后台虚拟序列图片解码完成后的 UI 线程回调。
     * @param sourceIndex 图片源序号。
     * @param highResolution 是否为高清图。
     * @param generation 序列代次，用于丢弃上一批内容遗留的异步结果。
     * @param requestSerial 视口请求序号，用于避免旧请求覆盖新视口状态。
     * @param image 解码完成的图片。
     */
    void onSequenceImageDecoded(int sourceIndex, bool highResolution, int generation,
        int requestSerial, QImage image);

signals:
    // =====================================================================
    // 对外通知信号
    // =====================================================================

    /**
     * @brief 用户完成一次几何图形绘制时发出。
     */
    void sigGeometryDrawn(DrawShape shapeType, QPainterPath path);

    /* 新扩展信号：几何 + layer/type/业务属性上下文。 */
    void sigAnnotationGeometryDrawn(AnnotationDrawContext context, DrawShape shapeType, QPainterPath path);

    /**
     * @brief DXF 测量模式下拾取到 Scene 点时发出。
     */
    void sigDxfMeasurePoint(QPointF scenePoint);

    /**
     * @brief 鼠标移动时通知当前光标所在图片和图片内部像素坐标。
     */
    void sigCursorInfoChanged(QString imgName, int x, int y);

    /**
     * @brief 通知当前视口左上角和右下角分别对应的图片/像素位置。
     */
    void sigViewInfoChanged(QString imgNameTL, int xTL, int yTL,
        QString imgNameBR, int xBR, int yBR);

    /**
     * @brief 通知 FPS、缓存、加载等 HUD 统计文本发生变化。
     */
    void sigStatsUpdated(QString stats);

    /**
     * @brief 用户请求右键菜单时发出。
     * @param globalPos 屏幕全局坐标。
     * @param itemsUnderMouse 鼠标位置命中的图元列表。
     */
    void sigContextMenuRequested(QPoint globalPos, QList<QGraphicsItem*> itemsUnderMouse);

    /**
     * @brief 用户左键双击时发出。
     */
    void sigDoubleClickedLeft(QPointF globalPos, QList<QGraphicsItem*> itemsUnderMouse);

    /**
     * @brief 病害/业务图元选中集合发生变化时发出。
     */
    void sigDefectsSelected(QList<DefectShapeItem*> selectedDefects);

    /**
     * @brief 区域导出完成时发出。
     */
    void sigRegionExported(QPixmap pixmap);

    /**
     * @brief 视口中心 Scene 坐标变化时发出。
     */
    void sigViewCenterSceneChanged(QPointF centerScenePos);

    /**
     * @brief 视口底部中心锚点变化时发出。
     * @note 包括程序调用导致的位置变化。
     */
    void sigViewBottomAnchorChanged(TiledViewAnchor anchor);

    /**
     * @brief 用户主动滚动/操作造成底部中心锚点变化时发出。
     * @note 双视图同步通常应优先监听该信号，避免程序同步造成循环回调。
     */
    void sigUserViewBottomAnchorChanged(TiledViewAnchor anchor);

    /**
     * @brief 虚拟序列某一帧进入解码缓存后发出。
     *
     * 主要供非阻塞高清预览使用：后台高清图解码完成后，外部可据此刷新预览。
     */
    void sigSequenceImageReady(int sourceIndex, bool highResolution);

protected:
    // =====================================================================
    // QWidget / QGraphicsView 事件重载
    // =====================================================================

    /** @brief 窗口尺寸变化：更新视口相关状态和资源调度。 */
    void resizeEvent(QResizeEvent* event) override;

    /** @brief View 绘制事件。 */
    void paintEvent(QPaintEvent* event) override;

    /**
     * @brief 滚动内容后更新视口状态，并触发后续可见资源调度。
     */
    void scrollContentsBy(int dx, int dy) override;

    /** @brief 鼠标按下：处理平移、绘制、选择、移动、卷帘拖动等交互。 */
    void mousePressEvent(QMouseEvent* event) override;

    /** @brief 鼠标移动：处理拖动、绘制过程、光标坐标更新等。 */
    void mouseMoveEvent(QMouseEvent* event) override;

    /** @brief 鼠标释放：结束当前拖动/绘制/卷帘操作。 */
    void mouseReleaseEvent(QMouseEvent* event) override;

    /**
     * @brief 鼠标滚轮事件：处理连续滚动、缩放或虚拟序列单帧导航。
     */
    void wheelEvent(QWheelEvent* event) override;

    /** @brief 鼠标双击事件。 */
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    /** @brief 键盘按下：处理 WASD/方向导航、快捷操作等。 */
    void keyPressEvent(QKeyEvent* event) override;

    /** @brief 键盘释放：结束连续导航状态并触发高清恢复调度。 */
    void keyReleaseEvent(QKeyEvent* event) override;

    /**
     * @brief 绘制 Scene 前景层，例如 HUD 或不适合放入普通 Item 的覆盖内容。
     */
    void drawForeground(QPainter* painter, const QRectF& rect) override;

private:
    // CurtainCompareItem 需要访问卷帘内部资源。
    friend class CurtainCompareItem;

    // =====================================================================
    // 卷帘对比内部数据结构
    // =====================================================================

    /** @brief 基准侧卷帘帧：引用已经存在的数据库图层，不复制其数据源。 */
    struct BaseCurtainFrame
    {
        TunnelSectionItem* item = nullptr;
        DbImageInfo info;
    };

    /** @brief 对比侧卷帘帧：维护缩略图和已经加载的瓦片缓存。 */
    struct CompareCurtainFrame
    {
        DbImageInfo info;
        QPixmap thumbnail;
        QHash<TileKey, QPixmap> loadedTiles;
        bool thumbnailLoading = false;
    };

    /** @brief 正在异步加载的卷帘瓦片与目标帧索引映射。 */
    struct CurtainPendingTile
    {
        int frameIndex = -1;
        TileKey key;
    };

    /** @brief 绘制卷帘对比覆盖内容。 */
    void paintCurtainCompare(QPainter* painter, const QRectF& exposedRect);

    /** @brief 根据当前视口重新计算卷帘需要的缩略图/瓦片资源。 */
    void updateCurtainCompareResources();

    /** @brief 获取当前卷帘区域对应的 viewport 矩形。 */
    QRect curtainViewportRect() const;

    /** @brief 判断鼠标位置是否靠近卷帘分割线。 */
    bool isNearCurtainLine(const QPoint& viewportPos) const;

    /** @brief 根据实际里程查找对比侧最合适的帧序号。 */
    int findCompareFrame(double mileage) const;

    /** @brief 异步请求指定卷帘帧的缩略图。 */
    void requestCurtainThumbnail(int frameIndex);

    /** @brief 异步请求指定卷帘帧中的一个高清瓦片。 */
    void requestCurtainTile(int frameIndex, int col, int row);

    /**
     * @brief 重新创建卷帘显示 Item。
     *
     * Scene 被 clear() 后旧 Item 会被 Qt 释放，因此需要同步重建，避免悬空指针。
     */
    void recreateCurtainItem();

    // =====================================================================
    // View 初始化与公共内部辅助
    // =====================================================================

    /** @brief 初始化 QGraphicsView 的渲染、滚动、交互等基础属性。 */
    void setupGraphicsView();

    /** @brief 建立滚动条、定时器及内部信号槽连接。 */
    void setupConnections();

    // =====================================================================
    // 虚拟图片序列内部流程（普通单图 / 当前 Pack 共用）
    // =====================================================================

    /**
     * @brief 清理虚拟序列模型、Chunk、解码任务状态和图片缓存。
     */
    void clearVirtualSequence();

    /**
     * @brief 在虚拟序列中按视觉顺序移动一帧。
     */
    bool stepVirtualSequenceFrame(int visualDelta);

    /**
     * @brief 请求指定序号图片的缩略图或高清图。
     * @param sourceIndex 数据源内部图片序号。
     * @param highResolution true 请求高清图；false 请求缩略图。
     * @param priority 线程池任务优先级。
     *
     * 本函数会先检查缓存和 Pending 状态，只有确实需要时才创建后台解码任务。
     */
    void requestSequenceImage(int sourceIndex, bool highResolution, int priority = 0);

    /**
     * @brief 请求与指定 Scene 区域相交的一组序列图片。
     *
     * 可视区域和前后预加载区域最终都通过该接口统一调度。
     */
    void requestSequenceRange(const QRectF& sceneRect, bool highResolution, int priority);

    /**
     * @brief 生成虚拟序列图片缓存 Key。
     *
     * 同一 sourceIndex 的缩略图和高清图使用不同 Key，可同时保存在缓存中。
     */
    QString sequenceCacheKey(int sourceIndex, bool highResolution) const;

    /**
     * @brief 按指定质量完整渲染虚拟序列中的一个 Scene 区域。
     * @note 主要用于正式导出。
     */
    QImage renderVirtualSequenceRegion(const QRectF& sceneRect,
        ExportQuality quality, bool drawDefects);

    /**
     * @brief 只使用当前缓存生成虚拟序列快速预览，不同步阻塞读取缺失高清图。
     */
    QImage renderCachedVirtualSequencePreview(const QRectF& sceneRect);

    /**
     * @brief 根据 m_renderBackend 切换 Raster/OpenGL viewport。
     */
    void applyRenderBackend();

    /**
     * @brief 计算当前视图中心并发出 sigViewCenterSceneChanged。
     */
    void emitViewCenterChanged();

    /**
     * @brief 用户主动改变浏览位置后发出底部锚点相关通知。
     *
     * 与程序调用的同步定位区分开，可避免两个 View 相互同步形成循环。
     */
    void emitUserViewBottomAnchorChanged();

    /**
     * @brief 计算“全部内容适配当前 viewport”时所需的缩放比例。
     */
    double getFitScale() const;

    /**
     * @brief 获取指定 View 坐标下实际可交互/可见的图元列表。
     */
    QList<QGraphicsItem*> getVisualItems(QPoint viewPos);

    /* DB 图片沿主轴有序，下面的二分辅助避免滚动/坐标换算每次扫描全部 m_items。 */
    QList<TunnelSectionItem*> databaseItemsIntersecting(const QRectF& sceneRect) const;
    int databaseItemIndexAtScenePos(const QPointF& scenePos, bool clampToEdge = false) const;
    void onDatabaseItemTilesUpdated(TunnelSectionItem* item);

    /** @brief 当前绘制工具使用的几何形状。 */
    DrawShape m_curDrawShape;

public:
    // =====================================================================
    // 历史公开成员（为兼容老 TunnelViewer 保留）
    //
    // 新代码优先使用 defectManager()/cp3Manager()/... 等 accessor。
    // 不建议新业务继续直接访问这些成员，以便未来逐步收敛公开接口。
    // =====================================================================

    DefectManager* m_vecCp3Manager = nullptr;              ///< CP3 图元管理器。
    DefectManager* m_vecPlatformManager = nullptr;         ///< 站台图元管理器。
    DefectManager* m_vecChainManager = nullptr;            ///< 链相关图元管理器。
    DefectManager* m_vecSleeperManager;                     ///< 轨枕图元管理器。
    DefectManager* m_defectManager = nullptr;              ///< 病害图元管理器。
    DefectManager* m_vecTunnelLocManager = nullptr;        ///< 隧道定位图元管理器。
    DefectManager* m_vecRingInfoManager = nullptr;         ///< 环信息图元管理器。
    DefectManager* m_vecSectionManager = nullptr;          ///< 断面图元管理器。
    DefectManager* m_vecReAutoRingManager = nullptr;       ///< 自动环图元管理器。
    DefectManager* m_vecArchitraveLineManager = nullptr;   ///< 拱脚线/用户线图元管理器。

    ViewMode m_currentMode = Mode_Browse;                  ///< 当前浏览/绘制工作模式。

private:
    // =====================================================================
    // 鼠标交互状态
    // =====================================================================

    QPoint m_lastMousePos;             ///< 上一次鼠标位置，用于计算拖动增量。
    bool m_isPanning = false;          ///< 当前是否正在平移 View。

private:
    // =====================================================================
    // Scene、数据库瓦片与卷帘对比状态
    // =====================================================================

    QGraphicsScene* m_scene;                       ///< View 使用的 Scene。
    QList<TunnelSectionItem*> m_items;             ///< 当前数据库瓦片图层 Item 列表。
    QRectF m_databaseImageSceneRect;                ///< DB 图片场景范围，addLayer 时增量维护，避免 O(N²) 重算。
    LayoutOrientation m_databaseLayoutOrientation = LayoutOrientation::Vertical; ///< DB Item 实际建立布局时的方向，防止事后修改 m_orientation 破坏二分索引。
    QSet<TunnelSectionItem*> m_databaseScheduledItems; ///< 上一次处于预取/高清调度范围的 DB Item。
    QHash<TunnelSectionItem*, int> m_databaseLoadedTileCounts; ///< HUD 增量统计。
    int m_databaseTotalLoadedTiles = 0;              ///< 当前 DB 模式已加载切片总数。
    QList<BaseCurtainFrame> m_baseCurtainFrames;   ///< 卷帘基准侧帧信息。
    QList<CompareCurtainFrame> m_compareCurtainFrames; ///< 卷帘对比侧帧信息。
    CurtainCompareItem* m_curtainItem = nullptr;   ///< 卷帘覆盖绘制 Item。
    QHash<QString, CurtainPendingTile> m_curtainPendingTiles; ///< 正在加载的卷帘瓦片。
    QSet<QString> m_curtainNeededTiles;            ///< 当前视口实际需要的卷帘瓦片集合。
    QSet<int> m_curtainNeededFrames;               ///< 当前视口实际需要的卷帘帧集合。
    bool m_curtainEnabled = false;                 ///< 是否启用卷帘对比。
    bool m_isDraggingCurtain = false;              ///< 用户是否正在拖动卷帘分割线。
    CurtainOrientation m_curtainOrientation = CurtainOrientation::Vertical; ///< 卷帘方向。
    int m_curtainLinePosition = -1;                ///< 卷帘线在 viewport 内的像素位置。

    /**
     * @brief 图片名到数据库 TunnelSectionItem 的快速查找缓存。
     * @note m_items 重建后必须同步清空，避免保存已经失效的 Item 指针。
     */
    mutable QHash<QString, TunnelSectionItem*> m_imageItemCache;

    /*
     * 通用标注图层注册表。旧 m_vecXXXManager 指针全部指向这里的对应实例，
     * 因而旧接口与新 Registry 共用同一批对象和生命周期。
     */
    QMap<QString, AnnotationManager*> m_annotationManagers;

    // =====================================================================
    // 虚拟序列：普通单图与当前 Pack 共用
    // =====================================================================

    ContentMode m_contentMode = ContentMode::DatabaseTiles; ///< 当前内容模式。
    QSharedPointer<IImageSource> m_sequenceSource;          ///< 统一图像源；与旧 ISequenceFrameSource 为同一接口。
    QSharedPointer<ImageLayoutManager> m_sequenceModel;      ///< 统一布局管理器；复用原 ImageSequenceModel 算法。
    QList<SequenceChunkItem*> m_sequenceChunks;              ///< 大序列按 Chunk 分组后的 Scene Item。
    ImageCacheManager m_sequencePreviewCache;                ///< 缩略图缓存，避免高清图挤掉高速浏览需要的预览。
    ImageCacheManager m_sequenceFullCache;                   ///< 高清图缓存。
    ImageCoordinateMapper m_coordinateMapper;                ///< 虚拟序列坐标映射器，不触发图像读取。

    /**
     * @brief 正在执行的图片解码请求。
     *
     * value 保存创建该 Pending Key 时的“视口请求序号”。
     * 这样旧视口遗留的后台任务完成时，不会误删除新视口已经重新发出的同 Key 请求。
     */
    QHash<QString, int> m_sequencePendingRequests;

    QThreadPool m_sequenceDecodePool;               ///< 缩略图/高清图后台解码线程池。
    SequenceLoadOptions m_sequenceOptions;          ///< 当前序列加载和缓存参数。
    int m_sequenceGeneration = 0;                   ///< 序列代次；重新加载内容后递增，使旧任务自动失效。
    int m_sequenceRequestSerial = 0;                ///< 当前视口调度序号。

    QRectF m_sequenceScheduleAnchorVisibleRect;     ///< 上一次执行资源调度时的可视区域锚点。
    bool m_sequenceScheduleAnchorValid = false;     ///< 上述锚点是否有效。
    bool m_sequenceScheduleAnchorHighResolution = false; ///< 上一次调度是否处于高清模式。
    QPointF m_sequenceLastVisibleCenter;                 ///< 用于判断当前滚动方向，动态调整预取前后范围。
    bool m_sequenceLastVisibleCenterValid = false;       ///< 上述中心是否有效。

    QElapsedTimer m_sequenceTraceClock;             ///< 序列性能/调度跟踪计时器。
    QHash<QString, qint64> m_sequenceLastTraceMs;   ///< 各类调试日志最后输出时间，用于限频。
    qint64 m_sequenceLastViewportTraceMs = -1000;   ///< 最近一次视口调试日志时间。

    ImageDisplayAdjustments m_imageAdjustments;     ///< 当前亮度、对比度、锐化参数。
    bool m_sequenceHorizontalMirror = false;        ///< 虚拟序列是否水平镜像。
    bool m_sequenceVerticalMirror = false;          ///< 虚拟序列是否垂直镜像。

    QString DrawModelStr;                           ///< 当前绘制模式相关显示文本。
    double m_currentScale = 1.0;                    ///< 当前 View 缩放比例。

    QElapsedTimer m_perfTimer;                      ///< FPS/性能统计计时器。

    /**
     * @brief 滚动后的防抖定时器。
     *
     * 避免滚动条连续变化时每一个像素都立即触发昂贵的图片/瓦片调度。
     */
    QTimer* m_debounceTimer;

    /**
     * @brief 连续键盘导航结束判定定时器。
     *
     * 长按方向键/WASD 会产生自动重复事件；重复期间保持缩略图模式，
     * 一段时间没有新按键后再恢复高清请求，避免快速浏览时反复解码高清大图。
     */
    QTimer* m_navigationSettleTimer = nullptr;

    LayoutOrientation m_orientation;                ///< 图片序列拼接方向。
    RenderBackend m_renderBackend = RenderBackend_Raster; ///< 当前渲染后端。

    DefectDrawTool* m_defaultDrawTool = nullptr;   ///< 旧接口使用的内置几何绘图工具。
    AbstractTool* m_currentTool = nullptr;          ///< 当前正在工作的通用工具。
    bool m_ownsCurrentTool = false;                 ///< 外部工具是否由 View 负责释放。
    AnnotationDrawContext m_drawContext;            ///< 当前自定义绘图业务上下文。

    int m_scrollSpeed;                              ///< 键盘/程序导航基础滚动速度。
    bool m_enableKeyNav = true;                     ///< 是否启用内置键盘导航。
    bool m_singleFrameNavigationEnabled = false;    ///< 是否启用一次一帧导航。
    bool m_isFastScrolling = false;                 ///< 是否处于快速滚动/导航状态；此时优先使用缩略图。

    /**
     * @brief 双视图程序同步保护标志。
     *
     * 程序主动同步位置时禁止立即再次发布“用户位置变化”，避免两个 View 相互触发形成递归循环。
     */
    bool m_isSynchronizingViewPosition;

    /**
     * @brief LOD 阈值倍率。
     *
     * setLodLevel() 会调整该倍率，从而控制瓦片/高清资源加载的积极程度。
     */
    double m_lodThresholdMultiplier = 1.0;

    bool m_isDraggingDefects = false;               ///< 当前是否正在拖动已选业务图元。
    bool m_dxfMeasureEnabled = false;               ///< 是否启用 DXF 测量点拾取。
    bool m_tunnelLocationMoveEnabled = false;       ///< 是否允许移动隧道定位类图元。

    QPointF m_lastDragScenePos;                     ///< 上一次图元拖动时的 Scene 坐标。

    QGraphicsRectItem* m_exportBoxItem = nullptr;   ///< 导出区域选择框。
    int exportBoxSize = 1680;                       ///< 默认导出框边长（Scene 像素）。

protected:
    QString m_strViewName;                          ///< 当前 View 的业务名称。
};

#endif // TILEDGRAPHICSVIEW_H
