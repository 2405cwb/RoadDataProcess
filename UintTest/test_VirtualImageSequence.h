#pragma once

#include <QObject>

class test_VirtualImageSequence : public QObject
{
    Q_OBJECT
private slots:
    void verticalReverseCoordinatesMatchLegacy();
    void indexesOneHundredThousandFrames();
    void chunkUsesViewportExposedRect();
};
