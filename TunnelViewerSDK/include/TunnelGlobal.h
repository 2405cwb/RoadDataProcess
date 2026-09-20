#ifndef TUNNEL_GLOBAL_H
#define TUNNEL_GLOBAL_H

#include <QBrush>
#include <QPainterPath>
#include <QPen>
#include <QString>
#include <QtGlobal>
#include <QVariantMap>
#include <QSize>
#include <QMetaType>

/*
 * 统一 SDK 的公共枚举全部显式赋值。
 * 关键原因：三个历史分支曾在 ElementType=7/8 上赋予不同含义，如果继续使用自动递增，
 * 后续在中间插入枚举会让数据库/配置文件中已经保存的整数发生语义漂移。
 */
enum ViewMode {
    Mode_Browse = 0,
    Mode_Draw = 1,
    Mode_ExportBox = 2,
    Mode_Move = 3
};

enum class LayoutOrientation {
    Vertical = 0,
    Horizontal = 1,
    VerticalReverse = 2,
    HorizontalReverse = 3
};

enum class CurtainOrientation {
    Vertical = 0,
    Horizontal = 1
};

struct PackRouteOptions {
    LayoutOrientation orientation = LayoutOrientation::Vertical;
    int scrollSpeed = 50;
    bool hMirrored = false;
    bool vMirrored = false;
    bool verifyOnOpen = false;
};

struct SequenceLoadOptions {
    QSize knownFrameSize;
    int chunkFrameCount = 512;
    int thumbnailMaxEdge = 1024;
    qint64 decodedCacheBytes = 384LL * 1024LL * 1024LL;
    int maxDecodeJobs = 2;
    int prefetchForwardScreens = 1;
    int prefetchBackwardScreens = 1;
};

enum class ContentMode {
    DatabaseTiles = 0,
    VirtualSequence = 1
};

struct PackRouteFrameInfo {
    QString imageName;
    quint64 globalIndex = 0;
    quint64 sourceIndex = 0;
    quint64 timeValue = 0;
    int width = 0;
    int height = 0;
};

inline bool isVerticalLayout(LayoutOrientation orientation)
{
    return orientation == LayoutOrientation::Vertical
        || orientation == LayoutOrientation::VerticalReverse;
}

inline bool isReverseLayout(LayoutOrientation orientation)
{
    return orientation == LayoutOrientation::VerticalReverse
        || orientation == LayoutOrientation::HorizontalReverse;
}

enum DrawShape {
    Shape_Point = 0,
    Shape_Line = 1,
    Shape_Polygon = 2,
    Shape_Rectangle = 3,
    Shape_ObliqueRectangle = 4,
    Shape_ThreePointRectangle = 5
};

enum ElementType {
    Type_Cp3 = 0,
    Type_Chain = 1,
    Type_Disease = 2,
    Type_Ring = 3,
    Type_Section = 4,
    Type_Platform = 5,
    Type_AUTORING = 6,

    // 新主干历史值：必须继续固定为 7/8，避免现有新软件数据失配。
    Type_CustomOverlay = 7,
    Type_Sleeper = 8,

    // 老 TunnelViewer 分支原来占用了 7/8。统一版改为独立值，读取老数据时必须显式映射。
    Type_SPRINGING_LOC = 9,
    Type_UserLine = 10
};

/*
 * 持久化枚举兼容策略。
 * 注意：历史值 7/8 本身存在歧义，SDK 无法仅凭一个整数自动判断它来自哪个软件。
 * 读取老 TunnelViewer 数据时使用 TunnelViewerLegacy；新数据统一使用 UnifiedModern。
 */
enum class ElementTypePersistenceProfile {
    UnifiedModern = 0,
    TunnelViewerLegacy = 1
};

