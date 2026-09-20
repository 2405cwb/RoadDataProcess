#include "ImagePackReader.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QLibrary>
#include <QStringList>

#include <climits>
#include <stdexcept>
#include <string>

namespace {
QString absoluteCleanPath(const QString& path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

/*
 * TunnelViewerSDK 本身是静态库。这里不直接链接 ImagePack.lib，而是运行时加载
 * 官方 ImagePack.dll 的稳定 C ABI。这样三个业务软件无需各自增加 ImagePack.lib
 * 链接项；只要最终 EXE 目录存在 ImagePack.dll 即可。
 */
struct ImagePackApiDyn
{
    typedef IMAGEPACK_U32 (IMAGEPACK_CALL *FnGetApiVersion)(void);
    typedef const char* (IMAGEPACK_CALL *FnGetApiVersionString)(void);
    typedef const char* (IMAGEPACK_CALL *FnGetResultDescription)(IMAGEPACK_I32);
    typedef IMAGEPACK_HANDLE (IMAGEPACK_CALL *FnCreate)(void);
    typedef void (IMAGEPACK_CALL *FnDestroy)(IMAGEPACK_HANDLE);
    typedef int (IMAGEPACK_CALL *FnGetLastError)(IMAGEPACK_HANDLE, char*, IMAGEPACK_U32, IMAGEPACK_U32*);
    typedef int (IMAGEPACK_CALL *FnOpenReader)(IMAGEPACK_HANDLE, const char*);
    typedef int (IMAGEPACK_CALL *FnClose)(IMAGEPACK_HANDLE);
    typedef int (IMAGEPACK_CALL *FnGetImageCount)(IMAGEPACK_HANDLE, IMAGEPACK_U64*);
    typedef int (IMAGEPACK_CALL *FnGetImageList)(IMAGEPACK_HANDLE, IMAGEPACK_U64, IMAGEPACK_U64, IMAGEPACK_API_IMAGE_INFO*, IMAGEPACK_U64, IMAGEPACK_U64*, IMAGEPACK_U64*);
    typedef int (IMAGEPACK_CALL *FnGetJpegSize)(IMAGEPACK_HANDLE, IMAGEPACK_U64, IMAGEPACK_U32*);
    typedef int (IMAGEPACK_CALL *FnReadJpeg)(IMAGEPACK_HANDLE, IMAGEPACK_U64, IMAGEPACK_U8*, IMAGEPACK_U32, IMAGEPACK_U32*);
    typedef int (IMAGEPACK_CALL *FnVerify)(IMAGEPACK_HANDLE, IMAGEPACK_I32, IMAGEPACK_API_VERIFY_REPORT*, IMAGEPACK_PROGRESS_CALLBACK, void*);

    QLibrary* library = nullptr;
    QString error;
    FnGetApiVersion getApiVersion = nullptr;
    FnGetApiVersionString getApiVersionString = nullptr;
    FnGetResultDescription getResultDescription = nullptr;
    FnCreate create = nullptr;
    FnDestroy destroy = nullptr;
    FnGetLastError getLastError = nullptr;
    FnOpenReader openReader = nullptr;
    FnClose close = nullptr;
    FnGetImageCount getImageCount = nullptr;
    FnGetImageList getImageList = nullptr;
    FnGetJpegSize getJpegSize = nullptr;
    FnReadJpeg readJpeg = nullptr;
    FnVerify verify = nullptr;

    ImagePackApiDyn()
    {
        QStringList candidates;
        if (QCoreApplication::instance()) {
            candidates << QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("ImagePack.dll"));
        }
        // 兼容调用方通过 Windows DLL 搜索路径部署的情况。
        candidates << QStringLiteral("ImagePack.dll") << QStringLiteral("ImagePack");
        candidates.removeDuplicates();

        for (const QString& candidate : candidates) {
            QLibrary* one = new QLibrary(candidate);
            if (one->load()) {
                library = one;
                break;
            }
            error = one->errorString();
            delete one;
        }
        if (!library) {
            if (error.isEmpty()) {
                error = QString::fromUtf8("ImagePack.dll \xE6\x9C\xAA\xE6\x89\xBE\xE5\x88\xB0");
            }
            return;
        }

#define RESOLVE_IMAGEPACK(member, symbol, type) \
        member = reinterpret_cast<type>(library->resolve(symbol)); \
        if (!member) { error = QStringLiteral("ImagePack.dll 缺少导出函数: ") + QString::fromLatin1(symbol); return; }

        RESOLVE_IMAGEPACK(getApiVersion, "ImagePack_GetApiVersion", FnGetApiVersion);
        RESOLVE_IMAGEPACK(getApiVersionString, "ImagePack_GetApiVersionString", FnGetApiVersionString);
        RESOLVE_IMAGEPACK(getResultDescription, "ImagePack_GetResultDescription", FnGetResultDescription);
        RESOLVE_IMAGEPACK(create, "ImagePack_Create", FnCreate);
        RESOLVE_IMAGEPACK(destroy, "ImagePack_Destroy", FnDestroy);
        RESOLVE_IMAGEPACK(getLastError, "ImagePack_GetLastError", FnGetLastError);
        RESOLVE_IMAGEPACK(openReader, "ImagePack_OpenReader", FnOpenReader);
        RESOLVE_IMAGEPACK(close, "ImagePack_Close", FnClose);
        RESOLVE_IMAGEPACK(getImageCount, "ImagePack_GetImageCount", FnGetImageCount);
        RESOLVE_IMAGEPACK(getImageList, "ImagePack_GetImageList", FnGetImageList);
        RESOLVE_IMAGEPACK(getJpegSize, "ImagePack_GetJpegSize", FnGetJpegSize);
        RESOLVE_IMAGEPACK(readJpeg, "ImagePack_ReadJpeg", FnReadJpeg);
        RESOLVE_IMAGEPACK(verify, "ImagePack_Verify", FnVerify);
#undef RESOLVE_IMAGEPACK
    }

    ~ImagePackApiDyn()
    {
        if (library) {
            library->unload();
            delete library;
            library = nullptr;
        }
    }

    bool valid() const
    {
        return library && error.isEmpty();
    }
};

ImagePackApiDyn& imagePackApi()
{
    // 故意让 DLL 适配器存活到进程退出，避免 Qt 线程局部 Reader 在静态析构阶段
    // 比 QLibrary 更晚释放时出现卸载顺序问题。该对象仅一个，进程退出由 OS 回收。
    static ImagePackApiDyn* api = new ImagePackApiDyn();
    return *api;
}
}

