#pragma once

#include <QObject>
#include <QGraphicsScene>
#include <QHash>
#include <QMap>
#include <QList>
#include <QSet>
#include <functional>

#include "IDefectStorage.h"
#include "items/DefectShapeItem.h"

/* 简单样式包，旧接口继续保留。 */
struct DefectStyle {
    QPen pen;
    QBrush brush;
};

/*
 * 自定义 Item 创建器。
 * creator 根据传入 DefectData 构造派生对象；如果派生类有额外成员，可在 creator 内从 attributes 初始化。
 * Manager 随后仍会统一恢复 DefectShapeItem 基础字段、兼容身份、样式和交互策略。
 */
using AnnotationItemCreator = std::function<DefectShapeItem*(const DefectData&)>;

/*
 * 通用标注管理器。
 *
 * 兼容说明：类名 DefectManager、原构造函数、原 add/remove/load/save 接口全部保留。
 * 新代码可通过 AnnotationManager 别名、layerKey/typeKey 和属性包使用通用能力。
 */
class DefectManager : public QObject
{
    Q_OBJECT
public:
    explicit DefectManager(QGraphicsScene* scene, QObject* parent = nullptr)
        : QObject(parent), m_scene(scene), m_storage(nullptr) {}

    ~DefectManager() {
        if (m_storage) delete m_storage;
        m_storage = nullptr;
    }

    int getTotalDefectCount() const { return m_items.size(); }
    QList<DefectShapeItem*> items() const { return m_items.values(); }

    /*
     * 解析新旧数据的统一身份。旧数据没有 typeKey/layerKey 时，按 Manager + typeCode + ElementType 兜底。
     * 该函数不修改 sourceData，便于外部 Storage/迁移工具复用同一套兼容规则。
     */
    AnnotationIdentity resolveIdentity(const DefectData& sourceData) const {
        AnnotationIdentity identity;
        identity.formatVersion = sourceData.attributes.value(AnnotationAttributeKey::FormatVersion, 1).toInt();
        identity.typeCode = sourceData.typeCode != 0 ? sourceData.typeCode : sourceData.defectCode;

        const QVariant legacyValue = sourceData.attributes.value(AnnotationAttributeKey::LegacyElementType);
        identity.legacyElementType = legacyValue.isValid()
            ? elementTypeFromPersistedValue(legacyValue.toInt(), m_persistenceProfile)
            : m_legacyElementType;

        identity.layerKey = sourceData.attributes.value(AnnotationAttributeKey::LayerKey).toString().trimmed();
        if (identity.layerKey.isEmpty()) identity.layerKey = m_layerKey.trimmed();
        if (identity.layerKey.isEmpty()) identity.layerKey = annotationLayerKeyFromLegacyElement(identity.legacyElementType);

        identity.typeKey = sourceData.attributes.value(AnnotationAttributeKey::TypeKey).toString().trimmed();
        if (identity.typeKey.isEmpty() && m_configs.contains(identity.typeCode))
            identity.typeKey = m_configs.value(identity.typeCode).typeKey.trimmed();
        if (identity.typeKey.isEmpty()) identity.typeKey = annotationTypeKeyFromLegacyElement(identity.legacyElementType);
        return identity;
    }

    /* 注册/注销外部自定义 Item 工厂；未注册或创建失败时永远回退 DefectShapeItem。 */
    void registerItemFactory(const QString& typeKey, const AnnotationItemCreator& creator) {
        const QString key = typeKey.trimmed();
        if (key.isEmpty()) return;
        if (creator) m_itemFactories.insert(key, creator);
        else m_itemFactories.remove(key);
    }
    void unregisterItemFactory(const QString& typeKey) { m_itemFactories.remove(typeKey.trimmed()); }
    bool hasItemFactory(const QString& typeKey) const { return m_itemFactories.contains(typeKey.trimmed()); }

