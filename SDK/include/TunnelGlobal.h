#ifndef TUNNEL_GLOBAL_H
#define TUNNEL_GLOBAL_H

#include <QBrush>
#include <QPainterPath>
#include <QPen>
#include <QString>
#include <QtGlobal>
#include <QVariantMap>

/* 视图模式 老项目继续用这些名字 */ enum ViewMode {
    Mode_Browse,
    Mode_Draw,
    Mode_ExportBox
};

/* 图片序列拼接方向 先覆盖常见正向和反向 */ enum class LayoutOrientation {
    Vertical,
    Horizontal,
    VerticalReverse,
    HorizontalReverse
};

struct PackRouteOptions {
    LayoutOrientation orientation = LayoutOrientation::Vertical;
    int scrollSpeed = 50;
    int virtualTileSize = 0;
    bool hMirrored = false;
    bool vMirrored = false;
    bool verifyOnOpen = false;
};

struct PackRouteFrameInfo {
    QString imageName;
    quint64 globalIndex = 0;
    quint64 sourceIndex = 0;
    quint64 timeValue = 0;
    int width = 0;
    int height = 0;
};

/* 判断当前布局是不是纵向 */ inline bool isVerticalLayout(LayoutOrientation orientation)
{
    return orientation == LayoutOrientation::Vertical
        || orientation == LayoutOrientation::VerticalReverse;
}

/* 判断当前布局是不是反向 */ inline bool isReverseLayout(LayoutOrientation orientation)
{
    return orientation == LayoutOrientation::VerticalReverse
        || orientation == LayoutOrientation::HorizontalReverse;
}

/* SDK 核心只认基础几何 业务名称放到外层解释 */ enum DrawShape {
    Shape_Point,
    Shape_Line,
    Shape_Polygon
};

/* 旧业务元素类型 先保留兼容 新项目建议用 category 或 attributes */ enum ElementType {
    Type_Cp3,
    Type_Chain,
    Type_Disease,
    Type_Ring,
    Type_Section,
    Type_Platform,
    Type_AUTORING,
    Type_CustomOverlay
};

/* 一段图片的基础信息 dbFilePath 现在也可以放普通图片路径 */ struct DbImageInfo {
    QString dbFilePath;
    QString originalName;
    int width = 0;
    int height = 0;
    int tileSize = 2048;
};

/* 标注类型配置 只描述形状和样式 不绑定具体行业 */ struct AnnotationTypeConfig {
    /* 没传配置时 默认按黄色线处理 */ AnnotationTypeConfig()
        : defaultShape(Shape_Line), typeCode(0), defectCode(0) {}

    /* 常用构造 业务编码 名称 默认形状 样式 */ AnnotationTypeConfig(int code, const QString& name, const DrawShape shape,
        const QPen& annotationPen, const QBrush& annotationBrush)
        : defaultShape(shape), typeCode(code), defectCode(code), typeName(name),
          pen(annotationPen), brush(annotationBrush) {}

    /* 兼容旧调用 多出来的 remark 放进 attributes */ AnnotationTypeConfig(int code, const QString& name, const DrawShape shape,
        const QPen& annotationPen, const QBrush& annotationBrush, const QString& remark)
        : defaultShape(shape), typeCode(code), defectCode(code), typeName(name),
          pen(annotationPen), brush(annotationBrush) {
        attributes.insert("remark", remark);
    }

    /* 兼容旧调用 没传形状时默认线 */ AnnotationTypeConfig(int code, const QString& name,
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

/* 老名字保留 新代码可以用 AnnotationTypeConfig */ using DefectTypeConfig = AnnotationTypeConfig;

/* 一条标注的纯数据 业务字段统一放 attributes */ struct AnnotationData {
    int uuid = 0;
    QString name;
    DrawShape type = Shape_Line;
    QPainterPath shape;
    int typeCode = 0;
    int defectCode = 0;
    QString category;
    QVariantMap attributes;
};

/* 老名字保留 新代码可以用 AnnotationData */ using DefectData = AnnotationData;

/* 导出质量 缩略图快 高清图准 */ enum ExportQuality {
    Export_Thumbnail,
    Export_HighRes
};

#endif // TUNNEL_GLOBAL_H