QString ImagePackReader::resolveIndexPath(const QString& packPath)
{
    const QFileInfo input(packPath);

    if (!input.exists())
        return QString();

    /*
     * 当前 ImagePackSDK V1.1.1 Reader 支持两套 Pack 文件命名：
     *
     * 新命名：
     *   img_xxx.jph
     *   img_xxx_0001.jpd
     *
     * 兼容命名：
     *   img-xxx.jph
     *   0001-img-xxx.jpd
     *
     * 注意：
     * TunnelViewerSDK 这里只负责找到 .jph 索引文件。
     *
     * 至于实际读取时使用：
     *   img_xxx_0001.jpd
     * 还是
     *   0001-img-xxx.jpd
     *
     * 完全交给官方 ImagePack.dll 处理。
     * ImagePackSDK V1.1.1 自身已经实现了两种数据分卷名称的自动 fallback，
     * 此处不要重复实现文件名判断，否则以后容易和 ImagePackSDK 本身产生分叉。
     */
    auto isSupportedIndexName = [](const QFileInfo& info) -> bool
        {
            if (!info.isFile())
                return false;

            if (info.suffix().compare(QStringLiteral("jph"),
                Qt::CaseInsensitive) != 0)
            {
                return false;
            }

            const QString name = info.fileName();

            // 当前正式命名：
            // img_xxx.jph
            const bool currentName =
                name.startsWith(QStringLiteral("img_"),
                    Qt::CaseInsensitive);

            // ImagePackSDK V1.1.1 保留兼容的命名：
            // img-xxx.jph
            const bool compatibleName =
                name.startsWith(QStringLiteral("img-"),
                    Qt::CaseInsensitive);

            return currentName || compatibleName;
        };

    /*
     * 情况一：
     * 调用方直接传入 .jph 文件。
     */
    if (input.isFile())
    {
        if (!isSupportedIndexName(input))
            return QString();

        return absoluteCleanPath(input.absoluteFilePath());
    }

    /*
     * 情况二：
     * 调用方传入 Pack 所在目录。
     *
     * 同时查找：
     *   img_*.jph
     *   img-*.jph
     */
    if (!input.isDir())
        return QString();

    QDir dir(input.absoluteFilePath());

    const QFileInfoList candidates =
        dir.entryInfoList(
            QStringList()
            << QStringLiteral("img_*.jph")
            << QStringLiteral("img-*.jph"),
            QDir::Files | QDir::Readable,
            QDir::Name);

    /*
     * 一个目录只允许存在一个 Pack。
     *
     * 如果有多个 .jph，不能偷偷任选一个，
     * 应让调用方明确传具体 .jph 文件。
     */
    if (candidates.size() != 1)
        return QString();

    return absoluteCleanPath(
        candidates.first().absoluteFilePath());
}

