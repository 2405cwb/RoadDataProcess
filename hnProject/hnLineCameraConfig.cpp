#include "hnLineCameraConfig.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>
#include <QSaveFile>
#include <QSettings>
#include <QTextStream>

namespace hnPro
{
	namespace
	{
		QString settingValueWithoutComment(const QVariant& value)
		{
			QString text = value.toString().trimmed();
			int commentPosition = -1;
			const QChar commentMarkers[] = { QLatin1Char(';'), QChar(0xFF1B), QLatin1Char('#') };
			for (const QChar marker : commentMarkers)
			{
				const int position = text.indexOf(marker);
				if (position >= 0 && (commentPosition < 0 || position < commentPosition))
				{
					commentPosition = position;
				}
			}
			if (commentPosition >= 0)
			{
				text.truncate(commentPosition);
			}
			return text.trimmed();
		}

		double readNumber(QSettings& settings, const QString& key,
			const QString& legacyKey, bool& ok, double defaultValue = 0.0)
		{
			QVariant value;
			if (settings.contains(key))
			{
				value = settings.value(key);
			}
			else if (!legacyKey.isEmpty() && settings.contains(legacyKey))
			{
				value = settings.value(legacyKey);
			}
			else
			{
				ok = false;
				return defaultValue;
			}
			return settingValueWithoutComment(value).toDouble(&ok);
		}

		hnLineCameraModuleInfo loadCameraModule(const QString& projectRoot, int cameraIndex)
		{
			hnLineCameraModuleInfo module;
			module.cameraIndex = cameraIndex;
			module.configPath = hnLineCameraConfig::cameraModuleConfigPath(projectRoot, cameraIndex);
			module.configExists = QFileInfo::exists(module.configPath);
			if (!module.configExists)
			{
				return module;
			}

			QSettings settings(module.configPath, QSettings::IniFormat);
			settings.setIniCodec("UTF-8");
			settings.beginGroup(QStringLiteral("Cam"));
			bool widthOk = false;
			bool heightOk = false;
			bool diffOk = false;
			module.mmPerPixelWidth = readNumber(settings,
				QStringLiteral("MmPerPix_W"), QStringLiteral("MmPerPix W"), widthOk);
			module.mmPerPixelHeight = readNumber(settings,
				QStringLiteral("MmPerPix_H"), QStringLiteral("MmPerPix H"), heightOk);
			module.diffValueMm = readNumber(settings,
				QStringLiteral("DiffValue"), QString(), diffOk);
			settings.endGroup();

			if (!widthOk || !qIsFinite(module.mmPerPixelWidth) || module.mmPerPixelWidth <= 0.0)
			{
				module.errorMessage = QStringLiteral("%1 的 MmPerPix_W 必须是大于 0 的数字。")
					.arg(module.configPath);
				return module;
			}
			if (!heightOk || !qIsFinite(module.mmPerPixelHeight) || module.mmPerPixelHeight <= 0.0)
			{
				module.errorMessage = QStringLiteral("%1 的 MmPerPix_H 必须是大于 0 的数字。")
					.arg(module.configPath);
				return module;
			}
			if (!diffOk || !qIsFinite(module.diffValueMm))
			{
				module.errorMessage = QStringLiteral("%1 的 DiffValue 不是有效数字。")
					.arg(module.configPath);
				return module;
			}

			module.configValid = true;
			return module;
		}
	}

	QString hnLineCameraConfig::cameraConfigPath(const QString& projectRoot)
	{
		return cameraModuleConfigPath(projectRoot, 0);
	}

	QString hnLineCameraConfig::cameraModuleConfigPath(const QString& projectRoot, int cameraIndex)
	{
		return QDir(projectRoot).filePath(
			QStringLiteral("RoadImg/Camera%1/LineCamSetting.ini").arg(cameraIndex));
	}

	QString hnLineCameraConfig::validAreaConfigPath(const QString& projectRoot)
	{
		return QDir(projectRoot).filePath(QStringLiteral("RoadImg/LineCamValidArea.ini"));
	}

	bool hnLineCameraConfig::isLineCameraProject(const QString& projectRoot)
	{
		return QFileInfo::exists(cameraConfigPath(projectRoot));
	}