    /* 图层运行状态不进入旧业务数据，切换后立即应用到当前已加载 Item。 */
    void setLayerState(const AnnotationLayerState& state) {
        const bool restoreZValue = m_layerState.hasZValue && !state.hasZValue;
        const bool restoreVisibility = !m_layerState.visible && state.visible;
        m_layerState = state;
        const QList<DefectShapeItem*> currentItems = m_items.values();
        for (DefectShapeItem* item : currentItems) {
            if (!item) continue;
            if (restoreZValue && m_itemOriginalZValues.contains(item->getUuid())) {
                item->setZValue(m_itemOriginalZValues.take(item->getUuid()));
            }
            if (restoreVisibility && m_itemOriginalVisibility.contains(item->getUuid())) {
                item->setVisible(m_itemOriginalVisibility.take(item->getUuid()));
            }
            applyLayerState(item);
        }
        if (!m_layerState.hasZValue) m_itemOriginalZValues.clear();
        if (m_layerState.visible) m_itemOriginalVisibility.clear();
    }
    AnnotationLayerState layerState() const { return m_layerState; }
    void setLayerVisible(bool visible) { AnnotationLayerState s = m_layerState; s.visible = visible; setLayerState(s); }
    void setLayerLocked(bool locked) { AnnotationLayerState s = m_layerState; s.locked = locked; setLayerState(s); }
    void setLayerSelectable(bool selectable) { AnnotationLayerState s = m_layerState; s.selectable = selectable; setLayerState(s); }
    void setLayerExportable(bool exportable) { AnnotationLayerState s = m_layerState; s.exportable = exportable; setLayerState(s); }
    void setLayerZValue(qreal zValue) { AnnotationLayerState s = m_layerState; s.hasZValue = true; s.zValue = zValue; setLayerState(s); }
    void clearLayerZValueOverride() { AnnotationLayerState s = m_layerState; s.hasZValue = false; setLayerState(s); }

    /* 当前 Manager 对应的通用图层键。 */
    void setLayerKey(const QString& layerKey) { m_layerKey = layerKey; }
    QString layerKey() const { return m_layerKey; }

    /*
     * 设置旧格式默认 ElementType。
     * 老存储没有 typeKey/layerKey/legacyElementType 时，按所属 Manager 补齐旧语义。
     */
    void setLegacyElementType(ElementType type) { m_legacyElementType = type; }
    ElementType legacyElementType() const { return m_legacyElementType; }

    /* 显式指定旧数据整数的解释方式，默认 UnifiedModern。 */
    void setPersistenceProfile(ElementTypePersistenceProfile profile) { m_persistenceProfile = profile; }
    ElementTypePersistenceProfile persistenceProfile() const { return m_persistenceProfile; }

    /* 批量注册标注类型。typeCode 继续兼容；新代码优先给 typeKey。 */
    void registerDefectConfigs(const QList<DefectTypeConfig>& configs) {
        m_configs.clear();
        m_configsByTypeKey.clear();
        for (const auto& cfg : configs) {
            const int code = cfg.typeCode != 0 ? cfg.typeCode : cfg.defectCode;
            DefectTypeConfig normalized = cfg;
            normalized.typeCode = code;
            normalized.defectCode = code;
            // typeKey 先归一化再同时写入两套索引，避免带首尾空格的配置在
            // getConfigsByShape() 中被当成两个不同类型重复返回。
            normalized.typeKey = normalized.typeKey.trimmed();
            m_configs[code] = normalized;
            if (!normalized.typeKey.isEmpty()) {
                m_configsByTypeKey[normalized.typeKey] = normalized;
            }
        }
    }

    QList<DefectTypeConfig> getConfigsByShape(DrawShape shapeType) const {
        QList<DefectTypeConfig> result;
        QSet<QString> seenTypeKeys;

        // 先严格保留旧 QMap<int,...> 的按 code 顺序，避免旧工具栏类型顺序变化。
        for (auto it = m_configs.begin(); it != m_configs.end(); ++it) {
            const DefectTypeConfig& cfg = it.value();
            if (cfg.defaultShape != shapeType) continue;
            result.append(cfg);
            if (!cfg.typeKey.isEmpty()) seenTypeKeys.insert(cfg.typeKey);
        }

        // 新 typeKey 允许多个类型共用/省略 legacy typeCode。旧 int map 无法表达这类冲突，
        // 因此只把尚未出现的 typeKey 配置补到结果尾部；旧配置的顺序和数量保持原样。
        for (auto it = m_configsByTypeKey.constBegin(); it != m_configsByTypeKey.constEnd(); ++it) {
            const DefectTypeConfig& cfg = it.value();
            if (cfg.defaultShape != shapeType || seenTypeKeys.contains(cfg.typeKey)) continue;
            result.append(cfg);
            seenTypeKeys.insert(cfg.typeKey);
        }
        return result;
    }

