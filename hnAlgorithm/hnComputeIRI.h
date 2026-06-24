#pragma once
#include <vector>
#include "stdafx.h"
using namespace std;

class HNALGORITHM_API hnComputeIRI
{
public:
	hnComputeIRI(void);
	~hnComputeIRI(void);

public:

	// º∆À„IRI
	void calculateIRI(double dIntervel, vector<double> listIRIInfo,  double& irival,
		double DeltLen = 0.25);

private:

	double m_sZU[16];
	double m_pZU[4];
	double m_zSU[4];
	double m_oldZSU[4];
};

