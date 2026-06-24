#pragma once
#include "..\hdCommon\point_types.h"
#include <vector>
#include "hdPointCloud.h"
#include "point_cloud.h"

using namespace std;
namespace hd
{
	class HDPOINTCLOUD_API CFitSphereNoR
	{
	public:
		CFitSphereNoR(void);
		~CFitSphereNoR(void);
	public:
		bool CFitSphereNoR::sphereFitting(vector<PointXYZ>& vcPts);

	public:
		float* m_xyzR;
	};
}