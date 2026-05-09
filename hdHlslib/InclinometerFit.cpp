#include "InclinometerFit.h"
#include "..\hdCore\Matrix.h"
#include "..\hdCore\hdMath.h"
#include "..\hdCore\hdMatrix.h"
//#include "opencv\cv.h"
//#include "opencv\cxcore.h"
#include <vector>

using namespace hd;
using namespace std;
namespace hd
{
	struct FIT_COORD
	{
		FIT_COORD()
			:x(0.0),y(0.0){}
		double x;
		double y;
	};
	//椭圆参数
	struct ELLIPSOID_TYPE
	{
		ELLIPSOID_TYPE()
			:center_x(0.0),center_y(0.0),major_semiAxis(0.0),minor_semiAxis(0.0),rotAngle(0.0){}
		double center_x;		//椭圆中心位置x
		double center_y;		//椭圆中心位置y
		double major_semiAxis;	//长半轴
		double minor_semiAxis;	//短半轴
		double rotAngle;		//长轴的转角（角度）
	};


	ELLIPSOID_TYPE GetEllipsoid(FIT_COORD* pSample,int num)
	{
		//椭圆方程 a*x^2 + b*x*y + c*y^2 + d*x + e*y + f = 0
		//为计算方便 上式可变换为 a*x^2 + b*x*y + c*y^2 + d*x + e*y - 1 = 0
		// Y = AX, Y = {1,1,…,1}; A为样本值矩阵
		//根据最小二乘方法得到各项系数
		//根据各项系数计算椭圆参数

		//样本点要大于5个点
		if(num <5)
			ELLIPSOID_TYPE();

		//计算矩阵A和Y
		double *pA = new double[num*5];
		memset(pA,0,sizeof(double)*num*5);
		double *pY = new double[num];
		memset(pY,0,sizeof(double)*num);
		for(int i = 0; i < num; i++)
		{
			pA[i*5 + 0] = pSample[i].x * pSample[i].x;
			pA[i*5 + 1] = pSample[i].x * pSample[i].y;
			pA[i*5 + 2] = pSample[i].y * pSample[i].y;
			pA[i*5 + 3] = pSample[i].x;
			pA[i*5 + 4] = pSample[i].y;

			pY[i] = 1;
		}

		//最小二乘
		CMatrix MATRIX_A(num,5,pA);
		CMatrix MATRIX_A_TEMP = MATRIX_A;
		CMatrix MATRIX_A_T = MATRIX_A_TEMP.getTranspose();
		CMatrix MATRIX_Y(num,1,pY);
		CMatrix MATRIX_P = MATRIX_A_T * MATRIX_A;
		CMatrix MATRIX_Q = MATRIX_A_T * MATRIX_Y;
		CMatrix MATRIX_X = MATRIX_P.getInverse()*MATRIX_Q;

		//计算椭圆参数
		double a,b,c,d,e,f;
		a = MATRIX_X.ptr[0];	b = MATRIX_X.ptr[1];	c = MATRIX_X.ptr[2];
		d = MATRIX_X.ptr[3];	e = MATRIX_X.ptr[4];	f = -1;

		//cout << a <<"	" << b << "	"<<c<<"	"<<d<<"	"<<e<<endl;

		ELLIPSOID_TYPE ellipResult;
		ellipResult.center_x = (b*e - 2*c*d) / (4*a*c - b*b);
		ellipResult.center_y = (b*d - 2*a*e) / (4*a*c - b*b);
		ellipResult.major_semiAxis = sqrt(-2*f/(a+c-sqrt(b*b+pow((a-c)/f,2))));
		ellipResult.minor_semiAxis = sqrt(-2*f/(a+c+sqrt(b*b+pow((a-c)/f,2))));
		//如果是圆，则角度为0
		if(a - c < 0.00001 && b < 0.00001)
			ellipResult.rotAngle = 0;
		else
			ellipResult.rotAngle = RADTODEG64 * (1.0 / 2 * atan(b/(a-c)));
		delete[] pA;
		delete[] pY;
		return ellipResult;
	}

