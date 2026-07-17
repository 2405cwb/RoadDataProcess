#pragma once

#include "hnproject_global.h"

#include <QString>
#include <QtGlobal>

namespace hnPro
{
	// 单个线阵相机模块的采集配置。当前业务仅使用 Camera0，Camera1
	// 作为后续双相机扩展接口保留，不参与当前的比例、有效区域和病害计算。
	struct HNPROJECT_EXPORT hnLineCameraModuleInfo
	{
		int cameraIndex = -1;
		bool configExists = false;
		bool configValid = false;
		double mmPerPixelWidth = 0.0;
		double mmPerPixelHeight = 0.0;
		double diffValueMm = 0.0;
		QString configPath;
		QString errorMessage;
	};

	struct HNPROJECT_EXPORT hnLineCameraInfo
	{
		bool isLineCamera = false;
		bool cameraConfigValid = false;
		bool validAreaConfigured = false;
		double mmPerPixelWidth = 0.0;
		double mmPerPixelHeight = 0.0;
		double diffValueMm = 0.0;
		int imageWidth = 0;
		int imageHeight = 0;
		int leftPixel = 0;
		int rightPixel = 0;
		QString projectRoot;
		QString previewImagePath;
		QString errorMessage;
		hnLineCameraModuleInfo camera0;
		hnLineCameraModuleInfo camera1;

		double meterPerPixelWidth() const { return mmPerPixelWidth / 1000.0; }
		double meterPerPixelHeight() const { return mmPerPixelHeight / 1000.0; }
		double roadWidthMeters() const
		{
			return validAreaConfigured ? (rightPixel - leftPixel) * meterPerPixelWidth() : 0.0;
		}
		double imageLengthMeters() const
		{
			return imageHeight > 0 ? imageHeight * meterPerPixelHeight() : 0.0;
		}
		bool containsPixelX(double pixelX) const
		{
			return validAreaConfigured && pixelX >= leftPixel && pixelX <= rightPixel;
		}
		double roadXFromPixel(double pixelX) const
		{
			return qBound(0.0, (pixelX - leftPixel) * meterPerPixelWidth(), roadWidthMeters());
		}
		double distanceFromRight(double pixelX) const
		{
			return qBound(0.0, (rightPixel - pixelX) * meterPerPixelWidth(), roadWidthMeters());
		}
		bool hasSecondaryCamera() const { return camera1.configExists; }
		bool centeredBoundsForRoadWidth(double requestedWidthMeters, int& centeredLeft, int& centeredRight) const
		{
			const double metersPerPixel = meterPerPixelWidth();
			if (!qIsFinite(requestedWidthMeters) || requestedWidthMeters <= 0.0 ||
				metersPerPixel <= 0.0 || imageWidth <= 0 ||
				requestedWidthMeters > imageWidth * metersPerPixel + 1e-9)
			{
				return false;
			}
			const double center = imageWidth / 2.0;
			const double halfPixels = requestedWidthMeters / metersPerPixel / 2.0;
			const int left = qRound(center - halfPixels);
			const int right = qRound(center + halfPixels);
			if (left < 0 || left >= right || right > imageWidth) return false;
			centeredLeft = left;
			centeredRight = right;
			return true;
		}
	};

	class HNPROJECT_EXPORT hnLineCameraConfig
	{
	public:
		// 兼容现有调用：cameraConfigPath() 始终返回左侧 Camera0 配置。
		static QString cameraConfigPath(const QString& projectRoot);
		static QString cameraModuleConfigPath(const QString& projectRoot, int cameraIndex);
		static QString validAreaConfigPath(const QString& projectRoot);
		static bool isLineCameraProject(const QString& projectRoot);
		static QString findPreviewImage(const QString& projectRoot);
		static hnLineCameraInfo load(const QString& projectRoot);
		static bool saveValidArea(const hnLineCameraInfo& info, QString* errorMessage = nullptr);
	};
}
