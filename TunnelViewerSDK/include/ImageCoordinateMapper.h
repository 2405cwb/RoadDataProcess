#pragma once

#include "VirtualImageSequence.h"
#include <QSharedPointer>

/*
 * 图片局部坐标的通用表达。
 * imageIndex 统一表示“当前显示顺序 visualIndex”：0 永远是 Scene 主轴起点处的第一张。
 * 正向布局下 visualIndex 与 sourceIndex 相同；反向布局下由 ImageSequenceModel 内部转换。
 * 这样数据库模式、虚拟序列以及 TiledViewAnchor/scrollToImagePixel 的序号语义保持一致。
 */
struct ImageCoordinate
{
    bool valid = false;
    int imageIndex = -1;
    QString imageName;
    QPointF localPos;
};

/*
 * 虚拟图片序列坐标映射器。
 * 只基于 ImageSequenceModel 做几何换算，不读取图片、不触发解码。
 */
class ImageCoordinateMapper
{
public:
    void setModel(const QSharedPointer<ImageSequenceModel>& model) { m_model = model; }
    void clear() { m_model.clear(); }
    bool isValid() const { return !m_model.isNull() && !m_model->isEmpty(); }

    bool imageToScene(int imageIndex, const QPointF& localPos, QPointF& scenePos) const
    {
        if (!isValid() || imageIndex < 0 || imageIndex >= m_model->count()) return false;
        const int sourceIndex = m_model->sourceIndexForVisualIndex(imageIndex);
        if (sourceIndex < 0) return false;
        const QRectF rect = m_model->frameRect(sourceIndex);
        scenePos = rect.topLeft() + localPos;
        return true;
    }

    bool imageNameToScene(const QString& imageName, const QPointF& localPos, QPointF& scenePos) const
    {
        if (!isValid()) return false;
        const int sourceIndex = m_model->sourceIndexForName(imageName);
        if (sourceIndex < 0) return false;
        const QRectF rect = m_model->frameRect(sourceIndex);
        scenePos = rect.topLeft() + localPos;
        return true;
    }

    bool sceneToImage(const QPointF& scenePos, ImageCoordinate& coordinate) const
    {
        coordinate = ImageCoordinate();
        if (!isValid()) return false;
        const int sourceIndex = m_model->sourceIndexAt(scenePos);
        if (sourceIndex < 0) return false;
        const QRectF rect = m_model->frameRect(sourceIndex);
        const int visualIndex = m_model->visualIndexForSourceIndex(sourceIndex);
        if (visualIndex < 0) return false;
        coordinate.valid = true;
        coordinate.imageIndex = visualIndex;
        coordinate.imageName = m_model->descriptor(sourceIndex).imageName;
        coordinate.localPos = scenePos - rect.topLeft();
        return true;
    }

private:
    QSharedPointer<ImageSequenceModel> m_model;
};