bool ImagePackReader::isSupportedPackPath(const QString& packPath)
{
    return !resolveIndexPath(packPath).isEmpty();
}

QString ImagePackReader::resultText(IMAGEPACK_HANDLE handle, int resultCode)
{
    ImagePackApiDyn& api = imagePackApi();
    if (!api.valid()) return api.error;

    IMAGEPACK_U32 required = 0;
    if (handle) api.getLastError(handle, nullptr, 0, &required);
    if (required > 1 && required <= static_cast<IMAGEPACK_U32>(INT_MAX)) {
        QByteArray buffer(static_cast<int>(required), '\0');
        if (api.getLastError(handle, buffer.data(), required, nullptr) == IMAGEPACK_OK)
            return QString::fromUtf8(buffer.constData());
    }

    const char* desc = api.getResultDescription(static_cast<IMAGEPACK_I32>(resultCode));
    return desc ? QString::fromUtf8(desc) : QStringLiteral("unknown ImagePack error");
}

void ImagePackReader::ensureOk(int resultCode, const char* action) const
{
    if (resultCode == IMAGEPACK_OK) return;
    const QString text = QString::fromLatin1(action) + QStringLiteral(": ") + resultText(m_handle, resultCode);
    throw std::runtime_error(text.toUtf8().constData());
}

ImagePackReader::ImagePackReader(const QString& packPath, bool verifyOnOpen)
{
    ImagePackApiDyn& api = imagePackApi();
    if (!api.valid()) {
        throw std::runtime_error((QStringLiteral("ImagePack.dll load failed: ") + api.error).toUtf8().constData());
    }

    // 防止最终 EXE 目录残留同名旧 DLL，造成“能启动但读包行为不一致”。
    const IMAGEPACK_U32 runtimeVersion = api.getApiVersion();
    const IMAGEPACK_U32 runtimeMajor = (runtimeVersion >> 16) & 0xFFU;
    if (runtimeMajor != IMAGEPACK_API_VERSION_MAJOR || runtimeVersion < IMAGEPACK_API_VERSION) {
        const QString actual = api.getApiVersionString ? QString::fromLatin1(api.getApiVersionString()) : QString::number(runtimeVersion);
        throw std::runtime_error((QStringLiteral("ImagePack.dll version too old. actual=") + actual
                                  + QStringLiteral(", required>=1.1.1")).toUtf8().constData());
    }

    m_indexPath = resolveIndexPath(packPath);
    if (m_indexPath.isEmpty())
    {
        throw std::runtime_error(
            "ImagePack index not found. "
            "Expected img_<tag>.jph or img-<tag>.jph");
    }
    m_packRoot = QFileInfo(m_indexPath).absolutePath();

    m_handle = api.create();
    if (!m_handle) throw std::runtime_error("ImagePack_Create failed");

    try {
        const QByteArray utf8Path = m_indexPath.toUtf8();
        ensureOk(api.openReader(m_handle, utf8Path.constData()), "ImagePack_OpenReader");

        IMAGEPACK_U64 countValue = 0;
        ensureOk(api.getImageCount(m_handle, &countValue), "ImagePack_GetImageCount");
        m_count = static_cast<quint64>(countValue);

        if (verifyOnOpen) {
            IMAGEPACK_API_VERIFY_REPORT report = {};
            report.structSize = static_cast<IMAGEPACK_U32>(sizeof(report));
            // 打开阶段只做结构校验，避免几十万张图片时因完整 JPEG 解码导致启动时间失控。
            ensureOk(api.verify(m_handle, IMAGEPACK_API_VERIFY_STRUCTURE,
                                &report, nullptr, nullptr), "ImagePack_Verify");
        }
    }
    catch (...) {
        if (m_handle) {
            api.close(m_handle);
            api.destroy(m_handle);
            m_handle = nullptr;
        }
        throw;
    }
}

