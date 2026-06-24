/*! HdBuildingExtract.h
********************************************************************************
<PRE>
模块名       : hdClassify
文件名       : HdRandomGenerator.h
相关文件     : 
文件实现功能 : 生成随机数
作者         : 蔡红云
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期            版本         修改人              修改内容
2014/05/29   1.0      蔡红云    				
</PRE>
*******************************************************************************/
#pragma once

#include "hdCommon.h"

namespace hd
{

		/** A thred-safe pseudo random number generator, based on an internal MT19937 randomness generator.
		  * The base algorithm for randomness is platform-independent. 
		  * For real thread-safety, each thread must create and use its own instance of this class.
		  *
		  * Single-thread programs can use the static object mrpt::random::randomGenerator
		 * \ingroup 
		  */
	// 生成随机数类
	class  HDCOMMON_API CHdRandomGenerator
	{
	protected:

		/** Data used internally by the MT19937 PRNG algorithm. */
		struct  TMT19937_data
		{
			TMT19937_data() : index(0), seed_initialized(false)
			{

			}
			unsigned int	MT[624]; // 矩阵
			unsigned int	index; // 索引
			bool		seed_initialized;// 是否初始化种子
		} m_MT19937_data;


		void MT19937_generateNumbers();// 产生随机数
		void MT19937_initializeGenerator(const unsigned int &seed);// 初始化随机数生成器

	public:

		// 默认构造函数
		CHdRandomGenerator() : m_MT19937_data() { randomize(); }

		/** Constructor for providing a custom random seed to initialize the PRNG */
		CHdRandomGenerator(const unsigned long seed) : m_MT19937_data() { randomize(seed); }

		void randomize(const unsigned int seed);  //!< Initialize the PRNG from the given random seed
		
		void randomize();	//!< Randomize the generators, based on current time
		
		unsigned int  drawUniform32bit(); // 生成32位种子
		unsigned long  long getCurrentTime(); // 获取系统时间

		// 获取高位
		inline unsigned int hiBit( const unsigned int u )
		{ 
			return u & 0x80000000UL;
		}

		// 获取低位
		inline unsigned int loBit( const unsigned int u ) 
		{ 
			return u & 0x00000001UL;
		}

		// 获取次低位
		inline unsigned int loBits( const unsigned int u )
		{
			return u & 0x7fffffffUL; 
		}

		// 获取混合位
		inline unsigned int mixBits( const unsigned int u, const unsigned int v ) 
		{ 
			return hiBit(u) | loBits(v); 
		}

		inline unsigned int twist( const unsigned int m, const unsigned int s0, const unsigned int s1 ) 
		{ 
			return m ^ (mixBits(s0,s1)>>1) ^ (-loBit(s1) & 0x9908b0dfUL); 
		}

		// 获取归一化向量模板
		template <class VEC>
		void drawUniformVector(
			VEC & v,
			const  double unif_min = 0,
			const  double unif_max = 1 )
		{
			const size_t N = v.size();
			for (size_t c=0;c<N;c++)
				v[c] = static_cast<typename VEC::value_type>( drawUniform(unif_min,unif_max) );
		}

		/** Generate a uniformly distributed pseudo-random number using the MT19937 algorithm, scaled to the selected range. */
		double drawUniform( const double Min, const double Max)
		{
			return Min + (Max-Min)* drawUniform32bit() * 2.3283064370807973754314699618685e-10; // 0xFFFFFFFF ^ -1
		}

		void drawUniformUnsignedInt(unsigned int &ret_number) { ret_number=drawUniform32bit(); }
		
		void drawUniformUnsignedIntRange(unsigned int &ret_number,const unsigned int min_val,const unsigned int max_val)
		{
			const unsigned int range = max_val-min_val+1;
			unsigned int rnd;
			drawUniformUnsignedInt(rnd);
			ret_number=min_val+ (rnd%range);
		}
	};
	extern HDCOMMON_API  CHdRandomGenerator randomGenerator;
}
