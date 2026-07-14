#include "tools/GridSelectionTool.h"

#include <QtGlobal>
#include <QtMath>

PhysicalGridSpec::PhysicalGridSpec()
    : cellSizeMeters(0.1, 0.1)
{
}

bool PhysicalGridSpec::isValid() const
{
    return imagePixelSize.width() > 0
        && imagePixelSize.height() > 0
        && physicalSizeMeters.width() > 0.0
        && physicalSizeMeters.height() > 0.0
        && cellSizeMeters.width() > 0.0
        && cellSizeMeters.height() > 0.0
        && effectiveGridArea().width() > 0.0
        && effectiveGridArea().height() > 0.0
        && columnCount() > 0
        && rowCount() > 0;
}

QRectF PhysicalGridSpec::effectiveGridArea() const
{
    if (gridAreaPixels.width() > 0.0 && gridAreaPixels.height() > 0.0) {
        return gridAreaPixels.normalized();
    }

    return QRectF(0.0, 0.0, imagePixelSize.width(), imagePixelSize.height());
}

int PhysicalGridSpec::columnCount() const
{
    if (physicalSizeMeters.width() <= 0.0 || cellSizeMeters.width() <= 0.0) {
        return 0;
    }

    return qMax(0, static_cast<int>(qFloor(physicalSizeMeters.width() / cellSizeMeters.width())));
}

int PhysicalGridSpec::rowCount() const
{
    if (physicalSizeMeters.height() <= 0.0 || cellSizeMeters.height() <= 0.0) {
        return 0;
    }

    return qMax(0, static_cast<int>(qFloor(physicalSizeMeters.height() / cellSizeMeters.height())));
}

GridCell::GridCell()
    : row(-1),
      column(-1)
{
}

bool GridCell::isValid() const
{
    return row >= 0
        && column >= 0
        && imageRect.width() > 0.0
        && imageRect.height() > 0.0;
}

GridCell GridSelectionTool::cellForPoint(const PhysicalGridSpec& spec, const QPointF& point)
{
    GridCell cell;
    if (!spec.isValid()) {
        return cell;
    }

    const int column = columnForX(spec, point.x());
    const int row = rowForY(spec, point.y());
    if (row < 0 || column < 0) {
        return cell;
    }

    cellAt(spec, row, column, cell);
    return cell;
}

QVector<GridCell> GridSelectionTool::selectByRect(const PhysicalGridSpec& spec,
    const QRectF& rect,
    GridRectHitMode hitMode)
{
    QVector<GridCell> result;
    if (!spec.isValid() || rect.isEmpty()) {
        return result;
    }

    const QRectF normalizedRect = rect.normalized().intersected(spec.effectiveGridArea());
    if (normalizedRect.isEmpty()) {
        return result;
    }

    const int startColumn = columnForX(spec, normalizedRect.left());
    const int endColumn = columnForX(spec, qMax(normalizedRect.left(), normalizedRect.right() - 0.001));
    const int startRow = rowForY(spec, normalizedRect.top());
    const int endRow = rowForY(spec, qMax(normalizedRect.top(), normalizedRect.bottom() - 0.001));
    if (startColumn < 0 || endColumn < 0 || startRow < 0 || endRow < 0) {
        return result;
    }

    for (int row = startRow; row <= endRow; ++row) {
        for (int column = startColumn; column <= endColumn; ++column) {
            GridCell cell;
            if (!cellAt(spec, row, column, cell)) {
                continue;
            }

            bool selected = false;
            if (hitMode == GridRectHitMode::FullyContained) {
                selected = normalizedRect.contains(cell.imageRect);
            }
            else if (hitMode == GridRectHitMode::CenterInside) {
                selected = normalizedRect.contains(cell.imageRect.center());
            }
            else {
                selected = normalizedRect.intersects(cell.imageRect);
            }

            if (selected) {
                appendCellIfMissing(result, cell);
            }
        }
    }

    return result;
}

