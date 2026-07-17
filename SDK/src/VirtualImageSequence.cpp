#include "VirtualImageSequence.h"
#include "TiledGraphicsView.h"

#include "PackReaderQt.h"

#include <QBuffer>
#include <QFileInfo>
#include <QImageReader>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace {
QImage decodeScaledFile(const QString& path, const QSize& target)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    if (target.isValid()) {
        QSize scaled = reader.size();
        scaled.scale(target, Qt::KeepAspectRatio);
        if (scaled.isValid()) reader.setScaledSize(scaled);
    }
    return reader.read();
}

QImage decodeScaledBytes(const QByteArray& bytes, const QSize& target)
{
    QBuffer buffer;
    buffer.setData(bytes);
    if (!buffer.open(QIODevice::ReadOnly)) return QImage();
    QImageReader reader(&buffer, "JPG");
    if (target.isValid()) {
        QSize scaled = reader.size();
        scaled.scale(target, Qt::KeepAspectRatio);
        if (scaled.isValid()) reader.setScaledSize(scaled);
    }
    return reader.read();
}
}

QSharedPointer<FileSequenceFrameSource> FileSequenceFrameSource::create(
    const QStringList& paths, const QSize& knownFrameSize, QStringList* skippedPaths)
{
    QSharedPointer<FileSequenceFrameSource> result(new FileSequenceFrameSource());
    result->m_frames.reserve(paths.size());
    for (int i = 0; i < paths.size(); ++i) {
        const QFileInfo info(paths.at(i));
        if (!info.exists() || !info.isFile()) {
            if (skippedPaths) skippedPaths->append(paths.at(i));
            continue;
        }
        QSize imageSize = knownFrameSize;
        if (!imageSize.isValid()) {
            QImageReader reader(info.absoluteFilePath());
            imageSize = reader.size();
        }
        if (!imageSize.isValid()) {
            if (skippedPaths) skippedPaths->append(paths.at(i));
            continue;
        }
        SequenceFrameDescriptor frame;
        frame.imageName = info.completeBaseName();
        frame.filePath = info.absoluteFilePath();
        frame.sourceKey = frame.filePath;
        frame.imageSize = imageSize;
        frame.globalIndex = static_cast<quint64>(i);
        frame.sourceIndex = static_cast<quint64>(i);
        result->m_frames.append(frame);
    }
    return result;
}

quint64 FileSequenceFrameSource::frameCount() const { return static_cast<quint64>(m_frames.size()); }
SequenceFrameDescriptor FileSequenceFrameSource::descriptor(quint64 index) const
{
    return index < static_cast<quint64>(m_frames.size()) ? m_frames.at(static_cast<int>(index)) : SequenceFrameDescriptor();
}
QImage FileSequenceFrameSource::decodeThumbnail(quint64 index, const QSize& target)
{
    const SequenceFrameDescriptor frame = descriptor(index);
    return frame.filePath.isEmpty() ? QImage() : decodeScaledFile(frame.filePath, target);
}
QImage FileSequenceFrameSource::decodeFullImage(quint64 index)
{
    const SequenceFrameDescriptor frame = descriptor(index);
    return frame.filePath.isEmpty() ? QImage() : decodeScaledFile(frame.filePath, QSize());
}

QSharedPointer<PackSequenceFrameSource> PackSequenceFrameSource::create(
    const QString& packRoot, const QSize& frameSize, bool verifyOnOpen, QString* errorMessage)
{
    try {
        QSharedPointer<PackSequenceFrameSource> result(new PackSequenceFrameSource());
        result->m_packRoot = QFileInfo(packRoot).absoluteFilePath();
        result->m_verifyOnOpen = verifyOnOpen;
        PackReaderQt reader(result->m_packRoot, verifyOnOpen);
        const quint64 count = reader.count();
        result->m_frames.reserve(count > static_cast<quint64>(INT_MAX) ? INT_MAX : static_cast<int>(count));
        for (quint64 i = 0; i < count; ++i) {
            const PACK_FRAME_INFO info = reader.frameInfo(i);
            SequenceFrameDescriptor frame;
            frame.imageName = QString::number(static_cast<qulonglong>(info.sourceIndex));
            frame.sourceKey = result->m_packRoot + QLatin1Char('#') + QString::number(static_cast<qulonglong>(info.globalIndex));
            frame.imageSize = frameSize;
            frame.globalIndex = static_cast<quint64>(info.globalIndex);
            frame.sourceIndex = static_cast<quint64>(info.sourceIndex);
            frame.timeValue = static_cast<quint64>(info.timeValue);
            result->m_frames.append(frame);
        }
        return result;
    }
    catch (const std::exception& ex) {
        if (errorMessage) *errorMessage = QString::fromLocal8Bit(ex.what());
        return QSharedPointer<PackSequenceFrameSource>();
    }
}