inline ElementType elementTypeFromPersistedValue(int value,
    ElementTypePersistenceProfile profile = ElementTypePersistenceProfile::UnifiedModern)
{
    if (profile == ElementTypePersistenceProfile::TunnelViewerLegacy) {
        if (value == 7) return Type_SPRINGING_LOC;
        if (value == 8) return Type_UserLine;
    }
    switch (value) {
    case Type_Cp3: return Type_Cp3;
    case Type_Chain: return Type_Chain;
    case Type_Disease: return Type_Disease;
    case Type_Ring: return Type_Ring;
    case Type_Section: return Type_Section;
    case Type_Platform: return Type_Platform;
    case Type_AUTORING: return Type_AUTORING;
    case Type_CustomOverlay: return Type_CustomOverlay;
    case Type_Sleeper: return Type_Sleeper;
    case Type_SPRINGING_LOC: return Type_SPRINGING_LOC;
    case Type_UserLine: return Type_UserLine;
    default: return Type_Disease;
    }
}

inline int elementTypeToPersistedValue(ElementType type,
    ElementTypePersistenceProfile profile = ElementTypePersistenceProfile::UnifiedModern)
{
    if (profile == ElementTypePersistenceProfile::TunnelViewerLegacy) {
        if (type == Type_SPRINGING_LOC) return 7;
        if (type == Type_UserLine) return 8;
    }
    return static_cast<int>(type);
}

/*
 * 一段图片/数据库的基础信息。
 * realStartMileage/realEndMileage 来自旧 TunnelViewer 的卷帘对比功能；普通图片、Pack 模式可忽略。
 */
struct DbImageInfo {
    QString dbFilePath;
    QString originalName;
    int width = 0;
    int height = 0;
    int tileSize = 2048;
    double realStartMileage = 0.0;
    double realEndMileage = 0.0;
    bool hasRealMileage = false;
};

struct AnnotationTypeConfig {
    AnnotationTypeConfig()
        : defaultShape(Shape_Line), typeCode(0), defectCode(0) {}

    AnnotationTypeConfig(int code, const QString& name, const DrawShape shape,
        const QPen& annotationPen, const QBrush& annotationBrush)
        : defaultShape(shape), typeCode(code), defectCode(code), typeName(name),
          pen(annotationPen), brush(annotationBrush) {}

    AnnotationTypeConfig(int code, const QString& name, const DrawShape shape,
        const QPen& annotationPen, const QBrush& annotationBrush, const QString& remark)
        : defaultShape(shape), typeCode(code), defectCode(code), typeName(name),
          pen(annotationPen), brush(annotationBrush) {
        attributes.insert("remark", remark);
    }

    AnnotationTypeConfig(int code, const QString& name,
        const QPen& annotationPen, const QBrush& annotationBrush)
        : defaultShape(Shape_Line), typeCode(code), defectCode(code), typeName(name),
          pen(annotationPen), brush(annotationBrush) {}

    DrawShape defaultShape;
    int typeCode;
    int defectCode;
    QString typeKey;
    QString typeName;
    QPen pen;
    QBrush brush;
    QVariantMap attributes;
};

using DefectTypeConfig = AnnotationTypeConfig;

/*
 * 标注身份解析结果。
 * 只描述“属于哪一层/哪一种类型”，不改变旧 AnnotationData 的字段布局。
 */
struct AnnotationIdentity {
    int formatVersion = 1;
    QString layerKey;
    QString typeKey;
    ElementType legacyElementType = Type_Disease;
    int typeCode = 0;
};

/*
 * 标注图层运行状态。
 * visible/locked/selectable/exportable 都是运行期状态，不要求写入历史业务数据。
 * hasZValue=false 时保持图元原有 Z 值，避免升级后覆盖旧业务自行设置的层级。
 */
struct AnnotationLayerState {
    bool visible = true;
    bool locked = false;
    bool selectable = true;
    bool exportable = true;
    bool hasZValue = false;
    qreal zValue = 0.0;
};

/*
 * 外部绘图工具的业务上下文。
 * 老 sigGeometryDrawn 仍只传几何；新业务可通过该上下文携带 layer/type/默认属性。
 */
struct AnnotationDrawContext {
    QString toolKey;
    QString layerKey;
    QString typeKey;
    QVariantMap attributes;

