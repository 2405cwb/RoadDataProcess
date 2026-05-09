#include "stdafx.h"
#include "HdDemSceneNode.h"
#include "..\..\hdCommon\HdGridObject.h"
#include "HdRasterDataset.h"
#include "HdRasterRenderer.h"
#include "HdImage.h"
#include "..\..\hdCore\hdColor.h"

using namespace hd::scene;

CHdDemSceneNode::CHdDemSceneNode(ISceneNode* parent, ISceneManager* mgr, irr::s32 id, CHdRasterDataset* pDataset):
	IObjectSceneNode(NULL, video::SColor(), video::SColor(), parent, mgr, id)
{
	m_pDemDataset = pDataset;
	m_pDemDataset->Reference();
	m_pTextureDataset = NULL;
	m_dfBaseZ = 0;
	m_nMeshBufferSize = 21;
	m_nMeshBufferRows = 30;
	m_nTextureSize = 2048;

	// 创建色带
	m_pColorRamp = NULL;
	//unsigned char pnBeginColor[4] = {255, 255, 255, 255};
	//unsigned char pnEndColor[4] = {0, 0, 0, 255};
	//m_pColorRamp = new CHdColorRamp(*((unsigned int*)pnBeginColor), *((unsigned int*)pnEndColor));

	// 获取 DEM 的无效值
	double dfNoDataValue;
	if (m_pDemDataset->GetNoDataValue(1, dfNoDataValue))
	{
		m_fNoDataZ = dfNoDataValue;
	}
	else
	{
		m_fNoDataZ = - 10000;
	}

	// 获取 DEM 的高程范围
	double dfMinZ, dfMaxZ;
	m_pDemDataset->GetMinMaxValue(1, dfMinZ, dfMaxZ);
	m_fMinZ = dfMinZ;
	m_fMaxZ = dfMaxZ;

	// 获取 DEM 在局部坐标系下的地理范围
	const CHdGeoEnvelope* pGeoEnv = m_pDemDataset->GetGeoEnvelope();
	float fLeftX = 0;
	float fRightX = pGeoEnv->GetMaxX() - pGeoEnv->GetMinX();
	float fUpY = pGeoEnv->GetMaxY() - pGeoEnv->GetMinY();
	float fDownY = 0;

	// 创建网格
	m_pMesh = new SMesh;
	for (int i = 0; i < m_nMeshBufferRows; i ++)
	{
		for (int j = 0; j < m_nMeshBufferRows; j ++)
		{
			SMeshBuffer* pMeshBuffer = new SMeshBuffer;
			pMeshBuffer->Material.Lighting = false;
			pMeshBuffer->Material.ZBuffer = video::ECFN_LESSEQUAL;
			pMeshBuffer->Material.ZWriteEnable = true;

			pMeshBuffer->setHardwareMappingHint(irr::scene::EHM_DYNAMIC);

			pMeshBuffer->Vertices.reallocate(m_nMeshBufferSize * m_nMeshBufferSize);
			pMeshBuffer->Indices.reallocate((m_nMeshBufferSize - 1) * (m_nMeshBufferSize - 1) * 2 * 3);

			m_pMesh->addMeshBuffer(pMeshBuffer);
		}
	}
	m_pMesh->BoundingBox.MinEdge.set(fLeftX, fDownY, m_fMinZ);
	m_pMesh->BoundingBox.MaxEdge.set(fRightX, fUpY, m_fMaxZ);

	// 初始化从局部坐标系到全局坐标系、场景坐标系的变换模型和变换矩阵
	double pdfParam[4][4] = {0};
	pdfParam[0][3] = pGeoEnv->GetMinX();
	pdfParam[1][3] = pGeoEnv->GetMinY();
	pdfParam[2][3] = m_dfBaseZ;
	pdfParam[0][0] = 1;
	pdfParam[1][1] = 1;
	pdfParam[2][2] = 1;
	pdfParam[3][3] = 1;
	m_absModel.CreateModelByMatrix(pdfParam[0]);

	//CBursaWolfModel* pViewModel = m_pView->GetTransModel();
	m_renderModel = m_absModel /** (*pViewModel)*/;

	// 初始化 DEM 节点到场景根节点的变换矩阵
	setPosition(irr::core::vector3df(m_renderModel.m_fOffset[0], m_renderModel.m_fOffset[1], m_renderModel.m_fOffset[2]));
}
CHdDemSceneNode::~CHdDemSceneNode()
{
	if (m_pDemDataset->Dereference() == 0)
	{
		CHdRasterDataset::Close(m_pDemDataset);
	}
	m_pDemDataset = NULL;

	if (m_pTextureDataset && m_pTextureDataset->Dereference() == 0)
	{
		CHdRasterDataset::Close(m_pTextureDataset);
	}
	m_pTextureDataset = NULL;

	delete m_pColorRamp;
	m_pColorRamp = NULL;

	irr::video::IVideoDriver* driver = SceneManager->getVideoDriver();
	unsigned int nMeshBufferCount = m_pMesh->getMeshBufferCount();
	for (unsigned int i = 0; i < nMeshBufferCount; i ++)
	{
		driver->removeHardwareBuffer(m_pMesh->getMeshBuffer(i));
		m_pMesh->getMeshBuffer(i)->drop();
	}
	
	driver->removeTexture(m_pMesh->getMeshBuffer(0)->getMaterial().getTexture(0));

	m_pMesh->drop();
	m_pMesh = NULL;
}
void CHdDemSceneNode::OnRegisterSceneNode()
{
	SceneManager->registerNodeForRendering(this);

	ISceneNode::OnRegisterSceneNode();
}
void CHdDemSceneNode::render()
{
	if (!IsVisible || !SceneManager->getActiveCamera() || !SceneManager->getVideoDriver())
	{
		return;
	}

	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	driver->setTransform(irr::video::ETS_WORLD, AbsoluteTransformation);
	driver->setMaterial(getMaterial(0));

	unsigned int nMeshBufferCount = m_pMesh->getMeshBufferCount();
	for (unsigned int i = 0; i < nMeshBufferCount; i ++)
	{
		driver->drawMeshBuffer(m_pMesh->getMeshBuffer(i));
	}
}
const core::aabbox3d<irr::f32>& CHdDemSceneNode::getBoundingBox() const
{
	return m_pMesh->BoundingBox;
}
video::SMaterial& CHdDemSceneNode::getMaterial(irr::u32 i)
{
	return m_pMesh->getMeshBuffer(0)->getMaterial();
}
irr::u32 CHdDemSceneNode::getMaterialCount() const
{
	return 1;
}
ESCENE_NODE_TYPE CHdDemSceneNode::getType() const 
{ 
	return ESNT_HD_DEM; 
}
void CHdDemSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{

}
void CHdDemSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{

}
ISceneNode* CHdDemSceneNode::clone(ISceneNode* newParent, ISceneManager* newManager)
{
	return NULL;
}
CHdRasterDataset* CHdDemSceneNode::GetDemDataset()
{
	return m_pDemDataset;
}
bool CHdDemSceneNode::ReloadData()
{
	if (!m_pMesh || !m_pDemDataset || !SceneManager->getActiveCamera())
	{
		return false;
	}

	unsigned int nRasterWidth = m_pDemDataset->GetRasterWidth();
	unsigned int nRasterHeight = m_pDemDataset->GetRasterHeight();
	EHdRasterDataType eDataType = m_pDemDataset->GetBandDataType(1);

	// 网格大小，即 X ( Y )方向包含的顶点数
	int nMeshSize = (m_nMeshBufferSize - 1) * m_nMeshBufferRows + 1;   

	// 计算待读取数据在局部坐标系下的地理范围
	float fLoadMinX, fLoadMaxX, fLoadMinY, fLoadMaxY;
	if (!CalcDataEnv(fLoadMinX, fLoadMaxX, fLoadMinY, fLoadMaxY))
	{
		return false;
	}

	// 根据地理范围计算出读取数据使用的金字塔索引、像素范围、目标栅格大小和实际的地理范围
	int nOvIndex, nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize, nDstXSize, nDstYSize;
	float fRealMinX, fRealMaxX, fRealMinY, fRealMaxY;
	CalcLoadPixels(m_pDemDataset, fLoadMinX, fLoadMaxX, fLoadMinY, fLoadMaxY, nMeshSize, nMeshSize,
		nOvIndex, nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize, nDstXSize, nDstYSize,
		fRealMinX, fRealMaxX, fRealMinY, fRealMaxY);

	// 如果新的地理范围与当前地理范围相同，则不必重新加载数据
	if (abs(fRealMinX - m_fLoadMinX) < 0.1 && abs(fRealMaxX - m_fLoadMaxX) < 0.1 &&
		abs(fRealMinY - m_fLoadMinY) < 0.1 && abs(fRealMaxY - m_fLoadMaxY) < 0.1)
	{
		return true;
	}

	// 读取 DEM 栅格数据
	CHdRasterBlock raster(nDstXSize, nDstYSize, 1, eDataType);
	if ((nOvIndex >= 0 && !m_pDemDataset->Read(nOvIndex, nSrcXOff, nSrcYOff, 1, NULL, &raster)) ||
		(nOvIndex < 0 && !m_pDemDataset->Read(nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize, 1, NULL, &raster)))
	{
		return false;
	}

	// 更新纹理
	float fTMinX, fTMaxX, fTMinY, fTMaxY;
	bool bExistTexture = m_pMesh->getMeshBuffer(0)->getMaterial().getTexture(0) != NULL;
	if (bExistTexture)
	{
		if (!UpdateTexture(fRealMinX, fRealMaxX, fRealMinY, fRealMaxY, fTMinX, fTMaxX, fTMinY, fTMaxY))
		{
			return false;
		}
	}

	// 修改当前地理范围
	m_fLoadMinX = fRealMinX;
	m_fLoadMaxX = fRealMaxX;
	m_fLoadMinY = fRealMinY;
	m_fLoadMaxY = fRealMaxY;

	// 修改顶点坐标和索引
	float fDstXResolution = (m_fLoadMaxX - m_fLoadMinX) / nDstXSize;
	float fDstYResolution = (m_fLoadMaxY - m_fLoadMinY) / nDstYSize;
	float fMeshBufferXLength = fDstXResolution * (m_nMeshBufferSize - 1);
	float fMeshBufferYLength = fDstYResolution * (m_nMeshBufferSize - 1);
	float ftXResolution = (fTMaxX - fTMinX) / nDstXSize;
	float ftYResolution = (fTMaxY - fTMinY) / nDstYSize;
	unsigned char* pMaskData = new unsigned char[m_nMeshBufferSize * m_nMeshBufferSize];
	for (int iRow = 0; iRow < m_nMeshBufferRows; iRow ++)
	{
		for (int iCol = 0; iCol < m_nMeshBufferRows; iCol ++)
		{
			SMeshBuffer* pMeshBuffer = (SMeshBuffer*)m_pMesh->getMeshBuffer(iRow * m_nMeshBufferRows + iCol);
			pMeshBuffer->Vertices.clear();
			pMeshBuffer->Indices.clear();
			memset(pMaskData, 0, m_nMeshBufferSize * m_nMeshBufferSize);

			// 计算该分块网格的地理范围
			float fMeshBufferMinX = m_fLoadMinX + fMeshBufferXLength * iCol;
			float fMeshBufferMinY = m_fLoadMaxY - fMeshBufferYLength * (iRow + 1);
			pMeshBuffer->BoundingBox.MinEdge.set(fMeshBufferMinX, fMeshBufferMinY, m_fMinZ);
			pMeshBuffer->BoundingBox.MaxEdge.set(fMeshBufferMinX + fMeshBufferXLength, 
				fMeshBufferMinY + fMeshBufferYLength, m_fMaxZ);

			// 分块网格左上角顶点在栅格块中的偏移，以及 X 和 Y 方向的顶点数
			int nVertexXOff = (m_nMeshBufferSize - 1) * iCol;
			int nVertexYOff = (m_nMeshBufferSize - 1) * iRow;
			if (nVertexXOff >= nDstXSize - 1 || nVertexYOff >= nDstYSize - 1)
			{
				continue;
			}
			int nVertexXSize = min(m_nMeshBufferSize, nDstXSize - nVertexXOff);
			int nVertexYSize = min(m_nMeshBufferSize, nDstYSize - nVertexYOff);

			// 设置顶点坐标并着色
			video::S3DVertex sVertex;
			sVertex.Normal.set(0, 0, 1);
			sVertex.Color.set(255, 255, 255, 255);
			if (bExistTexture)
			{
				for (int ivY = 0; ivY < nVertexYSize; ivY ++)
				{
					float fVertexX = fMeshBufferMinX;
					float fVertexY = fMeshBufferMinY + fMeshBufferYLength - ivY * fDstYResolution;

					float fTextureX = fTMinX + nVertexXOff * ftXResolution;
					float fTextureY = fTMinY + (nVertexYOff + ivY) * ftYResolution;

					for (int ivX = 0; ivX < nVertexXSize; ivX ++)
					{
						float fVertexZ = GetElevation(raster, nVertexXOff + ivX, nVertexYOff + ivY);

						sVertex.Pos.set(fVertexX, fVertexY, fVertexZ);
						sVertex.TCoords.set(fTextureX, fTextureY);
						pMeshBuffer->Vertices.push_back(sVertex);

						if (fVertexZ > m_fNoDataZ + 0.001)
						{
							pMaskData[ivY * nVertexXSize + ivX] = 1;
						}

						fVertexX += fDstXResolution;
						fTextureX += ftXResolution;
					}
				}
			}
			else
			{
				for (int ivY = 0; ivY < nVertexYSize; ivY ++)
				{
					float fVertexX = fMeshBufferMinX;
					float fVertexY = fMeshBufferMinY + fMeshBufferYLength - ivY * fDstYResolution;
					for (int ivX = 0; ivX < nVertexXSize; ivX ++)
					{
						float fVertexZ = GetElevation(raster, nVertexXOff + ivX, nVertexYOff + ivY);

						sVertex.Pos.set(fVertexX, fVertexY, fVertexZ);
						pMeshBuffer->Vertices.push_back(sVertex);

						if (fVertexZ > m_fNoDataZ + 0.001)
						{
							pMaskData[ivY * nVertexXSize + ivX] = 1;
						}

						fVertexX += fDstXResolution;
					}
				}
				RenderVertex(pMeshBuffer);
			}

			// 设置索引
			for (int iY = 0; iY < nVertexYSize - 1; iY ++)
			{
				for (int iX = 0; iX < nVertexXSize - 1; iX ++)
				{
					int nLeftUpIndex = iY * nVertexXSize + iX;
					int nRightUpIndex = nLeftUpIndex + 1;
					int nLeftDownIndex = (iY + 1) * nVertexXSize + iX;
					int nRightDownIndex = nLeftDownIndex + 1;

					if (pMaskData[nLeftDownIndex] && pMaskData[nLeftUpIndex] && pMaskData[nRightUpIndex])
					{
						pMeshBuffer->Indices.push_back(nLeftDownIndex);
						pMeshBuffer->Indices.push_back(nLeftUpIndex);
						pMeshBuffer->Indices.push_back(nRightUpIndex);
					}
					
					if (pMaskData[nLeftDownIndex] && pMaskData[nRightUpIndex] && pMaskData[nRightDownIndex])
					{
						pMeshBuffer->Indices.push_back(nLeftDownIndex);
						pMeshBuffer->Indices.push_back(nRightUpIndex);
						pMeshBuffer->Indices.push_back(nRightDownIndex);
					}
				}
			}
		}
	}

	delete []pMaskData;

	return true;
}
void CHdDemSceneNode::SetBaseHeight(double dfBaseZ)
{
	m_dfBaseZ = dfBaseZ;

	// 更新影像局部坐标系到全局坐标系和场景坐标系的变换模型
	const CHdGeoEnvelope* pGeoEnv = m_pDemDataset->GetGeoEnvelope();
	double pdfParam[4][4] = {0};
	pdfParam[0][3] = pGeoEnv->GetMinX();
	pdfParam[1][3] = pGeoEnv->GetMinY();
	pdfParam[2][3] = m_dfBaseZ;
	pdfParam[0][0] = 1;
	pdfParam[1][1] = 1;
	pdfParam[2][2] = 1;
	pdfParam[3][3] = 1;
	m_absModel.CreateModelByMatrix(pdfParam[0]);

	CBursaWolfModel* pViewModel = m_pView->GetTransModel();
	m_renderModel = m_absModel * (*pViewModel);

	// 更新 DEM 节点到场景根节点的变换矩阵
	setPosition(irr::core::vector3df(m_renderModel.m_fOffset[0], m_renderModel.m_fOffset[1], m_renderModel.m_fOffset[2]));

	// 更新 DEM 节点的绝对变换矩阵
	updateAbsolutePosition();
}
void CHdDemSceneNode::UpdateTransModel()
{
	CBursaWolfModel* pViewModel = m_pView->GetTransModel();
	m_renderModel = m_absModel * (*pViewModel);

	// 更新 DEM 节点到场景根节点的变换矩阵
	setPosition(irr::core::vector3df(m_renderModel.m_fOffset[0], m_renderModel.m_fOffset[1], m_renderModel.m_fOffset[2]));

	// 更新 DEM 节点的绝对变换矩阵
	updateAbsolutePosition();
}
void CHdDemSceneNode::SetTextureDataset(CHdRasterDataset* pDataset)
{
	if (m_pTextureDataset)
	{
		if (m_pTextureDataset->Dereference() == 0)
		{
			CHdRasterDataset::Close(m_pTextureDataset);
		}
		m_pTextureDataset = NULL;
	}

	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	ITexture* pTexture = m_pMesh->getMeshBuffer(0)->getMaterial().getTexture(0);
	if (pDataset)
	{
		m_pTextureDataset = pDataset;
		m_pTextureDataset->Reference();

		InitTextureRenderType();

		if (!pTexture)
		{
			irr::io::path texturePath(m_pTextureDataset->GetDatasetFullName());
			ITexture* pTexture = driver->addTexture(irr::core::dimension2d<irr::u32>(m_nTextureSize, m_nTextureSize), 
				texturePath, ECF_A8R8G8B8);
			unsigned int nMeshBufferCount = m_pMesh->getMeshBufferCount();
			for (unsigned int i = 0; i < nMeshBufferCount; i ++)
			{
				m_pMesh->getMeshBuffer(i)->getMaterial().setTexture(0, pTexture);
			}
		}
	}
	else
	{
		if (pTexture)
		{
			unsigned int nMeshBufferCount = m_pMesh->getMeshBufferCount();
			for (unsigned int i = 0; i < nMeshBufferCount; i ++)
			{
				m_pMesh->getMeshBuffer(i)->getMaterial().setTexture(0, NULL);
			}
			driver->removeTexture(pTexture);
		}
	}
}
CHdRasterDataset* CHdDemSceneNode::GetTextureDataset()
{
	return m_pTextureDataset;
}
void CHdDemSceneNode::SetTextureSize(int nSize)
{
	m_nTextureSize = nSize;

	//! 移除纹理后重新添加纹理
	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	ITexture* pTexture = m_pMesh->getMeshBuffer(0)->getMaterial().getTexture(0);
	if (pTexture)
	{
		const irr::io::path textureName = pTexture->getName();
		unsigned int nMeshBufferCount = m_pMesh->getMeshBufferCount();
		for (unsigned int i = 0; i < nMeshBufferCount; i ++)
		{
			m_pMesh->getMeshBuffer(i)->getMaterial().setTexture(0, NULL);
		}
		driver->removeTexture(pTexture);
		pTexture = driver->addTexture(irr::core::dimension2d<irr::u32>(m_nTextureSize, m_nTextureSize), 
			textureName, ECF_A8R8G8B8);
		for (unsigned int i = 0; i < nMeshBufferCount; i ++)
		{
			m_pMesh->getMeshBuffer(i)->getMaterial().setTexture(0, pTexture);
		}
	}
}
bool CHdDemSceneNode::CalcDataEnv(float& fGeoMinX, float& fGeoMaxX, float& fGeoMinY, float& fGeoMaxY)
{
	if (!m_pDemDataset)
	{
		return false;
	}

	// 获取数据集在局部坐标系下的地理范围
	const CHdGeoEnvelope* pGeoEnv = m_pDemDataset->GetGeoEnvelope();
	float fDatasetMinX = 0;
	float fDatasetMaxX = pGeoEnv->GetMaxX() - pGeoEnv->GetMinX();
	float fDatasetMaxY = pGeoEnv->GetMaxY() - pGeoEnv->GetMinY();
	float fDatasetMinY = 0;

	// 获取视图的宽高
	int nViewWidth = m_pView->GetWindowWidth();
	int nViewHeight = m_pView->GetWindowHeight();

	// 从视图上的两个点获取两条直线
	irr::core::line3df line1 = getSceneManager()->getSceneCollisionManager()->getRayFromScreenCoordinates(
		irr::core::vector2di(nViewWidth / 2, nViewHeight / 2));
	irr::core::line3df line2 = getSceneManager()->getSceneCollisionManager()->getRayFromScreenCoordinates(
		irr::core::vector2di(nViewWidth / 2 + nViewWidth, nViewHeight / 2));

	// 将直线坐标从场景坐标系转到数据集的局部坐标系
	m_renderModel.AntiTranslate(line1.start.X, line1.start.Y, line1.start.Z);
	m_renderModel.AntiTranslate(line1.end.X, line1.end.Y, line1.end.Z);
	m_renderModel.AntiTranslate(line2.start.X, line2.start.Y, line2.start.Z);
	m_renderModel.AntiTranslate(line2.end.X, line2.end.Y, line2.end.Z);

	// 定义一个经过 DEM 包围盒中心的平面，计算直线与平面的交点
	irr::core::plane3df planeBase(getBoundingBox().getCenter(), irr::core::vector3df(0, 0, 1));
	irr::core::vector3df point1, point2;
	planeBase.getIntersectionWithLine(line1.start, line1.getVector(), point1);
	planeBase.getIntersectionWithLine(line2.start, line2.getVector(), point2);

	// 根据交点坐标计算待读取数据的地理范围
	irr::f32 fDis = point1.getDistanceFrom(point2);
	fGeoMinX = point1.X - fDis;
	fGeoMaxX = point1.X + fDis;
	fGeoMinY = point1.Y - fDis;
	fGeoMaxY = point1.Y + fDis;

	fGeoMinX = max(fGeoMinX, fDatasetMinX);
	fGeoMaxX = min(fGeoMaxX, fDatasetMaxX);
	fGeoMinY = max(fGeoMinY, fDatasetMinY);
	fGeoMaxY = min(fGeoMaxY, fDatasetMaxY);
	if (fGeoMinX >= fGeoMaxX || fGeoMinY >= fGeoMaxY)
	{
		return false;
	}

	// 如果存在纹理数据集，则还需与纹理数据集的地理范围求交
	if (m_pTextureDataset)
	{
		const CHdGeoEnvelope* pTextureEnv = m_pTextureDataset->GetGeoEnvelope();
		float fTDatasetMinX = pTextureEnv->GetMinX() - pGeoEnv->GetMinX();
		float fTDatasetMaxX = pTextureEnv->GetMaxX() - pGeoEnv->GetMinX();
		float fTDatasetMinY = pTextureEnv->GetMinY() - pGeoEnv->GetMinY();
		float fTDatasetMaxY = pTextureEnv->GetMaxY() - pGeoEnv->GetMinY();
		fGeoMinX = max(fGeoMinX, fTDatasetMinX);
		fGeoMaxX = min(fGeoMaxX, fTDatasetMaxX);
		fGeoMinY = max(fGeoMinY, fTDatasetMinY);
		fGeoMaxY = min(fGeoMaxY, fTDatasetMaxY);
		if (fGeoMinX >= fGeoMaxX || fGeoMinY >= fGeoMaxY)
		{
			return false;
		}
	}

	return true;
}
bool CHdDemSceneNode::CalcLoadPixels(CHdRasterDataset* pDataset, float fLoadMinX, float fLoadMaxX, float fLoadMinY, 
	float fLoadMaxY, int nMaxXSize, int nMaxYSize, int& nOvIndex, int& nSrcXOff, int& nSrcYOff, 
	int& nSrcXSize, int& nSrcYSize, int& nDstXSize, int& nDstYSize, 
	float& fRealMinX, float& fRealMaxX, float& fRealMinY, float& fRealMaxY)
{
	if (!pDataset)
	{
		return false;
	}

	unsigned int nRasterWidth = pDataset->GetRasterWidth();
	unsigned int nRasterHeight = pDataset->GetRasterHeight();

	// 获取数据集在 X 和 Y 方向的分辨率
	double pGeoTransform[6];
	pDataset->GetGeoTransform(pGeoTransform);
	double dfXResolution = pGeoTransform[1];
	double dfYResolution = pGeoTransform[5];

	// 数据集在自身的局部坐标系下的地理范围
	const CHdGeoEnvelope* pGeoEnv = pDataset->GetGeoEnvelope();
	float fDatasetMinX = 0;
	float fDatasetMaxX = pGeoEnv->GetMaxX() - pGeoEnv->GetMinX();
	float fDatasetMaxY = pGeoEnv->GetMaxY() - pGeoEnv->GetMinY();
	float fDatasetMinY = 0;

	// 根据地理范围计算在数据集中的读取像素范围
	nSrcXOff = max(0, (int)(fLoadMinX / dfXResolution));
	nSrcXSize = min(nRasterWidth - 1, (int)(fLoadMaxX / dfXResolution)) - nSrcXOff + 1;
	if (dfYResolution < 0)
	{
		nSrcYOff = max(0, (int)((fDatasetMaxY - fLoadMaxY) / abs(dfYResolution)));
		nSrcYSize = min(nRasterHeight - 1, (int)((fDatasetMaxY - fLoadMinY) / abs(dfYResolution))) - nSrcYOff + 1;
	}
	else
	{
		nSrcYOff = max(0, (int)((fLoadMinY - fDatasetMinY) / abs(dfYResolution)));
		nSrcYSize = min(nRasterHeight - 1, (int)((fLoadMaxY - fDatasetMinY) / abs(dfYResolution))) - nSrcYOff + 1;
	}

	// 如果像素范围的大小满足限制，则直接从数据集读取，否则从金字塔读取
	if (nSrcXSize <= nMaxXSize && nSrcYSize <= nMaxYSize)
	{
		nOvIndex = -1;
		nDstXSize = nSrcXSize;
		nDstYSize = nSrcYSize;
		fRealMinX = dfXResolution * nSrcXOff;
		fRealMaxX = dfXResolution * (nSrcXOff + nSrcXSize);
		if (dfYResolution < 0)
		{
			fRealMinY = fDatasetMaxY + dfYResolution * (nSrcYOff + nSrcYSize);
			fRealMaxY = fDatasetMaxY + dfYResolution * nSrcYOff;
		}
		else
		{
			fRealMinY = fDatasetMinY + dfYResolution * nSrcYOff;
			fRealMaxY = fDatasetMinY + dfYResolution * (nSrcYOff + nSrcYSize);
		}
	}
	else
	{
		nOvIndex = -1;
		int nOvCount = pDataset->GetOverviewCount(1);
		if (nOvCount > 0)
		{
			for (int i = 0; i < nOvCount; i ++)
			{
				int nOvWidth, nOvHeight;
				if (pDataset->GetOverviewSize(1, i, nOvWidth, nOvHeight))
				{
					double dfOvXResolution = dfXResolution * nRasterWidth / nOvWidth;
					double dfOvYResolution = dfYResolution * nRasterHeight / nOvHeight;
					nSrcXOff = max(0, (int)(fLoadMinX / dfOvXResolution));
					nSrcXSize = min(nOvWidth - 1, (int)(fLoadMaxX / dfOvXResolution)) - nSrcXOff + 1;
					if (dfYResolution < 0)
					{
						nSrcYOff = max(0, (int)((fDatasetMaxY - fLoadMaxY) / abs(dfOvYResolution)));
						nSrcYSize = min(nOvHeight - 1, (int)((fDatasetMaxY - fLoadMinY) / abs(dfOvYResolution))) - nSrcYOff + 1;
					}
					else
					{
						nSrcYOff = max(0, (int)((fLoadMinY - fDatasetMinY) / abs(dfOvYResolution)));
						nSrcYSize = min(nOvHeight - 1, (int)((fLoadMaxY - fDatasetMinY) / abs(dfOvYResolution))) - nSrcYOff + 1;
					}
					if (nSrcXSize <= nMaxXSize && nSrcYSize <= nMaxYSize)
					{
						nOvIndex = i;
						nDstXSize = nSrcXSize;
						nDstYSize = nSrcYSize;
						fRealMinX = dfOvXResolution * nSrcXOff;
						fRealMaxX = dfOvXResolution * (nSrcXOff + nSrcXSize);
						if (dfYResolution < 0)
						{
							fRealMinY = fDatasetMaxY + dfOvYResolution * (nSrcYOff + nSrcYSize);
							fRealMaxY = fDatasetMaxY + dfOvYResolution * nSrcYOff;
						}
						else
						{
							fRealMinY = fDatasetMinY + dfOvYResolution * nSrcYOff;
							fRealMaxY = fDatasetMinY + dfOvYResolution * (nSrcYOff + nSrcYSize);
						}

						break;
					}
				}
			}
		}

		// 如果缺少金字塔，则从数据集降采样读取数据
		if (nOvIndex < 0)
		{
			nSrcXOff = max(0, (int)(fLoadMinX / dfXResolution));
			nSrcXSize = min(nRasterWidth - 1, (int)(fLoadMaxX / dfXResolution)) - nSrcXOff + 1;
			if (dfYResolution < 0)
			{
				nSrcYOff = max(0, (int)((fDatasetMaxY - fLoadMaxY) / abs(dfYResolution)));
				nSrcYSize = min(nRasterHeight - 1, (int)((fDatasetMaxY - fLoadMinY) / abs(dfYResolution))) - nSrcYOff + 1;
			}
			else
			{
				nSrcYOff = max(0, (int)((fLoadMinY - fDatasetMinY) / abs(dfYResolution)));
				nSrcYSize = min(nRasterHeight - 1, (int)((fLoadMaxY - fDatasetMinY) / abs(dfYResolution))) - nSrcYOff + 1;
			}
			nDstXSize = min(nMaxXSize, nSrcXSize);
			nDstYSize = min(nMaxYSize, nSrcYSize);
			fRealMinX = fLoadMinX;
			fRealMaxX = fLoadMaxX;
			fRealMinY = fLoadMinY;
			fRealMaxY = fLoadMaxY;
		}
	}

	return true;
}
bool CHdDemSceneNode::RenderVertex(SMeshBuffer* pMeshBuffer)
{
	float fScale = 255.0 / (m_fMaxZ - m_fMinZ);

	unsigned int nVertexCount = pMeshBuffer->Vertices.size();
	for (unsigned int i = 0; i < nVertexCount; i ++)
	{
		float fVertexZ = pMeshBuffer->Vertices[i].Pos.Z;
		unsigned char nGray = 0;
		
		if (fVertexZ >= m_fMaxZ)
		{
			nGray = 255;
		}
		else if (fVertexZ > m_fMinZ)
		{
			nGray = (fVertexZ - m_fMinZ) * fScale;
		}

		pMeshBuffer->Vertices[i].Color.set(255, nGray, nGray, nGray);
	}

	return true;
}
float CHdDemSceneNode::GetElevation(const CHdRasterBlock& raster, int nPixelX, int nPixelY)
{
	unsigned int nRasterWidth = raster.GetRasterWidth();
	unsigned int nRasterHeight = raster.GetRasterHeight();
	int nPixelOff = nPixelY * nRasterWidth + nPixelX;

	float fElevation = 0;
	EHdRasterDataType eDataType = raster.GetDataType();
	switch(eDataType)
	{
	case EHD_DT_Byte:
		{
			unsigned char* pData = (unsigned char*)raster.GetRasterData();
			fElevation = pData[nPixelOff];
			break;
		}
	case EHD_DT_Int16:
		{
			short* pData = (short*)raster.GetRasterData();
			fElevation = pData[nPixelOff];
			break;
		}
	case EHD_DT_UInt16:
		{
			unsigned short* pData = (unsigned short*)raster.GetRasterData();
			fElevation = pData[nPixelOff];
			break;
		}
	case EHD_DT_Int32:
		{
			int* pData = (int*)raster.GetRasterData();
			fElevation = pData[nPixelOff];
			break;
		}
	case EHD_DT_UInt32:
		{
			unsigned int* pData = (unsigned int*)raster.GetRasterData();
			fElevation = pData[nPixelOff];
			break;
		}
	case EHD_DT_Float32:
		{
			float* pData = (float*)raster.GetRasterData();
			fElevation = pData[nPixelOff];
			break;
		}
	case EHD_DT_Float64:
		{
			double* pData = (double*)raster.GetRasterData();
			fElevation = pData[nPixelOff];
			break;
		}
	default:break;
	}
	return fElevation;
}
bool CHdDemSceneNode::UpdateTexture(float fGeoMinX, float fGeoMaxX, float fGeoMinY, float fGeoMaxY, 
	float& fTMinX, float& fTMaxX, float& fTMinY, float& fTMaxY)
{
	if (!m_pMesh || !m_pDemDataset || !m_pTextureDataset)
	{
		return false;
	}

	unsigned int nRasterWidth = m_pTextureDataset->GetRasterWidth();
	unsigned int nRasterHeight = m_pTextureDataset->GetRasterHeight();
	EHdRasterDataType eDataType = m_pTextureDataset->GetBandDataType(1);

	// 获取纹理实际大小
	ITexture* pTexture = m_pMesh->getMeshBuffer(0)->getMaterial().getTexture(0);
	unsigned int nTextureWidth = pTexture->getSize().Width;
	unsigned int nTextureHeight = pTexture->getSize().Height;

	// 将待加载数据在 DEM 局部坐标系下的地理范围转到影像局部坐标系下的范围
	const CHdGeoEnvelope* pDemGeoEnv = m_pDemDataset->GetGeoEnvelope();
	const CHdGeoEnvelope* pImageGeoEnv = m_pTextureDataset->GetGeoEnvelope();
	float fLoadMinX = fGeoMinX + pDemGeoEnv->GetMinX() - pImageGeoEnv->GetMinX();
	float fLoadMaxX = fGeoMaxX + pDemGeoEnv->GetMinX() - pImageGeoEnv->GetMinX();
	float fLoadMinY = fGeoMinY + pDemGeoEnv->GetMinY() - pImageGeoEnv->GetMinY();
	float fLoadMaxY = fGeoMaxY + pDemGeoEnv->GetMinY() - pImageGeoEnv->GetMinY();

	// 根据地理范围计算出读取数据使用的金字塔索引、像素范围、目标栅格大小和实际的地理范围
	int nOvIndex, nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize, nDstXSize, nDstYSize;
	float fRealMinX, fRealMaxX, fRealMinY, fRealMaxY;
	CalcLoadPixels(m_pTextureDataset, fLoadMinX, fLoadMaxX, fLoadMinY, fLoadMaxY, nTextureWidth, nTextureHeight,
		nOvIndex, nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize, nDstXSize, nDstYSize,
		fRealMinX, fRealMaxX, fRealMinY, fRealMaxY);

	// 读取栅格数据
	CHdRasterBlock raster(nDstXSize, nDstYSize, m_nRenderBandCount, eDataType);
	if ((nOvIndex >= 0 && !m_pTextureDataset->Read(nOvIndex, nSrcXOff, nSrcYOff, m_nRenderBandCount, m_pRenderBandIndex, &raster)) ||
		(nOvIndex < 0 && !m_pTextureDataset->Read(nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize, m_nRenderBandCount, m_pRenderBandIndex, &raster)))
	{
		return false;
	}

	// 绘制栅格数据到纹理中
	unsigned char* pTextureData = (unsigned char*)pTexture->lock(false);
	memset(pTextureData, 0, nTextureWidth * nTextureHeight * 4);
	CHdImage renderImage(nTextureWidth, nTextureHeight, EHD_CF_A8R8G8B8, pTextureData, false);
	CHdRasterRenderer rasterRenderer;
	if ((m_nRenderType == 1 && !rasterRenderer.RenderByRGB(&raster, NULL, NULL, &renderImage, 0, 0, nDstXSize, nDstYSize)) ||
		(m_nRenderType == 2 && !rasterRenderer.RenderByStretch(&raster, m_dfMinValue, m_dfMaxValue, &renderImage, 0, 0, nDstXSize, nDstYSize)) ||
		(m_nRenderType == 3 && !rasterRenderer.RenderByColorMap(&raster, m_pTextureDataset->GetColorTable(), &renderImage, 0, 0, nDstXSize, nDstYSize)))
	{
		pTexture->unlock();
		return false;
	}
	pTexture->unlock();

	// 计算输入的地理范围对应的纹理坐标范围
	fTMinX = (fLoadMinX - fRealMinX) / (fRealMaxX - fRealMinX) * nDstXSize / nTextureWidth;
	fTMaxX = (fLoadMaxX - fRealMinX) / (fRealMaxX - fRealMinX) * nDstXSize / nTextureWidth;
	fTMinY = (fRealMaxY - fLoadMaxY) / (fRealMaxY - fRealMinY) * nDstYSize / nTextureHeight;
	fTMaxY = (fRealMaxY - fLoadMinY) / (fRealMaxY - fRealMinY) * nDstYSize / nTextureHeight;

	return true;
}
bool CHdDemSceneNode::InitTextureRenderType()
{
	if (!m_pTextureDataset)
	{
		return false;
	}
	unsigned int nRasterWidth = m_pTextureDataset->GetRasterWidth();
	unsigned int nRasterHeight = m_pTextureDataset->GetRasterHeight();
	unsigned int nBandCount = m_pTextureDataset->GetBandCount();
	EHdRasterDataType eDataType = m_pTextureDataset->GetBandDataType(1);

	// 获取栅格数据的绘制方式以及绘制使用的波段索引
	if (m_pTextureDataset->GetColorTable())
	{
		m_nRenderType = 3;
		m_nRenderBandCount = 1;
		m_pRenderBandIndex[0] = 1;
	}
	else
	{
		if (nBandCount >= 3)
		{
			int pBandIndex[4] = {3,2,1,-1}; // 依次为 B G R A
			for (int iBand = 1; iBand <= nBandCount; iBand ++)
			{
				if (m_pTextureDataset->GetBandColorInterp(iBand) == EHD_CI_Alpha)
				{
					pBandIndex[3] = iBand;
				}
				else if (m_pTextureDataset->GetBandColorInterp(iBand) == EHD_CI_Red)
				{
					pBandIndex[2] = iBand;
				}
				else if (m_pTextureDataset->GetBandColorInterp(iBand) == EHD_CI_Green)
				{
					pBandIndex[1] = iBand;
				}
				else if (m_pTextureDataset->GetBandColorInterp(iBand) == EHD_CI_Blue)
				{
					pBandIndex[0] = iBand;
				}
			}
			if (pBandIndex[3] > 0)
			{
				m_nRenderBandCount = 4;
				memcpy(m_pRenderBandIndex, pBandIndex, 4 * sizeof(int));
			}
			else
			{
				m_nRenderBandCount = 3;
				memcpy(m_pRenderBandIndex, pBandIndex, 3 * sizeof(int));
			}
			m_nRenderType = 1;
		}
		else
		{
			m_nRenderBandCount = 1;
			m_pRenderBandIndex[0] = 1;
			m_nRenderType = 2;

			// 获取最小最大值
			m_dfMinValue = 0;
			m_dfMaxValue = 255;
			m_pTextureDataset->GetMinMaxValue(1, m_dfMinValue, m_dfMaxValue);
		}
	}
	return true;
}