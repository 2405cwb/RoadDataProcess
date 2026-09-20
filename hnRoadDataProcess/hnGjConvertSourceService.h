#pragma once

#include <functional>
#include <vector>
#include <QString>
#include <QStringList>
#include <QVector>

namespace hnPro
{
	class hnProject;
}

// 当前开放的国检数据提交标准。新增标准时统一在服务内部扩展规则，界面不直接拼目录或文件名。
enum class hnGjExportStandard
{
	Standard2025,
	NationalRoad2026
};

// 用户在规范模式下选择的输出项。设备不支持的 SFC/SSR 不加入可选项。
struct hnGjOutputSelection
{
	bool dr;
	bool iri;
	bool rd;
	bool pb;
	bool mpd;
	bool smtd;
	bool lbiFile;
	bool haFile;
	bool riFile;
	bool rdFile;
	bool ttFile;
	bool lFile;

	hnGjOutputSelection();
	static hnGjOutputSelection allFor(hnGjExportStandard standard);
};

// 单工程生成结果。该结构只在主程序内部使用，不扩展对外 SDK。
struct hnGjProjectResult
{
	hnGjProjectResult();

	QString projectName;
	QString outputPath;
	QString errorMessage;
	QStringList warnings;
	QStringList skippedOutputs;
	int sourceIndex;
	bool success;
	bool skipped;
};

// 批量生成结果，用于界面汇总成功、失败、跳过和取消状态。
struct hnGjBatchResult
{
	hnGjBatchResult();

	QVector<hnGjProjectResult> projects;
	QStringList preflightErrors;
	bool canceled;

	int successCount() const;
	int completeSuccessCount() const;
	int partialSuccessCount() const;
	int failedCount() const;
	int skippedCount() const;
};

// 2025 通用国检转换中间数据服务。
class hnGjConvertSourceService
{
public:
	typedef std::function<bool(int, int, const QString&)> ProgressCallback;

	// 返回国检标准在界面和结果摘要中使用的名称。
	static QString standardDisplayName(hnGjExportStandard standard);

	// 从 ProjectInfo.txt 读取县级行政代码；读取失败时返回空字符串。
	static QString readCountyCode(hnPro::hnProject* project);

	// 逐工程预检并导出可用工程；单个工程失败不阻断同批次其他工程。
	hnGjBatchResult exportBatch(
		const std::vector<hnPro::hnProject*>& projects,
		const QString& batchCountyCode,
		hnGjExportStandard exportStandard,
		const hnGjOutputSelection& outputSelection,
		const ProgressCallback& progressCallback) const;
};
