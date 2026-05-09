#pragma once
#include <map>
#include "mydefs.hpp"
#include "HlzDefs31.h"
#include "HdLevel.h"

using namespace std;

namespace hd
{
	class  HLS_API CHdLevel31 : public CHdLevel
	{
	public:
		CHdLevel31();

		~CHdLevel31();

		//重载接口
		virtual void Update();

		virtual u64  GetPtCount();

	private:
		CHdLevel31(const CHdLevel31& other) {}

		CHdLevel31& operator=(const CHdLevel31& other) { return *this; }

		bool operator < (const CHdLevel31& other) const
		{
			return m_levelNo < other.m_levelNo;
		}

	public:
		//hlz 3.1 层索引信息
		HdLevel31 m_level31;
	};

}

