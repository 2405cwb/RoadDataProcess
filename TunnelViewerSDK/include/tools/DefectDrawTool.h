#pragma once
#include "AbstractTool.h"
#include <QGraphicsPathItem>
#include "TunnelGlobal.h"
#include <QPen>
#include <QRectF>
#include <QtMath>
class TiledGraphicsView;

class DefectDrawTool : public AbstractTool {
public:
    // 构造时传入要画什么形状
    DefectDrawTool(DrawShape shapeType) : m_shapeType(shapeType), m_previewItem(nullptr) {}

	void ChangeShapeType(DrawShape shapeType)
	{
		m_shapeType = shapeType;

		m_points.clear();
		if (m_previewItem)
		{
			m_previewItem->setPath(QPainterPath());
		}
	}

    void handleMousePress(QPointF scenePos, Qt::MouseButton button) override {
        if (button == Qt::LeftButton) {
            // 🟢 如果是画【点】，点一下就直接结束了
            if (m_shapeType == Shape_Point) {
                QPainterPath path;
                // 用一个小圆圈代表点 (半径这里默认给个 5，可调)
                path.addEllipse(scenePos, 5.0, 5.0);

                // 触发 View 发送信号给外部
                triggerSignalAndFinish(path);
                return;
            }

            // 矩形只记录一次左键起点，后续由鼠标移动位置实时生成预览。
            if (m_shapeType == Shape_Rectangle)
            {
                m_points.clear();
                m_points.append(scenePos);
                updatePreview(scenePos);
                return;
            }

            // 斜矩形和三点矩形统一使用真正的三点法：前两点定基准边，第三点定垂直宽度。
            if ((m_shapeType == Shape_ObliqueRectangle || m_shapeType == Shape_ThreePointRectangle))
            {
                if (m_points.size() < 2)
                {
                    m_points.append(scenePos);
                    updatePreview(scenePos);
                }
                else
                {
                    finishObliqueRectangle(scenePos);
                }
                return;
            }

            // 如果是线或面，记录点并更新橡皮筋
            m_points.append(scenePos);
            updatePreview(scenePos);
        }
        else if (button == Qt::RightButton) {
            // 矩形将右键位置作为对角终点，并立即完成绘制。
            if (m_shapeType == Shape_Rectangle)
            {
                finishRectangle(scenePos);
                return;
            }

            if ((m_shapeType == Shape_ObliqueRectangle || m_shapeType == Shape_ThreePointRectangle))
            {
                cancelDrawing();
                return;
            }

            // 右键结束画线/画面
            if (m_shapeType != Shape_Point) {
                finishDrawing();
            }
        }
    }

    void handleMouseMove(QPointF scenePos) override {
        if (!m_points.isEmpty()) {
            updatePreview(scenePos); // 实时更新橡皮筋
        }
    }

    void deactivate() override {
        if (m_previewItem && m_view) {
            m_view->scene()->removeItem(m_previewItem);
            delete m_previewItem;
            m_previewItem = nullptr;
        }
        m_points.clear();
    }

	void undoLastPoint()
	{
		// 1. 如果还没点下任何点，直接无视
		if (m_points.isEmpty()) {
			return;
		}

		// 2. 踢掉最后一个确定点！
		m_points.removeLast();

		// 3. 边缘情况：如果点全删光了，彻底清空画面，回到刚按下 F3/F4 的初始状态
		if (m_points.isEmpty()) {
			if (m_previewItem) {
				m_previewItem->setPath(QPainterPath());
			}
			qDebug() << QString::fromLocal8Bit("🔙 撤销了起点，等待重新绘制...");
			return;
		}

		// 4. 重建剩下的确定路线
		QPainterPath newPath;
		newPath.moveTo(m_points.first());
		for (int i = 1; i < m_points.size(); ++i) {
			newPath.lineTo(m_points[i]);
		}

		// 5. 让橡皮筋线重新连上当前的鼠标！
		// 因为你在 Tool 里保存了 view 的指针 m_view，我们可以随时获取当前鼠标的真实位置
		QPointF currentMousePos = m_view->mapToScene(m_view->mapFromGlobal(QCursor::pos()));

		QPainterPath tempPath = newPath;
		tempPath.lineTo(currentMousePos);

		// 6. 刷新屏幕
		if (m_previewItem) {
			m_previewItem->setPath(tempPath);
		}

		qDebug() << "🔙 成功撤销上一个点，当前剩余点数:" << m_points.size();
	}

