#pragma once
#include <vector>
#include "hnCoordTranslate.h"

namespace hd
{
	class  HNCOORDTRANSLATE_API CBursaWolfModel
	{
	public:
		CBursaWolfModel(void);
		CBursaWolfModel(const CBursaWolfModel& model);
		~CBursaWolfModel(void);

		CBursaWolfModel& operator=(const CBursaWolfModel& model);
		bool operator ==(const CBursaWolfModel& model) const;
		//乘法结果定义为两个连续的七参数模型转换后的等效模型 乘法顺序与数学上顺序相反
	    //即数学上定义C = A*B，则模型C = B*A；
		CBursaWolfModel operator*(const CBursaWolfModel& model) const ;

	public:
		// 参照VTK中的算法进行参数解算 by gongshulin
		bool Create(const double* pRefX,		// 目标控制点X
			const double* pRefY,				// 目标控制点Y
			const double* pRefZ,				// 目标控制点Z
			const double* pRegX,				// 源控制点X
			const double* pRegY,				// 源控制点Y
			const double* pRegZ,				// 源控制点Z
			int n,							// 控制点数
			int transType					// 变换类型 = HD_LANDMARK_RIGIDBODY
			);		

		// 重载上面的接口 [zfei 2014/12/2]
		bool Create(const float* pRefX,		// 目标控制点X
			const float* pRefY,				// 目标控制点Y
			const float* pRefZ,				// 目标控制点Z
			const float* pRegX,				// 源控制点X
			const float* pRegY,				// 源控制点Y
			const float* pRegZ,				// 源控制点Z
			int n,							// 控制点数
			int transType					// 变换类型 = HD_LANDMARK_RIGIDBODY
			);

		// 增加降阶模型的求解 [危迟] 
		bool CreateModel(const double* pRefX,		// 目标控制点X
				const double* pRefY,				// 目标控制点Y
				const double* pRefZ,				// 目标控制点Z
				const double* pRegX,				// 源控制点X
				const double* pRegY,				// 源控制点Y
				const double* pRegZ,				// 源控制点Z
				int n,								// 控制点数
				double& RefGeoX,double& RefGeoY,double& RefGeoZ,
				double& RegGeoX,double& RegGeoY,double& RegGeoZ);
		// 降阶模型的求解 [危迟]
		bool CreateModel(const double* pRefX,		
				const double* pRefY,				
				const double* pRefZ,				
				const double* pRegX,				
				const double* pRegY,				
				const double* pRegZ,				
				int n,								
				bool bAdjust = true);

		// 通过矩阵构建模型 [2015/07/24 危迟]
		bool CreateModelByMatrix(double* M);

		// 最小二乘法求解七参数 [危迟]
		//bool CreateNew(const double* pRefX, const double* pRefY, const double* pRefZ, const double* pRegX, const double* pRegY, const double* pRegZ, int n);
		// 最小二乘法求解七参数 原始版本 yaoli
		//bool Create(const float* pRefX, const float* pRefY, const float* pRefZ, const float* pRegX, const float* pRegY, const float* pRegZ, int n);
		//bool Create(const double* pRefX, const double* pRefY, const double* pRefZ, const double* pRegX, const double* pRegY, const double* pRegZ, int n);
		
		//使用计算得到的七参数，进行坐标转换,通过模型中的4*4矩阵来计算
		bool Translate(float& x, float& y, float& z) const ;
		bool Translate(double &x,double& y,double& z) const;
		// 框选范围角点坐标转换
		bool TranslateExtent(double& xmin,double& ymin,double& zmin,double& xmax,double& ymax,double& zmax) const;
		void TranslateExtentW(double& xmin,double& ymin,double& zmin,double& xmax,double& ymax,double& zmax) const;
		// 通过模型计算出的七参数来进行坐标变换
		bool TranslateOri(float& x, float& y, float& z) const ;
		bool TranslateOri(double &x,double& y,double& z) const;

		//bool TranslateGravity(float& x, float& y, float& z) const ;		// 点云显示绘制时调用，减去目标坐标系的重心坐标
		//bool TranslateGravity(double &x,double& y,double& z) const;
		//使用计算得到的七参数，进行坐标逆转
		bool AntiTranslate(float& x, float& y, float& z) const;
		bool AntiTranslate(double& x,double& y,double& z) const;
		//bool AntiTranslateGravity(float& x, float& y, float& z);
		//bool AntiTranslateGravity(double &x,double& y,double& z);
		//得到本转换模型的逆模型
		CBursaWolfModel getAntiModel() const;
		void getIdentity();
		bool IsIdentity() const ;
		// phi,omega,kappa角度单位是弧度
		void ComputeRotateMatrixByAngle(double phi,double omega,double kappa,double *R);

		// 根据iScan输出角度yaw,pitch,roll构建旋转矩阵 角度单位是弧度 [2014/07/17 危迟]
		// 旋转顺序是Z、X、Y，也即Yaw,Pitch,Roll，但由于构建过程旋转矩阵进行了转置，所以其实际的旋转顺序Y、X、Z
		// 坐标系绕自身Y轴顺时针旋转roll角，绕X轴顺时针旋转pitch角，绕Z轴逆时针旋转yaw角 [2014/11/07 危迟]
		void ComputeRotateMatrixByIScanAngle(double Yaw,double Pitch,double Roll,double *R);

