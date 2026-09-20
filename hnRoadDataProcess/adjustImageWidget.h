#pragma once

#include <QDialog>

class QSlider;
class QTabWidget;
class QLabel;
class QPushButton;

class adjustImageWidget : public QDialog
{
    Q_OBJECT
public:
    explicit adjustImageWidget(QWidget* parent = Q_NULLPTR);
    void setAdjustments(bool is2d, int brightness, int contrast, int sharpen);

signals:
    void signal_adjustmentsPreviewChanged(bool is2d, int brightness, int contrast, int sharpen);
    void signal_adjustmentsSaveRequested(bool is2d, int brightness, int contrast, int sharpen);

private slots:
    void slot_currentValuesChanged();
    void slot_saveCurrentValues();
    void slot_resetCurrentValues();

private:
    bool currentIs2d() const;
    void setupPage(bool is2d, QSlider*& brightness, QSlider*& contrast, QSlider*& sharpen,
        QWidget*& page, QLabel*& brightnessValue, QLabel*& contrastValue, QLabel*& sharpenValue);
    void updateValueLabels(bool is2d);
    void emitCurrentValues(bool save);

    QTabWidget* m_tabs;
    QSlider* m_2dBrightness;
    QSlider* m_2dContrast;
    QSlider* m_2dSharpen;
    QSlider* m_3dBrightness;
    QSlider* m_3dContrast;
    QSlider* m_3dSharpen;
    QLabel* m_2dBrightnessValue;
    QLabel* m_2dContrastValue;
    QLabel* m_2dSharpenValue;
    QLabel* m_3dBrightnessValue;
    QLabel* m_3dContrastValue;
    QLabel* m_3dSharpenValue;
};