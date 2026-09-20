#include "test_VirtualImageSequence.h"

#include "../TunnelViewerSDK/include/VirtualImageSequence.h"

#include <QtTest/QTest>

class SyntheticSequenceSource : public ISequenceFrameSource
{
public:
    SyntheticSequenceSource(int count, const QSize& size) : m_count(count), m_size(size) {}
    quint64 frameCount() const override { return static_cast<quint64>(m_count); }
    SequenceFrameDescriptor descriptor(quint64 index) const override
    {
        SequenceFrameDescriptor frame;
        frame.imageName = QStringLiteral("frame_%1").arg(index);
        frame.sourceKey = frame.imageName;
        frame.imageSize = m_size;
        frame.sourceIndex = index;
        frame.globalIndex = index;
        return frame;
    }
    QImage decodeThumbnail(quint64, const QSize&) override { return QImage(); }
    QImage decodeFullImage(quint64) override { return QImage(); }

private:
    int m_count;
    QSize m_size;
};

void test_VirtualImageSequence::verticalReverseCoordinatesMatchLegacy()
{
    QSharedPointer<ISequenceFrameSource> source(new SyntheticSequenceSource(3, QSize(4096, 2000)));
    ImageSequenceModel model;
    QVERIFY(model.build(source, LayoutOrientation::VerticalReverse));
    QCOMPARE(model.frameRect(0), QRectF(0, 0, 4096, 2000));
    QCOMPARE(model.frameRect(1), QRectF(0, -2000, 4096, 2000));
    QCOMPARE(model.frameRect(2), QRectF(0, -4000, 4096, 2000));
    QCOMPARE(model.sourceIndexForName(QStringLiteral("frame_1")), 1);
    QCOMPARE(model.sourceIndexAt(QPointF(100, -1000)), 1);
    QCOMPARE(model.visualIndexForSourceIndex(2), 0);
    QCOMPARE(model.sourceIndexForVisualIndex(0), 2);
}

void test_VirtualImageSequence::indexesOneHundredThousandFrames()
{
    const int count = 100000;
    QSharedPointer<ISequenceFrameSource> source(new SyntheticSequenceSource(count, QSize(4096, 2000)));
    ImageSequenceModel model;
    QVERIFY(model.build(source, LayoutOrientation::Vertical));
    QCOMPARE(model.count(), count);
    QCOMPARE(model.sourceIndexForName(QStringLiteral("frame_99999")), count - 1);
    QCOMPARE(model.sourceIndexAt(QPointF(100, 99999.0 * 2000.0 + 1000.0)), count - 1);
    QCOMPARE(model.firstVisualIndexIntersecting(QRectF(0, 50000.0 * 2000.0, 4096, 2000)), 50000);
}

void test_VirtualImageSequence::chunkUsesViewportExposedRect()
{
    QSharedPointer<ISequenceFrameSource> source(new SyntheticSequenceSource(512, QSize(4096, 2000)));
    QSharedPointer<ImageSequenceModel> model(new ImageSequenceModel());
    QVERIFY(model->build(source, LayoutOrientation::Vertical));
    SequenceChunkItem chunk(nullptr, model, 0, 511);
    QVERIFY(chunk.flags().testFlag(QGraphicsItem::ItemUsesExtendedStyleOption));
}
