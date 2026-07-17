#ifndef VIRTUAL_IMAGE_SEQUENCE_H
#define VIRTUAL_IMAGE_SEQUENCE_H

#include "TunnelGlobal.h"

#include <QGraphicsItem>
#include <QHash>
#include <QImage>
#include <QMutex>
#include <QSharedPointer>
#include <QThreadStorage>
#include <QVector>

class PackReaderQt;
class TiledGraphicsView;

// 单帧的轻量元数据。这里只保存定位和解码所需的信息，不持有解码后的图像，
// 因此工程包含数万张图片时，模型内存仍主要由路径和名称决定。
struct SequenceFrameDescriptor
{
    QString imageName;              // 对外显示和按名称跳转使用的图片名。
    QString sourceKey;              // 数据源内部的稳定键，普通图片和 Pack 可采用不同格式。
    QString filePath;               // 普通图片路径；Pack 帧可以为空或保存虚拟路径。
    QSize imageSize;                // 原始像素尺寸，用于建立连续 Scene 坐标。
    quint64 globalIndex = 0;        // Pack 全局帧序号，普通图片通常等于列表序号。
    quint64 sourceIndex = 0;        // Pack 中实际读取帧的 sourceIndex。
    quint64 timeValue = 0;          // 采集时间值，用于保留原有时间语义。
};

// 普通图片与 Pack 的统一帧源接口。
// 元数据读取与图像解码分离，使 Scene 建模不需要预先解码所有图片。
class ISequenceFrameSource
{
public:
    virtual ~ISequenceFrameSource() {}
    // 返回数据源中的总帧数。
    virtual quint64 frameCount() const = 0;
    // 返回指定帧的轻量元数据，不执行完整图像解码。
    virtual SequenceFrameDescriptor descriptor(quint64 index) const = 0;
    // 按目标尺寸缩放解码缩略图，禁止先解码完整图像再缩小。
    virtual QImage decodeThumbnail(quint64 index, const QSize& target) = 0;
    // 解码指定帧的完整图像。
    virtual QImage decodeFullImage(quint64 index) = 0;
};

// 文件图片序列数据源，每个路径对应一帧。
class FileSequenceFrameSource : public ISequenceFrameSource
{
public:
    // knownFrameSize 有效时直接复用统一尺寸，避免逐个扫描文件头；
    // 尺寸未知或文件无效时通过 skippedPaths 返回被跳过的路径。
    static QSharedPointer<FileSequenceFrameSource> create(const QStringList& paths,
        const QSize& knownFrameSize, QStringList* skippedPaths = nullptr);

    quint64 frameCount() const override;
    SequenceFrameDescriptor descriptor(quint64 index) const override;
    QImage decodeThumbnail(quint64 index, const QSize& target) override;
    QImage decodeFullImage(quint64 index) override;

private:
    QVector<SequenceFrameDescriptor> m_frames; // 按采集/输入顺序保存帧元数据。
};

// Pack v2 图片序列数据源，通过 PackReaderQt 按 sourceIndex 解码。
class PackSequenceFrameSource : public ISequenceFrameSource
{
public:
    static QSharedPointer<PackSequenceFrameSource> create(const QString& packRoot,
        const QSize& frameSize, bool verifyOnOpen, QString* errorMessage = nullptr);
    ~PackSequenceFrameSource() override;

    quint64 frameCount() const override;
    SequenceFrameDescriptor descriptor(quint64 index) const override;
    QImage decodeThumbnail(quint64 index, const QSize& target) override;
    QImage decodeFullImage(quint64 index) override;

private:
    // 每个解码线程复用自己的 Reader，避免跨线程共享 Reader 和反复打开索引。
    PackReaderQt* readerForCurrentThread();

    QString m_packRoot;                              // PackIndex.idx 所在的 Pack 根目录。
    bool m_verifyOnOpen = false;                     // 打开时是否执行 Pack 完整性校验。
    QVector<SequenceFrameDescriptor> m_frames;       // Pack 索引转换后的帧元数据。
    QThreadStorage<PackReaderQt*> m_threadReaders;   // 解码工作线程各自持有的 Reader。
};

