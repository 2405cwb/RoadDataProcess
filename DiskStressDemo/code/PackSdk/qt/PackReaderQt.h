#pragma once

#include "../PackSdk.h"

#include <QByteArray>
#include <QString>
#include <stdexcept>

class PackReaderQt
{
public:
	// 打开图片包目录，并在构造函数内创建 SDK handle。
	// Windows 下走宽字符接口，Ubuntu/Linux 下走 UTF-8 接口。
	explicit PackReaderQt(const QString& rootDir, bool verifyOnOpen = false)
		: m_handle(nullptr)
	{
		PACK_OPEN_OPTIONS options;
		options.structSize = sizeof(options);
		options.flags = verifyOnOpen ? PACK_OPEN_VERIFY_ON_OPEN : PACK_OPEN_DEFAULT;

#if defined(_WIN32)
		int ret = Pack_Open(reinterpret_cast<const wchar_t*>(rootDir.utf16()), &options, &m_handle);
#else
		QByteArray path = rootDir.toUtf8();
		int ret = Pack_OpenUtf8(path.constData(), &options, &m_handle);
#endif
		if (ret != PACK_OK)
		{
			throw std::runtime_error(lastError(nullptr).toStdString());
		}
	}

	// 析构时自动关闭 SDK handle，避免调用方忘记 Pack_Close。
	~PackReaderQt()
	{
		if (m_handle != nullptr)
		{
			Pack_Close(m_handle);
			m_handle = nullptr;
		}
	}

	PackReaderQt(const PackReaderQt&) = delete;
	PackReaderQt& operator=(const PackReaderQt&) = delete;

	// 返回图片总数，用于替代旧软件里的“jpg 文件数量”。
	quint64 count() const
	{
		quint64 value = 0;
		check(Pack_GetFrameCount(m_handle, reinterpret_cast<uint64_t*>(&value)));
		return value;
	}

	// 返回数据集摘要，例如 pack 数量、总帧数、时间范围、警告数量。
	PACK_DATASET_INFO datasetInfo() const
	{
		PACK_DATASET_INFO info;
		check(Pack_GetDatasetInfo(m_handle, &info));
		return info;
	}

	// 返回指定全局序号的帧信息，不读取 JPEG 数据。
	PACK_FRAME_INFO frameInfo(quint64 globalIndex) const
	{
		PACK_FRAME_INFO info;
		check(Pack_GetFrameInfo(m_handle, globalIndex, &info));
		return info;
	}

	// 通过采集帧号 sourceIndex 查找全局序号。
	quint64 findBySourceIndex(quint64 sourceIndex) const
	{
		uint64_t globalIndex = 0;
		check(Pack_FindBySourceIndex(m_handle, sourceIndex, &globalIndex));
		return globalIndex;
	}

	// 读取指定图片的 JPEG 字节。Qt 调用方可直接 QImage::loadFromData(bytes, "JPG")。
	QByteArray readJpeg(quint64 globalIndex) const
	{
		uint32_t required = 0;
		check(Pack_ReadJpeg(m_handle, globalIndex, nullptr, 0, &required));
		QByteArray bytes;
		bytes.resize(static_cast<int>(required));
		check(Pack_ReadJpeg(m_handle, globalIndex, bytes.data(), required, &required));
		return bytes;
	}

	// 保存指定图片为单张 jpg 文件。
	void saveJpeg(quint64 globalIndex, const QString& outputPath) const
	{
#if defined(_WIN32)
		check(Pack_SaveJpeg(m_handle, globalIndex, reinterpret_cast<const wchar_t*>(outputPath.utf16())));
#else
		QByteArray path = outputPath.toUtf8();
		check(Pack_SaveJpegUtf8(m_handle, globalIndex, path.constData()));
#endif
	}

	// 批量导出全部图片，默认导出成旧 Image_0000/000_time.jpg 目录结构。
	void exportAll(const QString& outputDir, PACK_NAMING_MODE namingMode = PACK_NAMING_LEGACY_DIRS) const
	{
#if defined(_WIN32)
		check(Pack_ExportAll(m_handle, reinterpret_cast<const wchar_t*>(outputDir.utf16()), namingMode, nullptr, nullptr));
#else
		QByteArray path = outputDir.toUtf8();
		check(Pack_ExportAllUtf8(m_handle, path.constData(), namingMode, nullptr, nullptr));
#endif
	}

	// 调用 SDK 校验 idx/dat 一致性。
	PACK_VERIFY_REPORT verify() const
	{
		PACK_VERIFY_REPORT report;
		check(Pack_Verify(m_handle, &report));
		return report;
	}

	// 暴露原生 handle，供示例程序调用高级 C ABI，例如带进度回调的批量导出。
	PACK_HANDLE nativeHandle() const
	{
		return m_handle;
	}

private:
	// 获取最近一次 SDK 错误并转换成 QString。
	static QString lastError(PACK_HANDLE handle)
	{
#if defined(_WIN32)
		const wchar_t* text = Pack_GetLastError(handle);
		return text == nullptr ? QString() : QString::fromWCharArray(text);
#else
		const char* text = Pack_GetLastErrorUtf8(handle);
		return text == nullptr ? QString() : QString::fromUtf8(text);
#endif
	}

	// 检查 SDK 返回值，失败时抛出 std::runtime_error，方便 Qt 业务代码统一处理。
	void check(int result) const
	{
		if (result != PACK_OK)
		{
			throw std::runtime_error(lastError(m_handle).toStdString());
		}
	}

	PACK_HANDLE m_handle;
};
