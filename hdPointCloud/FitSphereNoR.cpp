#include "StdAfx.h"
#include "FitSphereNoR.h"
//#include "CMatrix.h"
#include "..\hdCore\hdMatrix.h"
#include "Filters.h"
#include "math.h " 
#include "stdio.h " 
#include "hdSysSetting.h"

namespace hd
{
	CFitSphereNoR::CFitSphereNoR(void)
	{
		m_xyzR = NULL;
	}
	CFitSphereNoR::~CFitSphereNoR(void)
	{
		if (m_xyzR)
		{
			delete []m_xyzR;
			m_xyzR = NULL;
		}
	}

	bool planeJudge(float* pa,float* pb,float* pc,float* pd)
	{

		//pa、pb、pc构成平面
		float *A,*paraPlane,*b;
		A = new float[3*3];
		paraPlane = new float[3];
		b  = new float[3];
		float *pab,*pac;
		pab = new float[3];
		pac = new float[3];
		for (int i = 0;i < 3;i++)
		{
			pab[i] = pa[i] - pb[i];
			pac[i] = pa[i] - pc[i];
		}
		if ((pab[0] == pac[0] && pab[1] == pac[1] && pab[2] == pac[2])||(pab[0] == -1*pac[0] && pab[1] == -1*pac[1] && pab[2] == -1*pac[2]))
		{
			delete []pab;
			delete []pac;
			delete []A;
			delete []b;
			delete []paraPlane;
			return false;
		}
		else
			for (int i = 0;i < 3;i++)
			{
				b[i] = 1.0;
			}
			A[0] = pa[0];A[1] = pa[1];A[2] = pa[2];
			A[3] = pb[0];A[4] = pb[1];A[5] = pb[2];
			A[6] = pc[0];A[7] = pc[1];A[8] = pc[2];

			invers_matrix(A,3);
			mult(A,b,paraPlane,3,3,1);
			if (paraPlane[0] * pd[0] + paraPlane[1] * pd[1] + paraPlane[2] * pd[2] == 1)
			{   
				delete []pab;
				delete []pac;
				delete []A;
				delete []b;
				delete []paraPlane;
				return false;
			}
			else
			{   
				delete []pab;
				delete []pac;
				delete []A;
				delete []b;
				delete []paraPlane;
				return true;
			}
	}

