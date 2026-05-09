#include "StdAfx.h"
#include "hdTin.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

namespace hd
{
	CHdTin::CHdTin(void)
	{
	}


	CHdTin::~CHdTin(void)
	{
		destroy();
	}

	bool CHdTin::initial( int num, bool convex_hull/*=false*/ )
	{
		return TINclean(num,convex_hull);
	}

	void CHdTin::add( float* p ,int index)
	{
		TINadd(p,index);
	}

	void CHdTin::add(float* p)
	{
		TINadd(p);
	}

	void CHdTin::finish()
	{
		TINfinish();
	}

	int CHdTin::get_size()
	{
		return TINget_size();
	}

	TINtriangle* CHdTin::get_triangle( int t )
	{
		return TINget_triangle(t);
	}

	void CHdTin::destroy()
	{
		TINdestroy();
	}

	void CHdTin::save( const char* savepath )
	{
		int size = get_size();

		FILE* pFile = fopen(savepath,"wt");
		fprintf(pFile,"ply\n");
		fprintf(pFile,"format ascii 1.0\n");
		fprintf(pFile,"comment hdScene generated\n");
		fprintf(pFile,"element vertex %d\n",size * 3);
		fprintf(pFile,"property float x\n");
		fprintf(pFile,"property float y\n");
		fprintf(pFile,"property float z\n");
		fprintf(pFile,"element face %d\n",size);
		fprintf(pFile,"property list uchar int vertex_indices\n");
		fprintf(pFile,"end_header\n");

		int i,j;
		TINtriangle* t = get_triangle(0);
		int ptCount = 0;
		for (i = 0; i < size; i++, t++)
		{
			if (t->next < 0)
			{
				if (t->V[0])
				{
					for (j = 0; j < 3; j++)
					{
						fprintf(pFile,"%f\t%f\t%f\n",t->V[j][0],t->V[j][1],t->V[j][2]);
						ptCount++;
					}
				}
			}
		}

		t = get_triangle(0);
		int idx = 0;
		for (i = 0; i < size; i++, t++)
		{
			if (t->next < 0)
			{
				if (t->V[0])
				{
					//bool bRet = judgeCircle(t->V[0][0],t->V[1][0],t->V[2][0],
					//	         t->V[0][1],t->V[1][1],t->V[2][1],
					//			 t->V[0][2],t->V[1][2],t->V[2][2]);
					//if (bRet == true)
					//{
					fprintf(pFile,"3\t%d\t%d\t%d\n",idx * 3 + 0,idx * 3 + 1,idx * 3 + 2);
					idx++;
					//}
					//else
					//	continue;
				}
			}
		}
		fseek(pFile,0,SEEK_SET);
		fprintf(pFile,"ply\n");
		fprintf(pFile,"format ascii 1.0\n");
		fprintf(pFile,"comment hdScene generated\n");
		fprintf(pFile,"element vertex %d\n",ptCount);
		fprintf(pFile,"property float x\n");
		fprintf(pFile,"property float y\n");
		fprintf(pFile,"property float z\n");
		fprintf(pFile,"element face %d\n",idx);
		fprintf(pFile,"property list uchar int vertex_indices\n");
		fprintf(pFile,"end_header\n");

		fclose(pFile);
	}

	vector<float> CHdTin::get_triangle_vertices()
	{
		vector<float> tmpBuff;

		int size = get_size();
		int i,j;
		TINtriangle* t = get_triangle(0);
		int ptCount = 0;
		for (i = 0; i < size; i++, t++)
		{
			if (t->next < 0)
			{
				if (t->V[0])
				{
					for (j = 0; j < 3; j++)
					{
						tmpBuff.push_back(t->V[j][0]);
						tmpBuff.push_back(t->V[j][1]);
						tmpBuff.push_back(t->V[j][2]);
						ptCount++;
					}
				}
			}
		}
		return tmpBuff;
	}

