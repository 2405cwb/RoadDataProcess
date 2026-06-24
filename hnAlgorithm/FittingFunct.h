#ifndef FITTINGFUNCT_H
#define FITTINGFUNCT_H

#endif // FITTINGFUNCT_H
#include <vector>
#include <cmath>

using namespace std;

class FittingFunct {
public:
	// 多项式拟合函数，输出系数是y=a0+a1*x+a2*x*x+.........，按a0,a1,a2输出
	static vector<double> Polyfit(vector<double> y, vector<double> x, int order) {
		vector<vector<double>> guass = Get_Array(y, x, order);
		vector<double> ratio = Cal_Guass(guass, order + 1);
		return ratio;
	}

	// 一次拟合函数，y=a0+a1*x,输出次序是a0,a1
	static vector<double> Linear(vector<double> y, vector<double> x) {
		vector<double> ratio = Polyfit(y, x, 1);
		return ratio;
	}

	// 一次拟合函数，截距为0，y=a0x,输出次序是a0
	static vector<double> LinearInterceptZero(vector<double> y, vector<double> x) {
		double divisor = 0; //除数
		double dividend = 0; //被除数
		for (int i = 0; i < x.size(); i++) {
			divisor += x[i] * x[i];
			dividend += x[i] * y[i];
		}
		if (divisor == 0) {
			throw("除数不为0！");
		}
		return vector<double>{dividend / divisor};
	}

	// 二次拟合函数，y=a0+a1*x+a2x²,输出次序是a0,a1,a2
	static vector<double> TowTimesCurve(vector<double> y, vector<double> x) {
		vector<double> ratio = Polyfit(y, x, 2);
		return ratio;
	}

	// 对数拟合函数,.y= c*(ln x)+b,输出为b,c
	static vector<double> LOGEST(vector<double> y, vector<double> x) {
		vector<double> lnX(x.size());

		for (int i = 0; i < x.size(); i++) {
			if (x[i] == 0 || x[i] < 0) {
				throw("正对非正数取对数！");
			}
			lnX[i] = log(x[i]);
		}

		return Linear(y, lnX);
	}

	// 幂函数拟合模型, y=c*x^b,输出为c,b
	static vector<double> PowEST(vector<double> y, vector<double> x) {
		vector<double> lnX(x.size());
		vector<double> lnY(y.size());
		vector<double> dlinestRet;

		for (int i = 0; i < x.size(); i++) {
			lnX[i] = log(x[i]);
			lnY[i] = log(y[i]);
		}

		dlinestRet = Linear(lnY, lnX);

		dlinestRet[0] = exp(dlinestRet[0]);

		return dlinestRet;
	}

	// 指数函数拟合函数模型，公式为 y=c*m^x;输出为 c,m
	static vector<double> IndexEST(vector<double> y, vector<double> x) {
		vector<double> lnY(y.size());
		vector<double> ratio;
		for (int i = 0; i < y.size(); i++) {
			lnY[i] = log(y[i]);
		}

		ratio = Linear(lnY, x);
		for (int i = 0; i < ratio.size(); i++) {
			if (i == 0) {
				ratio[i] = exp(ratio[i]);
			}
		}
		return ratio;
	}

	// 相关系数R²部分
	static double Pearson(vector<double> dataA, vector<double> dataB) {
		int n = 0;
		double r = 0.0;
		double meanA = 0;
		double meanB = 0;
		double varA = 0;
		double varB = 0;
		int ii = 0;

		auto ieA = dataA.begin();
		auto ieB = dataB.begin();
		while (ieA != dataA.end()) {
			if (ieB == dataB.end()) {
				throw("两个输入数组长度不同，无法进行计算！");
			}
			n++;
			double deltaA = *ieA - meanA;
			double deltaB = *ieB - meanB;
			meanA += deltaA / n;
			meanB += deltaB / n;
			varA += deltaA * (*ieA - meanA);
			varB += deltaB * (*ieB - meanB);
			r += deltaA * deltaB * (n - 1.0) / n;
			ieA++;
			ieB++;
		}
		if (ieB != dataB.end()) {
			throw("两个输入数组长度不同，无法进行计算！");
		}
		if (n < 2) {
			throw("数据样本太小，无法进行计算！");
		}
		double denominator = sqrt(varA * varB);
		if (denominator == 0) {
			return 1.0;
		}
		else {
			return pow(r, 2) / (varA / (n - 1) * varB / (n - 1));
		}
	}
private:
	// 高斯消元法求解线性方程组
	static vector<double> Cal_Guass(vector<vector<double>> guass, int count)
	{
		double temp;
		vector<double> x_value;

		for (int j = 0; j < count; j++)
		{
			int k = j;
			double min = guass[j][j];

			for (int i = j; i < count; i++)
			{
				if (fabs(guass[i][ j]) < min)
				{
					min = guass[i][ j];
					k = i;
				}
			}

			if (k != j)
			{
				for (int x = j; x <= count; x++)
				{
					temp = guass[k][ x];
					guass[k][ x] = guass[j][x];
					guass[j][x] = temp;
				}
			}

			for (int m = j + 1; m < count; m++)
			{
				double div = guass[m][ j] / guass[j][ j];
				for (int n = j; n <= count; n++)
				{
					guass[m][n] = guass[m][ n] - guass[j][ n] * div;
				}
			}

			/* System.Console.WriteLine("初等行变换：");
			for (int i = 0; i < count; i++)
			{
			for (int m = 0; m < count + 1; m++)
			{
			System.Console.Write("{0,10:F6}", guass[i, m]);
			}
			Console.WriteLine();
			}*/
		}
		x_value = Get_Value(guass, count);

		return x_value; 
	}
	 
	 
private:
	// 高斯消元法求解线性方程组
	static vector<double> Get_Value(vector<vector<double>> guass, int count)
	{
		vector<double>  x(count);
		vector<vector<double>>X_Array (count,vector<double>(count)) ;
		//int rank = guass.Rank;//秩是从0开始的

		for (int i = 0; i < count; i++)
			for (int j = 0; j < count; j++)
				X_Array[i][j] = guass[i][j];

		if (X_Array.size() < guass.size())//表示无解
		{
			return{};
		}

		if (X_Array.size() < count - 1)//表示有多解
		{
			return{};
		}
		//回带计算x值
		x[count - 1] = guass[count - 1][ count] / guass[count - 1][ count - 1];
		for (int i = count - 2; i >= 0; i--)
		{
			double temp = 0;
			for (int j = i; j < count; j++)
			{
				temp += x[j] * guass[i][j];
			}
			x[i] = (guass[i][count] - temp) / guass[i][ i];
		}

		return x;
	}
	// 生成高斯消元法的增广矩阵
	static vector<vector<double>> Get_Array(vector<double> y, vector<double> x, int order)
	{
		int row = order + 1;
		int col = order + 2;
		vector<vector<double>> guass(row, vector<double>(col, 0));

		for (int i = 0; i < row; i++) {
			for (int j = 0; j < col - 1; j++) {
				double temp = 0;
				for (int k = 0; k < x.size(); k++) {
					temp += pow(x[k], i + j);
				}
				guass[i][j] = temp;
			}
		}

		for (int i = 0; i < row; i++) {
			double temp = 0;
			for (int j = 0; j < x.size(); j++) {
				temp += y[j] * pow(x[j], i);
			}
			guass[i][col - 1] = temp;
		}

		return guass;
	}
};

