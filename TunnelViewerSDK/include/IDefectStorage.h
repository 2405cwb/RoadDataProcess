#pragma once

#include "TunnelGlobal.h"
#include <QList>

class IDefectStorage {
public:
    /*
     * 兼容约定：旧业务只读写原字段仍可继续工作；
     * 若使用 typeKey/layerKey、自定义 Item Factory 等新扩展能力，Storage 必须原样保存/恢复 DefectData::attributes。
     * SDK 不新增纯虚函数，避免破坏现有外部 Storage 实现。
     */
    /* 存储接口由外部实现 所以析构必须 virtual */ virtual ~IDefectStorage() = default;
    /* 全量读取当前连接里的标注 */ virtual QList<DefectData> loadAll(const QString& uri) = 0;
    /* 全量保存当前标注 */ virtual bool saveAll(const QList<DefectData>& defects, const QString& uri) = 0;
    /* 增量新增一条 默认成功 简单存储可以不实现 */ virtual bool addOne(const DefectData& defect, const QString& uri) {
        Q_UNUSED(defect);
        Q_UNUSED(uri);
        return true;
    }
    /* 增量删除一条 默认成功 简单存储可以不实现 */ virtual bool deleteOne(const QString& uuid, const QString& uri) {
        Q_UNUSED(uuid);
        Q_UNUSED(uri);
        return true;
    }
    /* 增量更新一条 默认成功 简单存储可以不实现 */ virtual bool updateOne(const DefectData& defect, const QString& uri) {
        Q_UNUSED(defect);
        Q_UNUSED(uri);
        return true;
    }
    /* 按线性范围加载 参数名先兼容旧 mile 叫法 */ virtual QList<DefectData> loadByRange(const QString& uri, double startMile, double endMile) {
        Q_UNUSED(uri);
        Q_UNUSED(startMile);
        Q_UNUSED(endMile);
        return QList<DefectData>();
    }
};

/* 新代码可以用 IAnnotationStorage 老代码继续用 IDefectStorage */ using IAnnotationStorage = IDefectStorage;