	vector<PointXYZ> sphereRemove(vector<PointXYZ>&Pts,vector<PointXYZ>&vectPts)
	{
		int Nums = (int)Pts.size();

		float *pa,*pb,*pc,*pd;
		float *A,*b,*paraSphere;
		float x0,y0,z0,r0;
		A = new float[4*4];
		b = new float[4];
		paraSphere = new float[4];
		pa = new float[3];
		pb = new float[3];
		pc = new float[3];
		pd = new float[3];
		int *randNum;
		randNum = new int[Nums];
		int middle = 0,w = 0;
		int nTimes = 0;
		float maxCount = 0.0;
		float x = 0.0,y = 0.0,z = 0.0, r = 0.0;

		while(nTimes < 10000)
		{
			//每次得到四个随机点
			for (int i = 0;i < Nums;i++)
			{
				randNum[i] = i;
			}
			for (int i = 0;i < 4;i++)
			{
				w = rand()%(Nums -i) + i;
				middle = randNum[i];
				randNum[i] = randNum[w];
				randNum[w] = middle;
			}

			int nCounts = 0;        //统计球面附近的有效点个数
			nTimes++;

			pa[0] = Pts[randNum[0]].x; pa[1] = Pts[randNum[0]].y; pa[2] = Pts[randNum[0]].z;
			pb[0] = Pts[randNum[1]].x; pb[1] = Pts[randNum[1]].y; pb[2] = Pts[randNum[1]].z;
			pc[0] = Pts[randNum[2]].x; pc[1] = Pts[randNum[2]].y; pc[2] = Pts[randNum[2]].z;
			pd[0] = Pts[randNum[3]].x; pd[1] = Pts[randNum[3]].y; pd[2] = Pts[randNum[3]].z;

			if (planeJudge(pa,pb,pc,pd))
			{
				A[0] = pa[0];A[1] = pa[1];A[2] = pa[2];A[3] = 1.0;
				A[4] = pb[0];A[5] = pb[1];A[6] = pb[2];A[7] = 1.0;
				A[8] = pc[0];A[9] = pc[1];A[10] = pc[2];A[11] = 1.0;
				A[12] = pd[0];A[13] = pd[1];A[14] = pd[2];A[15] = 1.0;
				b[0] = -1 * (pa[0] * pa[0] + pa[1] * pa[1] + pa[2] * pa[2]);
				b[1] = -1 * (pb[0] * pb[0] + pb[1] * pb[1] + pb[2] * pb[2]);
				b[2] = -1 * (pc[0] * pc[0] + pc[1] * pc[1] + pc[2] * pc[2]);
				b[3] = -1 * (pd[0] * pd[0] + pd[1] * pd[1] + pd[2] * pd[2]);

				invers_matrix(A,4);
				mult(A,b,paraSphere,4,4,1);
				
				x0 = -0.5f * paraSphere[0];
				y0 = -0.5f * paraSphere[1];
				z0 = -0.5f * paraSphere[2];
				r0 = 0.5f * sqrt(paraSphere[0] * paraSphere[0] +paraSphere[1] * paraSphere[1] +paraSphere[2] * paraSphere[2] - 4 * paraSphere[3]);

				for (int i = 0;i < Nums;i++)
				{
					if (fabs(sqrt((Pts[i].x - x0) * (Pts[i].x - x0) + (Pts[i].y - y0) * (Pts[i].y - y0) + (Pts[i].z - z0) * (Pts[i].z - z0)) - r0) <= 0.01)
					{
						nCounts++;
					}
				}
				float count = (nCounts*1.0f)/(Nums*1.0f);
				if (maxCount < count)
				{
					x = x0; y = y0; z = z0; r = r0;
					maxCount = count;
				}

				if (maxCount > 0.8)
					break;
				else if (nTimes >= 1000)
					break;

			}
		}
		PointXYZ pts;
		vectPts.clear();
		for (int i = 0;i < Nums;i++)
		{
			if (fabs(sqrt((Pts[i].x - x) * (Pts[i].x - x) + (Pts[i].y - y) * (Pts[i].y - y) + (Pts[i].z - z) * (Pts[i].z - z)) - r) <= 0.01)
			{
				pts.x = Pts[i].x;
				pts.y = Pts[i].y;
				pts.z = Pts[i].z;
				vectPts.push_back(pts);
			}
		}
		delete []pa;
		delete []pb;
		delete []pc;
		delete []pd;
		delete []A;
		delete []b;
		delete []paraSphere;
		delete []randNum;

		return vectPts;

	}

