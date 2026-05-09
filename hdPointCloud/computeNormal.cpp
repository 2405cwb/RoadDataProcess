#include "StdAfx.h"
#include "computeNormal.h"
#include "..\hdCommon\eigen.h"

//////////////////////////////////////////////////////////////////////////////////////////////
unsigned int computeMeanAndCovarianceMatrix (
	const std::vector<Eigen::Vector3f> &pts,
	Eigen::Matrix3f &covariance_matrix,
	Eigen::Vector4f &centroid)
{
	// create the buffer on the stack which is much faster than using cloud.points[indices[i]] and centroid as a buffer
	Eigen::Matrix<float, 1, 9, Eigen::RowMajor> accu = Eigen::Matrix<float, 1, 9, Eigen::RowMajor>::Zero ();
	size_t point_count;

	point_count = pts.size ();
	for (std::vector<Eigen::Vector3f>::const_iterator iIt = pts.begin (); iIt != pts.end (); ++iIt)
	{
		const Eigen::Vector3f& point = (*iIt);
		accu [0] += point[0] * point[0];
		accu [1] += point[0] * point[1];
		accu [2] += point[0] * point[2];
		accu [3] += point[1] * point[1];

		accu [4] += point[1] * point[2];
		accu [5] += point[2] * point[2];
		accu [6] += point[0];
		accu [7] += point[1];
		accu [8] += point[2];
	}			

	accu /= static_cast<float> (point_count);
	centroid[0] = accu[6]; centroid[1] = accu[7]; centroid[2] = accu[8];
	centroid[3] = 0;
	covariance_matrix.coeffRef (0) = accu [0] - accu [6] * accu [6];
	covariance_matrix.coeffRef (1) = accu [1] - accu [6] * accu [7];
	covariance_matrix.coeffRef (2) = accu [2] - accu [6] * accu [8];
	covariance_matrix.coeffRef (4) = accu [3] - accu [7] * accu [7];
	covariance_matrix.coeffRef (5) = accu [4] - accu [7] * accu [8];
	covariance_matrix.coeffRef (8) = accu [5] - accu [8] * accu [8];
	covariance_matrix.coeffRef (3) = covariance_matrix.coeff (1);
	covariance_matrix.coeffRef (6) = covariance_matrix.coeff (2);
	covariance_matrix.coeffRef (7) = covariance_matrix.coeff (5);

	return (static_cast<unsigned int> (point_count));
}

//////////////////////////////////////////////////////////////////////////////////////////////
unsigned int computeMeanAndCovarianceMatrix (
	const std::vector<PointXYZIPRGBA> &pts,
	Eigen::Matrix3f &covariance_matrix,
	Eigen::Vector4f &centroid)
{
	// create the buffer on the stack which is much faster than using cloud.points[indices[i]] and centroid as a buffer
	Eigen::Matrix<float, 1, 9, Eigen::RowMajor> accu = Eigen::Matrix<float, 1, 9, Eigen::RowMajor>::Zero ();
	size_t point_count;

	point_count = pts.size ();
	for (std::vector<PointXYZIPRGBA>::const_iterator iIt = pts.begin (); iIt != pts.end (); ++iIt)
	{
		const PointXYZIPRGBA& point = (*iIt);
		accu [0] += point.x * point.x;
		accu [1] += point.x * point.y;
		accu [2] += point.x * point.z;
		accu [3] += point.y * point.y;
		accu [4] += point.y * point.z;
		accu [5] += point.z * point.z;
		accu [6] += point.x;
		accu [7] += point.y;
		accu [8] += point.z;
	}			

	accu /= static_cast<float> (point_count);
	centroid[0] = accu[6]; centroid[1] = accu[7]; centroid[2] = accu[8];
	centroid[3] = 0;
	covariance_matrix.coeffRef (0) = accu [0] - accu [6] * accu [6];
	covariance_matrix.coeffRef (1) = accu [1] - accu [6] * accu [7];
	covariance_matrix.coeffRef (2) = accu [2] - accu [6] * accu [8];
	covariance_matrix.coeffRef (4) = accu [3] - accu [7] * accu [7];
	covariance_matrix.coeffRef (5) = accu [4] - accu [7] * accu [8];
	covariance_matrix.coeffRef (8) = accu [5] - accu [8] * accu [8];
	covariance_matrix.coeffRef (3) = covariance_matrix.coeff (1);
	covariance_matrix.coeffRef (6) = covariance_matrix.coeff (2);
	covariance_matrix.coeffRef (7) = covariance_matrix.coeff (5);

	return (static_cast<unsigned int> (point_count));
}

