#pragma once

#include <QtGui/QPainterPath>
#include <QRect>
#include <QRectF>
#include <QString>
#include <QVector>

namespace hnApp
{
	struct LittleFrameRenderCell
	{
		QString imageName;
		QRect sourceRect;
		QRectF sceneRect;

		bool isValid() const
		{
			return !imageName.isEmpty() && sourceRect.isValid() && !sourceRect.isNull() &&
				sceneRect.isValid() && !sceneRect.isNull();
		}
	};

	enum LittleFrameRenderMode
	{
		LittleFrameRenderExactCells,
		LittleFrameRenderRowRuns
	};

	struct LittleFrameRenderResult
	{
		QPainterPath path;
		LittleFrameRenderMode mode = LittleFrameRenderExactCells;
		int segmentCount = 0;
	};

	class LittleFrameRenderPathBuilder
	{
	public:
		static const int ExactCellLimit = 256;

		static LittleFrameRenderResult build(const QVector<LittleFrameRenderCell>& cells);
	};
}