PackSequenceFrameSource::~PackSequenceFrameSource() {}
quint64 PackSequenceFrameSource::frameCount() const { return static_cast<quint64>(m_frames.size()); }
SequenceFrameDescriptor PackSequenceFrameSource::descriptor(quint64 index) const
{
    return index < static_cast<quint64>(m_frames.size()) ? m_frames.at(static_cast<int>(index)) : SequenceFrameDescriptor();
}
PackReaderQt* PackSequenceFrameSource::readerForCurrentThread()
{
    if (!m_threadReaders.hasLocalData()) m_threadReaders.setLocalData(new PackReaderQt(m_packRoot, m_verifyOnOpen));
    return m_threadReaders.localData();
}
QImage PackSequenceFrameSource::decodeThumbnail(quint64 index, const QSize& target)
{
    if (index >= static_cast<quint64>(m_frames.size())) return QImage();
    try { return decodeScaledBytes(readerForCurrentThread()->readJpeg(m_frames.at(static_cast<int>(index)).globalIndex), target); }
    catch (...) { return QImage(); }
}
QImage PackSequenceFrameSource::decodeFullImage(quint64 index)
{
    if (index >= static_cast<quint64>(m_frames.size())) return QImage();
    try { return decodeScaledBytes(readerForCurrentThread()->readJpeg(m_frames.at(static_cast<int>(index)).globalIndex), QSize()); }
    catch (...) { return QImage(); }
}

bool ImageSequenceModel::build(const QSharedPointer<ISequenceFrameSource>& source, LayoutOrientation orientation)
{
    m_frames.clear(); m_frameRects.clear(); m_visualStarts.clear(); m_visualEnds.clear(); m_nameToSource.clear();
    m_sceneRect = QRectF(); m_orientation = orientation;
    if (source.isNull() || source->frameCount() == 0 || source->frameCount() > static_cast<quint64>(INT_MAX)) return false;
    const int count = static_cast<int>(source->frameCount());
    m_frames.reserve(count); m_frameRects.resize(count);
    for (int i = 0; i < count; ++i) {
        const SequenceFrameDescriptor frame = source->descriptor(static_cast<quint64>(i));
        if (!frame.imageSize.isValid()) return false;
        m_frames.append(frame);
    }

    qreal totalExtent = 0.0;
    for (const SequenceFrameDescriptor& frame : m_frames) {
        totalExtent += isVerticalLayout(orientation) ? frame.imageSize.height() : frame.imageSize.width();
    }
    const qreal firstSourceExtent = isVerticalLayout(orientation)
        ? m_frames.first().imageSize.height() : m_frames.first().imageSize.width();
    // Match the legacy reverse layout exactly: source frame 0 stays at axis 0,
    // later source frames extend towards negative scene coordinates.
    qreal axis = isReverseLayout(orientation) ? firstSourceExtent - totalExtent : 0.0;
    m_visualStarts.reserve(count); m_visualEnds.reserve(count);
    for (int visual = 0; visual < count; ++visual) {
        const int sourceIndex = sourceIndexForVisualIndex(visual);
        const QSize size = m_frames.at(sourceIndex).imageSize;
        const qreal extent = isVerticalLayout(orientation) ? size.height() : size.width();
        QRectF rect = isVerticalLayout(orientation)
            ? QRectF(0.0, axis, size.width(), size.height())
            : QRectF(axis, 0.0, size.width(), size.height());
        m_frameRects[sourceIndex] = rect;
        m_visualStarts.append(axis);
        axis += extent;
        m_visualEnds.append(axis);
        m_sceneRect = m_sceneRect.isNull() ? rect : m_sceneRect.united(rect);
        const QStringList aliases = aliasesFor(m_frames.at(sourceIndex));
        for (const QString& alias : aliases) {
            const QString normalized = alias.toCaseFolded();
            if (!normalized.isEmpty() && !m_nameToSource.contains(normalized)) m_nameToSource.insert(normalized, sourceIndex);
        }
    }
    return !m_frames.isEmpty();
}

