#pragma once

#include <QVector>
#include "HnProjectEnums.h"

// 公里评定在指标计算前调整边界；模板使规则可脱离工程数据库单独验证。
class hnRuralEvaluationUnitMerger
{
public:
	// 输入为沿检测方向排列、已完成属性切分的单元；原地合并不足500米的农村路单元。
	template<typename Segment>
	void merge(QVector<Segment>& segments) const
	{
		int index = 0;
		while (index < segments.size())
		{
			if (segments[index].Type != HnProjectEnums::RuralRoadlowLevel
				|| segments[index].getRoadLength() >= 500.0)
			{
				++index;
				continue;
			}

			bool previous = index > 0 && canMerge(segments[index - 1], segments[index]);
			const bool next = index + 1 < segments.size()
				&& canMerge(segments[index], segments[index + 1]);
			if (!previous && !next)
			{
				++index;
				continue;
			}

			// 两侧都允许时优先形成更接近1000米的单元；相同时固定选低桩号一侧。
			if (previous && next)
			{
				const double previousDistance = qAbs(segments[index - 1].getRoadLength()
					+ segments[index].getRoadLength() - 1000.0);
				const double nextDistance = qAbs(segments[index + 1].getRoadLength()
					+ segments[index].getRoadLength() - 1000.0);
				previous = previousDistance < nextDistance
					|| (qAbs(previousDistance - nextDistance) < 0.000001
						&& segments[index - 1].getStartMile() < segments[index + 1].getEndMile());
			}

			const int leftIndex = previous ? index - 1 : index;
			// 同步桩号与DMI边界，后续IRI、PCI等直接按完整新单元重新计算。
			segments[leftIndex].setEndMile(segments[leftIndex + 1].getEndMile());
			segments[leftIndex].setEndDmi(segments[leftIndex + 1].getEndDmi());
			segments.removeAt(leftIndex + 1);
			index = qMax(0, leftIndex - 1);
		}
	}

private:
	// 仅合并连续且属性一致的低等级农村路；属性独立短段不能强行跨界。
	template<typename Segment>
	bool canMerge(const Segment& left, const Segment& right) const
	{
		return left.Type == HnProjectEnums::RuralRoadlowLevel && left.Type == right.Type
			&& left.RoadSurface == right.RoadSurface
			&& left.RoadGrad == right.RoadGrad && left.RoadDegreestr == right.RoadDegreestr
			&& qAbs(left.getSurveyWidth() - right.getSurveyWidth()) < 0.001
			&& left.getUnitStr() == right.getUnitStr()
			&& qAbs(left.getEndMile() - right.getStartMile()) < 0.000001
			&& qAbs(left.getEndDmi() - right.getStartDmi()) < 0.000001
			&& left.getRoadLength() > 0.0 && right.getRoadLength() > 0.0
			&& left.getRoadLength() + right.getRoadLength() <= 1500.0;
	}
};
