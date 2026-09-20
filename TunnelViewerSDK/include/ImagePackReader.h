#ifndef IMAGE_PACK_READER_H
#define IMAGE_PACK_READER_H

#include <QByteArray>
#include <QString>
#include <QtGlobal>
#include <QVector>

#include "../third_party/ImagePackSDK/include/ImagePackApi.h"

/*
 * TunnelViewerSDK 对当前 ImagePackSDK V1.1.1 的轻量读取封装。
 *
 * ImagePackSDK 当前 Writer 生成：
 *   img_<tag>.jph
 *   img_<tag>_0001.jpd
 *   img_<tag>_0002.jpd ...
 *
 * 当前 Reader 同时支持兼容命名：
 *   img-<tag>.jph
 *   0001-img-<tag>.jpd
 *   0002-img-<tag>.jpd ...
 *
 * TunnelViewerSDK 不自行解析 .jpd 文件名。
 * 数据分卷的定位和兼容规则统一交给官方 ImagePack.dll。
 *
 * Pack 与普通单图的差异仅存在于图片读取层；
 * JPEG 读取完成后，两者共用相同的缩略图、高清图、
 * 缓存、预加载、滚动和绘制流程。
 */
struct PACK_FRAME_INFO
{
    quint64 globalIndex = 0;
    quint64 sourceIndex = 0;
    quint64 timeValue = 0;
    quint32 dataFileNumber = 0;
    quint64 dataOffset = 0;
    quint32 jpegSize = 0;
};

class ImagePackReader
{
public:
    // packPath 可以是当前 Pack 的 .jph 文件，也可以是只包含一个当前 Pack 的目录。
    explicit ImagePackReader(const QString& packPath, bool verifyOnOpen = false);
    ~ImagePackReader();

    quint64 count() const;

    // 一次分页取回轻量索引，避免十几万帧逐条跨 DLL 查询。
    QVector<PACK_FRAME_INFO> frameInfos() const;

    // 只读取指定帧 JPEG 原始字节，不在 Reader 层做缩放、缓存或显示处理。
    QByteArray readJpeg(quint64 globalIndex) const;

    QString indexPath() const { return m_indexPath; }
    QString packRoot() const { return m_packRoot; }

    // 支持 ImagePackSDK V1.1.1 Reader 支持的
    // img_<tag>.jph 和 img-<tag>.jph。
    // 目录存在多个 Pack 时返回空，要求调用方明确指定具体 .jph。
    static QString resolveIndexPath(const QString& packPath);
    static bool isSupportedPackPath(const QString& packPath);

private:
    ImagePackReader(const ImagePackReader&);
    ImagePackReader& operator=(const ImagePackReader&);

    static QString resultText(IMAGEPACK_HANDLE handle, int resultCode);
    void ensureOk(int resultCode, const char* action) const;

    IMAGEPACK_HANDLE m_handle = nullptr;
    QString m_indexPath;
    QString m_packRoot;
    quint64 m_count = 0;
};

#endif // IMAGE_PACK_READER_H