    DefectTypeConfig getConfig(int defectCode) const {
        if (m_configs.contains(defectCode)) return m_configs.value(defectCode);
        return { 0, QStringLiteral("Unknown Annotation"), QPen(Qt::yellow, 3, Qt::SolidLine), Qt::NoBrush };
    }

    DefectTypeConfig getConfig(const QString& typeKey) const {
        const QString key = typeKey.trimmed();
        if (!key.isEmpty() && m_configsByTypeKey.contains(key)) return m_configsByTypeKey.value(key);
        return DefectTypeConfig();
    }

    bool hasConfig(int code) const { return m_configs.contains(code); }
    bool hasConfig(const QString& typeKey) const {
        const QString key = typeKey.trimmed();
        return !key.isEmpty() && m_configsByTypeKey.contains(key);
    }

    void setStorageStrategy(IDefectStorage* storage) {
        if (m_storage == storage) return;
        if (m_storage) delete m_storage;
        m_storage = storage;
    }

    void setConnectionUri(const QString& uri) { m_currentUri = uri; }
    QString connectionUri() const { return m_currentUri; }

    /* 添加标注。triggerSave=false 用于加载时避免回写。 */
    void addDefect(DefectShapeItem* item, bool triggerSave = true) {
        if (!item || !m_scene || m_items.contains(item->getUuid())) return;

        normalizeItem(item);

        const int code = annotationCode(item);
        DefectTypeConfig cfg;
        bool hasCfg = false;
        if (hasConfig(item->typeKey())) {
            cfg = getConfig(item->typeKey());
            hasCfg = true;
        }
        else if (hasConfig(code)) {
            cfg = getConfig(code);
            hasCfg = true;
        }

        if (hasCfg) {
            // 保持旧行为：注册过类型配置时由配置决定基础样式。
            item->setPen(cfg.pen);
            item->setBrush(cfg.brush);
            if (item->typeKey().isEmpty() && !cfg.typeKey.isEmpty()) item->setTypeKey(cfg.typeKey);

            // 配置属性是默认值；item 自己显式携带的属性优先。
            const QVariantMap defaults = cfg.attributes;
            for (auto it = defaults.constBegin(); it != defaults.constEnd(); ++it) {
                if (!item->getAttribute(it.key()).isValid()) item->setAttribute(it.key(), it.value());
            }
        }
        else {
            // 严格保持旧 DefectManager 行为：未注册 code 时使用黄色默认样式。
            const DefectTypeConfig legacyDefault = getConfig(code);
            item->setPen(legacyDefault.pen);
            item->setBrush(legacyDefault.brush);
        }

        applyMovePolicy(item);
        applyInteractionPolicy(item);
        applyLayerState(item);

        const int uuid = item->getUuid();
        m_items.insert(uuid, item);
        m_scene->addItem(item);

        connect(item, &DefectShapeItem::sigDefectMoved, this, [this](DefectShapeItem* movedItem) {
            emit sigDefectMoved(toStorageData(movedItem));
        });
        // Scene/外部如果先删除 Item，账本自动移除，避免留下悬空指针。
        connect(item, &QObject::destroyed, this, [this, uuid]() {
            m_items.remove(uuid);
            m_itemOriginalZValues.remove(uuid);
            m_itemOriginalVisibility.remove(uuid);
        });

        if (triggerSave && m_storage && !m_currentUri.isEmpty()) {
            m_storage->addOne(toStorageData(item), m_currentUri);
        }
    }

    void removeDefect(DefectShapeItem* item) {
        if (!item) return;
        const QString uuidText = QString::number(item->getUuid());
        if (!m_items.remove(item->getUuid())) return;
        m_itemOriginalZValues.remove(item->getUuid());
        m_itemOriginalVisibility.remove(item->getUuid());
        if (m_scene && item->scene() == m_scene) m_scene->removeItem(item);
        delete item;

        if (m_storage && !m_currentUri.isEmpty()) m_storage->deleteOne(uuidText, m_currentUri);
    }

    /*
     * 先清空账本再 delete，避免 QObject::destroyed 回调在遍历 QHash 时修改容器。
     */
    void clearDefects() {
        const QList<DefectShapeItem*> items = m_items.values();
        m_items.clear();
        m_itemOriginalZValues.clear();
        m_itemOriginalVisibility.clear();
        for (DefectShapeItem* item : items) {
            if (!item) continue;
            if (m_scene && item->scene() == m_scene) m_scene->removeItem(item);
            delete item;
        }
    }

