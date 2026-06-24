#include "StdAfx.h"
#include "HdRandomGenerator.h"

namespace hd
{
	CHdRandomGenerator randomGenerator;

	// 生成随机数
	void CHdRandomGenerator::randomize()
	{
		// 以系统时间为种子生成随机数
		MT19937_initializeGenerator( static_cast<unsigned long long>(getCurrentTime()) );
		m_MT19937_data.index = 0;
	}

	// 利用传递参数生成随机种子
	void CHdRandomGenerator::randomize(const unsigned int seed)
	{
		MT19937_initializeGenerator(seed);
		m_MT19937_data.index = 0;
	}

	// 获取系统时间
	unsigned long long  CHdRandomGenerator::getCurrentTime()
	{

		FILETIME		t;
		GetSystemTimeAsFileTime(&t);
		return (((unsigned long long)t.dwHighDateTime) << 32) | ((unsigned long long)t.dwLowDateTime);

	}

	// Generate an array of 624 untempered numbers
	void CHdRandomGenerator::MT19937_generateNumbers()
	{
		if (!m_MT19937_data.seed_initialized)	this->randomize();

		const int N = 624;	// length of state vector
		const int M = 397;	// period parameter

		register unsigned int *p = m_MT19937_data.MT;
		for( int i = N - M; i--; ++p )
		{
			*p = twist( p[M], p[0], p[1] );
		}

		for( int i = M; --i; ++p )
		{
			*p = twist( p[M-N], p[0], p[1] );
		}
		*p = twist( p[M-N], p[0], m_MT19937_data.MT[0] );
	}

	unsigned int CHdRandomGenerator:: drawUniform32bit()
	{

		if (!m_MT19937_data.index)
			MT19937_generateNumbers();

		unsigned int	y = m_MT19937_data.MT[m_MT19937_data.index];
		y ^= y >> 11;
		y ^= (y << 7) & 2636928640U; // 0x9d2c5680
		y ^= (y << 15) & 4022730752U; // 0xefc60000
		y ^= (y >> 18);

		// Wrap index to [0,623].
		m_MT19937_data.index++;
		if (m_MT19937_data.index>=624)
		
		{
			m_MT19937_data.index=0;
		}

		return y;
	}

	// Initialize the generator from a seed
	void CHdRandomGenerator::MT19937_initializeGenerator(const unsigned int &seed)
	{
		m_MT19937_data.seed_initialized = true;// 种子初始化为真
		m_MT19937_data.MT[0] = seed; // 矩阵赋值
		for (unsigned int  i=1;i<624;i++)
		{
				m_MT19937_data.MT[i] = static_cast<unsigned int  >( 1812433253 * (m_MT19937_data.MT[i-1] ^ ( m_MT19937_data.MT[i-1] >> 30 )) + i); // 0x6c078965
		}
		
	}
}