//////////////////////////////////////////////////////////////////////////////////////////////
unsigned int computeMeanAndCovarianceMatrix (
	const std::vector<PointXYZIPRGBA*> &pts,
	Eigen::Matrix3f &covariance_matrix,
	Eigen::Vector4f &centroid)
{
	// create the buffer on the stack which is much faster than using cloud.points[indices[i]] and centroid as a buffer
	Eigen::Matrix<float, 1, 9, Eigen::RowMajor> accu = Eigen::Matrix<float, 1, 9, Eigen::RowMajor>::Zero ();
	size_t point_count;

	point_count = pts.size ();
	for (std::vector<PointXYZIPRGBA*>::const_iterator iIt = pts.begin (); iIt != pts.end (); ++iIt)
	{
		const PointXYZIPRGBA* point = (*iIt);
		accu [0] += point->x * point->x;
		accu [1] += point->x * point->y;
		accu [2] += point->x * point->z;
		accu [3] += point->y * point->y;
		accu [4] += point->y * point->z;
		accu [5] += point->z * point->z;
		accu [6] += point->x;
		accu [7] += point->y;
		accu [8] += point->z;
	}			

	accu /= static_cast<float> (point_count);
	centroid[0] = accu[6]; centroid[1] = accu[7]; centroid[2] = accu[8];
	centroid[3] = 0;
	covariance_matrix.coeffRef (0) = accu [0] - accu [6] * accu [6];
	covariance_matrix.coeffRef (1) = accu [1] - accu [6] * accu [7];
	covariance_matrix.coeffRef (2) = accu [2] - accu [6] * accu [8];
	covariance_matrix.coeffRef (4) = accu [3] - accu [7] * accu [7];
	covariance_matrix.coeffRef (5) = accu [4] - accu [7] * accu [8];
	covariance_matrix.coeffRef (8) = accu [5] - accu [8] * accu [8];
	covariance_matrix.coeffRef (3) = covariance_matrix.coeff (1);
	covariance_matrix.coeffRef (6) = covariance_matrix.coeff (2);
	covariance_matrix.coeffRef (7) = covariance_matrix.coeff (5);

	return (static_cast<unsigned int> (point_count));
}

inline void solvePlaneParameters(const Eigen::Matrix3f &covariance_matrix,
	float &nx, float &ny, float &nz,float& curvature)
{
	EIGEN_ALIGN16 Eigen::Vector3f::Scalar eigen_value;
	EIGEN_ALIGN16 Eigen::Vector3f eigen_vector;
	hd::eigen33 (covariance_matrix, eigen_value, eigen_vector);

	nx = eigen_vector [0];
	ny = eigen_vector [1];
	nz = eigen_vector [2];

	// Compute the curvature surface change
	float eig_sum = covariance_matrix.coeff (0) + covariance_matrix.coeff (4) + covariance_matrix.coeff (8);
	if (eig_sum != 0)
		curvature = fabsf (eigen_value / eig_sum);
	else
		curvature = 0;
}

inline void flipNormalTowardsViewpoint (const Eigen::Vector3f &point, float vp_x, float vp_y, float vp_z,
	float &nx, float &ny, float &nz)
{
	// See if we need to flip any plane normals
	vp_x -= point[0];
	vp_y -= point[1];
	vp_z -= point[2];

	// Dot product between the (viewpoint - point) and the plane normal
	float cos_theta = (vp_x * nx + vp_y * ny + vp_z * nz);

	// Flip the plane normal
	if (cos_theta < 0)
	{
		nx *= -1;
		ny *= -1;
		nz *= -1;
	}
}

inline void computePointNormal ( const std::vector<Eigen::Vector3f> &pts,float &nx, float &ny, float &nz, float &curvature)
{
	EIGEN_ALIGN16 Eigen::Matrix3f covariance_matrix;
	Eigen::Vector4f				xyz_centroid;
	if (computeMeanAndCovarianceMatrix (pts, covariance_matrix, xyz_centroid) == 0)
	{
		nx = ny = nz = std::numeric_limits<float>::quiet_NaN ();
		return;
	}

	// Get the plane normal and surface curvature
	solvePlaneParameters (covariance_matrix, nx, ny, nz,curvature);
}

void computePointNormal( const std::vector<PointXYZIPRGBA>& pts,float &nx, float &ny, float &nz, float &curvature )
{
	EIGEN_ALIGN16 Eigen::Matrix3f covariance_matrix;
	Eigen::Vector4f				xyz_centroid;
	if (computeMeanAndCovarianceMatrix (pts, covariance_matrix, xyz_centroid) == 0)
	{
		nx = ny = nz = std::numeric_limits<float>::quiet_NaN ();
		return;
	}

	// Get the plane normal and surface curvature
	solvePlaneParameters (covariance_matrix, nx, ny, nz,curvature);
}

HDPOINTCLOUD_API void computePointNormal( const std::vector<PointXYZIPRGBA*>& pts,float &nx, float &ny, float &nz, float &curvature )
{
	EIGEN_ALIGN16 Eigen::Matrix3f covariance_matrix;
	Eigen::Vector4f				xyz_centroid;
	if (computeMeanAndCovarianceMatrix (pts, covariance_matrix, xyz_centroid) == 0)
	{
		nx = ny = nz = std::numeric_limits<float>::quiet_NaN ();
		return;
	}

	// Get the plane normal and surface curvature
	solvePlaneParameters (covariance_matrix, nx, ny, nz,curvature);
}