	bool CHdTin::judgeCircle(float x1,float x2,float x3,float y1,float y2,float y3,float z1,float z2,float z3)
	{
		float lengthA = 0.0f,lengthB = 0.0f,lengthC = 0.0f;
		lengthA = sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2) + (z1 - z2)* (z1 - z2));
		lengthB = sqrt((x1 - x3) * (x1 - x3) + (y1 - y3) * (y1 - y3) + (z1 - z3)* (z1 - z3));
		lengthC = sqrt((x3 - x2) * (x3 - x2) + (y3 - y2) * (y3 - y2) + (z3 - z2)* (z3 - z2));
		float sum = (lengthA + lengthB + lengthC) / 2.0f;
		float trianArea = sqrt(sum * (sum - lengthA) * (sum - lengthB) * (sum - lengthC));

		float circleRiadus = (lengthA * lengthB * lengthC) / (4.0f * trianArea);
		if (circleRiadus >= 2.0f )
		{
			return false;
		}
		else
			return true;
	}

	void CHdTin::save2( const char* savepath2 )
	{
		int size = get_size();

		FILE* pFile = fopen(savepath2,"wt");

		int i,j;
		TINtriangle* t = get_triangle(0);
		int ptCount = 0;
		for (i = 0; i < size; i++, t++)
		{
			if (t->next < 0)
			{
				if (t->V[0])
				{
					for (j = 0; j < 3; j++)
					{
						fprintf(pFile,"%f\n",t->V[j][0]);
						fprintf(pFile,"%f\n",t->V[j][1]);
						fprintf(pFile,"%f\n",t->V[j][2]);
						ptCount++;
					}
				}
			}
		}

		fprintf(pFile,"%s\n","nil");

		fclose(pFile);
	}

	void CHdTin::saveObj(const char* savepath)
	{
		// 文件指针
		FILE* fp = fopen(savepath, "w");

		// 判空
		if (!fp)
		{
			return;
		}

		// 文件前缀
		fputs("#Wavefront OBJ File", fp);
		fputs("\n", fp);
		fputs("#convert by HDSY HDScene v1.0", fp);
		fputs("\n", fp);

		// 写入三角形
		int i = 0;							// 三角形索引
		int j = 0;							// 三角形三个顶点索引
		int size = get_size();				// 三角形的个数
		TINtriangle* t = get_triangle(0);	// 三角形第一个值
		int ptCount = 0;					// 点数
		for (i = 0; i < size; i++, t++)
		{
			if (t->next < 0)
			{
				if (t->V[0])
				{
					for (j = 0; j < 3; j++)
					{
						fprintf(fp,"v %.6f %.6f %.6f \n", t->V[j][0], t->V[j][1], t->V[j][2]);
						ptCount++;
					}
				}
			}
		}

		// 写入点个数
		fprintf(fp,"# %d vertices\n", ptCount);
		fputs("\n", fp);

		// 写入三角形
		t = get_triangle(0);
		int idx = 0;
		for (i = 0; i < size; i++, t++)
		{
			if (t->next < 0)
			{
				if (t->V[0])
				{
					// 可以获取三角形包含的顶点的索引值 [2014/04/09 危迟]
					// 获取三角形三个顶点的索引值
					int index1 = t->index[0];
					int index2 = t->index[1];
					int index3 = t->index[2];
					fprintf(fp,"f %d %d %d\n",index1,index2,index3);
					idx++;
				}
			}
		}

		// 三角形个数
		fprintf(fp,"# %d triangles", idx);
		fputs("\n", fp);

	    // 关闭文件指针
		fclose(fp);
	}

	//! 获取三角网中包含点p的三角形 [2015/05/18 危迟]
	TINtriangle* CHdTin::locateTriangleIncludePoint( float* p )
	{
		TINtriangle* t = TINlocate(p);

		if (p)
		{
			return  t;
		}
		return NULL;
	}

}