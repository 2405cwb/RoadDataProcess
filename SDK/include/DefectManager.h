#pragma once

#include <QObject>
#include <QGraphicsScene>
#include <QHash>
#include <QMap>

#include "IDefectStorage.h"
#include "items/DefectShapeItem.h"

/* 简单样式包 旧接口还在用 */ struct DefectStyle {
    QPen pen;
    QBrush brush;
};

class DefectManager : public QObject
{
    Q_OBJECT
public:
    /* Manager 只管一类标注集合 scene 由 View 持有 */ explicit DefectManager(QGraphicsScene* scene, QObject* parent = nullptr)
        : QObject(parent), m_scene(scene), m_storage(nullptr) {}

    /* 当前实现接管 storage 指针 析构时一起释放 */ ~DefectManager() {
        if (m_storage) delete m_storage;
    }

    /* 返回当前场景里已加载的标注数量 */ int getTotalDefectCount() const {
        return m_items.size();
    }

    /* 批量注册标注类型 SDK 不解释业务含义 */ void registerDefectConfigs(const QList<DefectTypeConfig>& configs) {
        m_configs.clear();
        for (const auto& cfg : configs) {
            const int code = cfg.typeCode != 0 ? cfg.typeCode : cfg.defectCode;
            DefectTypeConfig normalized = cfg;
            normalized.typeCode = code;
            normalized.defectCode = code;
            m_configs[code] = normalized;
        }
    }

    /* 按默认图形筛类型 点线面工具栏会用到 */ QList<DefectTypeConfig> getConfigsByShape(DrawShape shapeType) const {
        QList<DefectTypeConfig> result;
        for (auto it = m_configs.begin(); it != m_configs.end(); ++it) {
            if (it.value().defaultShape == shapeType) {
                result.append(it.value());
            }
        }
        return result;
    }

    /* 按业务编码取样式 没配就给一个能看见的默认样式 */ DefectTypeConfig getConfig(int defectCode) const {
        if (m_configs.contains(defectCode)) {
            return m_configs.value(defectCode);
        }
        return{ 0, QStringLiteral("Unknown Annotation"), QPen(Qt::yellow, 3, Qt::SolidLine), Qt::NoBrush };
    }

    /* 注入存储策略 JSON SQLite 或服务接口都可以 */ void setStorageStrategy(IDefectStorage* storage) {
        if (m_storage) delete m_storage;
        m_storage = storage;
    }

    /* 设置当前存储连接串 */ void setConnectionUri(const QString& uri) {
        m_currentUri = uri;
    }

    /* 返回当前存储连接串 */ QString connectionUri() const {
        return m_currentUri;
    }

    /* 添加标注 triggerSave=false 用于加载时避免回写 */ void addDefect(DefectShapeItem* item, bool triggerSave = true) {
        if (!item || m_items.contains(item->getUuid())) {
            return;
        }

        const int code = annotationCode(item);
        DefectTypeConfig cfg = getConfig(code);
        item->setPen(cfg.pen);
        item->setBrush(cfg.brush);
        applyMovePolicy(item);

        m_items.insert(item->getUuid(), item);
        m_scene->addItem(item);

        connect(item, &DefectShapeItem::sigDefectMoved, this, [this](DefectShapeItem* movedItem) {
            emit sigDefectMoved(toStorageData(movedItem));
        });

        if (triggerSave && m_storage && !m_currentUri.isEmpty()) {
            m_storage->addOne(toStorageData(item), m_currentUri);
        }
    }

    /* 从场景和存储里删除一个标注 */ void removeDefect(DefectShapeItem* item) {
        if (!item) return;
        QString uuid = QString::number(item->getUuid());
        if (m_items.remove(item->getUuid())) {
            m_scene->removeItem(item);
            delete item;

            if (m_storage && !m_currentUri.isEmpty()) {
                m_storage->deleteOne(uuid, m_currentUri);
            }
        }
    }

    /* 清空当前 Manager 管的标注 */ void clearDefects() {
        for (auto item : m_items) {
            m_scene->removeItem(item);
            delete item;
        }
        m_items.clear();
    }