const SequenceFrameDescriptor& ImageSequenceModel::descriptor(int sourceIndex) const { return m_frames.at(sourceIndex); }
QRectF ImageSequenceModel::frameRect(int sourceIndex) const { return m_frameRects.value(sourceIndex); }
QRectF ImageSequenceModel::frameRectByVisualIndex(int visualIndex) const { return frameRect(sourceIndexForVisualIndex(visualIndex)); }
int ImageSequenceModel::sourceIndexForVisualIndex(int visualIndex) const
{
    if (visualIndex < 0 || visualIndex >= m_frames.size()) return -1;
    return isReverseLayout(m_orientation) ? m_frames.size() - 1 - visualIndex : visualIndex;
}
int ImageSequenceModel::visualIndexForSourceIndex(int sourceIndex) const
{
    if (sourceIndex < 0 || sourceIndex >= m_frames.size()) return -1;
    return isReverseLayout(m_orientation) ? m_frames.size() - 1 - sourceIndex : sourceIndex;
}
QStringList ImageSequenceModel::aliasesFor(const SequenceFrameDescriptor& frame) const
{
    QStringList aliases;
    aliases << frame.imageName << frame.filePath << frame.sourceKey;
    const QFileInfo info(frame.filePath.isEmpty() ? frame.imageName : frame.filePath);
    aliases << info.fileName() << info.completeBaseName();
    aliases.removeAll(QString()); aliases.removeDuplicates();
    return aliases;
}
int ImageSequenceModel::sourceIndexForName(const QString& imageName) const
{
    SequenceFrameDescriptor query;
    query.imageName = imageName;
    query.filePath = imageName;
    query.sourceKey = imageName;
    const QStringList candidates = aliasesFor(query);
    for (const QString& candidate : candidates) {
        const auto it = m_nameToSource.constFind(candidate.toCaseFolded());
        if (it != m_nameToSource.constEnd()) return it.value();
    }
    return -1;
}
int ImageSequenceModel::visualIndexAtAxis(qreal value) const
{
    if (m_visualStarts.isEmpty()) return -1;
    int low = 0, high = m_visualStarts.size();
    while (low < high) {
        const int mid = low + (high - low) / 2;
        if (m_visualStarts.at(mid) <= value) low = mid + 1; else high = mid;
    }
    const int visual = qBound(0, low - 1, m_visualStarts.size() - 1);
    return value <= m_visualEnds.at(visual) ? visual : -1;
}
int ImageSequenceModel::sourceIndexAt(const QPointF& scenePoint) const
{
    const int visual = visualIndexAtAxis(isVerticalLayout(m_orientation) ? scenePoint.y() : scenePoint.x());
    if (visual < 0) return -1;
    const int sourceIndex = sourceIndexForVisualIndex(visual);
    return frameRect(sourceIndex).contains(scenePoint) ? sourceIndex : -1;
}
int ImageSequenceModel::firstVisualIndexIntersecting(const QRectF& rect) const
{
    if (rect.isEmpty()) return -1;
    const qreal begin = isVerticalLayout(m_orientation) ? rect.top() : rect.left();
    return visualIndexAtAxis(qMax(firstAxisStart(), begin));
}
int ImageSequenceModel::lastVisualIndexIntersecting(const QRectF& rect) const
{
    if (rect.isEmpty()) return -1;
    const qreal end = isVerticalLayout(m_orientation) ? rect.bottom() : rect.right();
    return visualIndexAtAxis(qMin(lastAxisEnd() - 0.001, end - 0.001));
}

SequenceChunkItem::SequenceChunkItem(TiledGraphicsView* view, const QSharedPointer<ImageSequenceModel>& model,
    int firstVisualIndex, int lastVisualIndex)
    : m_view(view), m_model(model), m_firstVisualIndex(firstVisualIndex), m_lastVisualIndex(lastVisualIndex)
{
    for (int i = firstVisualIndex; i <= lastVisualIndex; ++i) {
        const QRectF frame = model->frameRectByVisualIndex(i);
        m_bounds = m_bounds.isNull() ? frame : m_bounds.united(frame);
    }
    // Without this flag Qt initializes option->exposedRect to the complete
    // chunk bounding rect.  A 512-frame chunk would then request and paint all
    // 512 frames after every small scroll, flooding the decode queue and
    // occasionally leaving the viewport at its black background until an
    // external full-window repaint.  With the extended option, exposedRect is
    // clipped to the actual viewport dirty region and paint remains O(K).
    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
    setAcceptedMouseButtons(Qt::NoButton);
    setZValue(-1000.0);
}

bool SequenceChunkItem::containsSourceIndex(int sourceIndex) const
{
    const int visual = m_model->visualIndexForSourceIndex(sourceIndex);
    return visual >= m_firstVisualIndex && visual <= m_lastVisualIndex;
}

void SequenceChunkItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
    if (!m_view || m_model.isNull()) return;
    const QRectF exposed = option ? option->exposedRect.intersected(m_bounds) : m_bounds;
    int first = m_model->firstVisualIndexIntersecting(exposed);
    int last = m_model->lastVisualIndexIntersecting(exposed);
    if (first < 0 || last < 0) return;
    first = qMax(first, m_firstVisualIndex); last = qMin(last, m_lastVisualIndex);
    const qreal lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(painter->worldTransform());
    const bool highResolution = lod >= 0.45;
    for (int visual = first; visual <= last; ++visual) {
        const int sourceIndex = m_model->sourceIndexForVisualIndex(visual);
        const QRectF target = m_model->frameRect(sourceIndex);
        QImage image = m_view->sequenceImageForPaint(sourceIndex, highResolution);
        if (image.isNull()) {
            painter->fillRect(target, QColor(18, 18, 18));
            continue;
        }
        painter->save();
        painter->setRenderHint(QPainter::SmoothPixmapTransform, !highResolution);
        if (m_view->sequenceHorizontalMirror() || m_view->sequenceVerticalMirror()) {
            painter->translate(target.left(), target.top());
            painter->translate(m_view->sequenceHorizontalMirror() ? target.width() : 0.0,
                m_view->sequenceVerticalMirror() ? target.height() : 0.0);
            painter->scale(m_view->sequenceHorizontalMirror() ? -1.0 : 1.0,
                m_view->sequenceVerticalMirror() ? -1.0 : 1.0);
            painter->drawImage(QRectF(0, 0, target.width(), target.height()), image);
        } else {
            painter->drawImage(target, image);
        }
        painter->restore();
    }
}
