#include "stdafx.h"
#include "HdDomSceneNode.h"
#include "HdRasterDataset.h"
#include "HdImage.h"
#include "HdRasterRenderer.h"
#include "..\..\hdCommon\HdGridObject.h"

#include "gdal_priv.h"

using namespace hd::scene;
using namespace hd;

CHdDomSceneNode::CHdDomSceneNode(ISceneNode* parent, ISceneManager* mgr, irr::s32 id, CHdRasterDataset* pDataset):
	IObjectSceneNode(NULL, video::SColor(), video::SColor(), parent, mgr, id)
{
	m_dfBaseZ = 0;
	m_pDataset = pDataset;
	m_pDataset->Reference();
	m_nTextureSize = 2048;

	// 获取影像在局部坐标系下的地理范围，局部坐标系以影像左下角为原点
	const CHdGeoEnvelope* pGeoEnv = m_pDataset->GetGeoEnvelope();
	float fLeftX = 0;
	float fRightX = pGeoEnv->GetMaxX() - pGeoEnv->GetMinX();
	float fUpY = pGeoEnv->GetMaxY() - pGeoEnv->GetMinY();
	float fDownY = 0;

	// 初始化绘制方式
	InitRenderType();

	// 创建网格
	m_pMeshBuffer = new SMeshBuffer;
	m_pMeshBuffer->Material.Lighting = false;
	m_pMeshBuffer->Material.ZBuffer = video::ECFN_LESSEQUAL;
	m_pMeshBuffer->Material.ZWriteEnable = true;
	m_pMeshBuffer->Material.GouraudShading = false;
	m_pMeshBuffer->Material.AntiAliasing = video::EAAM_OFF;
	m_pMeshBuffer->BoundingBox.MinEdge.set(fLeftX, fDownY, -0.01f);
	m_pMeshBuffer->BoundingBox.MaxEdge.set(fRightX, fUpY, 0.01f);

	m_pMeshBuffer->setHardwareMappingHint(irr::scene::EHM_DYNAMIC);
	
	// 添加纹理
	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	irr::io::path texturePath(m_pDataset->GetDatasetFullName());
	ITexture* pTexture = driver->addTexture(irr::core::dimension2d<irr::u32>(m_nTextureSize, m_nTextureSize), 
		texturePath, ECF_A8R8G8B8);
	m_pMeshBuffer->Material.setTexture(0, pTexture);
	
	// 初始化影像局部坐标系到全局坐标系、场景坐标系的变换模型
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

	// 初始化影像节点到场景根节点的变换矩阵
	setPosition(irr::core::vector3df(m_renderModel.m_fOffset[0], m_renderModel.m_fOffset[1], m_renderModel.m_fOffset[2]));
}
CHdDomSceneNode::~CHdDomSceneNode()
{
	if (m_pDataset->Dereference() == 0)
	{
		CHdRasterDataset::Close(m_pDataset);
	}
	m_pDataset = NULL;

	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	if (m_pMeshBuffer)
	{
		driver->removeTexture(m_pMeshBuffer->Material.getTexture(0));
		driver->removeHardwareBuffer(m_pMeshBuffer);
		m_pMeshBuffer->drop();
		m_pMeshBuffer = NULL;
	}
}
void CHdDomSceneNode::OnRegisterSceneNode()
{
	SceneManager->registerNodeForRendering(this);

	ISceneNode::OnRegisterSceneNode();
}
void CHdDomSceneNode::render()
{
	if (!IsVisible || !SceneManager->getActiveCamera() || !SceneManager->getVideoDriver())
	{
		return;
	}

	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	driver->setTransform(irr::video::ETS_WORLD, AbsoluteTransformation);
	if (m_pMeshBuffer)
	{
		driver->setMaterial(m_pMeshBuffer->getMaterial());
		driver->drawMeshBuffer(m_pMeshBuffer);
	}
}
const core::aabbox3d<irr::f32>& CHdDomSceneNode::getBoundingBox() const
{
	return m_pMeshBuffer->BoundingBox;
}
video::SMaterial& CHdDomSceneNode::getMaterial(irr::u32 i)
{
	return m_pMeshBuffer->Material;
}
irr::u32 CHdDomSceneNode::getMaterialCount() const
{
	return 1;
}
ESCENE_NODE_TYPE CHdDomSceneNode::getType() const 
{ 
	return ESNT_HD_DOM; 
}
void CHdDomSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
	return;
}
void CHdDomSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	return;
}
ISceneNode* CHdDomSceneNode::clone(ISceneNode* newParent, ISceneManager* newManager)
{
	return NULL;
}
CHdRasterDataset* CHdDomSceneNode::GetDataset()
{
	return m_pDataset;
}
bool CHdDomSceneNode::ReloadData()
{
	if (!m_pMeshBuffer || !m_pMeshBuffer->Material.getTexture(0) || !m_pDataset || !SceneManager->getActiveCamera())
	{
		return false;
	}

	unsigned int nRasterWidth = m_pDataset->GetRasterWidth();
	unsigned int nRasterHeight = m_pDataset->GetRasterHeight();
	EHdRasterDataType eDataType = m_pDataset->GetBandDataType(1);

	// 获取纹理实际大小
	ITexture* pTexture = m_pMeshBuffer->Material.getTexture(0);
	unsigned int nTextureWidth = pTexture->getSize().Width;
	unsigned int nTextureHeight = pTexture->getSize().Height;

	// 计算待读取数据在局部坐标系下的地理范围
	float fLoadMinX, fLoadMaxX, fLoadMinY, fLoadMaxY;
	if (!CalcDataEnv(fLoadMinX, fLoadMaxX, fLoadMinY, fLoadMaxY))
	{
		return false;
	}

	// 根据地理范围计算出读取数据使用的金字塔索引、像素范围、目标栅格大小和实际的地理范围
	int nOvIndex, nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize, nDstXSize, nDstYSize;
	float fRealMinX, fRealMaxX, fRealMinY, fRealMaxY;
	CalcLoadPixels(m_pDataset, fLoadMinX, fLoadMaxX, fLoadMinY, fLoadMaxY, nTextureWidth, nTextureHeight,
		nOvIndex, nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize, nDstXSize, nDstYSize,
		fRealMinX, fRealMaxX, fRealMinY, fRealMaxY);

	// 如果新的地理范围与当前地理范围相同，则不必更新纹理
	if (abs(fRealMinX - m_fLoadMinX) < 0.1 && abs(fRealMaxX - m_fLoadMaxX) < 0.1 &&
		abs(fRealMinY - m_fLoadMinY) < 0.1 && abs(fRealMaxY - m_fLoadMaxY) < 0.1)
	{
		return true;
	}
	
	// 读取栅格数据
	CHdRasterBlock raster(nDstXSize, nDstYSize, m_nRenderBandCount, eDataType);
	if ((nOvIndex >= 0 && !m_pDataset->Read(nOvIndex, nSrcXOff, nSrcYOff, m_nRenderBandCount, m_pRenderBandIndex, &raster)) ||
		(nOvIndex < 0 && !m_pDataset->Read(nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize, m_nRenderBandCount, m_pRenderBandIndex, &raster)))
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
		(m_nRenderType == 3 && !rasterRenderer.RenderByColorMap(&raster, m_pDataset->GetColorTable(), &renderImage, 0, 0, nDstXSize, nDstYSize)))
	{
		pTexture->unlock();
		return false;
	}
	pTexture->unlock();
	//pTexture->regenerateMipMapLevels();

	// 纹理更新成功后修改地理范围成员变量
	m_fLoadMinX = fRealMinX;
	m_fLoadMaxX = fRealMaxX;
	m_fLoadMinY = fRealMinY;
	m_fLoadMaxY = fRealMaxY;

	/*
	// 输出纹理图像
	GDALDriver* pDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	if (pDriver)
	{
		GDALDataset* pDataset = pDriver->Create("E:\\gray.tif", m_nTextureSize, m_nTextureSize, 3, GDT_Byte, NULL);
		if (pDataset)
		{
			pDataset->RasterIO(GF_Write, 0, 0, m_nTextureSize, m_nTextureSize, pTextureData, m_nTextureSize, m_nTextureSize, GDT_Byte,
				3, NULL, 4, 4 * m_nTextureSize, 1);
			delete pDataset;
		}
	}
	*/
	
	// 重新设置网格的顶点数组和索引数组
	m_pMeshBuffer->Vertices.clear();
	m_pMeshBuffer->Indices.clear();
	m_pMeshBuffer->Vertices.reallocate(4);
	m_pMeshBuffer->Indices.reallocate(6);

	irr::video::S3DVertex sVertex;
	sVertex.Color.set(255, 255, 255, 255);
	sVertex.Normal.set(0, 0, 1);

	m_pMeshBuffer->BoundingBox.MinEdge.set(m_fLoadMinX, m_fLoadMinY, -0.01f);
	m_pMeshBuffer->BoundingBox.MaxEdge.set(m_fLoadMaxX, m_fLoadMaxY, 0.01f);

	sVertex.Pos.set(m_fLoadMinX, m_fLoadMinY, 0.0f);                
	sVertex.TCoords.set(0, ((float)nDstYSize) / m_nTextureSize);
	m_pMeshBuffer->Vertices.push_back(sVertex);

	sVertex.Pos.set(m_fLoadMinX, m_fLoadMaxY, 0.0f);     
	sVertex.TCoords.set(0.0f, 0.0f);
	m_pMeshBuffer->Vertices.push_back(sVertex);

	sVertex.Pos.set(m_fLoadMaxX, m_fLoadMaxY, 0.0f);      
	sVertex.TCoords.set(((float)nDstXSize) / m_nTextureSize, 0);
	m_pMeshBuffer->Vertices.push_back(sVertex);

	sVertex.Pos.set(m_fLoadMaxX, m_fLoadMinY, 0.0f);      
	sVertex.TCoords.set(((float)nDstXSize) / m_nTextureSize, ((float)nDstYSize) / m_nTextureSize);
	m_pMeshBuffer->Vertices.push_back(sVertex);

	m_pMeshBuffer->Indices.push_back(0);
	m_pMeshBuffer->Indices.push_back(1);
	m_pMeshBuffer->Indices.push_back(2);

	m_pMeshBuffer->Indices.push_back(0);
	m_pMeshBuffer->Indices.push_back(2);
	m_pMeshBuffer->Indices.push_back(3);

	m_pMeshBuffer->setDirty();
	
	return true;
}
void CHdDomSceneNode::SetBaseHeight(double dfBaseZ)
{
	m_dfBaseZ = dfBaseZ;

	// 更新影像局部坐标系到全局坐标系和场景坐标系的变换模型
	const CHdGeoEnvelope* pGeoEnv = m_pDataset->GetGeoEnvelope();
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

	// 更新影像节点到场景根节点的变换矩阵
	setPosition(irr::core::vector3df(m_renderModel.m_fOffset[0], m_renderModel.m_fOffset[1], m_renderModel.m_fOffset[2]));

	// 更新影像节点的绝对变换矩阵
	updateAbsolutePosition();
}
void CHdDomSceneNode::SetTextureSize(unsigned int nSize)
{
	m_nTextureSize = nSize;

	//! 移除纹理后重新添加纹理
	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	ITexture* pTexture = m_pMeshBuffer->Material.getTexture(0);
	const irr::io::path textureName = pTexture->getName();
	m_pMeshBuffer->Material.setTexture(0, NULL);                  // 这句代码很重要
	driver->removeTexture(pTexture);
	pTexture = driver->addTexture(irr::core::dimension2d<irr::u32>(m_nTextureSize, m_nTextureSize), 
		textureName, ECF_A8R8G8B8);
	m_pMeshBuffer->Material.setTexture(0, pTexture);
}
void CHdDomSceneNode::UpdateTransModel()
{
	CBursaWolfModel* pViewModel = m_pView->GetTransModel();
	m_renderModel = m_absModel * (*pViewModel);

	// 更新影像节点到场景根节点的变换矩阵
	setPosition(irr::core::vector3df(m_renderModel.m_fOffset[0], m_renderModel.m_fOffset[1], m_renderModel.m_fOffset[2]));

	// 更新影像节点的绝对变换矩阵
	updateAbsolutePosition();
}
bool CHdDomSceneNode::InitRenderType()
{
	if (!m_pDataset)
	{
		return false;
	}
	unsigned int nRasterWidth = m_pDataset->GetRasterWidth();
	unsigned int nRasterHeight = m_pDataset->GetRasterHeight();
	unsigned int nBandCount = m_pDataset->GetBandCount();
	EHdRasterDataType eDataType = m_pDataset->GetBandDataType(1);

	// 获取栅格数据的绘制方式以及绘制使用的波段索引
	if (m_pDataset->GetColorTable())
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
				if (m_pDataset->GetBandColorInterp(iBand) == EHD_CI_Alpha)
				{
					pBandIndex[3] = iBand;
				}
				else if (m_pDataset->GetBandColorInterp(iBand) == EHD_CI_Red)
				{
					pBandIndex[2] = iBand;
				}
				else if (m_pDataset->GetBandColorInterp(iBand) == EHD_CI_Green)
				{
					pBandIndex[1] = iBand;
				}
				else if (m_pDataset->GetBandColorInterp(iBand) == EHD_CI_Blue)
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
			m_pDataset->GetMinMaxValue(1, m_dfMinValue, m_dfMaxValue);
		}
	}
	return true;
}
bool CHdDomSceneNode::CalcDataEnv(float& fGeoMinX, float& fGeoMaxX, float& fGeoMinY, float& fGeoMaxY)
{
	if (!m_pDataset)
	{
		return false;
	}

	// 获取数据集在局部坐标系下的地理范围
	const CHdGeoEnvelope* pGeoEnv = m_pDataset->GetGeoEnvelope();
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

	// 计算两条直线与影像平面的交点
	irr::core::plane3df planeDom(0, 0, 0, 0, 0, 1);
	irr::core::vector3df point1, point2;
	planeDom.getIntersectionWithLine(line1.start, line1.getVector(), point1);
	planeDom.getIntersectionWithLine(line2.start, line2.getVector(), point2);

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

	return true;
}
bool CHdDomSceneNode::CalcLoadPixels(CHdRasterDataset* pDataset, float fLoadMinX, float fLoadMaxX, float fLoadMinY, 
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