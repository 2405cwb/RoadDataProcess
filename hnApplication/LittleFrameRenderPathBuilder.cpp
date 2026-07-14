#include "LittleFrameRenderPathBuilder.h"

#include <QMap>
#include <QtGlobal>
#include <algorithm>

namespace
{
	struct LittleFrameRowRun
	{
		QString imageName;
		QRect sourceRect;
		QRectF sceneRect;
	};

	QString rowKey(const hnApp::LittleFrameRenderCell& cell)
	{
		const QRect rect = cell.sourceRect.normalized();
		return QString::fromLatin1("%1|%2|%3")
			.arg(cell.imageName)
			.arg(rect.top())
			.arg(rect.bottom());
	}

	bool cellLeftLessThan(const hnApp::LittleFrameRenderCell& lhs,
		const hnApp::LittleFrameRenderCell& rhs)
	{
		const QRect lhsRect = lhs.sourceRect.normalized();
		const QRect rhsRect = rhs.sourceRect.normalized();
		if (lhsRect.left() != rhsRect.left())
		{
			return lhsRect.left() < rhsRect.left();
		}
		return lhsRect.right() < rhsRect.right();
	}
}

namespace hnApp
{
	const int LittleFrameRenderPathBuilder::ExactCellLimit;

	LittleFrameRenderResult LittleFrameRenderPathBuilder::build(const QVector<LittleFrameRenderCell>& inputCells)
	{
		LittleFrameRenderResult result;
		QVector<LittleFrameRenderCell> cells;
		cells.reserve(inputCells.size());
		for (const LittleFrameRenderCell& cell : inputCells)
		{
			if (cell.isValid())
			{
				LittleFrameRenderCell normalized = cell;
				normalized.sourceRect = normalized.sourceRect.normalized();
				normalized.sceneRect = normalized.sceneRect.normalized();
				cells.append(normalized);
			}
		}

		if (cells.isEmpty())
		{
			return result;
		}

		if (cells.size() <= ExactCellLimit)
		{
			for (const LittleFrameRenderCell& cell : qAsConst(cells))
			{
				result.path.addRect(cell.sceneRect);
			}
			result.mode = LittleFrameRenderExactCells;
			result.segmentCount = cells.size();
			return result;
		}

		QMap<QString, QVector<LittleFrameRenderCell> > rows;
		for (const LittleFrameRenderCell& cell : qAsConst(cells))
		{
			rows[rowKey(cell)].append(cell);
		}

		QVector<LittleFrameRowRun> runs;
		for (auto rowIter = rows.begin(); rowIter != rows.end(); ++rowIter)
		{
			QVector<LittleFrameRenderCell>& rowCells = rowIter.value();
			std::sort(rowCells.begin(), rowCells.end(), cellLeftLessThan);
			if (rowCells.isEmpty())
			{
				continue;
			}

			LittleFrameRowRun currentRun;
			currentRun.imageName = rowCells.first().imageName;
			currentRun.sourceRect = rowCells.first().sourceRect;
			currentRun.sceneRect = rowCells.first().sceneRect;
			for (int i = 1; i < rowCells.size(); ++i)
			{
				const LittleFrameRenderCell& cell = rowCells.at(i);
				const bool adjacent = cell.sourceRect.left() <= currentRun.sourceRect.right() + 2;
				if (adjacent)
				{
					currentRun.sourceRect = currentRun.sourceRect.united(cell.sourceRect);
					currentRun.sceneRect = currentRun.sceneRect.united(cell.sceneRect);
					continue;
				}

				runs.append(currentRun);
				currentRun.imageName = cell.imageName;
				currentRun.sourceRect = cell.sourceRect;
				currentRun.sceneRect = cell.sceneRect;
			}
			runs.append(currentRun);
		}

		for (const LittleFrameRowRun& run : qAsConst(runs))
		{
			result.path.addRect(run.sceneRect);
		}
		result.mode = LittleFrameRenderRowRuns;
		result.segmentCount = runs.size();
		return result;
	}
}