		// 根据旋转矩阵输出iScan中定义yaw,pitch,roll 角度单位是弧度 [2014/07/17 危迟]
		// 旋转顺序是Z、X、Y，也即Yaw,Pitch,Roll，但由于构建过程旋转矩阵进行了转置，所以其实际的旋转顺序Y、X、Z
		// 坐标系绕自身Y轴顺时针旋转roll角，绕X轴顺时针旋转pitch角，绕Z轴逆时针旋转yaw角 [2014/11/07 危迟]
		void ComputeIScanAngleByRotateMatrix(double& Yaw,double& Pitch,double& Roll);

		// 根据iScan角度构建旋转矩阵 角度单位是弧度 [2014/07/17 危迟]
		void IScanAngle2RotateMatrix(double Yaw,double Pitch,double Roll);

		// 根据irr角度angleX，angleY，angleZ构建旋转矩阵 角度单位是弧度 [2014/07/19 危迟]
		// 旋转顺序为Z、Y、X
		// 几何意义为坐标轴绕自身Z轴逆时针旋转angleZ，再次绕Y轴逆时针旋转angleY，最后绕X轴逆时针旋转angleX [2014/11/07 危迟]
		void ComputeRotateMatrixByIrrAngle(double angleX,double angleY,double angleZ,double *R);

		// 根据旋转矩阵输出Irr角度angleX,angleY,angleZ 角度单位是弧度 [2014/07/19 危迟]
		// 旋转顺序为Z、Y、X
		// 几何意义为坐标轴绕自身Z轴逆时针旋转angleZ，再次绕Y轴逆时针旋转angleY，最后绕X轴逆时针旋转angleX [2014/11/07 危迟]
		void ComputeIrrAngleByRotateMatrix(double& angleX,double& angleY,double& angleZ);

		// 根据Irr角度构建旋转矩阵 角度单位是弧度 [2014/07/19 危迟]
		void IrrAngle2RotateMatrix(double angleX,double angleY,double angleZ);

		// 根据旋转中心、旋转角度(单位是角度)、旋转轴向量构建模型参数 
		// 首先构建4*4矩阵 然后再转换为7个参数存储 [2014/03/23 危迟]
		void BuildModelByRotateVectorAndCenter(	double degree,								 // 旋转角度	
												double centerX,double centerY,double centerZ, // 旋转中心
												double vecterX,double vecterY,double vecterZ);// 旋转轴向量

		// 点P绕X轴旋转，求解旋转后的坐标需要的旋转矩阵R 逆时针为正 [2015/04/15 危迟]
		void CalculateRotateMatrixByRotateXAxis(double omega);

		// 点P绕Y轴旋转，求解旋转后的坐标需要的旋转矩阵R 顺时针为正 [2015/04/15 危迟]
		void CalculateRotateMatrixByRotateYAxis(double phi);

		// 点P绕Z轴旋转，求解旋转后的坐标需要的旋转矩阵R 逆时针为正 [2015/04/15 危迟]
		void CalculateRotateMatrixByRotateZAxis(double kappa);

		//7参数转换为矩阵
		void Parameter2matrix();
		//三个角度转换成旋转矩阵
		void Angle2RotateMatrix();
	//private:
		// 将矩阵转换为6参数
		void matrix2Parameter();

		// 旋转矩阵获得旋转角(欧拉角)[zhangfei 2014/5/9]
		void ComputeAngleByRotateMatrix();

		// 设置平移参数（朱立雄 2017-3-18）
		void SetOffset(double X, double Y, double Z){ m_fOffset[0] = X; m_fOffset[1] = Y; m_fOffset[2] = Z; }

		// 设置角度参数（朱立雄 2017-3-18）
		void SetRotation(double phi, double omega, double kappa){ m_fAngle[0] = phi; m_fAngle[1] = omega; m_fAngle[2] = kappa; }

	public:
		double	m_fScale;			//一个缩放因子
		double	m_fOffset[3];		//三个平移量
	//	double  m_fOffsetGravity[3];//重心化后三个平移量 ——用于显示
	//	double	m_dGravityCenter[3];//目标坐标系统的重心坐标 
		// HDScene转角系统采用以Y轴为主轴的phi-omega-kappa系统 [2014/04/29 危迟]
		// 三个旋转角代表着从目标坐标系依次绕自身坐标系进行旋转后，变为源坐标系，也即源坐标系在目标坐标系中的姿态 [2015/09/23]
		// 即以Y轴为主轴旋转phi角，然后以X轴为主轴旋转omega角，最后绕Z轴旋转kappa角
		// phi为顺时针旋转角度，omega为逆时针旋转角度，kappa角为逆时针旋转角度 [2014/11/06 危迟]
		double	m_fAngle[3];		//三个旋转角, 仅作为调试时查看，不存储到hws文件中 单位为弧度 
									//m_angle[0] 表示phi角,注意这里是绕Y轴旋转角,对应HLS rotateY
									//m_angle[1] 表示omega角,注意这里是绕x轴旋转,对应HLS rotateX
									//m_angle[2] 表示kappa角,绕Z轴旋转
		double	m_fRotateMatrix[9];	//旋转矩阵

		double	m_fError;			//拼接中误差

		double m_matrix[4][4];
		//irr::core::matrix4d m_matrix;
	};


}
