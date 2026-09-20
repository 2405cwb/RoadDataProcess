#include "adjustImageWidget.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QTabWidget>
#include <QVBoxLayout>

adjustImageWidget::adjustImageWidget(QWidget* parent)
    : QDialog(parent), m_tabs(new QTabWidget(this)),
      m_2dBrightness(nullptr), m_2dContrast(nullptr), m_2dSharpen(nullptr),
      m_3dBrightness(nullptr), m_3dContrast(nullptr), m_3dSharpen(nullptr),
      m_2dBrightnessValue(nullptr), m_2dContrastValue(nullptr), m_2dSharpenValue(nullptr),
      m_3dBrightnessValue(nullptr), m_3dContrastValue(nullptr), m_3dSharpenValue(nullptr)
{
	setWindowTitle(QStringLiteral("\u56fe\u50cf\u8c03\u8282"));
    setWindowFlags(windowFlags() | Qt::Tool);
    setModal(false);
    resize(360, 210);

    QWidget* twoDPage = nullptr;
    QWidget* threeDPage = nullptr;
    setupPage(true, m_2dBrightness, m_2dContrast, m_2dSharpen, twoDPage,
        m_2dBrightnessValue, m_2dContrastValue, m_2dSharpenValue);
    setupPage(false, m_3dBrightness, m_3dContrast, m_3dSharpen, threeDPage,
        m_3dBrightnessValue, m_3dContrastValue, m_3dSharpenValue);
	m_tabs->addTab(twoDPage, QStringLiteral("\u4e8c\u7ef4\u56fe"));
	m_tabs->addTab(threeDPage, QStringLiteral("\u4e09\u7ef4\u56fe"));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_tabs);
    setLayout(layout);
}

void adjustImageWidget::setupPage(bool is2d, QSlider*& brightness, QSlider*& contrast, QSlider*& sharpen,
    QWidget*& page, QLabel*& brightnessValue, QLabel*& contrastValue, QLabel*& sharpenValue)
{
    page = new QWidget(this);
    QFormLayout* form = new QFormLayout(page);
    brightness = new QSlider(Qt::Horizontal, page);
    contrast = new QSlider(Qt::Horizontal, page);
    sharpen = new QSlider(Qt::Horizontal, page);
    brightness->setRange(-100, 100);
    contrast->setRange(0, 200);
    sharpen->setRange(0, 100);
    brightness->setValue(0);
    contrast->setValue(100);
    sharpen->setValue(0);
    brightnessValue = new QLabel(page);
    contrastValue = new QLabel(page);
    sharpenValue = new QLabel(page);

    auto addRow = [form, page](const QString& label, QSlider* slider, QLabel* value) {
        QWidget* row = new QWidget(page);
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(slider);
        value->setMinimumWidth(42);
        layout->addWidget(value);
        form->addRow(label, row);
    };
	addRow(QStringLiteral("\u4eae\u5ea6"), brightness, brightnessValue);
	addRow(QStringLiteral("\u5bf9\u6bd4\u5ea6"), contrast, contrastValue);
	addRow(QStringLiteral("\u9510\u5316"), sharpen, sharpenValue);

	QPushButton* reset = new QPushButton(QStringLiteral("\u6062\u590d\u9ed8\u8ba4"), page);
    form->addRow(QString(), reset);
    connect(brightness, &QSlider::valueChanged, this, &adjustImageWidget::slot_currentValuesChanged);
    connect(contrast, &QSlider::valueChanged, this, &adjustImageWidget::slot_currentValuesChanged);
    connect(sharpen, &QSlider::valueChanged, this, &adjustImageWidget::slot_currentValuesChanged);
    connect(brightness, &QSlider::sliderReleased, this, &adjustImageWidget::slot_saveCurrentValues);
    connect(contrast, &QSlider::sliderReleased, this, &adjustImageWidget::slot_saveCurrentValues);
    connect(sharpen, &QSlider::sliderReleased, this, &adjustImageWidget::slot_saveCurrentValues);
    connect(reset, &QPushButton::clicked, this, &adjustImageWidget::slot_resetCurrentValues);
    Q_UNUSED(is2d);
}

bool adjustImageWidget::currentIs2d() const
{
    return m_tabs->currentIndex() == 0;
}

void adjustImageWidget::setAdjustments(bool is2d, int brightness, int contrast, int sharpen)
{
    QSlider* brightnessSlider = is2d ? m_2dBrightness : m_3dBrightness;
    QSlider* contrastSlider = is2d ? m_2dContrast : m_3dContrast;
    QSlider* sharpenSlider = is2d ? m_2dSharpen : m_3dSharpen;
    QSignalBlocker blockBrightness(brightnessSlider);
    QSignalBlocker blockContrast(contrastSlider);
    QSignalBlocker blockSharpen(sharpenSlider);
    brightnessSlider->setValue(qBound(-100, brightness, 100));
    contrastSlider->setValue(qBound(0, contrast, 200));
    sharpenSlider->setValue(qBound(0, sharpen, 100));
    updateValueLabels(is2d);
}

void adjustImageWidget::updateValueLabels(bool is2d)
{
    QLabel* brightnessValue = is2d ? m_2dBrightnessValue : m_3dBrightnessValue;
    QLabel* contrastValue = is2d ? m_2dContrastValue : m_3dContrastValue;
    QLabel* sharpenValue = is2d ? m_2dSharpenValue : m_3dSharpenValue;
    QSlider* brightness = is2d ? m_2dBrightness : m_3dBrightness;
    QSlider* contrast = is2d ? m_2dContrast : m_3dContrast;
    QSlider* sharpen = is2d ? m_2dSharpen : m_3dSharpen;
    brightnessValue->setText(QString::number(brightness->value()));
    contrastValue->setText(QStringLiteral("%1%").arg(contrast->value()));
    sharpenValue->setText(QStringLiteral("%1%").arg(sharpen->value()));
}

void adjustImageWidget::emitCurrentValues(bool save)
{
    const bool is2d = currentIs2d();
    QSlider* brightness = is2d ? m_2dBrightness : m_3dBrightness;
    QSlider* contrast = is2d ? m_2dContrast : m_3dContrast;
    QSlider* sharpen = is2d ? m_2dSharpen : m_3dSharpen;
    emit signal_adjustmentsPreviewChanged(is2d, brightness->value(), contrast->value(), sharpen->value());
    if (save)
        emit signal_adjustmentsSaveRequested(is2d, brightness->value(), contrast->value(), sharpen->value());
}

void adjustImageWidget::slot_currentValuesChanged()
{
    updateValueLabels(currentIs2d());
    emitCurrentValues(false);
}

void adjustImageWidget::slot_saveCurrentValues()
{
    emitCurrentValues(true);
}

void adjustImageWidget::slot_resetCurrentValues()
{
    const bool is2d = currentIs2d();
    setAdjustments(is2d, 0, 100, 0);
    emitCurrentValues(true);
}
