#pragma once
#include <QDialog>
namespace hnPro { class hnProject; }
class hnResultDataDialog : public QDialog
{
public:
    explicit hnResultDataDialog(hnPro::hnProject* project, QWidget* parent = nullptr);
};