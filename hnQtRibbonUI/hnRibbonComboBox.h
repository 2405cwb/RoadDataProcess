#ifndef HNRIBBONCOMBOBOX_H
#define HNRIBBONCOMBOBOX_H
#include "hnQTRibbonGlobal.h"
#include <QComboBox>


///
/// \brief QComboBox的Ribbon显示，可以显示QIcon和windowTitle在左侧
///
class HN_QTRIBBON_EXPORT hnRibbonComboBox : public QComboBox
{
    Q_OBJECT
public:
	hnRibbonComboBox(QWidget *parent = Q_NULLPTR);
};

#endif // HNRIBBONCOMBOBOX_H
