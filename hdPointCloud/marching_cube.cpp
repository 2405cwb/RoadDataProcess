#include "StdAfx.h"
#include "marching_cube.h"
#include "..\hdCommon\hdkdtree.hpp"

using namespace hdkdtree;

namespace hd
{
	// 定义三维点云kdtree索引
	typedef KDTreeSingleIndexAdaptor<
		L2_Simple_Adaptor<float, PointCloud >,
		PointCloud,
		3 /* dim */
	> hd_kd_tree_t;

	marching_cube::marching_cube(void)
		:m_res_x(50),m_res_y(50),m_res_z(50),m_percentage_extend_grid(0.0f),
		m_off_surface(0.01f),m_pcd(NULL)
	{
	}


	marching_cube::~marching_cube(void)
	{
	}

	float marching_cube::getGridValue (Eigen::Vector3i pos)
	{
		/// TODO what to return?
		if (pos[0] < 0 || pos[0] >= m_res_x) //res_x_
			return -1.0f;
		if (pos[1] < 0 || pos[1] >= m_res_y)//res_y_
			return -1.0f;
		if (pos[2] < 0 || pos[2] >= m_res_z)//res_z_
			return -1.0f;

		//return grid_[pos[0]*res_y_*res_z_ + pos[1]*res_z_ + pos[2]];
		return m_grid[pos[0]*m_res_y*m_res_z + pos[1]*m_res_z + pos[2]];
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	void marching_cube::voxelizeData ()
	{
		float xyz[3];
		hd_kd_tree_t* kdIndex = (hd_kd_tree_t*)m_pcd->getKdIndex();

		std::vector<size_t> nn_indices(1);
		std::vector<float> nn_sqr_dists(1);

		for (int x = 0; x < m_res_x; ++x)//res_x_
			for (int y = 0; y < m_res_y; ++y)//res_y_
				for (int z = 0; z < m_res_z; ++z)//res_z_
				{
					xyz[0] = m_min_p.x + (m_max_p.x - m_min_p.x) * x / m_res_x;//res_x_
					xyz[1] = m_min_p.y + (m_max_p.y - m_min_p.y) * y / m_res_y;//res_y_
					xyz[2] = m_min_p.z + (m_max_p.z - m_min_p.z) * z / m_res_z;//res_z_

					kdIndex->knnSearch(xyz,1,nn_indices._Myfirst(),nn_sqr_dists._Myfirst());
					if (nn_sqr_dists[0] > 0.2)
					{
						continue;
					}
					
					size_t indices = nn_indices[0];
					const PointXYZIPRGBA& pt = (*m_pcd)[indices];
					const Normal& normal = m_pcd->getNormal(indices);

					//grid_[x * res_y_*res_z_ + y * res_z_ + z] = input_->points[nn_indices[0]].getNormalVector3fMap ().dot (
					//	point - input_->points[nn_indices[0]].getVector3fMap ());

					xyz[0] -= pt.x;
					xyz[1] -= pt.y;
					xyz[2] -= pt.z;

					m_grid[x * m_res_y * m_res_z + y * m_res_z + z] =
						xyz[0] * normal.nx + xyz[1] * normal.ny + xyz[2] * normal.nz;
				}
	}

	void marching_cube::getNeighborList1D (std::vector<float> &leaf,
		Eigen::Vector3i &index3d)
	{
		leaf = std::vector<float> (8, 0.0f);

		leaf[0] = getGridValue (index3d);
		leaf[1] = getGridValue (index3d + Eigen::Vector3i (1, 0, 0));
		leaf[2] = getGridValue (index3d + Eigen::Vector3i (1, 0, 1));
		leaf[3] = getGridValue (index3d + Eigen::Vector3i (0, 0, 1));
		leaf[4] = getGridValue (index3d + Eigen::Vector3i (0, 1, 0));
		leaf[5] = getGridValue (index3d + Eigen::Vector3i (1, 1, 0));
		leaf[6] = getGridValue (index3d + Eigen::Vector3i (1, 1, 1));
		leaf[7] = getGridValue (index3d + Eigen::Vector3i (0, 1, 1));
	}
		
	void marching_cube::createSurface (std::vector<float> &leaf_node,
		Eigen::Vector3i &index_3d)
	{
		int cubeindex = 0;
		Eigen::Vector3f vertex_list[12];
		if (leaf_node[0] < m_iso_level) cubeindex |= 1;
		if (leaf_node[1] < m_iso_level) cubeindex |= 2;
		if (leaf_node[2] < m_iso_level) cubeindex |= 4;
		if (leaf_node[3] < m_iso_level) cubeindex |= 8;
		if (leaf_node[4] < m_iso_level) cubeindex |= 16;
		if (leaf_node[5] < m_iso_level) cubeindex |= 32;
		if (leaf_node[6] < m_iso_level) cubeindex |= 64;
		if (leaf_node[7] < m_iso_level) cubeindex |= 128;

		// Cube is entirely in/out of the surface
		if (edgeTable[cubeindex] == 0)
			return;

		//Eigen::Vector4f index_3df (index_3d[0], index_3d[1], index_3d[2], 0.0f);
		Eigen::Vector3f center;// TODO coeff wise product = min_p_ + Eigen::Vector4f (1.0f/res_x_, 1.0f/res_y_, 1.0f/res_z_) * index_3df * (max_p_ - min_p_);
		center[0] = m_min_p.x + (m_max_p.x - m_min_p.x) / m_res_x;//res_x_
		center[1] = m_min_p.y + (m_max_p.y - m_min_p.y) / m_res_y;//res_y_
		center[2] = m_min_p.z + (m_max_p.z - m_min_p.z) / m_res_z;//res_z_
			
		std::vector<Eigen::Vector3f> p;
		p.resize (8);
		for (int i = 0; i < 8; ++i)
		{
			Eigen::Vector3f point = center;
			if(i & 0x4)
				point[1] = static_cast<float> (center[1] + (m_max_p.y - m_min_p.y) / float(m_res_y));//res_y_

			if(i & 0x2)
				point[2] = static_cast<float> (center[2] + (m_max_p.z - m_min_p.z) / float(m_res_z));//res_z_

			if((i & 0x1) ^ ((i >> 1) & 0x1))
				point[0] = static_cast<float> (center[0] + (m_max_p.x - m_min_p.x) / float(m_res_x));//res_x_

			p[i] = point;
		}


		// Find the vertices where the surface intersects the cube
		if (edgeTable[cubeindex] & 1)
			interpolateEdge (p[0], p[1], leaf_node[0], leaf_node[1], vertex_list[0]);
		if (edgeTable[cubeindex] & 2)
			interpolateEdge (p[1], p[2], leaf_node[1], leaf_node[2], vertex_list[1]);
		if (edgeTable[cubeindex] & 4)
			interpolateEdge (p[2], p[3], leaf_node[2], leaf_node[3], vertex_list[2]);
		if (edgeTable[cubeindex] & 8)
			interpolateEdge (p[3], p[0], leaf_node[3], leaf_node[0], vertex_list[3]);
		if (edgeTable[cubeindex] & 16)
			interpolateEdge (p[4], p[5], leaf_node[4], leaf_node[5], vertex_list[4]);
		if (edgeTable[cubeindex] & 32)
			interpolateEdge (p[5], p[6], leaf_node[5], leaf_node[6], vertex_list[5]);
		if (edgeTable[cubeindex] & 64)
			interpolateEdge (p[6], p[7], leaf_node[6], leaf_node[7], vertex_list[6]);
		if (edgeTable[cubeindex] & 128)
			interpolateEdge (p[7], p[4], leaf_node[7], leaf_node[4], vertex_list[7]);
		if (edgeTable[cubeindex] & 256)
			interpolateEdge (p[0], p[4], leaf_node[0], leaf_node[4], vertex_list[8]);
		if (edgeTable[cubeindex] & 512)
			interpolateEdge (p[1], p[5], leaf_node[1], leaf_node[5], vertex_list[9]);
		if (edgeTable[cubeindex] & 1024)
			interpolateEdge (p[2], p[6], leaf_node[2], leaf_node[6], vertex_list[10]);
		if (edgeTable[cubeindex] & 2048)
			interpolateEdge (p[3], p[7], leaf_node[3], leaf_node[7], vertex_list[11]);

		// Create the triangle
		
		for (int i = 0; triTable[cubeindex][i] != -1; i+=3)
		{
			PointXYZ pt1,pt2,pt3;
			pt1.x = vertex_list[triTable[cubeindex][i  ]][0];
			pt1.y = vertex_list[triTable[cubeindex][i  ]][1];
			pt1.z = vertex_list[triTable[cubeindex][i  ]][2];
			m_mesh.points.push_back(pt1);

			pt2.x = vertex_list[triTable[cubeindex][i+1]][0];
			pt2.y = vertex_list[triTable[cubeindex][i+1]][1];
			pt2.z = vertex_list[triTable[cubeindex][i+1]][2];
			m_mesh.points.push_back(pt2);

			pt3.x = vertex_list[triTable[cubeindex][i+2]][0];
			pt3.y = vertex_list[triTable[cubeindex][i+2]][1];
			pt3.z = vertex_list[triTable[cubeindex][i+2]][2];
			m_mesh.points.push_back(pt3);
		}
	}

	void marching_cube::interpolateEdge (Eigen::Vector3f &p1,
		Eigen::Vector3f &p2,
		float val_p1,
		float val_p2,
		Eigen::Vector3f &output)
	{
		float mu = (m_iso_level - val_p1) / (val_p2-val_p1);
		output = p1 + mu * (p2 - p1);
	}

	void marching_cube::reconstructSurface()
	{
		if (m_off_surface <= 0.0f || m_res_x <=0 || 
			m_res_y <= 0 || m_res_z <= 0 ||m_pcd == NULL)
			return;
		
		m_pcd->computeNormal(100,0.0);
		
		m_grid.resize(m_res_x * m_res_y * m_res_z,0.0f);

		m_mesh.polygons.clear ();
		//m_mesh.polygons.reserve (2*indices_->size ()); /// NOTE: usually the number of triangles is around twice the number of vertices
		m_mesh.polygons.reserve ((u32)(2 * m_pcd->m_simpleHeader.number_of_point_records));

		voxelizeData();

		for (int x = 1; x < m_res_x-1; ++x)//res_x_
			for (int y = 1; y < m_res_y-1; ++y)//res_y_
				for (int z = 1; z < m_res_z-1; ++z)//res_z_
				{
					Eigen::Vector3i index_3d (x, y, z);
					std::vector<float> leaf_node;
					getNeighborList1D (leaf_node, index_3d);
					createSurface (leaf_node, index_3d);
				}

		m_mesh.polygons.resize(u32(m_mesh.points.count() / 3));
		for (size_t i = 0; i < m_mesh.polygons.size(); ++i)
		{
			//Vertices v;
			//v.vertices.resize (3);
			m_mesh.polygons[i].vertices.resize(3);
			for (int j = 0; j < 3; ++j)
				m_mesh.polygons[i].vertices[j] = static_cast<int> (i) * 3 + j;
		}
	}

	void marching_cube::setPointCloud(PointCloud* pcd )
	{
		if(pcd == NULL)
			return;
		m_pcd = pcd;

		m_min_p.x = m_pcd->m_simpleHeader.min_x;
		m_min_p.y = m_pcd->m_simpleHeader.min_y;
		m_min_p.z = m_pcd->m_simpleHeader.min_z;

		m_max_p.x = m_pcd->m_simpleHeader.max_x;
		m_max_p.y = m_pcd->m_simpleHeader.max_y;
		m_max_p.z = m_pcd->m_simpleHeader.max_z;

		m_res_x = (int)hd_round((m_max_p.x - m_min_p.x) / 2.0f);
		m_res_y = (int)hd_round((m_max_p.y - m_min_p.y) / 2.0f);
		m_res_z = (int)hd_round((m_max_p.z - m_min_p.z) / 2.0f);
	}

}