    bool loadDefects() {
        if (!m_storage || m_currentUri.isEmpty()) return false;
        clearDefects();
        const QList<DefectData> dataList = m_storage->loadAll(m_currentUri);
        for (const auto& data : dataList) addDefect(createItemFromData(data), false);
        return true;
    }

    bool loadDefectsByRange(double startMile, double endMile) {
        if (!m_storage || m_currentUri.isEmpty()) return false;
        clearDefects();
        const QList<DefectData> dataList = m_storage->loadByRange(m_currentUri, startMile, endMile);
        for (const auto& data : dataList) addDefect(createItemFromData(data), false);
        return true;
    }

    bool saveDefects(const QString& uri = "") {
        if (!m_storage) return false;
        const QString targetUri = uri.isEmpty() ? m_currentUri : uri;
        if (targetUri.isEmpty()) return false;
        QList<DefectData> dataList;
        for (DefectShapeItem* item : m_items) {
            if (item) dataList.append(toStorageData(item));
        }
        return m_storage->saveAll(dataList, targetUri);
    }

    void setMoviesType(DefectShapeItem* item) { applyMovePolicy(item); }
    DefectShapeItem* getItemUseId(int uuid) { return m_items.value(uuid, nullptr); }
    const DefectShapeItem* getItemUseId(int uuid) const { return m_items.value(uuid, nullptr); }

signals:
    void sigDefectMoved(DefectData item);

private:
    int annotationCode(DefectShapeItem* item) const {
        if (!item) return 0;
        int code = item->property("typeCode").toInt();
        if (code == 0) code = item->property("defectCode").toInt();
        if (code == 0) code = item->getAttribute("typeCode").toInt();
        if (code == 0) code = item->getAttribute("defectCode").toInt();
        return code;
    }

    void normalizeItem(DefectShapeItem* item) const {
        if (!item) return;
        if (item->layerKey().isEmpty()) {
            item->setLayerKey(!m_layerKey.isEmpty() ? m_layerKey : annotationLayerKeyFromLegacyElement(item->m_elementType));
        }
        if (item->typeKey().isEmpty()) {
            const int code = annotationCode(item);
            if (m_configs.contains(code) && !m_configs.value(code).typeKey.isEmpty())
                item->setTypeKey(m_configs.value(code).typeKey.trimmed());
            else
                item->setTypeKey(annotationTypeKeyFromLegacyElement(item->m_elementType));
        }
    }

    DefectData toStorageData(DefectShapeItem* item) const {
        DefectData data = item->toData();
        const int code = annotationCode(item);
        data.typeCode = code;
        data.defectCode = code;
        data.category = item->getAttribute("category").toString();
        data.attributes[AnnotationAttributeKey::FormatVersion] = 2;
        data.attributes[AnnotationAttributeKey::LayerKey] = item->layerKey().isEmpty() ? m_layerKey : item->layerKey();
        data.attributes[AnnotationAttributeKey::TypeKey] = item->typeKey();
        data.attributes[AnnotationAttributeKey::LegacyElementType] =
            elementTypeToPersistedValue(item->m_elementType, m_persistenceProfile);
        return data;
    }

    DefectShapeItem* createItemFromData(const DefectData& sourceData) const {
        const AnnotationIdentity identity = resolveIdentity(sourceData);

        // 给 Factory 一份“补齐身份后的数据副本”，但绝不要求旧 Storage 迁移原始记录。
        DefectData data = sourceData;
        data.attributes[AnnotationAttributeKey::FormatVersion] = qMax(1, identity.formatVersion);
        data.attributes[AnnotationAttributeKey::LayerKey] = identity.layerKey;
        data.attributes[AnnotationAttributeKey::TypeKey] = identity.typeKey;
        data.attributes[AnnotationAttributeKey::LegacyElementType] =
            elementTypeToPersistedValue(identity.legacyElementType, m_persistenceProfile);

        DefectShapeItem* item = nullptr;
        const auto factoryIt = m_itemFactories.constFind(identity.typeKey);
        if (factoryIt != m_itemFactories.constEnd() && factoryIt.value()) {
            item = factoryIt.value()(data);
        }
        if (!item) {
            item = new DefectShapeItem(data.shape, data.name, data.type, data.uuid);
        }

        // 不论默认 Item 还是外部派生 Item，都由 Manager 统一恢复旧数据和 SDK 元数据。
        item->m_elementType = identity.legacyElementType;
        item->fromData(data, m_persistenceProfile);
        item->m_elementType = identity.legacyElementType;
        item->setLayerKey(identity.layerKey);
        item->setTypeKey(identity.typeKey);

        const int code = identity.typeCode;
        item->setProperty("typeCode", code);
        item->setProperty("defectCode", code);
        if (!data.category.isEmpty()) item->setAttribute("category", data.category);
        return item;
    }