QVector<GridCell> GridSelectionTool::selectByPolyline(const PhysicalGridSpec& spec,
    const QVector<QPointF>& points)
{
    QVector<GridCell> result;
    if (!spec.isValid() || points.size() < 2) {
        return result;
    }

    auto appendSupercoverLine = [&spec, &result](const QPointF& start, const QPointF& end)
    {
        const QRectF gridArea = spec.effectiveGridArea();
        const QSizeF pixelSize = cellPixelSize(spec);
        if (pixelSize.width() <= 0.0 || pixelSize.height() <= 0.0) {
            return;
        }

        GridCell startCell = cellForPoint(spec, start);
        GridCell endCell = cellForPoint(spec, end);
        if (!startCell.isValid() || !endCell.isValid()) {
            const QRectF lineBounds(start, end);
            const QRectF searchRect = lineBounds.normalized().adjusted(
                -pixelSize.width(), -pixelSize.height(),
                pixelSize.width(), pixelSize.height()).intersected(gridArea);
            if (searchRect.isEmpty()) {
                return;
            }

            const int startColumn = columnForX(spec, searchRect.left());
            const int endColumn = columnForX(spec, qMax(searchRect.left(), searchRect.right() - 0.001));
            const int startRow = rowForY(spec, searchRect.top());
            const int endRow = rowForY(spec, qMax(searchRect.top(), searchRect.bottom() - 0.001));
            if (startColumn < 0 || endColumn < 0 || startRow < 0 || endRow < 0) {
                return;
            }

            const QLineF line(start, end);
            for (int row = startRow; row <= endRow; ++row) {
                for (int column = startColumn; column <= endColumn; ++column) {
                    GridCell cell;
                    if (cellAt(spec, row, column, cell) && lineTouchesCell(line, cell.imageRect)) {
                        appendCellIfMissing(result, cell);
                    }
                }
            }
            return;
        }

        int row = startCell.row;
        int column = startCell.column;
        const int endRow = endCell.row;
        const int endColumn = endCell.column;
        GridCell cell;
        if (cellAt(spec, row, column, cell)) {
            appendCellIfMissing(result, cell);
        }

        const qreal dx = end.x() - start.x();
        const qreal dy = end.y() - start.y();
        const int stepColumn = dx > 0.0 ? 1 : (dx < 0.0 ? -1 : 0);
        const int stepRow = dy > 0.0 ? 1 : (dy < 0.0 ? -1 : 0);
        const qreal inf = 1.0e100;

        const qreal nextVertical = stepColumn > 0
            ? gridArea.left() + (column + 1) * pixelSize.width()
            : gridArea.left() + column * pixelSize.width();
        const qreal nextHorizontal = stepRow > 0
            ? gridArea.top() + (row + 1) * pixelSize.height()
            : gridArea.top() + row * pixelSize.height();

        qreal tMaxX = stepColumn == 0 ? inf : (nextVertical - start.x()) / dx;
        qreal tMaxY = stepRow == 0 ? inf : (nextHorizontal - start.y()) / dy;
        const qreal tDeltaX = stepColumn == 0 ? inf : pixelSize.width() / qAbs(dx);
        const qreal tDeltaY = stepRow == 0 ? inf : pixelSize.height() / qAbs(dy);

        int guard = (spec.rowCount() + spec.columnCount()) * 4 + 16;
        while ((row != endRow || column != endColumn) && guard-- > 0) {
            if (qAbs(tMaxX - tMaxY) <= 1.0e-9) {
                const int nextColumn = column + stepColumn;
                const int nextRow = row + stepRow;

                // 线段穿过格子角点时，把横向、纵向和对角三个格子都收进来，避免斜线漏格。
                if (stepColumn != 0 && cellAt(spec, row, nextColumn, cell)) {
                    appendCellIfMissing(result, cell);
                }
                if (stepRow != 0 && cellAt(spec, nextRow, column, cell)) {
                    appendCellIfMissing(result, cell);
                }
                column = nextColumn;
                row = nextRow;
                tMaxX += tDeltaX;
                tMaxY += tDeltaY;
            }
            else if (tMaxX < tMaxY) {
                column += stepColumn;
                tMaxX += tDeltaX;
            }
            else {
                row += stepRow;
                tMaxY += tDeltaY;
            }

            if (cellAt(spec, row, column, cell)) {
                appendCellIfMissing(result, cell);
            }
            else {
                break;
            }
        }
    };

    for (int index = 1; index < points.size(); ++index) {
        appendSupercoverLine(points[index - 1], points[index]);
    }

    return result;
}

