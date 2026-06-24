#ifndef _HNPLANEFIT_H_
#define _HNPLANEFIT_H_

#include <vector>
#include "Eigen/Eigenvalues"
#include "Eigen/Dense"
#include "hnDataCombineStructInfo.h"

using namespace Eigen;
using namespace std;

class Class_FitPlane
{
public:
	bool LSPlaneFit(vector<hnPoint3d> &vecpoints,vector<double> &m_PlaneParameters);

	void LSPlaneFit_Denoise(vector<hnPoint3d> &vecpoints,vector<double> &m_PlaneParameters);

private:
	vector<double> m_PlaneParameters;

};


#endif