    void applyInteractionPolicy(DefectShapeItem* item) {
        if (!item) return;
        const bool selectable = m_layerState.selectable && item->isSelectableByPolicy();
        item->setFlag(QGraphicsItem::ItemIsSelectable, selectable);
        if (!selectable && item->isSelected()) item->setSelected(false);
    }

    void applyLayerState(DefectShapeItem* item) {
        if (!item) return;
        if (!m_layerState.visible) {
            if (!m_itemOriginalVisibility.contains(item->getUuid()))
                m_itemOriginalVisibility.insert(item->getUuid(), item->isVisible());
            item->setVisible(false);
        }
        item->setLayerLocked(m_layerState.locked);
        const bool selectable = m_layerState.selectable && item->isSelectableByPolicy();
        item->setFlag(QGraphicsItem::ItemIsSelectable, selectable);
        if ((!m_layerState.visible || !selectable) && item->isSelected()) item->setSelected(false);
        if (m_layerState.hasZValue) {
            if (!m_itemOriginalZValues.contains(item->getUuid()))
                m_itemOriginalZValues.insert(item->getUuid(), item->zValue());
            item->setZValue(m_layerState.zValue);
        }
    }

    /* 显式属性优先；没有配置时继续走原 ElementType 策略。 */
    void applyMovePolicy(DefectShapeItem* item) {
        if (!item) return;
        QVariant lockedValue = item->property(AnnotationAttributeKey::Locked);
        if (!lockedValue.isValid()) lockedValue = item->getAttribute(AnnotationAttributeKey::Locked);
        const bool hasExplicitLocked = lockedValue.isValid();
        if (hasExplicitLocked) item->setLocked(lockedValue.toBool());

        const QVariant movableValue = item->getAttribute(AnnotationAttributeKey::Movable);
        if (movableValue.isValid() && !movableValue.toBool()) item->setLocked(true);

        QVariant axisValue = item->property(AnnotationAttributeKey::MoveAxis);
        if (!axisValue.isValid()) axisValue = item->getAttribute(AnnotationAttributeKey::MoveAxis);
        if (axisValue.isValid()) {
            item->setMoveAxis(static_cast<MoveAxis>(axisValue.toInt()));
            return;
        }

        switch (item->m_elementType) {
        case Type_Cp3:
        case Type_Chain:
            if (!hasExplicitLocked) item->setLocked(true);
            break;
        case Type_Sleeper:
            if (!hasExplicitLocked) item->setLocked(false);
            item->setMoveAxis(Axis_Vertical);
            break;
        case Type_SPRINGING_LOC:
            if (!hasExplicitLocked) item->setLocked(false);
            item->setMoveAxis(Axis_Vertical);
            break;
        case Type_Ring:
        case Type_Section:
        case Type_Platform:
            item->setMoveAxis(Axis_Horizontal);
            break;
        case Type_Disease:
        case Type_AUTORING:
        case Type_CustomOverlay:
        case Type_UserLine:
        default:
            item->setMoveAxis(Axis_Free);
            break;
        }
    }

private:
    QGraphicsScene* m_scene = nullptr;
    QHash<int, DefectShapeItem*> m_items;
    IDefectStorage* m_storage = nullptr;
    QMap<int, DefectStyle> m_styles;
    QString m_currentUri;
    QMap<int, DefectTypeConfig> m_configs;
    QMap<QString, DefectTypeConfig> m_configsByTypeKey;
    QHash<QString, AnnotationItemCreator> m_itemFactories;
    QHash<int, qreal> m_itemOriginalZValues;
    QHash<int, bool> m_itemOriginalVisibility;
    AnnotationLayerState m_layerState;
    QString m_layerKey;
    ElementType m_legacyElementType = Type_Disease;
    ElementTypePersistenceProfile m_persistenceProfile = ElementTypePersistenceProfile::UnifiedModern;
};

/* 新代码使用 AnnotationManager，老代码继续使用 DefectManager。 */
using AnnotationManager = DefectManager;
