#pragma once

#include "..\hnCommon\hnRoadStruct.h"

#include <QString>
#include <QStringList>
#include <functional>
#include <vector>

struct ProjectImportValidationResult
{
	ProjectImportValidationResult();

	QStringList errors;
	QStringList warnings;
	// Non-blocking batch notices are collected and shown once by the UI.
	QStringList notices;
	qint64 imageFileCount;

	bool isValid() const;
};

class hnProjectImportValidator
{
public:
	typedef std::function<void(int, int, const QString&, qint64)> ProgressCallback;

	ProjectImportValidationResult validate(
		const std::vector<hnCommon::hnProjectDataInfo>& projects,
		const ProgressCallback& progressCallback = ProgressCallback()) const;

private:
	QString projectDisplayName(const hnCommon::hnProjectDataInfo& project, int projectIndex) const;
	QString twoDProjectPath(const hnCommon::hnProjectDataInfo& project) const;
	QString threeDProjectPath(const hnCommon::hnProjectDataInfo& project) const;
	void validateRequiredDirectory(
		const QString& projectName,
		const QString& description,
		const QString& path,
		ProjectImportValidationResult& result) const;
	void validateRequiredFile(
		const QString& projectName,
		const QString& description,
		const QString& path,
		ProjectImportValidationResult& result) const;
	void validateTwoDProject(
		const hnCommon::hnProjectDataInfo& project,
		int projectIndex,
		int projectCount,
		ProjectImportValidationResult& result,
		const ProgressCallback& progressCallback) const;
	void validateThreeDProject(
		const hnCommon::hnProjectDataInfo& project,
		int projectIndex,
		ProjectImportValidationResult& result) const;
	qint64 validateImageChannel(
		const QString& projectName,
		const QString& channelPath,
		int projectIndex,
		int projectCount,
		qint64 currentProjectImageCount,
		ProjectImportValidationResult& result,
		const ProgressCallback& progressCallback) const;
	void addError(
		ProjectImportValidationResult& result,
		const QString& projectName,
		const QString& message,
		const QString& path = QString()) const;
	void addNotice(
		ProjectImportValidationResult& result,
		const QString& projectName,
		const QString& message,
		const QString& path = QString()) const;
};