	void YPR(double yaw ,double pitch,double roll,double *R);
	void horizontalization(const char *filePath, double &pitch, double &roll)
	{
		double PI=3.1415926535897;
		FILE *fp=NULL;
		fp=fopen(filePath,"rt");
		if(fp==NULL)
		{
			return;
		}

		double angle[3600][3];
		int angleCount=0;
		char strBuf[1024];
		
		while(!feof(fp))
		{
			double angle0 = 0,angle1 = 0,angle2 = 0;
			fgets(strBuf,1024,fp);
			int field = sscanf(strBuf,"%lf%lf%lf",&angle0,&angle1,&angle2);

			//int field = fscanf(fp,"%lf %lf %lf",&angle[angleCount][0],&angle[angleCount][1],&angle[angleCount][2]);
			if(field == 3)
			{
				angle[angleCount][0]=angle0/180.0*PI;
				angle[angleCount][1]=angle1/180.0*PI;
				angle[angleCount][2]=angle2/180.0*PI;
				angleCount++;
			}

			if (angleCount >= 3600)
			{
				break;
			}
		}
		fclose(fp);
		//计算方向向量点
		//CvBox2D32f* box;
		double *vpts=new double[angleCount*3];

		FIT_COORD* PointArray2D32f = new FIT_COORD[angleCount];
		//PointArray2D32f= (CvPoint2D32f*)malloc( angleCount*sizeof(CvPoint2D32f) );
		//box = (CvBox2D32f*)malloc(sizeof(CvBox2D32f));

		for(int i=0; i<angleCount; i++)
		{
			//计算倾角仪方向向量
			double A_z=-angle[i][0];
			double A_x= angle[i][1];								//y轴输出
			double A_y= asin(sin(angle[i][2])/cos(angle[i][1]));	//x轴输出

			double R1[3][3];
			double R2[3][3];

			YPR(0,A_x,A_y,&R1[0][0]);
			MatInv(&R1[0][0],3);

			YPR(A_z,0,0,&R2[0][0]);
			MatInv(&R2[0][0],3);

			double *v1=new double[3];
			double *v2=new double[3];
			v1[0]=0;v1[1]=0;v1[2]=1;

			double tmpMat[3][3];
			MatMult(&R2[0][0],&R1[0][0],&tmpMat[0][0],3,3,3);
			MatMult(&tmpMat[0][0],v1,v2,3,3,1);

			vpts[i*3+0]=v2[0];
			vpts[i*3+1]=v2[1];
			vpts[i*3+2]=v2[2];

			//printf("%.4f %.4f %.4f\n",v2[0],v2[1],v2[2]);

			PointArray2D32f[i].x = (float)v2[0];
			PointArray2D32f[i].y = (float)v2[1];

			delete []v1;
			delete []v2;
		}

		//cvFitEllipse(PointArray2D32f, angleCount, box);
		ELLIPSOID_TYPE ellips = GetEllipsoid(PointArray2D32f,angleCount);
		double center_x = ellips.center_x;//(box->center.x);
		double center_y = ellips.center_y;//(box->center.y);
		double center_z=sqrt(1-center_x*center_x-center_y*center_y);

		//printf("%lf %lf %lf\n",center_x,center_y,center_z);

		pitch=-atan(center_y/center_z);
		roll=-acos(center_z/cos(pitch));

		double dPitch = pitch / PI * 180.0;
		double dRoll = roll / PI * 180.0;

		delete[] vpts;
		delete[] PointArray2D32f;
		printf("pitch:%lf°\n roll:%lf°\n",dPitch,dRoll);
	}