	void cancelDrawing() override {
		// 如果本来就没开始点，直接返回
		if (m_points.isEmpty()) {
			return;
		}

		// 1. 清空所有点位数组
		m_points.clear();

		// 2. 擦除屏幕上的临时预览虚线
		if (m_previewItem) {
			m_previewItem->setPath(QPainterPath());
		}
	}

private:
    void updatePreview(const QPointF& currentMousePos) {
        if (m_points.isEmpty() || !m_view) return;

        QPainterPath path;

        // 矩形使用左键起点和当前鼠标位置构造实时预览路径。
        if (m_shapeType == Shape_Rectangle)
        {
            const QRectF rectangle =
                QRectF(m_points.first(), currentMousePos).normalized();
            path.addRect(rectangle);
        }
        else if ((m_shapeType == Shape_ObliqueRectangle || m_shapeType == Shape_ThreePointRectangle))
        {
            path = createObliqueRectanglePath(currentMousePos);
        }
        else
        {
            path.moveTo(m_points.first());
            for (int i = 1; i < m_points.size(); ++i) path.lineTo(m_points[i]);
            path.lineTo(currentMousePos);

            // 多边形预览连接回起点，便于确认最终闭合范围。
            if (m_shapeType == Shape_Polygon && m_points.size() > 1) {
                path.lineTo(m_points.first());
            }
        }

        if (!m_previewItem) {
            m_previewItem = new QGraphicsPathItem();
			QPen pen(Qt::red, 1, Qt::DashLine);
			pen.setCosmetic(true);
            m_previewItem->setPen(pen);
            m_view->scene()->addItem(m_previewItem);
        }
        m_previewItem->setPath(path);
    }

    /**
     * @brief 使用左键起点和右键终点完成矩形绘制。
     * @param endPosition 右键单击时的场景坐标。
     */
    void finishRectangle(const QPointF& endPosition)
    {
        if (m_points.size() != 1)
        {
            return;
        }

        // normalized 保证从任意方向移动鼠标都能得到有效矩形。
        const QRectF rectangle =
            QRectF(m_points.first(), endPosition).normalized();
        if (rectangle.isEmpty())
        {
            cancelDrawing();
            return;
        }

        QPainterPath finalPath;
        finalPath.addRect(rectangle);
        triggerSignalAndFinish(finalPath);
    }

    /**
     * @brief 根据前两个基准点和当前宽度点构造斜矩形路径。
     * @param widthPosition 用于确定矩形有向宽度的场景坐标。
     * @return 基准边有效时返回斜矩形或基准边预览路径，否则返回空路径。
     */
    QPainterPath createObliqueRectanglePath(const QPointF& widthPosition) const
    {
        QPainterPath path;
        if (m_points.isEmpty())
        {
            return path;
        }

        path.moveTo(m_points.first());
        if (m_points.size() == 1)
        {
            path.lineTo(widthPosition);
            return path;
        }

        const QPointF firstPoint = m_points.at(0);
        const QPointF secondPoint = m_points.at(1);
        const QPointF baseVector = secondPoint - firstPoint;
        const double baseLength = qSqrt(QPointF::dotProduct(baseVector, baseVector));
        if (baseLength <= 0.000001)
        {
            return QPainterPath();
        }

        const QPointF normalVector(-baseVector.y() / baseLength, baseVector.x() / baseLength);
        const double signedWidth = QPointF::dotProduct(widthPosition - firstPoint, normalVector);
        const QPointF widthVector = normalVector * signedWidth;
        const QPointF thirdPoint = secondPoint + widthVector;
        const QPointF fourthPoint = firstPoint + widthVector;

        path.lineTo(secondPoint);
        path.lineTo(thirdPoint);
        path.lineTo(fourthPoint);
        path.closeSubpath();
        return path;
    }

    /**
     * @brief 使用第三个左键点完成斜矩形绘制。
     * @param widthPosition 第三个点的场景坐标，用于确定矩形宽度和所在方向。
     */
    void finishObliqueRectangle(const QPointF& widthPosition)
    {
        if (m_points.size() != 2)
        {
            return;
        }

        const QPointF baseVector = m_points.at(1) - m_points.at(0);
        const double baseLengthSquared = QPointF::dotProduct(baseVector, baseVector);
        if (baseLengthSquared <= 0.000001)
        {
            cancelDrawing();
            return;
        }

        const double baseLength = qSqrt(baseLengthSquared);
        const QPointF normalVector(-baseVector.y() / baseLength, baseVector.x() / baseLength);
        const double signedWidth = QPointF::dotProduct(widthPosition - m_points.at(0), normalVector);
        const QPainterPath finalPath = createObliqueRectanglePath(widthPosition);
        if (qAbs(signedWidth) <= 0.000001 || finalPath.isEmpty())
        {
            cancelDrawing();
            return;
        }

        triggerSignalAndFinish(finalPath);
    }

    void finishDrawing() {
        if (m_points.size() < 2) { deactivate(); return; }

        QPainterPath finalPath;
        finalPath.moveTo(m_points.first());
        for (int i = 1; i < m_points.size(); ++i) finalPath.lineTo(m_points[i]);

        // 🟢 如果是【面】，必须闭合路径；如果是【线】，就不闭合
        if (m_shapeType == Shape_Polygon) {
            finalPath.closeSubpath();
        }

        triggerSignalAndFinish(finalPath);
    }

    void triggerSignalAndFinish(const QPainterPath& path) {
        if (m_view) {
            // 工具不创建 Item！而是通知 View 把纯几何数据抛给上层！
            m_view->emitGeometryDrawn(m_shapeType, path);

			
        }
		if (m_previewItem)
		{
			deactivate();

		}
    }

private:
    DrawShape m_shapeType;
    QVector<QPointF> m_points;
    QGraphicsPathItem* m_previewItem;
};