QVector<GridCell> GridSelectionTool::cellsForImage(const PhysicalGridSpec& spec)
{
    QVector<GridCell> cells;
    if (!spec.isValid()) {
        return cells;
    }

    const QRectF gridArea = spec.effectiveGridArea();
    const int columns = spec.columnCount();
    const int rows = spec.rowCount();
    const qreal meterToPixelX = gridArea.width() / spec.physicalSizeMeters.width();
    const qreal meterToPixelY = gridArea.height() / spec.physicalSizeMeters.height();
    const qreal cellPixelWidth = spec.cellSizeMeters.width() * meterToPixelX;
    const qreal cellPixelHeight = spec.cellSizeMeters.height() * meterToPixelY;

    cells.reserve(columns * rows);
    for (int row = 0; row < rows; ++row) {
        const qreal top = gridArea.top() + row * cellPixelHeight;
        const qreal bottom = gridArea.top() + (row + 1) * cellPixelHeight;

        for (int column = 0; column < columns; ++column) {
            const qreal left = gridArea.left() + column * cellPixelWidth;
            const qreal right = gridArea.left() + (column + 1) * cellPixelWidth;

            GridCell cell;
            cell.imageName = spec.imageName;
            cell.row = row;
            cell.column = column;
            cell.imageRect = QRectF(QPointF(left, top), QPointF(right, bottom)).normalized();
            cell.physicalSizeMeters = spec.cellSizeMeters;
            cell.attributes = spec.attributes;
            cells.append(cell);
        }
    }

    return cells;
}

QVector<GridCell> GridSelectionTool::selectByPolyline(const QVector<QPointF>& points,
    const QVector<GridCell>& cells)
{
    QVector<GridCell> result;
    if (points.size() < 2 || cells.isEmpty()) {
        return result;
    }

    for (int index = 1; index < points.size(); ++index) {
        const QVector<GridCell> segmentCells = selectByLine(QLineF(points[index - 1], points[index]), cells);
        for (const GridCell& cell : segmentCells) {
            appendCellIfMissing(result, cell);
        }
    }

    return result;
}

QVector<GridCell> GridSelectionTool::selectByLine(const QLineF& line,
    const QVector<GridCell>& cells)
{
    QVector<GridCell> result;
    for (const GridCell& cell : cells) {
        if (cell.isValid() && lineTouchesCell(line, cell.imageRect)) {
            appendCellIfMissing(result, cell);
        }
    }

    return result;
}

QVector<GridCell> GridSelectionTool::selectByRect(const QRectF& rect,
    const QVector<GridCell>& cells,
    GridRectHitMode hitMode)
{
    QVector<GridCell> result;
    if (rect.isEmpty() || cells.isEmpty()) {
        return result;
    }

    const QRectF normalizedRect = rect.normalized();
    for (const GridCell& cell : cells) {
        if (!cell.isValid()) {
            continue;
        }

        bool selected = false;
        if (hitMode == GridRectHitMode::FullyContained) {
            selected = normalizedRect.contains(cell.imageRect);
        }
        else if (hitMode == GridRectHitMode::CenterInside) {
            selected = normalizedRect.contains(cell.imageRect.center());
        }
        else {
            selected = normalizedRect.intersects(cell.imageRect);
        }

        if (selected) {
            appendCellIfMissing(result, cell);
        }
    }

    return result;
}

