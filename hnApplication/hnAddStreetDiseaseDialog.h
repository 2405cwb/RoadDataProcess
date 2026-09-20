#pragma once
#include <QDialog>
#include "hnDataManager.h"
#include "../hnCommon/hnRoadStruct.h"
class QTreeWidget;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QLabel;

// 标准景观批量选择及自定义景观录入，确认后统一事务保存。
class hnAddStreetDiseaseDialog : public QDialog
{
    Q_OBJECT
public:
    hnAddStreetDiseaseDialog(QVector<hnCommon::hnDiseaseSetInfo> LJInfo,
        QVector<hnCommon::hnDiseaseSetInfo> YXInfo, QWidget* parent = Q_NULLPTR);
    ~hnAddStreetDiseaseDialog();
    void setCurrentHnMile(const hnMile& mile);
    void setImageSide(int side);
    QVector<hnRoadDiseaseInfo> getSelectDiseases();
private slots:
    void saveSelection();
    void filterDiseases(const QString& text);
private:
    void addStandardGroup(const QString& name, const QVector<hnDiseaseSetInfo>& infos);
    QTreeWidget* m_tree;
    QLineEdit* m_search;
    QComboBox* m_customName;
    QSpinBox* m_customCount;
    QLineEdit* m_customRemark;
    QLabel* m_location;
    QVector<hnDiseaseSetInfo> m_settings;
    hnMile m_currentMile;
    QVector<hnRoadDiseaseInfo> m_selectDiseases;
    int m_side;
};
