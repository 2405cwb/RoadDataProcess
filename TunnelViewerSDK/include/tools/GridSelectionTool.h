#ifndef GRID_SELECTION_TOOL_H
#define GRID_SELECTION_TOOL_H

#include <QLineF>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QVector>
#include <QVariantMap>

/* 一张图上的物理网格配置。这里不写公路、地铁这些业务词，只描述图片和真实尺寸。 */
struct PhysicalGridSpec
{
    /* 默认按 0.1m x 0.1m 建格子，调用方只要补图片和真实尺寸就能用。 */
    PhysicalGridSpec();

    /* 检查配置够不够算格子，不够就直接返回 false，避免后面除 0。 */
    bool isValid() const;

    /* 返回真正参与建格子的图片区域；没传局部区域时，就用整张图。 */
    QRectF effectiveGridArea() const;

    /* 按真实宽度算横向格子数，余下不足一个格子的边角先丢掉，和旧逻辑保持一致。 */
    int columnCount() const;

    /* 按真实高度算纵向格子数，余下不足一个格子的边角先丢掉，和旧逻辑保持一致。 */
    int rowCount() const;

    QString imageName;
    QSize imagePixelSize;
    QSizeF physicalSizeMeters;
    QRectF gridAreaPixels;
    QSizeF cellSizeMeters;
    QVariantMap attributes;
};

/* 一个被选中的小格子。业务层可以拿 row/col 或 imageRect 去继续算里程、面积和数据库字段。 */
struct GridCell
{
    /* 空格子用于容器默认构造，不代表有效选择结果。 */
    GridCell();

    /* 判断这个格子有没有基本坐标，调用方过滤异常数据时会用到。 */
    bool isValid() const;

    QString imageName;
    int row;
    int column;
    QRectF imageRect;
    QSizeF physicalSizeMeters;
    QVariantMap attributes;
};

/* 拉框选格子的命中方式。默认 FullyContained，对应旧项目 D 模式“框住的小格子”。 */
enum class GridRectHitMode
{
    FullyContained,
    CenterInside,
    Intersects
};

class GridSelectionTool
{
public:
    /* 直接根据单图像素点计算所在格子，不生成整张图所有候选格子。 */
    static GridCell cellForPoint(const PhysicalGridSpec& spec, const QPointF& point);

    /* 直接按矩形覆盖范围计算命中的格子，SDK 小框拉框模式使用。 */
    static QVector<GridCell> selectByRect(const PhysicalGridSpec& spec,
        const QRectF& rect,
        GridRectHitMode hitMode = GridRectHitMode::FullyContained);

    /* 直接按折线每段的包围范围计算命中的格子，SDK 小框轨迹和 B 折线模式使用。 */
    static QVector<GridCell> selectByPolyline(const PhysicalGridSpec& spec,
        const QVector<QPointF>& points);

    /* 根据图片尺寸和真实尺寸，生成整张图的隐藏小格子。 */
    static QVector<GridCell> cellsForImage(const PhysicalGridSpec& spec);

    /* 用连续点击形成的折线选格子；B 模式可以直接用这个方法。 */
    static QVector<GridCell> selectByPolyline(const QVector<QPointF>& points,
        const QVector<GridCell>& cells);

    /* 用单条线段选格子；多段线内部也是一段段调用它。 */
    static QVector<GridCell> selectByLine(const QLineF& line,
        const QVector<GridCell>& cells);

    /* 用一个拖拽框选格子；D 模式默认要求小格子完整落在大框里。 */
    static QVector<GridCell> selectByRect(const QRectF& rect,
        const QVector<GridCell>& cells,
        GridRectHitMode hitMode = GridRectHitMode::FullyContained);

    /* 判断线段是否碰到某个格子，端点在格子里也算命中。 */
    static bool lineTouchesCell(const QLineF& line, const QRectF& cellRect);

private:
    /* 合并选择结果时去重，同一张图同一行列只保留一份。 */
    static void appendCellIfMissing(QVector<GridCell>& result, const GridCell& cell);

    /* 判断两个格子是不是同一个网格位置。 */
    static bool isSameCell(const GridCell& left, const GridCell& right);

    static QSizeF cellPixelSize(const PhysicalGridSpec& spec);
    static bool cellAt(const PhysicalGridSpec& spec, int row, int column, GridCell& cell);
    static int columnForX(const PhysicalGridSpec& spec, qreal x);
    static int rowForY(const PhysicalGridSpec& spec, qreal y);
};

#endif // GRID_SELECTION_TOOL_H