bool GridSelectionTool::lineTouchesCell(const QLineF& line, const QRectF& cellRect)
{
    const QRectF normalizedRect = cellRect.normalized();
    if (normalizedRect.contains(line.p1()) || normalizedRect.contains(line.p2())) {
        return true;
    }

    const QLineF edges[] = {
        QLineF(normalizedRect.topLeft(), normalizedRect.topRight()),
        QLineF(normalizedRect.topRight(), normalizedRect.bottomRight()),
        QLineF(normalizedRect.bottomRight(), normalizedRect.bottomLeft()),
        QLineF(normalizedRect.bottomLeft(), normalizedRect.topLeft())
    };

    for (const QLineF& edge : edges) {
        if (line.intersect(edge, nullptr) == QLineF::BoundedIntersection) {
            return true;
        }
    }

    return false;
}

void GridSelectionTool::appendCellIfMissing(QVector<GridCell>& result, const GridCell& cell)
{
    for (const GridCell& existing : result) {
        if (isSameCell(existing, cell)) {
            return;
        }
    }

    result.append(cell);
}

bool GridSelectionTool::isSameCell(const GridCell& left, const GridCell& right)
{
    return left.imageName == right.imageName
        && left.row == right.row
        && left.column == right.column;
}

QSizeF GridSelectionTool::cellPixelSize(const PhysicalGridSpec& spec)
{
    if (!spec.isValid()) {
        return QSizeF();
    }

    const QRectF gridArea = spec.effectiveGridArea();
    return QSizeF(
        spec.cellSizeMeters.width() * gridArea.width() / spec.physicalSizeMeters.width(),
        spec.cellSizeMeters.height() * gridArea.height() / spec.physicalSizeMeters.height());
}

bool GridSelectionTool::cellAt(const PhysicalGridSpec& spec, int row, int column, GridCell& cell)
{
    cell = GridCell();
    if (!spec.isValid() || row < 0 || column < 0 ||
        row >= spec.rowCount() || column >= spec.columnCount()) {
        return false;
    }

    const QRectF gridArea = spec.effectiveGridArea();
    const QSizeF pixelSize = cellPixelSize(spec);
    if (pixelSize.width() <= 0.0 || pixelSize.height() <= 0.0) {
        return false;
    }

    const qreal left = gridArea.left() + column * pixelSize.width();
    const qreal top = gridArea.top() + row * pixelSize.height();
    cell.imageName = spec.imageName;
    cell.row = row;
    cell.column = column;
    cell.imageRect = QRectF(QPointF(left, top),
        QPointF(left + pixelSize.width(), top + pixelSize.height())).normalized();
    cell.physicalSizeMeters = spec.cellSizeMeters;
    cell.attributes = spec.attributes;
    return cell.isValid();
}

int GridSelectionTool::columnForX(const PhysicalGridSpec& spec, qreal x)
{
    const QSizeF pixelSize = cellPixelSize(spec);
    const QRectF gridArea = spec.effectiveGridArea();
    if (pixelSize.width() <= 0.0 || x < gridArea.left() || x > gridArea.right()) {
        return -1;
    }

    const int column = static_cast<int>(qFloor((x - gridArea.left()) / pixelSize.width()));
    return qBound(0, column, qMax(0, spec.columnCount() - 1));
}

int GridSelectionTool::rowForY(const PhysicalGridSpec& spec, qreal y)
{
    const QSizeF pixelSize = cellPixelSize(spec);
    const QRectF gridArea = spec.effectiveGridArea();
    if (pixelSize.height() <= 0.0 || y < gridArea.top() || y > gridArea.bottom()) {
        return -1;
    }

    const int row = static_cast<int>(qFloor((y - gridArea.top()) / pixelSize.height()));
    return qBound(0, row, qMax(0, spec.rowCount() - 1));
}