    bool isEmpty() const {
        return toolKey.isEmpty() && layerKey.isEmpty() && typeKey.isEmpty() && attributes.isEmpty();
    }
};
Q_DECLARE_METATYPE(AnnotationDrawContext)

/*
 * 通用标注纯数据。
 *
 * 兼容原则：这个结构体严格保持旧字段的名称、顺序和数量不变。
 * 第一、二阶段新增的 SDK 元数据统一写入既有 attributes，避免改变 DefectData
 * 的 C++ 对象布局；这样既兼容旧文件，也尽量降低旧 DLL/调用方的 ABI 风险。
 */
struct AnnotationData {
    int uuid = 0;
    QString name;
    DrawShape type = Shape_Line;
    QPainterPath shape;
    int typeCode = 0;
    int defectCode = 0;
    QString category;
    QVariantMap attributes;
};

/*
 * 旧 ElementType 到新通用 key 的默认映射。
 * 这里只做兼容兜底，不把这些业务名作为新 SDK 的强制类型系统。
 */
inline QString annotationTypeKeyFromLegacyElement(ElementType type)
{
    switch (type) {
    case Type_Cp3: return QStringLiteral("legacy.cp3");
    case Type_Chain: return QStringLiteral("legacy.chain");
    case Type_Disease: return QStringLiteral("legacy.disease");
    case Type_Ring: return QStringLiteral("legacy.ring");
    case Type_Section: return QStringLiteral("legacy.section");
    case Type_Platform: return QStringLiteral("legacy.platform");
    case Type_AUTORING: return QStringLiteral("legacy.autoring");
    case Type_CustomOverlay: return QStringLiteral("legacy.custom_overlay");
    case Type_Sleeper: return QStringLiteral("legacy.sleeper");
    case Type_SPRINGING_LOC: return QStringLiteral("legacy.springing_loc");
    case Type_UserLine: return QStringLiteral("legacy.user_line");
    default: return QStringLiteral("legacy.disease");
    }
}

inline QString annotationLayerKeyFromLegacyElement(ElementType type)
{
    switch (type) {
    case Type_Cp3: return QStringLiteral("cp3");
    case Type_Chain: return QStringLiteral("chain");
    case Type_Disease: return QStringLiteral("disease");
    case Type_Ring: return QStringLiteral("ring");
    case Type_Section: return QStringLiteral("section");
    case Type_Platform: return QStringLiteral("platform");
    case Type_AUTORING: return QStringLiteral("auto_ring");
    case Type_CustomOverlay: return QStringLiteral("custom_overlay");
    case Type_Sleeper: return QStringLiteral("sleeper");
    case Type_SPRINGING_LOC: return QStringLiteral("tunnel_location");
    case Type_UserLine: return QStringLiteral("user_line");
    default: return QStringLiteral("disease");
    }
}

/* 标注行为属性 key，集中定义后避免不同业务工程各写一套字符串。 */
namespace AnnotationAttributeKey {
    static const char* const Locked = "locked";
    static const char* const Movable = "movable";
    static const char* const Selectable = "selectable";
    static const char* const RubberBandSelectable = "rubberBandSelectable";
    static const char* const MoveAxis = "moveAxis";
    static const char* const ShowLabel = "showLabel";
    static const char* const Exportable = "exportable";
    static const char* const HitTolerance = "hitTolerance";
    static const char* const LabelMinLod = "labelMinLod";
    static const char* const InteractiveInBrowseMode = "interactiveInBrowseMode";
    static const char* const InteractiveInMoveMode = "interactiveInMoveMode";
    static const char* const TypeKey = "typeKey";
    static const char* const LayerKey = "layerKey";

    // SDK 内部兼容元数据。放在 attributes 中，避免修改 AnnotationData 二进制布局。
    static const char* const FormatVersion = "__sdk.formatVersion";
    static const char* const LegacyElementType = "__sdk.legacyElementType";
}

using DefectData = AnnotationData;

enum ExportQuality {
    Export_Thumbnail = 0,
    Export_HighRes = 1
};

#endif // TUNNEL_GLOBAL_H