	void YPR(double yaw ,double pitch,double roll,double *R)
	{
		//标准YPR矩阵
		double a1,a2,a3,b1,b2,b3,c1,c2,c3;
		a1=	cos(roll)*cos(yaw) + sin(pitch)*sin(roll)*sin(yaw);
		a2=-cos(pitch)*sin(yaw);
		a3=	cos(roll)*sin(pitch)*sin(yaw) - cos(yaw)*sin(roll);

		b1=	cos(roll)*sin(yaw) - cos(yaw)*sin(pitch)*sin(roll);
		b2=	cos(pitch)*cos(yaw);
		b3=-sin(roll)*sin(yaw) - cos(roll)*cos(yaw)*sin(pitch);

		c1=	cos(pitch)*sin(roll);
		c2=	sin(pitch);
		c3=	cos(pitch)*cos(roll);

		R[0]=a1;
		R[1]=a2;
		R[2]=a3;
		R[3]=b1;
		R[4]=b2;
		R[5]=b3;
		R[6]=c1;
		R[7]=c2;
		R[8]=c3;
	}

CInclinometerFit::CInclinometerFit( void )
{
	m_pRollPara = NULL;
	m_pPitchPara = NULL;
	m_errorRoll = 0.403308;
	m_errorPitch = 0.068995;
	
	memset(m_rotateMat,0,sizeof(double) * 9);
}

CInclinometerFit::~CInclinometerFit( void )
{
	Clear();
}

void CInclinometerFit::Clear()
{
	if (m_pRollPara)
	{
		delete m_pRollPara;
		m_pRollPara = NULL;
	}
	if (m_pPitchPara)
	{
		delete m_pPitchPara;
		m_pPitchPara = NULL;
	}
	m_n = 0;
}

void CInclinometerFit::SetPolyN( int n )
{
	if(n < 3 || n > 32)
		return;
	if (m_pRollPara)
	{
		delete m_pRollPara;
		m_pRollPara = NULL;
	}
	if (m_pPitchPara)
	{
		delete m_pPitchPara;
		m_pPitchPara = NULL;
	}
	m_n = n;
	m_pRollPara = new double[m_n + 1];
	m_pPitchPara = new double[m_n + 1];
}

BOOL CInclinometerFit::FitBySimpleData( const char* path )
{
	Clear();
	FILE* pFile = fopen(path,"rt");
	if(pFile == NULL)
		return FALSE;

	SetPolyN(5);
	// 观测值
	vector<double> vecObs;
	vecObs.resize(3600);
	// 左右侧翻
	vector<double> vecRoll;
	vecRoll.resize(3600);
	// 前后翻滚
	vector<double> vecPitch;
	vecPitch.resize(3600);
	int index = 0;
	// 每一行字符缓存
	char strBuf[1024];  
	double obs = 0,roll = 0,pitch = 0;
	while(!feof(pFile))
	{
		//double& obs = *(vecObs._Myfirst() + index);
		//double& roll = *(vecRoll._Myfirst() + index);
		//double& pitch = *(vecPitch._Myfirst() + index);
		fgets(strBuf,1024,pFile);

		//int field = fscanf(pFile,"%lf\t%lf\t%lf\n",&obs,&roll,&pitch);
		int field = sscanf(strBuf,"%lf%lf%lf",&obs,&roll,&pitch);

		if(field == 3)
		{
			*(vecObs._Myfirst() + index) = obs;
			*(vecRoll._Myfirst() + index) = roll;
			 *(vecPitch._Myfirst() + index) = pitch;
			index++;
		}
		else 
		{
			//if (field == 0 && (obs != 0 || roll != 0 || pitch != 0))
			{
				//Clear();
				//return FALSE;
				continue;
			}
		}
	}
	vecObs.resize(index);
	vecRoll.resize(index);
	vecPitch.resize(index);
	fclose(pFile);
	pFile = NULL;

	// 获取整体倾角,及旋转矩阵
	double roll1 = -(vecRoll[0] - m_errorRoll);
	double pitch1 = -(vecPitch[0] - m_errorPitch);

	roll1 *= DEGTORAD64;
	pitch1 *= DEGTORAD64;
	
	//double roll = 0.0,pitch = 0.0;
	horizontalization(path,roll,pitch);

	double yaw = 0.0;

	double a1,a2,a3,b1,b2,b3,c1,c2,c3;
	//double phi,double omega,double kappa;

	a1 = cos(pitch)*cos(yaw) - sin(pitch)*sin(roll)*sin(yaw);
	a2 = -cos(pitch)*sin(yaw) - sin(pitch)*sin(roll)*cos(yaw);
	a3 = -sin(pitch)*cos(roll);
	b1 = cos(roll)*sin(yaw);
	b2 = cos(roll)*cos(yaw);
	b3 = -sin(roll);
	c1 = sin(pitch)*cos(yaw) + cos(pitch)*sin(roll)*sin(yaw);                                                                  
	c2 = -sin(pitch)*sin(yaw) + cos(pitch)*sin(roll)*cos(yaw);
	c3 = cos(pitch)*cos(roll);

	m_rotateMat[0] = a1; m_rotateMat[1] = a2; m_rotateMat[2] = a3;
	m_rotateMat[3] = b1; m_rotateMat[4] = b2; m_rotateMat[5] = b3;
	m_rotateMat[6] = c1; m_rotateMat[7] = c2; m_rotateMat[8] = c3;

	//m_pRotTrans->setRotateDegree(-pitch,-roll,0.0);
	// 计算roll多项式参数
	GetFittingPara(m_n,vecObs._Myfirst(),vecRoll._Myfirst(),index,0.0,m_pRollPara);
	// 计算pitch多项式参数
	GetFittingPara(m_n,vecObs._Myfirst(),vecPitch._Myfirst(),index,0.0,m_pPitchPara);
	return TRUE;
}

BOOL CInclinometerFit::GetFittingPara( 
	int n,					/* 多项式指数 */ 
	double *pObsArgument,	/* 为观测值向量 */ 
	double *pRealArgument,	/* 真实值向量 */ 
	int num,				/* 观测值个数 */ 
	double offset,			/* 偏移因子（防止数据泄露）*/ 
	double *pPara )			// 为系数向量,长度为指数加1
{
	int row = num ;
	int col = n+1 ;

	double *ptemp = new double[row];
	for (int i = 0 ; i<row; i++)
	{
		ptemp[i] = pObsArgument[i] - offset;
	}

	double * pArgumentMat = new double [row * col];	//自变量矩阵
	int k = 0;
	for ( int i = 0; i < row; i++)
	{
		for( int j = col-1; j >= 0; j--)
		{
			if (j !=0 )
			{
				pArgumentMat[k] = pow(ptemp[i],j);
				k++;
			}
			else
			{
				pArgumentMat[k] = 1;
				k++;
			}
		}
	}

	CMatrix Matrix_Xtemp(row,col,pArgumentMat);
	CMatrix Matrix_X = Matrix_Xtemp;
	CMatrix Matrix_Y(row,1,pRealArgument);
	CMatrix Matrix_XT = Matrix_Xtemp.getTranspose();
	CMatrix Matrix_P = Matrix_XT * Matrix_X;
	CMatrix Matrix_Q = Matrix_XT * Matrix_Y;
	CMatrix Matrix_Para = Matrix_P.getInverse() * Matrix_Q;

	//得到拟合方程系数
	for (int i = 0 ; i < col; i++)
	{
		pPara[i] = Matrix_Para.ptr[i];
	}

	delete [] ptemp;
	delete [] pArgumentMat;
	return TRUE;
}

BOOL CInclinometerFit::GetFitValue( double obs,double& roll,double& pitch )
{
	if(obs < 0.0 || obs > 360.0)
		return FALSE;

	roll = 0.0;
	pitch = 0.0;
	for (int i = 0;i <= m_n;i++)
	{
		roll += (m_pRollPara[m_n - i] * pow(obs,i));

		pitch += (m_pPitchPara[m_n - i] * pow(obs,i));
	}

	return TRUE;
}

BOOL CInclinometerFit::GetSurfaceFitPara( double *pSampleX,double *pSampleY,double *pSampleZ,int numSample, double *pPara )
{
	double *pZ = new double [numSample];

	//样本变换矩阵
	double * pArgumentMat = new double [numSample * 6];

	int k = 0;
	for ( int i = 0; i < numSample; i++)
	{
		pArgumentMat[i*6+0] = pSampleX[i] * pSampleX[i];
		pArgumentMat[i*6+1] = pSampleX[i] * pSampleY[i];
		pArgumentMat[i*6+2] = pSampleY[i] * pSampleY[i];
		pArgumentMat[i*6+3] = pSampleX[i];
		pArgumentMat[i*6+4] = pSampleY[i];
		pArgumentMat[i*6+5] =1;

		pZ[i] = pSampleX[i];
	}

	CMatrix MATRIX_A(numSample,6,pArgumentMat);  
	CMatrix MATRIX_A_temp = MATRIX_A;
	CMatrix MATRIX_Z(numSample,1,pZ);
	CMatrix MATRIX_AT = MATRIX_A_temp.getTranspose();
	CMatrix MATRIX_P = MATRIX_AT * MATRIX_A;
	CMatrix MATRIX_Q = MATRIX_AT * MATRIX_Z;
	CMatrix MATRIX_PARA = MATRIX_P.getInverse() * MATRIX_Q;
	
	//得到拟合方程系数
	for (int i = 0 ; i < 6; i++)
	{
		pPara[i] = MATRIX_PARA.ptr[i];
	}

	delete [] pZ;
	delete [] pArgumentMat;
	return TRUE;
}

BOOL CInclinometerFit::GetIntersectOfLine( double* pPara,double ptX1,double ptY1,double ptZ1, double ptX2,double ptY2,double ptZ2, double& outX,double& outY,double& outZ )
{
	//COORDINATE_TYPE resultPoint;

	double m,n,p;		//单位向量
	double vecDis;		//向量长度
	double t1,t2;		//空间直线参数方程
	double a,b,c,d,e,f;//曲面拟合参数

	//1.获取曲面拟合参数
	a = pPara[0];	b = pPara[1];	c = pPara[2];
	d = pPara[3];	e = pPara[4];	f = pPara[5];

	//2.得到空间单位向量
	double vec_X = ptX2 - ptX1;
	double vec_Y = ptY2 - ptY1;
	double vec_Z = ptZ2 - ptZ1;
	vecDis = vec_X*vec_X + vec_Y*vec_Y + vec_Z*vec_Z;
	vecDis = sqrt(vecDis);
	m = vec_X/vecDis;	n = vec_Y/vecDis;	p = vec_Z/vecDis;

	//3.联立曲面方程与空间直线参数方程，解求空间直线参数，并计算直线与曲面交点
	//空间曲面方程
	// z = f(x,y) = a*x^2 + b*x*y + c*y^2 + d*x + e*y + f
	//空间直线参数方程
	// x = x0 + m*t; y = y0 + n*t; z = z0 + p*t
	//联立两个方程，则可以得到
	// z0+p*t = a*(x0+m*t)^2 + b*(x0+m*t)*(y0+n*t) + c*(y0+n*t)^2 + d*(x0+m*t) + e*(y0+n*t) + f
	//利用matlab求解参数t，得到方程两个根
	double x = ptX1;	double y = ptY1;	double z = ptZ1;

	t1 = -1.0/2*(b*m*y+b*x*n+e*n+2*m*x*a+m*d+2*c*y*n-p-sqrt(-2*m*d*p+b*b*x*x*n*n+b*b*m*m*y*y+2*b*m*m*y*d
		+2*b*x*n*n*e+2*e*n*m*d-2*b*m*y*p-2*b*x*n*p-4*m*x*a*p-4*c*y*n*p-2*e*n*p-4*m*m*a*f+4*m*m*a*z-4*c*n*n*f
		+4*c*n*n*z+e*e*n*n+m*m*d*d+p*p+8*m*x*a*c*y*n-2*b*b*m*y*x*n-2*b*m*y*e*n-2*b*x*n*m*d+4*e*n*m*x*a+4*m*d*c*y*n
		-4*m*m*a*e*y-4*m*m*a*c*y*y-4*c*n*n*a*x*x-4*c*n*n*d*x-4*b*m*n*f+4*b*m*n*z))/(m*m*a+b*m*n+c*n*n);

	t2 = -1.0/2*(b*m*y+b*x*n+e*n+2*m*x*a+m*d+2*c*y*n-p+sqrt(-2*m*d*p+b*b*x*x*n*n+b*b*m*m*y*y+2*b*m*m*y*d+2*b*x*n*n*e+
		2*e*n*m*d-2*b*m*y*p-2*b*x*n*p-4*m*x*a*p-4*c*y*n*p-2*e*n*p-4*m*m*a*f+4*m*m*a*z-4*c*n*n*f+4*c*n*n*z+e*e*n*n+
		m*m*d*d+p*p+8*m*x*a*c*y*n-2*b*b*m*y*x*n-2*b*m*y*e*n-2*b*x*n*m*d+4*e*n*m*x*a+4*m*d*c*y*n-4*m*m*a*e*y-4*m*m*a*c*y*y
		-4*c*n*n*a*x*x-4*c*n*n*d*x-4*b*m*n*f+4*b*m*n*z))/(m*m*a+b*m*n+c*n*n);

	/*COORDINATE_TYPE candidate_1,candidate_2;
	candidate_1.x = ptX1 + m * t1;		candidate_2.x = ptX2 + m * t2;	
	candidate_1.y = ptY1 + n * t1;		candidate_2.y = ptY2 + n * t2;
	candidate_1.z = ptZ1 + p * t1;		candidate_2.z = ptZ2 + p * t2;*/

	//4.比较两个解，选取合适的值
	/*if(candidate_1.z < 0)
		resultPoint = candidate_1;
	else
		resultPoint = candidate_2;

	return resultPoint;*/
	return TRUE;
}

void CInclinometerFit::RectifyPoint( double obs,float angleZ,float& x,float& y,float& z )
{
	if (!HasInc())
	{
		return;
	}
	float tmpX = x;
	float tmpY = y;
	float tmpZ = z;
	x = (float)(m_rotateMat[0]*tmpX + m_rotateMat[1]*tmpY + m_rotateMat[2]*tmpZ);
	y = (float)(m_rotateMat[3]*tmpX + m_rotateMat[4]*tmpY + m_rotateMat[5]*tmpZ);
	z = (float)(m_rotateMat[6]*tmpX + m_rotateMat[7]*tmpY + m_rotateMat[8]*tmpZ);
}

void CInclinometerFit::SetError( double errRoll,double errPitch )
{	
	m_errorRoll = errRoll;
	m_errorPitch = errPitch;
}

}