    /* 从当前存储连接全量加载标注 */ bool loadDefects() {
        if (!m_storage || m_currentUri.isEmpty()) return false;

        clearDefects();
        QList<DefectData> dataList = m_storage->loadAll(m_currentUri);
        for (const auto& data : dataList) {
            addDefect(createItemFromData(data), false);
        }
        return true;
    }

    /* 按范围加载 参数名先兼容 startMile/endMile */ bool loadDefectsByRange(double startMile, double endMile) {
        if (!m_storage || m_currentUri.isEmpty()) return false;

        clearDefects();
        QList<DefectData> dataList = m_storage->loadByRange(m_currentUri, startMile, endMile);
        for (const auto& data : dataList) {
            addDefect(createItemFromData(data), false);
        }
        return true;
    }

    /* 全量保存 不传 uri 就用当前连接串 */ bool saveDefects(const QString& uri = "") {
        if (!m_storage) return false;

        QString targetUri = uri.isEmpty() ? m_currentUri : uri;
        if (targetUri.isEmpty()) return false;

        QList<DefectData> dataList;
        for (auto item : m_items) {
            dataList.append(toStorageData(item));
        }
        return m_storage->saveAll(dataList, targetUri);
    }

    /* 兼容老名字 实际走通用移动策略 */ void setMoviesType(DefectShapeItem* item) {
        applyMovePolicy(item);
    }

    /* 按 id 找标注 */ DefectShapeItem* getItemUseId(int uuid) {
        return m_items.value(uuid);
    }

signals:
    /* 标注移动后抛出纯数据 */ void sigDefectMoved(DefectData item);

private:
    /* 从 item 上取编码 兼容新旧字段和属性包 */ int annotationCode(DefectShapeItem* item) const {
        int code = item->property("typeCode").toInt();
        if (code == 0) code = item->property("defectCode").toInt();
        if (code == 0) code = item->getAttribute("typeCode").toInt();
        if (code == 0) code = item->getAttribute("defectCode").toInt();
        return code;
    }

    /* 保存前补齐 typeCode 和 defectCode */ DefectData toStorageData(DefectShapeItem* item) const {
        DefectData data = item->toData();
        const int code = annotationCode(item);
        data.typeCode = code;
        data.defectCode = code;
        data.category = item->getAttribute("category").toString();
        return data;
    }

    /* 从纯数据创建图元 并把编码写回 property */ DefectShapeItem* createItemFromData(const DefectData& data) const {
        DefectShapeItem* item = new DefectShapeItem(data.shape, data.name, data.type, data.uuid);
        item->fromData(data);
        const int code = data.typeCode != 0 ? data.typeCode : data.defectCode;
        item->setProperty("typeCode", code);
        item->setProperty("defectCode", code);
        if (!data.category.isEmpty()) {
            item->setAttribute("category", data.category);
        }
        return item;
    }

    /* 优先读通用 moveAxis/locked 没配再走旧 ElementType */ void applyMovePolicy(DefectShapeItem* item) {
        QVariant lockedValue = item->property("locked");
        if (!lockedValue.isValid()) lockedValue = item->getAttribute("locked");
        if (lockedValue.isValid()) {
            item->setLocked(lockedValue.toBool());
        }

        QVariant axisValue = item->property("moveAxis");
        if (!axisValue.isValid()) axisValue = item->getAttribute("moveAxis");
        if (axisValue.isValid()) {
            item->setMoveAxis(static_cast<MoveAxis>(axisValue.toInt()));
            return;
        }

        switch (item->m_elementType) {
        case Type_Cp3:
        case Type_Chain:
            item->setLocked(true);
            break;
        case Type_Ring:
        case Type_Section:
        case Type_Platform:
            item->setMoveAxis(Axis_Horizontal);
            break;
        case Type_Disease:
        case Type_AUTORING:
        case Type_CustomOverlay:
        default:
            item->setMoveAxis(Axis_Free);
            break;
        }
    }

private:
    QGraphicsScene* m_scene;
    QHash<int, DefectShapeItem*> m_items;
    IDefectStorage* m_storage;
    QMap<int, DefectStyle> m_styles;
    QString m_currentUri;
    QMap<int, DefectTypeConfig> m_configs;
};

/* 新代码可以用 AnnotationManager 老代码继续用 DefectManager */ using AnnotationManager = DefectManager;
