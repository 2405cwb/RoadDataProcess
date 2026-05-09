#include "stdafx.h"
#include "HdGeoDataset.h"
#include "ogr_spatialref.h"

using namespace hd::scene;

CHdSpatialReference::CHdSpatialReference()
{
	m_pOgrSR = NULL;
}
CHdSpatialReference::~CHdSpatialReference()
{
	delete m_pOgrSR;
}
void CHdSpatialReference::CreateFromWKT(const char* pcWkt)
{

}
void CHdSpatialReference::CreateFromEPSG(int epsg)
{

}
bool CHdSpatialReference::IsGeographic() const
{
	return false;
}
bool CHdSpatialReference::IsProjected() const
{
	return false;
}
bool CHdSpatialReference::IsGeocentric() const
{
	return false;
}
bool CHdSpatialReference::IsSame(const CHdSpatialReference& anotherSR) const
{
	return false;
}
CHdSpatialReference* CHdSpatialReference::Clone() const
{
	return NULL;
}