ImagePackReader::~ImagePackReader()
{
    if (m_handle) {
        ImagePackApiDyn& api = imagePackApi();
        if (api.valid()) {
            api.close(m_handle);
            api.destroy(m_handle);
        }
        m_handle = nullptr;
    }
}

quint64 ImagePackReader::count() const
{
    return m_count;
}


QVector<PACK_FRAME_INFO> ImagePackReader::frameInfos() const
{
    QVector<PACK_FRAME_INFO> result;
    if (!m_handle || m_count == 0) return result;
    if (m_count > static_cast<quint64>(INT_MAX))
        throw std::runtime_error("Pack frame count exceeds Qt container limit");

    result.reserve(static_cast<int>(m_count));
    ImagePackApiDyn& api = imagePackApi();
    const IMAGEPACK_U64 pageSize = 4096;
    IMAGEPACK_U64 begin = 0;

    while (begin < static_cast<IMAGEPACK_U64>(m_count)) {
        const IMAGEPACK_U64 wanted = qMin(pageSize, static_cast<IMAGEPACK_U64>(m_count) - begin);
        QVector<IMAGEPACK_API_IMAGE_INFO> page(static_cast<int>(wanted));
        IMAGEPACK_U64 written = 0;
        IMAGEPACK_U64 totalMatched = 0;
        ensureOk(api.getImageList(m_handle, begin, wanted, page.data(), wanted, &written, &totalMatched),
                 "ImagePack_GetImageList");
        if (written == 0) break;

        for (IMAGEPACK_U64 i = 0; i < written; ++i) {
            const IMAGEPACK_API_IMAGE_INFO& sdkInfo = page.at(static_cast<int>(i));
            PACK_FRAME_INFO info;
            info.globalIndex = static_cast<quint64>(sdkInfo.globalIndex);
            info.sourceIndex = static_cast<quint64>(sdkInfo.sourceIndex);
            info.timeValue = static_cast<quint64>(sdkInfo.timeValue);
            info.dataFileNumber = static_cast<quint32>(sdkInfo.dataFileNumber);
            info.dataOffset = static_cast<quint64>(sdkInfo.dataOffset);
            info.jpegSize = static_cast<quint32>(sdkInfo.jpegSize);
            result.append(info);
        }
        begin += written;
    }

    if (static_cast<quint64>(result.size()) != m_count)
        throw std::runtime_error("ImagePack_GetImageList returned incomplete metadata");
    return result;
}

QByteArray ImagePackReader::readJpeg(quint64 globalIndex) const
{
    if (!m_handle || globalIndex >= m_count)
        throw std::runtime_error("Pack frame index out of range");

    ImagePackApiDyn& api = imagePackApi();
    IMAGEPACK_U32 jpegSize = 0;
    ensureOk(api.getJpegSize(m_handle, static_cast<IMAGEPACK_U64>(globalIndex), &jpegSize),
             "ImagePack_GetJpegSize");
    if (jpegSize == 0 || jpegSize > static_cast<IMAGEPACK_U32>(INT_MAX))
        throw std::runtime_error("Invalid JPEG size returned by ImagePack SDK");

    QByteArray bytes;
    bytes.resize(static_cast<int>(jpegSize));
    IMAGEPACK_U32 actual = 0;
    ensureOk(api.readJpeg(m_handle,
                          static_cast<IMAGEPACK_U64>(globalIndex),
                          reinterpret_cast<IMAGEPACK_U8*>(bytes.data()),
                          jpegSize,
                          &actual),
             "ImagePack_ReadJpeg");

    if (actual > jpegSize) throw std::runtime_error("ImagePack_ReadJpeg returned invalid size");
    bytes.resize(static_cast<int>(actual));
    return bytes;
}