	QString hnLineCameraConfig::findPreviewImage(const QString& projectRoot)
	{
		const QString roadImageRoot = QDir(projectRoot).filePath(QStringLiteral("RoadImg"));
		QDirIterator iterator(roadImageRoot,
			QStringList() << QStringLiteral("*.jpg") << QStringLiteral("*.jpeg")
				<< QStringLiteral("*.png") << QStringLiteral("*.bmp"),
			QDir::Files, QDirIterator::Subdirectories);
		return iterator.hasNext() ? iterator.next() : QString();
	}

	hnLineCameraInfo hnLineCameraConfig::load(const QString& projectRoot)
	{
		hnLineCameraInfo info;
		info.projectRoot = QDir::cleanPath(projectRoot);
		const QString cameraPath = cameraConfigPath(info.projectRoot);
		info.isLineCamera = QFileInfo::exists(cameraPath);
		if (!info.isLineCamera)
		{
			return info;
		}

		info.camera0 = loadCameraModule(info.projectRoot, 0);
		info.camera1 = loadCameraModule(info.projectRoot, 1);
		if (!info.camera0.configValid)
		{
			info.errorMessage = info.camera0.errorMessage;
			return info;
		}
		// 当前版本的比例和有效区域始终以左侧 Camera0 为准。
		// Camera1 与 DiffValue 仅保存在模型中，暂不参与拼接或坐标偏移。
		info.mmPerPixelWidth = info.camera0.mmPerPixelWidth;
		info.mmPerPixelHeight = info.camera0.mmPerPixelHeight;
		info.diffValueMm = info.camera0.diffValueMm;

		info.previewImagePath = findPreviewImage(info.projectRoot);
		if (info.previewImagePath.isEmpty())
		{
			info.errorMessage = QStringLiteral("RoadImg 中没有找到可用于设置有效区域的路面图片。");
			return info;
		}
		QImageReader reader(info.previewImagePath);
		const QSize imageSize = reader.size();
		if (!imageSize.isValid())
		{
			info.errorMessage = QStringLiteral("无法读取线阵路面图片尺寸：%1").arg(info.previewImagePath);
			return info;
		}
		info.imageWidth = imageSize.width();
		info.imageHeight = imageSize.height();
		info.cameraConfigValid = true;

		QSettings areaSettings(validAreaConfigPath(info.projectRoot), QSettings::IniFormat);
		areaSettings.beginGroup(QStringLiteral("ValidArea"));
		bool leftOk = false;
		bool rightOk = false;
		bool imageWidthOk = false;
		const int version = areaSettings.value(QStringLiteral("Version"), 0).toInt();
		const int left = areaSettings.value(QStringLiteral("LeftPixel")).toInt(&leftOk);
		const int right = areaSettings.value(QStringLiteral("RightPixel")).toInt(&rightOk);
		const int configuredImageWidth = areaSettings.value(QStringLiteral("ImageWidth")).toInt(&imageWidthOk);
		areaSettings.endGroup();
		if (version == 1 && leftOk && rightOk && imageWidthOk && configuredImageWidth == info.imageWidth &&
			left >= 0 && left < right && right <= info.imageWidth)
		{
			info.leftPixel = left;
			info.rightPixel = right;
			info.validAreaConfigured = true;
		}
		return info;
	}

	bool hnLineCameraConfig::saveValidArea(const hnLineCameraInfo& info, QString* errorMessage)
	{
		if (!info.isLineCamera || !info.cameraConfigValid || info.imageWidth <= 0 ||
			info.leftPixel < 0 || info.leftPixel >= info.rightPixel || info.rightPixel > info.imageWidth)
		{
			if (errorMessage)
			{
				*errorMessage = QStringLiteral("线阵相机有效区域参数无效。");
			}
			return false;
		}

		QSaveFile output(validAreaConfigPath(info.projectRoot));
		if (!output.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			if (errorMessage)
			{
				*errorMessage = QStringLiteral("无法写入有效区域配置：%1").arg(output.fileName());
			}
			return false;
		}
		QTextStream stream(&output);
		stream.setCodec("UTF-8");
		stream << QStringLiteral("[ValidArea]\n");
		stream << QStringLiteral("Version=1\n");
		stream << QStringLiteral("LeftPixel=") << info.leftPixel << QLatin1Char('\n');
		stream << QStringLiteral("RightPixel=") << info.rightPixel << QLatin1Char('\n');
		stream << QStringLiteral("ImageWidth=") << info.imageWidth << QLatin1Char('\n');
		stream.flush();
		if (!output.commit())
		{
			if (errorMessage)
			{
				*errorMessage = QStringLiteral("提交有效区域配置失败：%1").arg(output.fileName());
			}
			return false;
		}
		return true;
	}
}