// 连续图片的坐标与索引模型。
// sourceIndex 表示数据源原始顺序；visualIndex 表示当前布局下从 Scene 起点到终点
// 的显示顺序。反向布局时二者不同，调用方不能混用。
class ImageSequenceModel
{
public:
    // 根据图片尺寸建立累计 Scene 坐标、正反向索引和名称哈希。
    bool build(const QSharedPointer<ISequenceFrameSource>& source, LayoutOrientation orientation);
    bool isEmpty() const { return m_frames.isEmpty(); }
    int count() const { return m_frames.size(); }
    LayoutOrientation orientation() const { return m_orientation; }
    QRectF sceneRect() const { return m_sceneRect; }
    const SequenceFrameDescriptor& descriptor(int sourceIndex) const;
    QRectF frameRect(int sourceIndex) const;                 // 按数据源序号取得帧的 Scene 矩形。
    QRectF frameRectByVisualIndex(int visualIndex) const;    // 按显示序号取得帧的 Scene 矩形。
    int sourceIndexForVisualIndex(int visualIndex) const;    // 显示序号转换为数据源序号。
    int visualIndexForSourceIndex(int sourceIndex) const;    // 数据源序号转换为显示序号。
    int sourceIndexForName(const QString& imageName) const;  // 名称或完整路径哈希查找，平均 O(1)。
    int sourceIndexAt(const QPointF& scenePoint) const;      // Scene 点定位到数据源序号，O(log N)。
    int firstVisualIndexIntersecting(const QRectF& sceneRect) const; // 可见矩形命中的首帧。
    int lastVisualIndexIntersecting(const QRectF& sceneRect) const;  // 可见矩形命中的末帧。
    qreal firstAxisStart() const { return m_visualStarts.isEmpty() ? 0.0 : m_visualStarts.first(); }
    qreal lastAxisEnd() const { return m_visualEnds.isEmpty() ? 0.0 : m_visualEnds.last(); }

private:
    QStringList aliasesFor(const SequenceFrameDescriptor& frame) const;
    int visualIndexAtAxis(qreal axisValue) const;

    QVector<SequenceFrameDescriptor> m_frames; // 下标始终是 sourceIndex。
    QVector<QRectF> m_frameRects;              // 与 m_frames 一一对应的 Scene 矩形。
    QVector<qreal> m_visualStarts;             // 主轴累计起点，按 visualIndex 升序。
    QVector<qreal> m_visualEnds;               // 主轴累计终点，供二分定位使用。
    QHash<QString, int> m_nameToSource;         // 图片名、路径别名到 sourceIndex 的哈希。
    LayoutOrientation m_orientation = LayoutOrientation::Vertical; // 主轴方向及正反布局。
    QRectF m_sceneRect;                         // 全序列在 Scene 中的总范围。
};

// 虚拟序列的分块 Scene 图元。一个图元代理一段连续帧，只在 paint() 时处理
// 当前暴露区域内的帧，避免为每张图片创建 QGraphicsItem 和信号连接。
class SequenceChunkItem : public QGraphicsItem
{
public:
    SequenceChunkItem(TiledGraphicsView* view, const QSharedPointer<ImageSequenceModel>& model,
        int firstVisualIndex, int lastVisualIndex);
    QRectF boundingRect() const override { return m_bounds; }
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    bool containsSourceIndex(int sourceIndex) const;

private:
    TiledGraphicsView* m_view = nullptr;                 // 提供缓存查询和异步解码请求。
    QSharedPointer<ImageSequenceModel> m_model;          // 共享轻量坐标模型。
    int m_firstVisualIndex = 0;                          // 本 Chunk 包含的首个显示序号。
    int m_lastVisualIndex = -1;                          // 本 Chunk 包含的末个显示序号。
    QRectF m_bounds;                                     // Chunk 在 Scene 中的合并包围框。
};

#endif