	bool CFitSphereNoR::sphereFitting(vector<PointXYZ>&vcPts)
	{
		vector<PointXYZ> vecPts;           
		vecPts = sphereRemove(vcPts,vecPts);    //随机取点寻找最优球去噪
		int nCountAll = (int)vcPts.size();
		int nCountValid = (int)vecPts.size();
		double nCountJudge = (nCountValid * 1.0) / (nCountAll * 1.0);
		if (nCountJudge < 0.7)
		{
			return false;
		}

		int Num = (int)vecPts.size();           //去噪后点进行拟合计算

		PointXYZ uvwInit;                       //u,v,w
		vector<PointXYZ> transPts;              //Rho,Alpha,Beta
		transPts.clear();
		transPts.resize(Num);
		double r = 0.0;
		uvwInit.x = 0.0,uvwInit.y = 0.0,uvwInit.z = 0.0;
		for (int i = 0;i < Num;i++)                                    //初始变量u0、v0、w0
		{
			uvwInit.x += vecPts[i].x;
			uvwInit.y += vecPts[i].y;
			uvwInit.z += vecPts[i].z;
		}
		uvwInit.x = uvwInit.x / Num;
		uvwInit.y = uvwInit.y / Num;
		uvwInit.z = uvwInit.z / Num;

		double *transXYZ;
		transXYZ = new double[Num*3];


		for (int i = 0;i < Num;i++)
		{
			transXYZ[i*3] = vecPts[i].x - uvwInit.x;
			transXYZ[i*3+1] = vecPts[i].y - uvwInit.y;
			transXYZ[i*3+2] = vecPts[i].z - uvwInit.z;
			transPts[i].x = (float)sqrt(transXYZ[i*3] * transXYZ[i*3] + transXYZ[i*3+1] * transXYZ[i*3+1] + transXYZ[i*3+2] * transXYZ[i*3+2]);
			transPts[i].y = (float)atan2l(transXYZ[i*3+1],transXYZ[i*3]);
			transPts[i].z = (float)atan2l(transXYZ[i*3+2],sqrt(transXYZ[i*3] * transXYZ[i*3]+transXYZ[i*3+1] * transXYZ[i*3+1]));
		}

		double *A,*XX,*AT,*ATA,*L,*ATL;
		A = new double [Num*4];
		XX = new double [4];
		AT = new double [4*Num];
		ATA = new double [4*4];
		L = new double [Num];
		ATL = new double [4*Num];

		for (int i = 0;i < Num;i++)
		{	
			A[i*4] = cos(transPts[i].y) * cos(transPts[i].z);
			A[i*4+1] = sin(transPts[i].y) * cos(transPts[i].z);
			A[i*4+2] = sin(transPts[i].z);
			A[i*4+3] = 1.0;
			L[i] = transPts[i].x;
		}

		transpose(A, AT, Num, 4);
		mult(AT, A, ATA, 4,Num, 4);	
		invers_matrix(ATA, 4);
		mult(ATA, AT, ATL, 4, 4, Num);
		mult(ATL,L,XX,4,Num,1);

		delete []A;
		delete []L;
		delete []AT;
		delete []ATA;
		delete []ATL;

		double Ex = 0.0,Ey = 0.0,Ez = 0.0,Er = 0.0;

		A = new double[Num*4];
		L = new double[Num];
		AT = new double[4*Num];
		ATA = new double[4*4];
		ATL = new double[4*Num];

		m_xyzR = new float[4];
		memset(m_xyzR,0,sizeof(float)*4);
		m_xyzR[0] = (float)(uvwInit.x + XX[0]);
		m_xyzR[1] = (float)(uvwInit.y + XX[1]);
		m_xyzR[2] = (float)(uvwInit.z + XX[2]);
		m_xyzR[3] = (float)XX[3];
		int nTimes = 0;

		while(fabs(XX[0]) > 0.000001 || fabs(XX[1]) > 0.000001 || fabs(XX[2]) > 0.000001 || fabs(Ex) > 0.000000001 || fabs(Ey) > 0.000000001 || fabs(Ez) > 0.000000001 || fabs(Er) > 0.000000001)
		{
			for (int i = 0;i < Num;i++)
			{
				transXYZ[i*3] = transXYZ[i*3] - XX[0];
				transXYZ[i*3+1] = transXYZ[i*3+1] - XX[1];
				transXYZ[i*3+2] = transXYZ[i*3+2] - XX[2];
				transPts[i].x = (float)sqrt(transXYZ[i*3] * transXYZ[i*3] + transXYZ[i*3+1] * transXYZ[i*3+1] + transXYZ[i*3+2] * transXYZ[i*3+2]);
				transPts[i].y = (float)atan2l(transXYZ[i*3+1],transXYZ[i*3]);
				transPts[i].z = (float)atan2l(transXYZ[i*3+2],sqrt(transXYZ[i*3] * transXYZ[i*3]+transXYZ[i*3+1] * transXYZ[i*3+1]));

				A[i*4] = cos(transPts[i].y) * cos(transPts[i].z);
				A[i*4+1] = sin(transPts[i].y) * cos(transPts[i].z);
				A[i*4+2] = sin(transPts[i].z);
				A[i*4+3] = 1.0;
				L[i] = transPts[i].x;
			}
			transpose(A, AT, Num, 4);
			mult(AT, A, ATA, 4,Num, 4);	
			invers_matrix(ATA, 4);
			mult(ATA, AT, ATL, 4, 4, Num); 
			mult(ATL,L,XX,4,Num,1);

			m_xyzR[0] += (float)XX[0];
			m_xyzR[1] += (float)XX[1];
			m_xyzR[2] += (float)XX[2];
			m_xyzR[3] = (float)XX[3];

			Ex = Er = Ey = Ez = 0.0;
			for (int i = 0;i < Num;i++)
			{
				Ex += (sqrt((vecPts[i].x - m_xyzR[0]) * (vecPts[i].x - m_xyzR[0]) + (vecPts[i].y - m_xyzR[1]) * (vecPts[i].y - m_xyzR[1]) + (vecPts[i].z - m_xyzR[2]) * (vecPts[i].z - m_xyzR[2])) - m_xyzR[3]) * (vecPts[i].x - m_xyzR[0])
					/sqrt((vecPts[i].x - m_xyzR[0]) * (vecPts[i].x - m_xyzR[0]) + (vecPts[i].y - m_xyzR[1]) * (vecPts[i].y - m_xyzR[1]) + (vecPts[i].z - m_xyzR[2]) * (vecPts[i].z - m_xyzR[2]));

				Ey += (sqrt((vecPts[i].x - m_xyzR[0]) * (vecPts[i].x - m_xyzR[0]) + (vecPts[i].y - m_xyzR[1]) * (vecPts[i].y - m_xyzR[1]) + (vecPts[i].z - m_xyzR[2]) * (vecPts[i].z - m_xyzR[2])) - m_xyzR[3]) * (vecPts[i].y - m_xyzR[1])
					/sqrt((vecPts[i].x - m_xyzR[0]) * (vecPts[i].x - m_xyzR[0]) + (vecPts[i].y - m_xyzR[1]) * (vecPts[i].y - m_xyzR[1]) + (vecPts[i].z - m_xyzR[2]) * (vecPts[i].z - m_xyzR[2]));

				Ez += (sqrt((vecPts[i].x - m_xyzR[0]) * (vecPts[i].x - m_xyzR[0]) + (vecPts[i].y - m_xyzR[1]) * (vecPts[i].y - m_xyzR[1]) + (vecPts[i].z - m_xyzR[2]) * (vecPts[i].z - m_xyzR[2])) - m_xyzR[3]) * (vecPts[i].z - m_xyzR[2])
					/sqrt((vecPts[i].x - m_xyzR[0]) * (vecPts[i].x - m_xyzR[0]) + (vecPts[i].y - m_xyzR[1]) * (vecPts[i].y - m_xyzR[1]) + (vecPts[i].z - m_xyzR[2]) * (vecPts[i].z - m_xyzR[2]));
				Er += sqrt((vecPts[i].x - m_xyzR[0]) * (vecPts[i].x - m_xyzR[0]) + (vecPts[i].y -  m_xyzR[1]) * (vecPts[i].y -  m_xyzR[1]) + (vecPts[i].z - m_xyzR[2]) * (vecPts[i].z - m_xyzR[2])) - m_xyzR[3];
			}	

			nTimes++;
			if(nTimes > 100)
				break;
		}

		delete []A;
		delete []AT;
		delete []ATA;
		delete []L;
		delete []ATL;
		delete []XX;
		delete []transXYZ;
		return true;
	}

}