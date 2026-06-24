#include "HdCoreData.h"
#include "HlzDefs.h"
#include "HlzDefs31.h"
#include "..\hdCommon\point_types2.h"
#include "..\hd3DEngine\include\irrMath.h"
#include "HdLevel.h"
#include "HdLevel31.h"
#include "HdBlock.h"
#include "HdBlock31.h"
#include "HdBlockset.h"
#include "HdBlockset31.h"
#include "HdParcel.h"
#include "HdParcel31.h"
#include "PtEncoder.h"
#include <algorithm>
#include "..\hdGeoPosition\PositionTranfrom.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

using namespace irr::core;

namespace hd
{
	bool cmpAddr(CHdParcelBase* pt0, CHdParcelBase* pt1)
	{
		return pt0->GetIntensityAddr().GetAddress() < pt1->GetIntensityAddr().GetAddress();
	}

	CHdCoreData::CHdCoreData(void)
		:IHLSReader(),m_pDataFile(NULL),m_fileMap(NULL),m_fileFlag(0),m_bUseMemMap(false),
		m_mapAddress(NULL),m_curFileIndex(0xffff),m_isOpen(FALSE),m_currentLevel(-1), m_bSplitHld(true), m_bNewHlz(false)
	{
		memset(m_dir,0,256);
		memset(m_name,0,256);
		strcpy(m_datExt,".hld");
		::InitializeCriticalSectionAndSpinCount( &m_cs, 0x80000402 );
	}

	CHdCoreData::~CHdCoreData(void)
	{
		Close();

		::DeleteCriticalSection(&m_cs);
	}

	// 关闭对象
	 BOOL CHdCoreData::Close()
	{
		map<U16,CHdLevel*>::iterator it;
		for (it = m_lstLevel.begin();it != m_lstLevel.end();++it)
		{
			if(it->second)
			{
				delete it->second;
				it->second = NULL;
			}
		}
		m_lstLevel.clear();

		CloseFile();

		m_hlzHeader.clean();

		m_mapAddress = NULL;
		m_curFileIndex = 0;
		m_isOpen = FALSE;
		m_currentLevel = -1;
		return TRUE;
	}

	// 关闭文件及映射
	void CHdCoreData::CloseFile()
	{
		EnterCriticalSection(&m_cs);
		CloseMapdata();
		if (m_pDataFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_pDataFile);
			m_pDataFile = NULL;
		}
		m_curFileIndex = 0xffff;
		LeaveCriticalSection(&m_cs);
	}

	BOOL CHdCoreData::OpenFile(const char* filePath)
	{
		//// 对该接口加密
		//string strSoftName = "hdVector";
		//char strMsg[256] = {0};
		//if (!CheckLicense(strSoftName.c_str(), strMsg))
		//{
		//	::MessageBox(NULL,"请向武汉汉宁轨道交通技术有限公司申请加密狗！", "提示",MB_OK);
		//	return false;
		//}

		CloseFile();

		EnterCriticalSection(&m_cs);
		m_pDataFile = CreateFile(filePath, 
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ|FILE_SHARE_WRITE, 
			NULL,
			OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL, 
			NULL);

		if (m_pDataFile == INVALID_HANDLE_VALUE)
		{
			LeaveCriticalSection(&m_cs);
			//fprintf(stderr, "ERROR: cannot open file '%s'\n", filePath);
			return FALSE;
		}

		if (!m_bUseMemMap)
		{
			LeaveCriticalSection(&m_cs);
			return TRUE;
		}

		DWORD sizeHigh = 0;
		U64 fileSize = ::GetFileSize(m_pDataFile,&sizeHigh);
		if (sizeHigh != 0)
		{
			fileSize = ((U64)sizeHigh << 32) | fileSize;
		}

		m_fileMap = ::CreateFileMapping(m_pDataFile, NULL, PAGE_READWRITE, 0, 0, NULL);//PAGE_READONLY
		if (m_fileMap == NULL)
		{
			LeaveCriticalSection(&m_cs);
			fprintf(stderr, "ERROR: cannot create file mapping\n");
			return FALSE;
		}

		m_mapAddress = (char*)(::MapViewOfFile (m_fileMap,FILE_MAP_ALL_ACCESS, 0,0,0));// FILE_MAP_WRITE | FILE_MAP_READ

		LeaveCriticalSection(&m_cs);
		return m_mapAddress != NULL;		
	}

	bool CHdCoreData::ParseIndex(char* buf)
	{
		//根据hlz版本转调不同的解析函数
		return !m_bNewHlz ? ParseOldIndex(buf) : ParseNewIndex(buf);
	}

	//hlz 3.0格式空间索引解析
	bool CHdCoreData::ParseOldIndex(char* buf)
	{
		if(NULL == buf)
		{
			return false;
		}

		m_lstLevel.clear();

		// 定义中间变量
		double dTmpX,dTmpY,dTmpZ;
		dTmpX = dTmpY = dTmpZ = 0.0;

		CHdBox3df fullBox;
		fullBox.MinEdge.set(0.0,0.0,m_hlzHeader.min_z);
		fullBox.MaxEdge.set(m_hlzHeader.max_x,m_hlzHeader.max_y,m_hlzHeader.max_z);

		U32 bsIndexLen = sizeof(HdBlockset), blkIndexLen = sizeof(HdBlock), pclIndexLen = sizeof(HdParcel);
		if(!m_hlzHeader.isCompress)
		{
			bsIndexLen -= sizeof(U32);
			blkIndexLen -= sizeof(U32);
			pclIndexLen -= sizeof(U32);
		}

		// 读取层记录
		int lvlCount = m_hlzHeader.number_of_level;
	
		CHdLevel* pLevel = NULL;
		int i = 0;
		for (i = 0;i < lvlCount;i++)
		{
			pLevel = new CHdLevel;
			pLevel->m_levelNo = i;
			memcpy(&pLevel->m_level, buf, sizeof(HdLevel));
			buf += sizeof(HdLevel);

			// 添加块记录
			AddLevelRec(pLevel);

			// 读取块集索引
			//int bsCount = pLevel->m_level.numBlocksetX * pLevel->m_level.numBlocksetY;// * pLevel->m_level.numBlocksetZ;
			int bsCount = pLevel->m_level.numBlockset;
			int iBs;
			CHdBlockset* pBlockSet = NULL;

			for (iBs = 0;iBs < bsCount;iBs++)
			{
				pBlockSet = new CHdBlockset;
				memcpy(&pBlockSet->m_blockSet, buf, bsIndexLen);
				buf += bsIndexLen;
			
				pBlockSet->UpdateExtent(pBlockSet->m_blockSet.box);
				pBlockSet->m_levelNo = i;
				CHdBox3df blockSetBox = pBlockSet->GetExtent();
				double dCoordLfX,dCoordLfY,dCoordLfZ;
				dCoordLfX = dCoordLfY = dCoordLfZ = 0.0;

				if (m_hlzHeader.iZoneID == 0)
				{
					pBlockSet->m_nBlockSetNo = iBs;
				}
				else
				{
					// 由绝对坐标计算块集编号
					U32 blockSetNoTmp = 0;
					// 不使用硬编码，获取更大的灵活性   袁亮  20161105
					//GetBlockSetNO(i,fullBox,blockSetBox,blockSetNoTmp);
					GetBlockSetNO(pLevel->m_level.sizeX, pLevel->m_level.sizeY, fullBox,blockSetBox,blockSetNoTmp);
					pBlockSet->m_nBlockSetNo = blockSetNoTmp;
				}

				pBlockSet->m_pParent = NULL;

				// 添加块集记录
				pLevel->AddBlockSetRec(pBlockSet);

				// 空的块集记录
				if (pBlockSet->m_blockSet.hasSubBlock == 0)
				{	
					continue;
				}

				int iBk;
				CHdBlock* pBlock = NULL;
				for (iBk = 0;iBk < pBlockSet->m_blockSet.numBlock;iBk++)
				{
					pBlock = new CHdBlock;
					memcpy(&pBlock->m_block, buf, blkIndexLen);
					buf += blkIndexLen;

					pBlock->UpdateExtent(pBlock->m_block.box);
					pBlock->m_bsNo = pBlockSet->m_nBlockSetNo;

					// 定义中间变量
					CHdBox3df blockBox = pBlock->GetExtent();
					double dCrdBlockLfX,dCrdBlockLfY,dCrdBlockLfZ;
					dCrdBlockLfX = dCrdBlockLfY = dCrdBlockLfZ = 0.0;

					// 计算块编号
					if (m_hlzHeader.iZoneID == 0)
					{
						pBlock->m_nBlockNo = iBk;
					}
					else
					{
						// 由块集编号及块外包围盒计算块编号
						U16 blockNoTmp = 0;
						GetBlockNO(blockSetBox,blockBox,blockNoTmp);
						pBlock->m_nBlockNo = blockNoTmp;
					}

					pBlock->m_pParent = pBlockSet; // 记录块的父节点块集
					// 添加块记录
					pBlockSet->AddBlockRec(pBlock);

					// 添加块内部的包
					//int pclCount = pBlock->m_block.numParcelX * pBlock->m_block.numParcelY * pBlock->m_block.numParcelZ;
					if (pBlock->m_block.hasSubParcel == 0)
					{
						continue;
					}
					int iPcl;
					CHdParcel* pParcel = NULL;
					for (iPcl = 0;iPcl < /*pclCount*/pBlock->m_block.numParcel;iPcl++)
					{
						pParcel = new CHdParcel;
						memcpy(&pParcel->m_parcel, buf, pclIndexLen);
						buf += pclIndexLen;

						pParcel->UpdateExtent(pParcel->m_parcel.box);
						pParcel->m_bkNo = pBlock->m_nBlockNo;

						// 定义中间变量
						CHdBox3df parcelBox = pParcel->GetExtent();
						double dCrdParcelLfX,dCrdParcelLfY,dCrdParcelLfZ;
						dCrdParcelLfX = dCrdParcelLfY = dCrdParcelLfZ = 0.0;

						// 计算包编号
						//if (m_hlzHeader.iZoneID == 0)
						{
							pParcel->m_nParcelNo = iPcl;
						}
						//else
						//{
						//	// 由包的包围盒获得相对块的编号
						//	dCrdParcelLfX = parcelBox.MinEdge.X + m_hlzHeader.offsetX;
						//	dCrdParcelLfY = parcelBox.MinEdge.Y + m_hlzHeader.offsetY;
						//	dCrdParcelLfZ = parcelBox.MinEdge.Z + m_hlzHeader.offsetZ;

						//	// 由绝对坐标计算包相对于块的编号

						//}


						pParcel->m_pParent = pBlock; // 记录包的父节点块

						pBlock->AddParcel(pParcel);

					}
					pBlock->Update();
				}//for (iBk = 0;iBk < pBlockSet->m_blockSet.numBlock;iBk++)
				pBlockSet->Update();
			}//for (iBs = 0;iBs < bsCount;iBs++)

			pLevel->Update();
		}//for (i = 0;i < lvlCount;i++)

		return true;
	}

	bool CHdCoreData::ParseNewIndex(char* buf)
	{
		if(NULL == buf)
		{
			return false;
		}

		//清空原有层索引
		m_lstLevel.clear();

		// 获得头文件绝对坐标范围
		f64 header_max_x = m_hlzHeader.max_x + m_hlzHeader.offsetX;
		f64 header_max_y = m_hlzHeader.max_y + m_hlzHeader.offsetY;
		f64 header_max_z = m_hlzHeader.max_z + m_hlzHeader.offsetZ;

		f64 header_min_x = m_hlzHeader.min_x + m_hlzHeader.offsetX;
		f64 header_min_y = m_hlzHeader.min_y + m_hlzHeader.offsetY;
		f64 header_min_z = m_hlzHeader.min_z + m_hlzHeader.offsetZ;

		//点云包围盒
		CHdBox3df full_box;
		full_box.MinEdge.set(0.0,0.0,m_hlzHeader.min_z);
		full_box.MaxEdge.set(m_hlzHeader.max_x,m_hlzHeader.max_y,m_hlzHeader.max_z);

		//2.定位到层索引起始处，解析索引
		CHdLevel31* level = NULL;
		int level_count =m_hlzHeader.number_of_level;
		//2.1解析层索引
		for(int i = 0; i < level_count; i++)
		{
			level = new CHdLevel31;
			level->m_levelNo = i;
			memcpy(&level->m_level31,  buf, sizeof(HdLevel31));
			buf += sizeof(HdLevel31);

			AddLevelRec(level);

			//2.2解析块集索引
			u64 blockset_count = level->m_level31.numBlockset;
			CHdBlockset31* blockset = NULL;
			for(u32 bs_index = 0; bs_index < blockset_count; bs_index++)
			{
				blockset = new CHdBlockset31;
				HdBlockset31& blockset_info = blockset->m_blockSet31;

				//2.2.1先解析块集索引前21字节(必含)
				u16 bs_count = sizeof(HdBlockset31) - sizeof(HdAddr) * 4;
				memcpy(&blockset_info, buf, bs_count);
				buf += bs_count;

				// 根据层级计算包围盒，先计算绝对坐标
				f64 blockset_box_size = pow(2.0,level->m_level31.levelNo + 1);
				f64 blockset_box_min_x = blockset_box_size * blockset_info.XNo;
				f64 blockset_box_min_y = blockset_box_size * blockset_info.YNo;
				f64 blockset_box_min_z = blockset_box_size * blockset_info.ZNo;

				f64 blockset_box_max_x = blockset_box_size * (blockset_info.XNo + 1);
				f64 blockset_box_max_y = blockset_box_size * (blockset_info.YNo + 1);
				f64 blockset_box_max_z = blockset_box_size * (blockset_info.ZNo + 1);

				// 记录块集绝对坐标包围盒
				CHdBox3dd blockset_extent;
				blockset_extent.MinEdge.set(blockset_box_min_x,blockset_box_min_y,blockset_box_min_z);
				blockset_extent.MaxEdge.set(blockset_box_max_x,blockset_box_max_y,blockset_box_max_z);

				// 包围盒计算，减去偏移量后记录为相对坐标
				CHdBox3df blockset_box;
				blockset_box.MinEdge.X = (f32)(blockset_box_min_x - m_hlzHeader.offsetX);
				blockset_box.MinEdge.Y = (f32)(blockset_box_min_y - m_hlzHeader.offsetY);
				blockset_box.MinEdge.Z = (f32)(blockset_box_min_z - m_hlzHeader.offsetZ);

				blockset_box.MaxEdge.X = (f32)(blockset_box_max_x - m_hlzHeader.offsetX);
				blockset_box.MaxEdge.Y = (f32)(blockset_box_max_y - m_hlzHeader.offsetY);
				blockset_box.MaxEdge.Z = (f32)(blockset_box_max_z - m_hlzHeader.offsetZ);

				// 包围盒更新
				blockset->UpdateExtent(blockset_box);
				blockset->SetBlocksetExtent(blockset_extent);

				blockset->m_levelNo = i;
				double coord_left_x,coord_left_y,coord_left_z;
				coord_left_x = coord_left_y = coord_left_z = 0.0;

				//是否有投影带号
				if (m_hlzHeader.iZoneID == 0)
				{
					blockset->m_nBlockSetNo = bs_index;
				}
				else
				{
					// 由绝对坐标计算块集编号
					u32 temp_blockset_no = 0;
					u32 step = 2 << level->m_level31.levelNo;
					GetBlockSetNO(step, step, full_box,blockset_box,temp_blockset_no);
					blockset->m_nBlockSetNo = temp_blockset_no;
				}

				blockset->m_pParent = NULL;
				level->AddBlockSetRec(blockset);

				//2.2.2再解析块集数据地址
				// 空的块集记录, 说明块集索引中包含数据地址信息，继续解析数据地址
				if (blockset_info.getBlockNum() == 0)
				{
					//1.解析基本属性地址信息
					memcpy(&blockset_info.addrBaseAttri, buf, sizeof(HdAddr));
					buf += sizeof(HdAddr);

					//2.再解析可选数据项
					//a.包含颜色时间数据
					if(blockset_info.hasColor() && blockset_info.hasTime())
					{
						memcpy(&blockset_info.addrColor, buf, 2 * sizeof(HdAddr));
						buf += 2 * sizeof(HdAddr);
					}
					//b.仅包含颜色
					else if(blockset_info.hasColor())
					{
						memcpy(&blockset_info.addrColor, buf, sizeof(HdAddr));
						buf += sizeof(HdAddr);
					}
					//c.仅包含时间
					else if(blockset_info.hasTime())
					{
						memcpy(&blockset_info.addrTime, buf, sizeof(HdAddr));
						buf += sizeof(HdAddr);
					}
					//d.不包含可选项
					else
					{
					}

					//3.最后解析压缩长度信息
					if(blockset_info.isCompress())
					{
						memcpy(&blockset_info.attriCmpLen, buf, sizeof(HdAddr));
						buf += sizeof(HdAddr);
					}

					blockset->Update();
					continue;
				}

				u8 block_num = blockset_info.getBlockNum();
				CHdBlock31* block = NULL;
				for(u8 block_index = 0; block_index < block_num; block_index++)
				{
					block = new CHdBlock31;
					HdBlock31& block_info = block->m_block31;

					//首先读取块索引前7个字节（必含）
					u16 b_count = sizeof(HdBlock31) - sizeof(HdAddr) * 4;
					memcpy(&block_info, buf, b_count);
					buf += b_count;

					// 根据层级计算包围盒，先计算绝对坐标
					f64 block_box_size = blockset_box_size * 0.5;
					f64 block_box_min_x = blockset_box_min_x + block_box_size * block_info.getXNo();
					f64 block_box_min_y = blockset_box_min_y + block_box_size * block_info.getYNo();
					f64 block_box_min_z = blockset_box_min_z + block_box_size * block_info.getZNo();

					f64 block_box_max_x = blockset_box_min_x + block_box_size * (block_info.getXNo() + 1);
					f64 block_box_max_y = blockset_box_min_y + block_box_size * (block_info.getYNo() + 1);
					f64 block_box_max_z = blockset_box_min_z + block_box_size * (block_info.getZNo() + 1);

					// 包围盒计算，减去偏移量后记录为相对坐标
					CHdBox3df block_box;
					block_box.MinEdge.X = (f32)(block_box_min_x - m_hlzHeader.offsetX);
					block_box.MinEdge.Y = (f32)(block_box_min_y - m_hlzHeader.offsetY);
					block_box.MinEdge.Z = (f32)(block_box_min_z - m_hlzHeader.offsetZ);

					block_box.MaxEdge.X = (f32)(block_box_max_x - m_hlzHeader.offsetX);
					block_box.MaxEdge.Y = (f32)(block_box_max_y - m_hlzHeader.offsetY);
					block_box.MaxEdge.Z = (f32)(block_box_max_z - m_hlzHeader.offsetZ);

					// 包围盒更新
					block->UpdateExtent(block_box);
					block->SetBlocksetExtent(blockset_extent);

					block->m_bsNo = blockset->m_nBlockSetNo;

					// 定义中间变量
					//hdBoxf block_box = block->getExtent();
					double block_left_x,block_left_y,block_left_z;
					block_left_x = block_left_y = block_left_z = 0.0;

					// 计算块编号
					if (m_hlzHeader.iZoneID == 0)
					{
						block->m_nBlockNo = block_index;
					}
					else
					{
						// 由块集编号及块外包围盒计算块编号
						u16 temp_block_no = 0;
						GetBlockNO(blockset_box,block_box,temp_block_no);
						block->m_nBlockNo = temp_block_no;
					}

					// 记录块的父节点块集
					block->m_pParent = blockset; 

					// 添加块记录
					blockset->AddBlockRec(block);

					// 添加块内部的包
					//不包含包，说明块索引中包含数据地址信息，继续解析数据地址
					if (block_info.numParcel == 0)
					{
						//1.解析基本属性地址信息
						memcpy(&block_info.addrBaseAttri, buf, sizeof(HdAddr));
						buf +=  sizeof(HdAddr);

						//2.再解析可选数据项
						//a.包含颜色时间数据
						if(block_info.hasColor() && block_info.hasTime())
						{
							memcpy(&block_info.addrColor, buf, 2 * sizeof(HdAddr));
							buf += 2 * sizeof(HdAddr);
						}
						//b.仅包含颜色
						else if(block_info.hasColor())
						{
							memcpy(&block_info.addrColor, buf, sizeof(HdAddr));
							buf += sizeof(HdAddr);
						}
						//c.仅包含时间
						else if(block_info.hasTime())
						{
							memcpy(&block_info.addrTime, buf, sizeof(HdAddr));
							buf += sizeof(HdAddr);
						}
						//d.不包含可选项
						else
						{
						}

						//3.最后解析压缩长度信息
						if(block_info.isCompress())
						{
							memcpy(&block_info.attriCmpLen, buf, sizeof(HdAddr));
							buf += sizeof(HdAddr);
						}

						block->Update();
						continue;
					}

					u16 parcel_num = block_info.numParcel;
					CHdParcel31* parcel = NULL;
					for(u16 parcel_index = 0; parcel_index < parcel_num; parcel_index++)
					{
						parcel = new CHdParcel31;
						HdParcel31& parcel_info = parcel->m_parcel31;

						u16 pcl_count = sizeof(HdParcel31) - sizeof(HdAddr) * 3;
						memcpy(&parcel_info, buf, pcl_count);
						buf += pcl_count;

						// 根据层级计算包围盒，先计算绝对坐标，所有计算均以左下角为起点
						f64 parcel_box_min_x = block_box_min_x + block_box_size * parcel_info.XNo / pow(2.0,parcel_info.getDivCount()+1);
						f64 parcel_box_min_y = block_box_min_y + block_box_size * parcel_info.YNo / pow(2.0,parcel_info.getDivCount()+1);
						f64 parcel_box_min_z = block_box_min_z + block_box_size * parcel_info.ZNo / pow(2.0,parcel_info.getDivCount()+1);

						f64 parcel_box_max_x = block_box_min_x + block_box_size * (parcel_info.XNo + 1) / pow(2.0,parcel_info.getDivCount()+1);
						f64 parcel_box_max_y = block_box_min_y + block_box_size * (parcel_info.YNo + 1) / pow(2.0,parcel_info.getDivCount()+1);
						f64 parcel_box_max_z = block_box_min_z + block_box_size * (parcel_info.ZNo + 1) / pow(2.0,parcel_info.getDivCount()+1);

						// 包围盒计算，减去偏移量后记录为相对坐标
						CHdBox3df parcel_box;
						parcel_box.MinEdge.X = (f32)(parcel_box_min_x - m_hlzHeader.offsetX);
						parcel_box.MinEdge.Y = (f32)(parcel_box_min_y - m_hlzHeader.offsetY);
						parcel_box.MinEdge.Z = (f32)(parcel_box_min_z - m_hlzHeader.offsetZ);

						parcel_box.MaxEdge.X = (f32)(parcel_box_max_x - m_hlzHeader.offsetX);
						parcel_box.MaxEdge.Y = (f32)(parcel_box_max_y - m_hlzHeader.offsetY);
						parcel_box.MaxEdge.Z = (f32)(parcel_box_max_z - m_hlzHeader.offsetZ);

						// 包围盒更新
						parcel->UpdateExtent(parcel_box);
						parcel->SetBlocksetExtent(blockset_extent);

						parcel->m_bkNo = block->m_nBlockNo;

						// 计算包编号
						parcel->m_nParcelNo = parcel_index;

						// 记录包的父节点块
						parcel->m_pParent = block; 

						//解析可选数据项
						//a.包含颜色时间数据
						if(parcel_info.hasColor() && parcel_info.hasTime())
						{
							memcpy(&parcel_info.addrColor, buf, 2 * sizeof(HdAddr));
							buf += 2 * sizeof(HdAddr);
						}
						//b.仅包含颜色
						else if(parcel_info.hasColor())
						{
							memcpy(&parcel_info.addrColor, buf, sizeof(HdAddr));
							buf += sizeof(HdAddr);
						}
						//c.仅包含时间
						else if(parcel_info.hasTime())
						{
							memcpy(&parcel_info.addrTime, buf, sizeof(HdAddr));
							buf += sizeof(HdAddr);
						}

						//解析压缩长度信息
						if(parcel_info.isCompress())
						{
							memcpy(&parcel_info.attriCmpLen, buf, sizeof(HdAddr));
							buf += sizeof(HdAddr);
						}

						//添加包记录
						block->AddParcel(parcel);
					} //for (u16 parcel_index = 0; parcel_index < block_info.num_parcel;parcel_index++)

					block->Update();
				} //for (u8 block_index = 0;block_index < block_num; block_index++)

				blockset->Update();
			} //for (u64 bs_index = 0; bs_index < blockset_count; bs_index++)

			level->Update();
		}//for(int i = 0; i < level_count; i++)

		return true;
	}

	void CHdCoreData::CloseMapdata()
	{
		if (m_fileMap)
		{
			if (m_mapAddress)
			{
				UnmapViewOfFile (m_mapAddress);
				m_mapAddress = 0;
			}
			CloseHandle (m_fileMap);
			m_fileMap = 0;
		}
	}

	void CHdCoreData::UpdateScale()
	{		
		// 假设屏幕分辨率是1440*900,DPI
		int scrWidth = ::GetSystemMetrics(SM_CXFULLSCREEN);
		int scrHeight = ::GetSystemMetrics(SM_CYFULLSCREEN);
		HWND wnd = GetDesktopWindow();
		HDC dc = ::GetDC(wnd);
		int nDpi = ::GetDeviceCaps(dc,LOGPIXELSX);
		ReleaseDC(wnd,dc);
		//int nLegend = (int)(nLen / 2.54); // 一英寸对应25.4mm,nLegend1cm对应像素个数
		// 计算屏幕宽度,单位米
		F64 fsrcWidth = ((F64)scrWidth / nDpi) * 2.54 / 100;
		F64 fsrcHeight = ((F64)scrHeight / nDpi) * 2.54 / 100;
		F64 xMin,yMin,zMin,xMax,yMax,zMax;
		m_hlzHeader.getGlobalExtent(xMin,yMin,zMin,xMax,yMax,zMax);
		F64 scaleX = (xMax - xMin) / fsrcWidth;
		F64 scaleY = (yMax - yMin) / fsrcHeight;
		F64 scale = MIN(scaleX,scaleY);
		scale /= 1.5;
		int count = m_lstLevel.size();
		int i = 0;
		for(i = count - 1;i >= 0;i--)
		{
			CHdLevel* pLevel = m_lstLevel.at(i);
			pLevel->m_level.dispScale = scale / (pow(sqrt((F64)SIMPLE_SCALE),(F64)(count - i - 1)));
		}		
	}

	BOOL CHdCoreData::Open( const char* strFileName )
	{
		Close();
		m_strIndexFilePath = strFileName;

		// 获取文件系统,NTFS单个数据文件,非NTFS多个数据文件，每个数据文件2GB并以此编号
		// 解析路径得到m_dir,m_name
		char drive[256] = {0};
		char dir[256] = {0};
		char filename[256] = {0};
		char ext[256] = {0};

		_splitpath(strFileName,drive,dir,filename,ext);
		_makepath(m_dir,drive,dir,NULL,NULL);


		char VolName[256]={0};// 磁盘驱动器卷标名称

		DWORD SerailNumber;// 磁盘驱动器卷标序列号
		DWORD MaxCLenth;// 系统允许的最大文件名长度
		DWORD FileSysFlag;// 文件系统标识
		char FileSysName[256]={0};// 文件操作系统名称

		GetVolumeInformation(drive,VolName,255,&SerailNumber,&MaxCLenth,&FileSysFlag,FileSysName,255);

		if (strcmp("FAT32",FileSysName) == 0)
		{
			m_fileFlag = 1;
		}
		else if (strcmp("NTFS",FileSysName) == 0)
		{
			m_fileFlag = 0;
		}

		strcpy(m_name,filename);

		// 打开索引文件
		FILE* pIndexFile = fopen(strFileName,"rb");
		if (pIndexFile == NULL)
		{
			return FALSE;
		}

		// 读取文件头
		char  tmp[256] = {0};		
		fread(tmp,sizeof(char),256,pIndexFile);
		m_hlzHeader.prase(tmp);

		if (m_hlzHeader.file_signature[0] != 'H' ||
			m_hlzHeader.file_signature[1] != 'L' ||
			m_hlzHeader.file_signature[2] != 'Z' ||
			m_hlzHeader.file_signature[3] != 0)
		{
			fclose(pIndexFile);
			pIndexFile = NULL;
			return FALSE;
		}

		//判断是否为hlz 3.2 格式文件（3.1格式被污染，视为3.0格式）
		m_bNewHlz = (m_hlzHeader.version_major == 3 && m_hlzHeader.version_minor == 2);

		//空间索引读入内存
		fseek(pIndexFile, 0, SEEK_END);
		long size = ftell(pIndexFile) - 256;
		char* buf = new char[size];
		fseek(pIndexFile, 256, SEEK_SET);
	    fread(buf, sizeof(char), size, pIndexFile);

		//解析空间索引
		if(!ParseIndex(buf))
		{
			delete[] buf;
			fclose(pIndexFile);
			return FALSE;
		}

		delete[] buf;
		fclose(pIndexFile);

		// 预先打开数据文件
		m_curFileIndex = 0;
		char strDataFile[512] = {0};
		sprintf_s(strDataFile,512,"%s\\%s-%04d%s",m_dir,m_name,m_curFileIndex,m_datExt);
		if(!OpenFile(strDataFile))
		{
			m_curFileIndex = 0;
			strcpy(m_datExt,".hlz");
			sprintf_s(strDataFile,512,"%s\\%s-%04d%s",m_dir,m_name,m_curFileIndex,m_datExt);
			if(!OpenFile(strDataFile))
			{
				return FALSE;
			}
		}

		//hlz 3.2 格式数据文件都是分卷存储的
		if(m_bNewHlz)
		{
			m_bSplitHld = true;
		}
		else
		{
			//获取最顶层空间实体中文件地址最大的那一个，据此判断数据文件是否分文件存储   袁亮   20160823
			map<U16,CHdLevel*>& listLevel = GetListLevel();
			if(listLevel.empty())
			{
				return FALSE;
			}

			int index = listLevel.size()-1;
			CHdLevel* pLevelRec = GetLevelRec(index);
			CHdListAreaNoInf lstArea;
			if(GetLevelAreas(pLevelRec, &lstArea) <= 0)
			{
				return FALSE;
			}

			auto it = std::max_element(lstArea.begin(), lstArea.end(), cmpAddr);
			U64 maxAddr = (*it)->GetIntensityAddr().GetAddress();
			maxAddr += (*it)->GetPtCount() * sizeof(HdIntensity);
			U64 maxAddr2 = maxAddr + (*it)->GetPtCount() * sizeof(HdPtColor);

			DWORD sizeHigh = 0;
			U64 fileSize = ::GetFileSize(m_pDataFile,&sizeHigh);
			if (sizeHigh != 0)
			{
				fileSize = ((U64)sizeHigh << 32) | fileSize;
			}
			//原有hld文件可能没有写入颜色数据，也需要兼容它   袁亮   20160823
			m_bSplitHld = !(maxAddr == fileSize || maxAddr2 == fileSize);
		}
		
		// 由于OpenFile内部重新设置了m_curFileIndex编号值，需要重新设置
		m_curFileIndex = 0;

		// 计算转换坐标矩阵
		ComputeMatrix();

		m_isOpen = TRUE;

		strcpy(m_filepath,strFileName);

		return TRUE;
	}

	void CHdCoreData::ComputeMatrix()
	{
		m_hlzHeader.computeMatrix(m_matrix);
	}

	void CHdCoreData::GetIntersectBlock()
	{

	}

	BOOL CHdCoreData::AddLevelRec( CHdLevel* pLevelRec )
	{
		m_lstLevel.insert(HdLevelPair(pLevelRec->m_levelNo,pLevelRec));
		return TRUE;
	}

	CHdLevel* CHdCoreData::GetLevelRec( U16 nLevelNo )
	{
		map<U16,CHdLevel*>::iterator it = m_lstLevel.find(nLevelNo);
		if (it != m_lstLevel.end())
		{
			return it->second;
		} 
		else
		{
			return NULL;
		}
	}

	BOOL CHdCoreData::AddBlockSetRec( CHdBlockset* pBlockSetRec )
	{
		EnterCriticalSection(&m_cs);
		CHdLevel* pLevelRec = GetLevelRec(pBlockSetRec->m_levelNo);	
		if(0 == pLevelRec)
		{
			return FALSE;
			LeaveCriticalSection(&m_cs);
		}

		BOOL bRet = pLevelRec->AddBlockSetRec(pBlockSetRec); 
		LeaveCriticalSection(&m_cs);
		return bRet;
	}

	CHdBlockset* CHdCoreData::GetBlockSetRec( U16 nLevelNo, U32 nBlockSetNo )
	{
		EnterCriticalSection(&m_cs);
		CHdLevel* pLevelRec = GetLevelRec(nLevelNo);

		if(0 == pLevelRec)
		{
			LeaveCriticalSection(&m_cs);
			return NULL;
		}
		CHdBlockset* pBS = pLevelRec->GetBlockSetRec(nBlockSetNo);
		LeaveCriticalSection(&m_cs);
		return 	pBS;
	}

	// 根据比例尺设置当前层,dScale比例尺分母
	void CHdCoreData::SetCurrentLevelByScale(F64 dScale)
	{
		EnterCriticalSection(&m_cs);
		map<U16,CHdLevel*>::iterator it = m_lstLevel.begin();

		U16 curLevel = it->first;
		bool flag = false;// 标记是否找到合适的比例

		for (;it != m_lstLevel.end();++it)
		{
			if (dScale <= (F64)(it->second->m_level.dispScale))
			{
				// 找到最接近的一个退出
				curLevel = it->first;

				if (m_currentLevel!=curLevel)
				{
					// 切换层时，先卸载当前层数据
					UnLoadCurLevlData();
					m_currentLevel = curLevel;

				}
				flag = true;
				break;
			}
		}

		if (!flag) // 没有找到的话 用最小比例尺，即最顶层
		{
			size_t lvcnt = m_lstLevel.size();
			u32 dispscal = m_lstLevel.at(lvcnt-1)->m_level.dispScale;
			
			if (dScale>dispscal && m_currentLevel != lvcnt-1)
			{
				// 切换层时，先卸载当前层数据
				UnLoadCurLevlData();
				m_currentLevel = lvcnt-1;
			}
		}

		LeaveCriticalSection(&m_cs);
	}

	// 根据点数判断设置当前层
	void CHdCoreData::SetCurrentLevelByPtNum(const int ptNum,const CHdBox3df& rect)
	{
		EnterCriticalSection(&m_cs);
		map<U16,CHdLevel*>::iterator it = m_lstLevel.begin();

		bool bfind = false; // 判断是否找到合适层

		for (;it != m_lstLevel.end();++it)
		{
			U16 LevelNum = it->first;

			CHdListAreaNoInf listArea;

			// 获取满足条件的显示列表
			GetIntersectAreas(LevelNum,rect,&listArea);

			int ptCount = 0;

			for (int i = 0; i<listArea.size(); i++)
			{
				CHdParcelBase* parcl = listArea[i];

				if (parcl)
				{
					ptCount += parcl->GetPtCount();
				}

			}

			if (ptCount < ptNum) // 统计点数，如果找到大于阈值的层级就退出
			{
				if (m_currentLevel != LevelNum)
				{
					// 切换层时，先卸载当前层数据
					UnLoadCurLevlData();
					m_currentLevel = LevelNum;

				}

				break;
			}
		}	
		LeaveCriticalSection(&m_cs);
	}

	// 根据层号设置当前层
	void CHdCoreData::SetCurrentLevelByNum(int num)
	{
		EnterCriticalSection(&m_cs);
		map<U16,CHdLevel*>::iterator it = m_lstLevel.begin();

		U16 curLevel = it->first;
		for (;it != m_lstLevel.end();++it)
		{
			if (num == it->first)
			{
				if (m_currentLevel != num)
				{
					// 切换层时，先卸载当前层数据
					UnLoadCurLevlData();
					m_currentLevel = num;
					break;
				}
			
			}
		}	
		LeaveCriticalSection(&m_cs);
	}

	CHdLevel* CHdCoreData::GetCurrentLevelRec()
	{
		return GetLevelRec(m_currentLevel);
	}

	short CHdCoreData::GetCurrentLevelNo()
	{
		return m_currentLevel;
	}

	BOOL CHdCoreData::ChangeCurrentLevel( short nLevelNo, BOOL bChangeScale )
	{
		return FALSE;
	}

	BOOL CHdCoreData::ChangeOneUpLevel( BOOL bChangeScale )
	{
		return FALSE;
	}

	BOOL CHdCoreData::ChangeOneDownLevel( BOOL bChangeScale )
	{
		return FALSE;
	}

	hd::CHdBox3df CHdCoreData::CalcParcelRect( short nLevelNo,short nBlockSetNo,short nBlockNo,short nParcelNo )
	{
		return CHdBox3df();
	}

	BOOL CHdCoreData::GetIntersectArea( short nLevelNo, F32 dPubX, F32 dPubY, F32 dPubZ, 
		unsigned short& nBlockSetNo, unsigned short& nBlockNo, unsigned short& nParcelNo )
	{
		return FALSE;
	}

	int CHdCoreData::GetIntersectAreas( CHdLevel* pLevelRec, const CHdBox3df& rect, CHdListAreaNoInf* pLstAreaNoInf )
	{
		if(pLevelRec == 0)
			return 0;

		EnterCriticalSection(&m_cs);
		CHdBox3df curRect;
		// 获取Blockset列表
		map<I32,CHdBlockset*>::iterator it;
		for (it = pLevelRec->m_pListBlockset.begin();it != pLevelRec->m_pListBlockset.end();it++)
		{
			CHdBlockset* pHdBlockset = it->second;
			if (!pHdBlockset->GetExtent().intersectsWithBox(rect))
			{
				continue;
			}

			// 如果当前块集不包含子块,则计算其范围,
			if (!pHdBlockset->m_blockSet.hasSubBlock)
			{
				pLstAreaNoInf->push_back(pHdBlockset);
				continue;
			}

			map<I32,CHdBlock*>::iterator itBlk;
			for (itBlk = pHdBlockset->m_pListBlock.begin();itBlk != pHdBlockset->m_pListBlock.end();itBlk++)
			{
				CHdBlock* pBlock = itBlk->second;
				if (!pBlock->GetExtent().intersectsWithBox(rect))
				{
					continue;
				}

				// 如果当前块集不包含子块,则计算其范围,
				if (!pBlock->m_block.hasSubParcel)
				{
					pLstAreaNoInf->push_back(pBlock);
					continue;
				}

				map<I32,CHdParcel*>::iterator itPcl;
				for (itPcl = pBlock->m_pListParcel.begin();itPcl != pBlock->m_pListParcel.end();itPcl++)
				{
					CHdParcel* pParcel = itPcl->second;
					if (pParcel->GetExtent().intersectsWithBox(rect))
					{
						pLstAreaNoInf->push_back(pParcel);
					}
				}
			}
		}
		LeaveCriticalSection(&m_cs);
		return pLstAreaNoInf->size();
	}


	int CHdCoreData::GetLevelAreas(CHdLevel* pLevelRec, CHdListAreaNoInf* pLstAreaNoInf)
	{
		if(pLevelRec == 0)
		{
			return 0;
		}
		EnterCriticalSection(&m_cs);
		map<I32,CHdBlockset*>::iterator it;
		for (it = pLevelRec->m_pListBlockset.begin();it != pLevelRec->m_pListBlockset.end();it++)
		{
			CHdBlockset* pHdBlockset = it->second;

			if (!pHdBlockset->m_blockSet.hasSubBlock)
			{
				pLstAreaNoInf->push_back(pHdBlockset);
				continue;
			}

			map<I32,CHdBlock*>::iterator itBlk;
			for (itBlk = pHdBlockset->m_pListBlock.begin();itBlk != pHdBlockset->m_pListBlock.end();itBlk++)
			{
				CHdBlock* pBlock = itBlk->second;

				if (!pBlock->m_block.hasSubParcel)
				{
					pLstAreaNoInf->push_back(pBlock);
					continue;
				}

				map<I32,CHdParcel*>::iterator itPcl;
				for (itPcl = pBlock->m_pListParcel.begin();itPcl != pBlock->m_pListParcel.end();itPcl++)
				{
					CHdParcel* pParcel = itPcl->second;

					pLstAreaNoInf->push_back(pParcel);				
				}
			}
		}
		LeaveCriticalSection(&m_cs);
		return pLstAreaNoInf->size();
	}

	int CHdCoreData::GetCurLevelAreas(const CHdBox3df& rect,CHdListAreaNoInf* pLstAreaNoInf)
	{
		//减少冗余代码，直接转调GetIntersectAreas()接口    袁亮    20161012
		return GetIntersectAreas(GetCurrentLevelRec(), rect, pLstAreaNoInf);
		// 获取当前层
		/*CHdLevel* pCurHdLevel = GetCurrentLevelRec();

		if (!pCurHdLevel)
		{
			return 0;
		}
		EnterCriticalSection(&m_cs);
		CHdBox3df curRect;

		// 获取Blockset列表
		map<I32,CHdBlockset*>::iterator it;
		for (it = pCurHdLevel->m_pListBlockset.begin();it != pCurHdLevel->m_pListBlockset.end();it++)
		{
			CHdBlockset* pHdBlockset = it->second;

			// 无相交执行下次
			if (!pHdBlockset->GetExtent().intersectsWithBox(rect))
			{
				continue;
			}

			// 如果当前块集不包含子块,则计算其范围,
			if (!pHdBlockset->m_blockSet.hasSubBlock)
			{
				pLstAreaNoInf->push_back(pHdBlockset);
				continue;
			}

			map<I32,CHdBlock*>::iterator itBlk;
			for (itBlk = pHdBlockset->m_pListBlock.begin();itBlk != pHdBlockset->m_pListBlock.end();itBlk++)
			{
				CHdBlock* pBlock = itBlk->second;

				// 不相交执行下次
				if (!pBlock->GetExtent().intersectsWithBox(rect))
				{
					continue;
				}

				// 如果当前块集不包含子块,则计算其范围,
				if (!pBlock->m_block.hasSubParcel)
				{
					pLstAreaNoInf->push_back(pBlock);
					continue;
				}

				map<I32,CHdParcel*>::iterator itPcl;
				for (itPcl = pBlock->m_pListParcel.begin();itPcl != pBlock->m_pListParcel.end();itPcl++)
				{
					CHdParcel* pParcel = itPcl->second;

					pLstAreaNoInf->push_back(pParcel);

				}
			}
		}

		LeaveCriticalSection(&m_cs);
		return pLstAreaNoInf->size();*/
	}

	// 获取指定层的BlockSet列表
	int CHdCoreData::GetLevelBSAreas(short nLevelNo, CHdListAreaNoInf* pListAreaNoinf)
	{
		// 获取当前层
		CHdLevel* pLevelRec = GetLevelRec(nLevelNo);

		if (!pLevelRec || pListAreaNoinf == NULL)
		{
			return 0;
		}
		EnterCriticalSection(&m_cs);
		pListAreaNoinf->clear();
		map<I32,CHdBlockset*>::iterator it;
		for (it = pLevelRec->m_pListBlockset.begin();it != pLevelRec->m_pListBlockset.end();it++)
		{
			CHdBlockset* pHdBlockset = it->second;
			pListAreaNoinf->push_back(pHdBlockset);			
		}
		LeaveCriticalSection(&m_cs);
		return pListAreaNoinf->size();
	}
	
	// 获取指定层的一定范围的BlockSet列表
	int CHdCoreData::GetLevelBSAreas(short nLevelNo, const CHdBox3df& rect,CHdListAreaNoInf* pListAreaNoinf)
	{

		// 获取当前层
		CHdLevel* pLevelRec = GetLevelRec(nLevelNo);

		if (!pLevelRec || pListAreaNoinf == NULL)
		{
			return 0;
		}
		EnterCriticalSection(&m_cs);
		pListAreaNoinf->clear();
		map<I32,CHdBlockset*>::iterator it;
		for (it = pLevelRec->m_pListBlockset.begin();it != pLevelRec->m_pListBlockset.end();it++)
		{
			CHdBlockset* pHdBlockset = it->second;

			// 无相交执行下次
			if (!pHdBlockset->GetExtent().intersectsWithBox(rect))
			{
				continue;
			}

			pListAreaNoinf->push_back(pHdBlockset);			
		}
		LeaveCriticalSection(&m_cs);
		return pListAreaNoinf->size();

	}

	int CHdCoreData::GetCurLevelAreasByEye(const CHdBox3df& rect,const F32 eye[3],CHdListAreaNoInf* pLstAreaNoInf)
	{
		// 获取当前层
		CHdLevel* pCurHdLevel = GetCurrentLevelRec();

		if (!pCurHdLevel)
		{
			return 0;
		}
		EnterCriticalSection(&m_cs);
		CHdBox3df curRect;
	
		CHdVector3df center;// 数据集的范围中心

		F32 dist = 0.0; //距离

		F32 angle = 0.0; //角度

		// 获取Blockset列表
		map<I32,CHdBlockset*>::iterator it;
		for (it = pCurHdLevel->m_pListBlockset.begin();it != pCurHdLevel->m_pListBlockset.end();it++)
		{
			CHdBlockset* pHdBlockset = it->second;

			// 无相交执行下次
			if (!pHdBlockset->GetExtent().intersectsWithBox(rect))
			{
				continue;
			}
			
			// 如果当前块集不包含子块,则计算其范围,
			if (!pHdBlockset->m_blockSet.hasSubBlock)
			{
				center = pHdBlockset->GetExtent().getCenter();

				dist = (center.X- eye[0])*(center.X- eye[0]) + (center.Y- eye[1])*(center.Y- eye[1]) +
					(center.Z- eye[2])*(center.Z- eye[2]);

				dist = sqrt(dist);
				pHdBlockset->m_distance = dist;

				pLstAreaNoInf->push_back(pHdBlockset);
				continue;
			}

			map<I32,CHdBlock*>::iterator itBlk;
			for (itBlk = pHdBlockset->m_pListBlock.begin();itBlk != pHdBlockset->m_pListBlock.end();itBlk++)
			{
				CHdBlock* pBlock = itBlk->second;

				// 不相交执行下次
				if (!pBlock->GetExtent().intersectsWithBox(rect))
				{
					continue;
				}
	
				// 如果当前块集不包含子块,则计算其范围,
				if (!pBlock->m_block.hasSubParcel)
				{
					center =  pBlock->GetExtent().getCenter();

					dist = (center.X- eye[0])*(center.X- eye[0]) + (center.Y- eye[1])*(center.Y- eye[1]) +
						(center.Z- eye[2])*(center.Z- eye[2]);

					dist = sqrt(dist);
					pBlock->m_distance = dist;
					pLstAreaNoInf->push_back(pBlock);
					continue;
				}

				map<I32,CHdParcel*>::iterator itPcl;
				for (itPcl = pBlock->m_pListParcel.begin();itPcl != pBlock->m_pListParcel.end();itPcl++)
				{
					CHdParcel* pParcel = itPcl->second;

					center =  pParcel->GetExtent().getCenter();

					dist = (center.X- eye[0])*(center.X- eye[0]) + (center.Y- eye[1])*(center.Y- eye[1]) +
						(center.Z- eye[2])*(center.Z- eye[2]);

					dist = sqrt(dist);
					pParcel->m_distance = dist;
					pLstAreaNoInf->push_back(pParcel);

				}
			}
		}
		LeaveCriticalSection(&m_cs);
		return pLstAreaNoInf->size();
	}


	// 卸载范围外点云
	int CHdCoreData::UnloadOutOfExtent(const CHdBox3df& rect)
	{
		EnterCriticalSection(&m_cs);
		CHdLevel* pLevelRec = GetCurrentLevelRec();
		map<I32,CHdBlockset*>::iterator it;
		for (it = pLevelRec->m_pListBlockset.begin();it != pLevelRec->m_pListBlockset.end();it++)
		{
			CHdBlockset* pHdBlockset = it->second;

			bool bIntersect = pHdBlockset->GetExtent().intersectsWithBox(rect);

			// 如果范围相交且不包含子块,则无需卸载
			if (bIntersect && !pHdBlockset->m_blockSet.hasSubBlock)
			{
				continue;
			}

			// 不相交且不包含子块,卸载内存数据
			if (!bIntersect && !pHdBlockset->m_blockSet.hasSubBlock )
			{
				if(pHdBlockset->m_pHlzPoint)
				{
					delete[] pHdBlockset->m_pHlzPoint;
					pHdBlockset->m_pHlzPoint = NULL;
				}
				continue;
			}			

			// ---------------包含子块情况----------------------

			map<I32,CHdBlock*>::iterator itBlk;
			for (itBlk = pHdBlockset->m_pListBlock.begin();itBlk != pHdBlockset->m_pListBlock.end();itBlk++)
			{
				CHdBlock* pBlock = itBlk->second;
				bIntersect = pBlock->GetExtent().intersectsWithBox(rect);

				// 如果范围相交且不包含子块,则无需卸载
				if (bIntersect && !pBlock->m_block.hasSubParcel)
				{
					continue;
				}

				// 不相交且不包含子块,卸载内存数据
				if (!bIntersect && !pBlock->m_block.hasSubParcel )
				{
					if(pBlock->m_pHlzPoint)
					{
						delete[] pBlock->m_pHlzPoint;
						pBlock->m_pHlzPoint = NULL;
					}
					continue;
				}

				// ---------------包含子包情况----------------------
				map<I32,CHdParcel*>::iterator itPcl;
				for (itPcl = pBlock->m_pListParcel.begin();itPcl != pBlock->m_pListParcel.end();itPcl++)
				{
					CHdParcel* pParcel = itPcl->second;
					if (!pParcel)
					{
						continue;
					}

					bIntersect = pParcel->GetExtent().intersectsWithBox(rect);

					// 如果范围不相交,则卸载
					if (!bIntersect && pParcel->m_pHlzPoint)
					{						
						delete[] pParcel->m_pHlzPoint;
						pParcel->m_pHlzPoint = NULL;						
					}					
				}
			}
		}
		LeaveCriticalSection(&m_cs);
		return 1;
	}


	// 卸载当前层数据
	int CHdCoreData::UnLoadCurLevlData()
	{
		CHdLevel* pLevelRec = GetCurrentLevelRec();

		if (!pLevelRec)
		{
			return 0;
		}
		EnterCriticalSection(&m_cs);
		map<I32,CHdBlockset*>::iterator it;
		for (it = pLevelRec->m_pListBlockset.begin();it != pLevelRec->m_pListBlockset.end();it++)
		{
			CHdBlockset* pHdBlockset = it->second;

			// 不相交且不包含子块,卸载内存数据
			if (!pHdBlockset->m_blockSet.hasSubBlock )
			{
				if(pHdBlockset->m_pHlzPoint)
				{
					delete[] pHdBlockset->m_pHlzPoint;
					pHdBlockset->m_pHlzPoint = NULL;
				}
				continue;
			}			

			// ---------------包含子块情况----------------------

			map<I32,CHdBlock*>::iterator itBlk;
			for (itBlk = pHdBlockset->m_pListBlock.begin();itBlk != pHdBlockset->m_pListBlock.end();itBlk++)
			{
				CHdBlock* pBlock = itBlk->second;


				// 不相交且不包含子块,卸载内存数据
				if (!pBlock->m_block.hasSubParcel )
				{
					if(pBlock->m_pHlzPoint)
					{
						delete[] pBlock->m_pHlzPoint;
						pBlock->m_pHlzPoint = NULL;
					}
					continue;
				}

				// ---------------包含子包情况----------------------
				map<I32,CHdParcel*>::iterator itPcl;
				for (itPcl = pBlock->m_pListParcel.begin();itPcl != pBlock->m_pListParcel.end();itPcl++)
				{
					CHdParcel* pParcel = itPcl->second;

					// 如果范围相交,则卸载
					if (pParcel->m_pHlzPoint)
					{			
						delete[] pParcel->m_pHlzPoint;
						pParcel->m_pHlzPoint = NULL;						
					}					
				}
			}
		}
		LeaveCriticalSection(&m_cs);
		return 1;
	}


	int CHdCoreData::GetIntersectAreas( short nLevelNo, const CHdBox3df& rect, CHdListAreaNoInf* pLstAreaNoInf )
	{
		CHdLevel* pLevelRec = GetLevelRec(nLevelNo);
		if (pLevelRec == NULL)
		{
			pLstAreaNoInf->clear();
			return 0;
		}
		return GetIntersectAreas(pLevelRec,rect,pLstAreaNoInf);		
	}

	BOOL CHdCoreData::LoadSpecParcel( CHdParcelBase* noInfo )
	{
		// 判断是否已经有内存点云数据
		if (noInfo->m_pHlzPoint)
		{
			return TRUE;
		}

		EnterCriticalSection(&m_cs);
		//不论文件系统，hlz数据文件都按照2GB大小进行划分    袁亮  20160818
		//兼容原有大文件方式  袁亮  20160823
		if (m_bSplitHld)
		{
			U16 fileIndex = noInfo->GetCoordAddr().GetFileNo();
			if (fileIndex != m_curFileIndex)
			{
				char strDataFile[256] = {0};
				sprintf(strDataFile,"%s\\%s-%04d%s",m_dir,m_name,fileIndex,m_datExt);
				if(!OpenFile(strDataFile))
				{
					strcpy(m_datExt,".hlz");
					sprintf(strDataFile,"%s\\%s-%04d%s",m_dir,m_name,fileIndex,m_datExt);
					if(!OpenFile(strDataFile))
					{
						LeaveCriticalSection(&m_cs);
						return FALSE;
					}
				}
			}

			m_curFileIndex = fileIndex;
		}

		BOOL bRetC = FALSE;
		if(noInfo->GetCoordAddr().IsInValid())
		{
			LeaveCriticalSection(&m_cs);
			return FALSE;
		}
		LARGE_INTEGER li;		
		U64 addrCoord = 0;
		if (m_bSplitHld)
		{
			addrCoord = noInfo->GetCoordAddr().GetAddrInFile();
		}
		else
		{
			addrCoord = noInfo->GetCoordAddr().GetAddress();
		}

		U64 addrInt = 0;		
		BOOL bRetI = noInfo->GetIntensityAddr().IsInValid() ? FALSE : TRUE;
		if(bRetI)
		{
			if (m_bSplitHld)
			{
				addrInt = noInfo->GetIntensityAddr().GetAddrInFile();
			}
			else
			{
				addrInt = noInfo->GetIntensityAddr().GetAddress();
			}
		}
		//读取颜色数据的地址信息   袁亮  20160625
		U64 addrColor = 0;
		BOOL bWithColor = !noInfo->GetColorAddr().IsInValid();
		if(bWithColor)
		{
			addrColor = m_bSplitHld ? noInfo->GetColorAddr().GetAddrInFile() : noInfo->GetColorAddr().GetAddress();
		}

		DWORD numRead = 0;

		U32 ptNum = noInfo->GetPtCount();
		if (ptNum <= 0)    //没有点
		{
			LeaveCriticalSection(&m_cs);
			return FALSE;
		}

		U32 ptBufSize = noInfo->GetCmpCoordLen();      // 数据大小
		U32 ptIntenSize = noInfo->GetCmpIntenLen();
		U8* ptBuf = NULL;   // 点的数据流;
		HdPointXYZ* pPtArray = new HdPointXYZ[ptNum];   //点数据
		/*int szIntensity = ptNum * sizeof(HdIntensity);
		U8* pIntBuf = new U8[szIntensity];*/
		//先判断是否有强度和颜色数据，再决定是否进行内存分配   袁亮  20160628
		U8* pIntBuf = NULL;
		int szIntensity = 0;
		if(bRetI)
		{
			szIntensity = ptNum * sizeof(HdIntensity);
			pIntBuf = new U8[szIntensity];
		}
		HdPtColor* pPtColor = NULL;
		int szColorBytes = 0;
		if(bWithColor)
		{
			pPtColor = new HdPtColor[ptNum];
			szColorBytes = ptNum * sizeof(HdPtColor);
		}
		
		if (!m_hlzHeader.isCompress)
		{
			// 数据没有压缩
			// 不使用内存映射
			int sizeRead = ptNum * sizeof(HdPointXYZ);
			if (!m_bUseMemMap)
			{		
				// 读取坐标
				li.QuadPart = addrCoord;
				SetFilePointer(m_pDataFile,li.LowPart,&li.HighPart,FILE_BEGIN);
				bRetC = ReadFile(m_pDataFile, pPtArray, sizeRead, &numRead,NULL);
				bRetC = (numRead == sizeRead);
				// 读取强度
				if(bRetI)
				{
					li.QuadPart = addrInt;
					SetFilePointer(m_pDataFile,li.LowPart,&li.HighPart,FILE_BEGIN);
					bRetI = ReadFile(m_pDataFile,pIntBuf,szIntensity,&numRead,NULL);
					bRetI = (numRead == szIntensity);			
				}
				//读取颜色   袁亮  20160625
				if(bWithColor)
				{
					li.QuadPart = addrColor;
					SetFilePointer(m_pDataFile,li.LowPart,&li.HighPart,FILE_BEGIN);
					bWithColor = ReadFile(m_pDataFile, pPtColor, szColorBytes, &numRead,NULL);
					bWithColor = (numRead == szColorBytes);
				}
			}
			else if (m_mapAddress)
			{		
				// 读取坐标
				errno_t err = memcpy_s(pPtArray, sizeRead, m_mapAddress + addrCoord, sizeRead);			
				// 读取强度		
				if(bRetI)
				{
					err = memcpy_s(pIntBuf,szIntensity, m_mapAddress + addrInt, szIntensity);
				}
				//读取颜色   袁亮  20160625
				if(bWithColor)
				{
					err = memcpy_s(pPtColor, szColorBytes, m_mapAddress + addrColor, szColorBytes);
				}
			}
		}
		else
		{
			// 不使用内存映射
			if (!m_bUseMemMap)
			{		
				// 读取坐标
				li.QuadPart = addrCoord;
				SetFilePointer(m_pDataFile,li.LowPart,&li.HighPart,FILE_BEGIN);
				//再根据数据大小来读取坐标数据
				ptBuf = new U8[ptBufSize];
				bRetC = ReadFile(m_pDataFile, ptBuf,ptBufSize, &numRead,NULL);

				//解压
				CPtXYZEncoder ptDecode(ptBuf, ptBufSize, ptNum);
				ptDecode.decodePoints(pPtArray);

				delete[] ptBuf;
				ptBuf = NULL;
				bRetC = (numRead == ptBufSize);

				// 读取强度
				if(bRetI)
				{
					li.QuadPart = addrInt;
					SetFilePointer(m_pDataFile,li.LowPart,&li.HighPart,FILE_BEGIN);
					if(!m_bNewHlz)
					{
						bRetI = ReadFile(m_pDataFile,pIntBuf,szIntensity,&numRead,NULL);
						bRetI = (numRead == szIntensity);		
					}
					else
					{
						u8* cmpIntenBuf = new u8[ptIntenSize];
						ReadFile(m_pDataFile, cmpIntenBuf, ptIntenSize, &numRead, NULL);

						CPtIEncoder intenDecoder(cmpIntenBuf, ptIntenSize, ptNum);
						intenDecoder.decodeInten(pIntBuf);

						delete[] cmpIntenBuf;
					}		
				}

				//读取颜色   袁亮  20160625
				if(bWithColor)
				{
					li.QuadPart = addrColor;
					SetFilePointer(m_pDataFile,li.LowPart,&li.HighPart,FILE_BEGIN);
					bWithColor = ReadFile(m_pDataFile,pPtColor,szColorBytes,&numRead,NULL);
					bWithColor = (numRead == szColorBytes);
				}
			}
			else if (m_mapAddress)
			{		
				// 读取坐标
				ptBuf = new U8[ptBufSize];
				errno_t err = memcpy_s(ptBuf, ptBufSize, m_mapAddress + addrCoord + sizeof(int), ptBufSize);

				//解压
				CPtXYZEncoder ptDecode(ptBuf, ptBufSize, ptNum);
				ptDecode.decodePoints(pPtArray);

				delete[] ptBuf;
				ptBuf = NULL;

				// 读取强度		
				if(bRetI)
				{
					if(!m_bNewHlz)
					{
						err = memcpy_s(pIntBuf,szIntensity,
							m_mapAddress + addrInt,szIntensity);
					}
					else
					{
						u8* cmpIntenBuf = new u8[ptIntenSize];
						memcpy(cmpIntenBuf, m_mapAddress + addrInt, ptIntenSize);

						CPtIEncoder intenDecoder(cmpIntenBuf, ptIntenSize, ptNum);
						intenDecoder.decodeInten(pIntBuf);

						delete[] cmpIntenBuf;
					}
				}
				//读取颜色   袁亮  20160625
				if(bWithColor)
				{
					err = memcpy_s(pPtColor, szColorBytes, m_mapAddress + addrColor, szColorBytes);
				}
			}
		}

		//hlz3.1格式文件坐标解析
		BOOL bRet = FALSE;
		if(m_bNewHlz)
		{
			f64 offsetX = m_hlzHeader.offsetX;
			f64 offsetY = m_hlzHeader.offsetY;
			f64 offsetZ = m_hlzHeader.offsetZ;

			HdRefPointd refPoint;
			refPoint.FromBox(noInfo->GetBlocksetExtent());

			bRet = PtBlock2PtArrayWithHeader(pPtArray, noInfo->GetPtCount(), &refPoint, &noInfo->m_pHlzPoint, pIntBuf, pPtColor, offsetX, offsetY, offsetZ);
		}
		//hlz 3.0格式文件坐标解析
		else
		{
			//// 根据头文件中是否记录带号进行计算
			if (m_hlzHeader.iZoneID != 0)
			{
				// 获得当前包/块/块集所在的块集的包围盒
				// 尝试将基类转子类,再根据子类重组数据库唯一字段
				//只需要包所属块集的包围盒信息，无关代码删除   袁亮   20161012
				//int zoneID,levelID,blockSetID,blockID,parcelID;
				CHdBlockset* pBlockSet = dynamic_cast<CHdBlockset*>(noInfo);
				CHdBlock* pBlock = dynamic_cast<CHdBlock*>(noInfo);
				CHdParcel* pParcel = dynamic_cast<CHdParcel*>(noInfo);
				//zoneID = m_hlzHeader.iZoneID;
				CHdBox3df blockSetBox;

				if (pBlockSet) // 若块集存在
				{
					/*levelID = pBlockSet->m_levelNo;
					blockSetID = pBlockSet->m_nBlockSetNo;*/
					blockSetBox = pBlockSet->GetExtent();
				}

				// 若块存在
				if (pBlock)
				{
					// 获得父节点块集
					pBlockSet = dynamic_cast<CHdBlockset*>(pBlock->m_pParent);
					/*levelID = pBlockSet->m_levelNo; // 层级
					blockSetID = pBlockSet->m_nBlockSetNo; // 块集
					blockID = pBlock->m_bsNo;*/
					blockSetBox = pBlockSet->GetExtent();
				}

				// 若包存在
				if (pParcel)
				{
					// 逐级获得父节点
					pBlock = dynamic_cast<CHdBlock*>(pParcel->m_pParent);
					pBlockSet = dynamic_cast<CHdBlockset*>(pBlock->m_pParent);

					// 包信息赋值
					/*levelID = pBlockSet->m_levelNo; // 层级
					blockSetID = pBlockSet->m_nBlockSetNo; // 块集
					blockID = pBlock->m_bsNo;
					parcelID = pParcel->m_nParcelNo;*/
					blockSetBox = pBlockSet->GetExtent();
				}

				// 将坐标强度转换为显示坐标
				HdRefPoint refPt;
				refPt.FromBox(noInfo->GetExtent());	

				// 重载计算
				//加入颜色信息  袁亮  20160625
				bRet = PtBlock2PtArray(pPtArray,noInfo->GetPtCount(),&refPt,&noInfo->m_pHlzPoint,blockSetBox.MinEdge.X,blockSetBox.MinEdge.Y,(U8*)pIntBuf, pPtColor);

				//// 根据块集编号获得相对块集编号的偏移量
				//double dTmpX,dTmpY,dTmpZ;
				//double dBlockSetOffX,dBlockSetOffY,dBlockSetOffZ;
				//dTmpX = dTmpY = dTmpZ = dBlockSetOffX = dBlockSetOffY = dBlockSetOffZ = 0.0;

				//// 计算相对块集的偏移量
				//CHdBox3df extent = noInfo->GetExtent();
				//dTmpX = extent.MinEdge.X + m_hlzHeader.offsetX;
				//dTmpY = extent.MinEdge.Y + m_hlzHeader.offsetY;
				//GetParcelDataOffsetXY(zoneID,levelID,dTmpX,dTmpY,dBlockSetOffX,dBlockSetOffY);

				//// 更新显示坐标
				//for (int i = 0;i < noInfo->GetPtCount();i++)
				//{
				//	// 获得绝对坐标
				//	dTmpX = noInfo->m_pHlzPoint[i].x + dBlockSetOffX;
				//	dTmpY = noInfo->m_pHlzPoint[i].y + dBlockSetOffY;
				//	dTmpZ = noInfo->m_pHlzPoint[i].z + dBlockSetOffZ;

				//	// 获得相对点云头文件偏移量的相对坐标
				//	noInfo->m_pHlzPoint[i].x = dTmpX - m_hlzHeader.offsetX;
				//	noInfo->m_pHlzPoint[i].y = dTmpY - m_hlzHeader.offsetY;
				//	noInfo->m_pHlzPoint[i].z = dTmpZ - m_hlzHeader.offsetZ;
				//}
			}
			else
			{
				// 将坐标强度转换为显示坐标
				HdRefPoint refPt;
				refPt.FromBox(noInfo->GetExtent());	

				bRet = PtBlock2PtArray(pPtArray,noInfo->GetPtCount(),&refPt,&noInfo->m_pHlzPoint,(U8*)pIntBuf, pPtColor);
			}
		}

		// 内存释放
		if(pIntBuf != NULL)
		{
			delete[] pIntBuf;
			pIntBuf = NULL;
		}
		if(pPtArray != NULL)
		{
			delete[] pPtArray;
			pPtArray = NULL;
		}
		if(pPtColor != NULL)
		{
			delete[] pPtColor;
			pPtColor = NULL;
		}
		LeaveCriticalSection(&m_cs);
		return bRet;
	}

	// 获取指定块数据
	BOOL CHdCoreData::LoadSpecBlock(CHdBlock* poBlock)
	{		
		if (!poBlock->m_block.hasSubParcel)
		{
			LoadSpecParcel(poBlock);
			return TRUE;
		}
		// 遍历子块
		map<I32,CHdParcel*>::iterator itPcl;
		for (itPcl = poBlock->m_pListParcel.begin();itPcl != poBlock->m_pListParcel.end();itPcl++)
		{
			CHdParcel* pParcel = itPcl->second;
			LoadSpecParcel(pParcel);				
		}
		poBlock->Update();
		return TRUE;
	}

	// 获取指定块集数据
	BOOL CHdCoreData::LoadSpecBlockSet(CHdBlockset* poBlockSet)
	{
		if (!poBlockSet->m_blockSet.hasSubBlock)
		{
			LoadSpecParcel(poBlockSet);
			return TRUE;
		}
		// 遍历子块集
		map<I32,CHdBlock*>::iterator itBlk;
		for (itBlk = poBlockSet->m_pListBlock.begin();itBlk != poBlockSet->m_pListBlock.end();itBlk++)
		{
			CHdBlock* pBlock = itBlk->second;

			if (!pBlock->m_block.hasSubParcel)
			{
				LoadSpecParcel(pBlock);
				continue;
			}

			// 遍历子块
			map<I32,CHdParcel*>::iterator itPcl;
			for (itPcl = pBlock->m_pListParcel.begin();itPcl != pBlock->m_pListParcel.end();itPcl++)
			{
				CHdParcel* pParcel = itPcl->second;
				LoadSpecParcel(pParcel);			
			}
		}
		poBlockSet->Update();
		return TRUE;
	}

    // 根据头文件解析ID
    bool CHdCoreData::ResolveParcelID(CHdParcelBase* pParcelBase, int& nZone, int& nLevelID, int& nBlockSetID, int& nBlockID, int& nParcelID, string& strParcelID)
    {
        // 初始化
        nZone = -1;
        nLevelID = -1;
        nBlockSetID = -1;
        nBlockID = -1;
        nParcelID = -1;

        char strTmp[128];
        nZone = m_hlzHeader.iZoneID;

        // 尝试将基类转子类,再根据子类重组数据库唯一字段
        CHdBlockset* pBlockSet = dynamic_cast<CHdBlockset*>(pParcelBase);
        CHdBlock* pBlock = dynamic_cast<CHdBlock*>(pParcelBase);
        CHdParcel* pParcel = dynamic_cast<CHdParcel*>(pParcelBase);

		bool bRet = false;

        if (pBlockSet) // 若块集存在
        {
            nLevelID = pBlockSet->m_levelNo;
            nBlockSetID = pBlockSet->m_nBlockSetNo;

            sprintf_s(strTmp, "%d-%d-%d", nZone, nLevelID, nBlockSetID);
            strParcelID = strTmp;
			bRet = true;
        }

        // 若块存在
        if (pBlock)
        {
            // 获得父节点块集
            pBlockSet = dynamic_cast<CHdBlockset*>(pBlock->m_pParent);
            nLevelID = pBlockSet->m_levelNo; // 层级
            nBlockSetID = pBlockSet->m_nBlockSetNo; // 块集
			nBlockID = pBlock->m_nBlockNo;
            //nBlockID = pBlock->m_bsNo;

            sprintf_s(strTmp, "%d-%d-%d-%d", nZone, nLevelID, nBlockSetID, nBlockID);
            strParcelID = strTmp;
			bRet = true;
        }

        // 若包存在
        if (pParcel)
        {
            // 逐级获得父节点
            pBlock = dynamic_cast<CHdBlock*>(pParcel->m_pParent);
            pBlockSet = dynamic_cast<CHdBlockset*>(pBlock->m_pParent);

            // 包信息赋值
            nLevelID = pBlockSet->m_levelNo; // 层级
            nBlockSetID = pBlockSet->m_nBlockSetNo; // 块集
            nBlockID = pBlock->m_nBlockNo;
            nParcelID = pParcel->m_nParcelNo;

            sprintf_s(strTmp, "%d-%d-%d-%d-%d", nZone, nLevelID, nBlockSetID, nBlockID, nParcelID);
            strParcelID = strTmp;
			bRet = true;
        }

        return bRet;
    }

	// 获取指定数据包
	//获取数据包的颜色信息   袁亮   20160625
	BOOL CHdCoreData::LoadSpecParcelData(CHdParcelBase* noInfo, HdPointXYZ** pPtBlock, U8** pPIntensity, HdPtColor** ppPtColor)
	{
		if(pPtBlock == NULL || pPIntensity == NULL || noInfo == NULL)
			return FALSE;
		EnterCriticalSection(&m_cs);
		//兼容原有大文件方式  袁亮  20160823
		if (m_bSplitHld)
		{
			U16 fileIndex = noInfo->GetCoordAddr().GetFileNo();
			if (fileIndex != m_curFileIndex)
			{
				char strDataFile[512] = {0};
				sprintf(strDataFile,"%s\\%s-%04d%s",m_dir,m_name,fileIndex,m_datExt);
				if(!OpenFile(strDataFile))
				{
					strcpy(m_datExt,".hlz");
					sprintf(strDataFile,"%s\\%s-%04d%s",m_dir,m_name,fileIndex,m_datExt);
					if(!OpenFile(strDataFile))
						return FALSE;
				}
			}
			
			m_curFileIndex = fileIndex;
		}

		BOOL bRetC = FALSE;
		BOOL bRetI = FALSE;
		BOOL bRetClr = FALSE;
		U64 addrCoord = 0;
		//先判断Parcel中是否有坐标数据   袁亮  20160628
		bRetC = !noInfo->GetCoordAddr().IsInValid();
		if(bRetC)
		{
			if (m_bSplitHld)
			{
				addrCoord = noInfo->GetCoordAddr().GetAddrInFile();
			}
			else
			{
				addrCoord = noInfo->GetCoordAddr().GetAddress();
			}
		}
		
		U64 addrInt = 0;
		bRetI = !noInfo->GetIntensityAddr().IsInValid();
		//先判断Parcel中是否有强度数据   袁亮  20160628
		if(bRetI)
		{
			if (m_bSplitHld)
			{
				addrInt = noInfo->GetIntensityAddr().GetAddrInFile();
			}
			else
			{
				addrInt = noInfo->GetIntensityAddr().GetAddress();
			}
		}
		
		//获取颜色数据索引信息，先判断是否有颜色数据  袁亮  20160628
		U64 addrColor = 0;
		bRetClr = !noInfo->GetColorAddr().IsInValid();
		if(bRetClr)
		{
			addrColor = m_bSplitHld ? noInfo->GetIntensityAddr().GetAddrInFile() : noInfo->GetIntensityAddr().GetAddress();
		}

		DWORD numRead = 0;

		//先判断是否存在坐标、强度、颜色数据，再进行内存分配
		int szCoordBuf = 0;
		int szIntensity = 0;
		int szColorBuf = 0;
		if(bRetC)
		{
			(*pPtBlock) = new HdPointXYZ[noInfo->GetPtCount()];
			szCoordBuf = noInfo->GetPtCount() * sizeof(HdPointXYZ);
		}
		if(bRetI)
		{
			(*pPIntensity) = new HdIntensity[noInfo->GetPtCount()];
			szIntensity = noInfo->GetPtCount() * sizeof(HdIntensity);
		}
		if(bRetClr)
		{
			(*ppPtColor) = new HdPtColor[noInfo->GetPtCount()];
			szColorBuf = noInfo->GetPtCount() * sizeof(HdPtColor);
		}

		// 不使用内存映射
		LARGE_INTEGER li;
		if (!m_bUseMemMap)
		{		
			// 读取坐标
			//先判断是否有坐标数据  袁亮  20160628
			if(bRetC)
			{
				li.QuadPart = addrCoord;
				SetFilePointer(m_pDataFile,li.LowPart,&li.HighPart,FILE_BEGIN);
				bRetC = ReadFile(m_pDataFile,(*pPtBlock),szCoordBuf,&numRead,NULL);
				bRetC = (numRead == szCoordBuf);
			}
			
			// 读取强度
			// 先判断是否有强度数据  袁亮  20160628
			if(bRetI)
			{
				li.QuadPart = addrInt;
				SetFilePointer(m_pDataFile,li.LowPart,&li.HighPart,FILE_BEGIN);
				bRetI = ReadFile(m_pDataFile,*pPIntensity,szIntensity,&numRead,NULL);
				bRetI = (numRead == szIntensity);
			}
		
			//读取颜色  先判断是否有强度数据 袁亮  20160628
			if(bRetClr)
			{
				li.QuadPart = addrColor;
				SetFilePointer(m_pDataFile, li.LowPart, &li.HighPart,FILE_BEGIN);
				bRetClr = ReadFile(m_pDataFile, (*ppPtColor), szColorBuf, &numRead, NULL);
				bRetClr = (numRead == szColorBuf);
			}
		}
		else if (m_mapAddress)
		{		
			// 读取坐标
			// 先判断是否有坐标数据  袁亮  20160628
			errno_t err = 0;
			if(bRetC)
			{
				err = memcpy_s(*pPtBlock,szCoordBuf,m_mapAddress + addrCoord,szCoordBuf);	
			}
			// 读取强度			
			// 先判断是否有强度数据  袁亮  20160628
			if(bRetI)
			{
				err = memcpy_s(*pPIntensity,szIntensity,m_mapAddress + addrInt,szIntensity);
			}
			//读取颜色,先判断是否有强度数据   袁亮  20160628
			if(bRetClr)
			{
				err = memcpy_s(*ppPtColor, szColorBuf, m_mapAddress + addrColor, szColorBuf);
			}
		}

		LeaveCriticalSection(&m_cs);
		return TRUE;
	}

    // 获取指定数据包
    BOOL CHdCoreData::LoadSpecParcelInfo(
        CHdParcelBase* noInfo,          // 输入, 块数据
        HdPointXYZ** pPointArray,        // 输出, 点坐标数据
        HdIntensity** pIntensityArray,   // 输出, 强度数据
        HdPtColor** pColorArray,         // 输出, 颜色数据
        HdPtClass** pClassArray,         // 输出, 分类数据
        U32 ptNum)                      // 输出, 点个数
    {
        // 判断是否已经有内存点云数据
		EnterCriticalSection(&m_cs);
   
		//不论文件系统，hlz数据文件都按照2GB大小进行划分    袁亮  20160818
		//兼容原有大文件方式  袁亮  20160823
        if (m_bSplitHld)
        {
            U16 fileIndex = noInfo->GetCoordAddr().GetFileNo();
            if (fileIndex != m_curFileIndex)
            {
                char strDataFile[256] = {0};
                sprintf(strDataFile,"%s\\%s-%04d%s", m_dir, m_name, fileIndex,m_datExt);
				if(!OpenFile(strDataFile))
				{
					strcpy(m_datExt,".hlz");
					sprintf(strDataFile,"%s\\%s-%04d%s",m_dir,m_name,fileIndex,m_datExt);
					if(!OpenFile(strDataFile))
					{
						LeaveCriticalSection(&m_cs);
						return FALSE;
					}
				}
            }

            m_curFileIndex = fileIndex;
       }

        // 读取坐标数据地址
        LARGE_INTEGER li;
        BOOL bRetCoord = noInfo->GetCoordAddr().IsInValid() ? FALSE : TRUE;
        U64 addrCoord = 0;
        if(noInfo->GetCoordAddr().IsInValid())
        {
			LeaveCriticalSection(&m_cs);
            return FALSE;
        }

        if (bRetCoord)
        {
            addrCoord = m_bSplitHld ? noInfo->GetCoordAddr().GetAddrInFile()
                  : noInfo->GetCoordAddr().GetAddress();
        }

        // 读取强度数据地址
        U64 addrInt = 0;
        BOOL bRetIntensity = noInfo->GetIntensityAddr().IsInValid() ? FALSE : TRUE;
        if(bRetIntensity)
        {
            addrInt = m_bSplitHld ? noInfo->GetIntensityAddr().GetAddrInFile()
                : noInfo->GetIntensityAddr().GetAddress();
        }

        // 读取颜色数据地址
        U64 addrColor = 0;
        BOOL bRetColor = noInfo->GetColorAddr().IsInValid() ? FALSE : TRUE;
        if (bRetColor)
        {
            addrColor = m_bSplitHld ? noInfo->GetColorAddr().GetAddrInFile()
                : noInfo->GetColorAddr().GetAddress();
        }

        // 读取分类数据地址
        U64 addrClass = 0;
        BOOL bRetClass = noInfo->GetClassAddr().IsInValid() ? FALSE : TRUE;
        if (bRetClass)
        {
            addrClass = m_bSplitHld ? noInfo->GetClassAddr().GetAddrInFile()
                : noInfo->GetClassAddr().GetAddress();
        }

        DWORD numRead = 0;

        // 获取点数量
        ptNum = noInfo->GetPtCount();
        if (ptNum <= 0 || ptNum > 64000)    //没有点
        {
			LeaveCriticalSection(&m_cs);
            return FALSE;
        }

        U32 ptBufSize = noInfo->GetCmpCoordLen();           // 数据大小
		U32 ptIntenSize = noInfo->GetCmpIntenLen();
        U8* ptBuf = NULL;                               // 点的数据流;
		U8* ptIntenBuf = NULL;
        //*pPointArray = new HdPointXYZ[ptNum];   // 点数据
        //*pIntensityArray = new HdIntensity[ptNum];// 强度数据
        //pColorArray = new HdPtColor[ptNum];  // 颜色数据
        //pClassArray = new HdPtClass[ptNum];  // 分类数据

        int sizeCoordRead = ptNum * sizeof(HdPointXYZ);
        int sizeIntensityRead = ptNum * sizeof(HdIntensity);
        int sizeColorRead = ptNum * sizeof(HdPtColor);
        int sizeClassRead = ptNum * sizeof(HdPtClass);

        if (!m_hlzHeader.isCompress)
        {
            // 数据没有压缩
            // 不使用内存映射

            if (!m_bUseMemMap)
            {
                // 读取坐标
                li.QuadPart = addrCoord;
                SetFilePointer(m_pDataFile, li.LowPart, &li.HighPart, FILE_BEGIN);
                bRetCoord = ReadFile(m_pDataFile, *pPointArray, sizeCoordRead, &numRead, NULL);
                bRetCoord = (numRead == sizeCoordRead);
                // 读取强度
                if(bRetIntensity)
                {
                    li.QuadPart = addrInt;
                    SetFilePointer(m_pDataFile, li.LowPart, &li.HighPart, FILE_BEGIN);
                    bRetIntensity = ReadFile(m_pDataFile, *pIntensityArray, sizeIntensityRead, &numRead, NULL);
                    bRetIntensity = (numRead == sizeIntensityRead);
                }

                // 读取颜色
                if (bRetColor)
                {
                    li.QuadPart = addrColor;
                    SetFilePointer(m_pDataFile, li.LowPart, &li.HighPart, FILE_BEGIN);
                    bRetColor = ReadFile(m_pDataFile, *pColorArray, sizeColorRead, &numRead, NULL);
                    bRetColor = (numRead == sizeColorRead);
                }

                // 读取分类信息
                if (bRetClass)
                {
                    li.QuadPart = addrClass;
                    SetFilePointer(m_pDataFile, li.LowPart, &li.HighPart, FILE_BEGIN);
                    bRetClass = ReadFile(m_pDataFile, *pClassArray, sizeClassRead, &numRead, NULL);
                    bRetClass = (numRead == sizeClassRead);
                }
            }
            else if (m_mapAddress)
            {
                // 读取坐标
                errno_t err = memcpy_s(*pPointArray, sizeCoordRead, m_mapAddress + addrCoord, sizeCoordRead);
                // 读取强度
                if(bRetIntensity)
                {
                    err = memcpy_s(*pIntensityArray, sizeIntensityRead, m_mapAddress + addrInt, sizeIntensityRead);
                }

                // 读取颜色
                if (bRetColor)
                {
                    err = memcpy_s(*pColorArray, sizeColorRead, m_mapAddress + addrColor, sizeColorRead);
                }

                // 读取分类
                if (bRetClass)
                {
                    err = memcpy_s(*pClassArray, sizeClassRead, m_mapAddress + addrClass, sizeClassRead);
                }
            }
        }
        else
        {
            // 不使用内存映射
            if (!m_bUseMemMap)
            {
                // 读取坐标
                li.QuadPart = addrCoord;
                SetFilePointer(m_pDataFile, li.LowPart, &li.HighPart, FILE_BEGIN);
                //再根据数据大小来读取坐标数据
                ptBuf = new U8[ptBufSize];
                bRetCoord = ReadFile(m_pDataFile, ptBuf, ptBufSize, &numRead, NULL);
				bRetCoord = (numRead == ptBufSize);

				//解压到目标buffer中
				CPtXYZEncoder ptDecode(ptBuf, ptBufSize, ptNum);
				ptDecode.decodePoints(*pPointArray);

				delete[] ptBuf;
				ptBuf = NULL;
               // bRetCoord = ReadFile(m_pDataFile, *pPointArray, sizeCoordRead, &numRead, NULL);

                //解压
                //CPtXYZEncoder ptDecode(ptBuf, ptBufSize, ptNum);
                //CPtXYZEncoder ptDecode(pPointArray, sizeCoordRead, ptNum);
                //ptDecode.decodePoints(pPointArray);

                // delete ptBuf;
                // ptBuf = NULL;
                //bRetCoord = (numRead == sizeCoordRead);

                // 读取强度
                if(bRetIntensity)
                {
                    li.QuadPart = addrInt;
                    SetFilePointer(m_pDataFile, li.LowPart, &li.HighPart, FILE_BEGIN);
					
					if(!m_bNewHlz)
					{
						bRetIntensity = ReadFile(m_pDataFile, *pIntensityArray, sizeIntensityRead, &numRead, NULL);
						bRetIntensity = (numRead == sizeIntensityRead);
					}
					else
					{
						ptIntenBuf = new U8[ptIntenSize];
						bRetIntensity = ReadFile(m_pDataFile, ptIntenBuf, ptIntenSize, &numRead, NULL);
						bRetIntensity = (numRead == ptIntenSize);

						CPtIEncoder intenDecoder(ptIntenBuf, ptIntenSize, ptNum);
						intenDecoder.decodeInten(*pIntensityArray);

						delete[] ptIntenBuf;
						ptIntenBuf = NULL;
					}
                }

                // 读取颜色
                if (bRetColor)
                {
                    li.QuadPart = addrColor;
                    SetFilePointer(m_pDataFile, li.LowPart, &li.HighPart, FILE_BEGIN);
                    bRetColor = ReadFile(m_pDataFile, *pColorArray, sizeColorRead, &numRead, NULL);
                    bRetColor = (numRead == sizeColorRead);
                }

                // 读取分类
                if (bRetClass)
                {
                    li.QuadPart = addrClass;
                    SetFilePointer(m_pDataFile, li.LowPart, &li.HighPart, FILE_BEGIN);
                    bRetClass = ReadFile(m_pDataFile, *pClassArray, sizeClassRead, &numRead, NULL);
                    bRetClass = (numRead == sizeClassRead);
                }
            }
            else if (m_mapAddress)
            {
                // 读取坐标
                ptBuf = new U8[ptBufSize];
                errno_t err = memcpy_s(ptBuf, ptBufSize, m_mapAddress + addrCoord /*+ sizeof(int)*/, ptBufSize);

                //解压
                CPtXYZEncoder ptDecode(ptBuf, ptBufSize, ptNum);
                ptDecode.decodePoints(*pPointArray);

                delete[] ptBuf;
                ptBuf = NULL;

                // 读取强度
                if(bRetIntensity)
                {
					if(!m_bNewHlz)
					{
						err = memcpy_s(*pIntensityArray, sizeIntensityRead,
							m_mapAddress + addrInt, sizeIntensityRead);
					}
					else
					{
						ptIntenBuf = new U8[ptIntenSize];
						memcpy(ptIntenBuf, m_mapAddress + addrInt, ptIntenSize);

						CPtIEncoder intenDecoder(ptIntenBuf, ptIntenSize, ptNum);
						intenDecoder.decodeInten(*pIntensityArray);

						delete[] ptIntenBuf;
						ptIntenBuf = NULL;
					}
                    
                }

                // 读取颜色
                if (bRetColor)
                {
                    err = memcpy_s(*pColorArray, sizeColorRead,
                        m_mapAddress + addrColor, sizeColorRead);
                }

                // 读取分类
                if (bRetClass)
                {
                    err = memcpy_s(*pClassArray, sizeClassRead,
                        m_mapAddress + addrClass, sizeClassRead);
                }
            }
        }

        // 将坐标强度转换为显示坐标
        // HdRefPoint refPt;
        // refPt.FromBox(noInfo->GetExtent());
        // BOOL bRet = PtBlock2PtArray(*pPointArray, noInfo->GetPtCount(), &refPt, &noInfo->m_pHlzPoint, (U8*)pIntensityArray);
		LeaveCriticalSection(&m_cs);
        return bRetCoord;
    }

	BOOL CHdCoreData::UnLoadSpecParcel(CHdParcelBase* noInfo)
	{
		EnterCriticalSection(&m_cs);
		if (noInfo)
		{
			noInfo->Release();
			LeaveCriticalSection(&m_cs);
			return  TRUE;
		}

		LeaveCriticalSection(&m_cs);
		return FALSE;
	}


	//void CHdCoreData::GetCoordinate( double& x,double& y,double& z )
	//{
	//	// 将本地坐标转换为全局坐标
	//	double tmpX = x;
	//	double tmpY = y;
	//	double tmpZ = z;
	//	x = m_matrix[0]*tmpX + m_matrix[1]*tmpY + m_matrix[2]*tmpZ + m_matrix[3];
	//	y = m_matrix[4]*tmpX + m_matrix[5]*tmpY + m_matrix[6]*tmpZ + m_matrix[7];
	//	z = m_matrix[8]*tmpX + m_matrix[9]*tmpY + m_matrix[10]*tmpZ + m_matrix[11];
	//	double w = m_matrix[12]*tmpX + m_matrix[13]*tmpY + m_matrix[14]*tmpZ + m_matrix[15];
	//
	//	double f = 1.0/w;
	//	x = static_cast<double>(x*f);
	//	y = static_cast<double>(y*f);
	//	z = static_cast<double>(z*f);
	//}

	// 读取一个点
	BOOL CHdCoreData::Read_Point(PointXYZIPRGBA& pt,U64 index) 
	{
		return TRUE;
	}

	// 读取一圈,将数据拷贝到目标数组
	BOOL CHdCoreData::ReadLoop(
		hdVector<PointXYZIPRGBA>& ptBuf,		// 外部传入的数组,外部管理指针	
		I32 loop,								// 圈号
		U32 simple) 				// 抽稀加载间隔	
	{
		return TRUE;
	}

	// 读取一圈,将数据拷贝到目标数组,包含无效点
	BOOL CHdCoreData::ReadLoopFull(
		hdVector<PointXYZIPRGBA>& ptBuf,		// 外部传入的数组,外部管理指针	
		I32 loop,								// 圈号
		U32 simple )					// 抽稀加载间隔
	{

		return TRUE;
	}

	// 更新一圈数据,到数据文件
	BOOL CHdCoreData::UpdateLoop(
		const hdVector<PointXYZIPRGBA>& ptBuf,	// 需要更新的列
		I32 loop)
	{
		return TRUE;
	}

	// 写入数据
	//支持同步写入颜色数据信息   袁亮  20160625
	BOOL CHdCoreData::WriteData( const HdPointXYZ* ptBuf, /* 待写入的坐标数据 */ const U8* intenBuf, /* 待写入的强度数据 */ const HdPtColor* ptColorBuf, U64 count, /* 点数 */ U64& addrCoord, /* ?氐淖甑刂 */ U64& addrIntensity, U64& addrColor)
	{
		EnterCriticalSection(&m_cs);
		if (ptBuf == NULL || intenBuf == NULL || count <= 0 || m_pDataFile == INVALID_HANDLE_VALUE)
		{
			LeaveCriticalSection(&m_cs);
			return FALSE;
		}

		LARGE_INTEGER li;
		li.HighPart = 0;
		li.LowPart = 0;

		LARGE_INTEGER li1;
		li1.HighPart = 0;
		li1.LowPart = 0;

		SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);
		U64 curFileAddr = (U64)m_curFileIndex * (U64)0x80000000;
		addrCoord = curFileAddr + (U64)li1.QuadPart;

		//不论文件系统，hlz数据文件都按照2GB大小进行划分    袁亮  20160818
		/*if(m_fileFlag == 1)
		{*/
		U64 curDataAddr = addrCoord + sizeof(HdPointXYZ) * count + sizeof(U8) * count;
		if(curDataAddr > curFileAddr + 0x80000000)//0x80000000 2GB
		{
			// 关闭上次文件
			if (m_pDataFile!=INVALID_HANDLE_VALUE)
			{
				// 关闭数据文件
				CloseHandle(m_pDataFile);
				m_pDataFile = NULL;
			}

			// 打开数据文件dir+name-dxxxx.hls			
			m_curFileIndex++;
			curFileAddr = (U64)m_curFileIndex * (U64)0x80000000;

			char strDataFile[256] = {0};
			sprintf(strDataFile,"%s\\%s-%04d%s",m_dir,m_name,m_curFileIndex,m_datExt);
			m_pDataFile = CreateFile(strDataFile, 
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ|FILE_SHARE_WRITE, 
				NULL,
				CREATE_ALWAYS, 
				FILE_ATTRIBUTE_NORMAL, 
				NULL);

			if (m_pDataFile == NULL)
			{
				LeaveCriticalSection(&m_cs);
				return FALSE;
			}

			SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);
			addrCoord = curFileAddr + (U64)li1.QuadPart;
		}
		//}
		// 写入坐标
		DWORD dwResult;
		if(!WriteFile (m_pDataFile, ptBuf, (DWORD)count * sizeof(HdPointXYZ), &dwResult, NULL))
		{
			LeaveCriticalSection(&m_cs);
			return FALSE;
		}

		// 强度的首地址	
		SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);
		addrIntensity = curFileAddr + (U64)li1.QuadPart;

		// 写入强度
		if(!WriteFile (m_pDataFile, intenBuf, (DWORD)count, &dwResult, NULL))
		{
			LeaveCriticalSection(&m_cs);
			return FALSE;
		}

		if( ptColorBuf != NULL)
		{
			//颜色的首地址及颜色数据       袁亮   20160625
			SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);
			addrColor = curFileAddr + (U64)li1.QuadPart;

			if(!WriteFile(m_pDataFile, ptColorBuf, (DWORD)count * sizeof(HdPtColor), &dwResult, NULL))
				return FALSE;
		}

		LeaveCriticalSection(&m_cs);
		return TRUE;
	}

	BOOL CHdCoreData::UpdateIndex()
	{
		return !m_bNewHlz ? UpdateOldIndex() : UpdateNewIndex();
	}

	// 更新hlz3.0索引文件信息
	BOOL CHdCoreData::UpdateOldIndex()
	{
//#pragma  pack(push,1)
		// 打开索引文件
		EnterCriticalSection(&m_cs);
		FILE* pIndexFile = fopen(m_strIndexFilePath.data(),"wb");
		if (pIndexFile == NULL)
		{
			LeaveCriticalSection(&m_cs);
			return FALSE;
		}

		// 先抹去原来内容
		fseek(pIndexFile, 0, SEEK_END);
		long pos = ftell(pIndexFile);
		long size = pos - 256;
		if (size > 0)
		{
			char* cTmp = new char[size];
			memset(cTmp,0,size);
			fseek(pIndexFile, 256, SEEK_SET);
			fwrite(cTmp,1,size,pIndexFile);
			delete[] cTmp;
			cTmp = NULL;
		}

		// 首先写入文件头,256字节
		fseek(pIndexFile,0,SEEK_SET);

		// 写入文件头
		size_t headerSize = sizeof(HLZheader);
		char hdBuf[256] = {0};
		m_hlzHeader.serialize(hdBuf);

		fwrite(hdBuf,sizeof(char),256,pIndexFile);

		// 定位到索引起始位置
		fseek(pIndexFile, 256, SEEK_SET);
		//int lvlCount = m_header.number_of_level;
		int lvlCount = m_lstLevel.size();
		CHdLevel* pLevel = NULL;
		U16 i = 0;
		for (i = 0;i < lvlCount;i++)
		{
			pLevel = m_lstLevel[i];
			fwrite(&(pLevel->m_level),sizeof(HdLevel),1,pIndexFile);

			CHdBlockset* pBlockSet = NULL;
			map<I32,CHdBlockset*>::const_iterator itLevel; 
			for (itLevel = pLevel->m_pListBlockset.begin();itLevel != pLevel->m_pListBlockset.end();itLevel++)
			{
				pBlockSet = itLevel->second;
				//if (!m_header.isCompress)
					fwrite(&pBlockSet->m_blockSet,sizeof(HdBlockset) - sizeof(U32),1,pIndexFile);
				//else
				//	fwrite(&pBlockSet->m_blockSet,sizeof(HdBlockset),1,m_pIndexFile);

				// 空的块集记录
				if (pBlockSet->m_blockSet.hasSubBlock == 0)
				{
					continue;
				}

				CHdBlock* pBlock = NULL;
				map<I32,CHdBlock*>::const_iterator itBK;
				for (itBK = pBlockSet->m_pListBlock.begin();itBK != pBlockSet->m_pListBlock.end();itBK++)
				{
					pBlock = itBK->second;
					//if (!m_header.isCompress)
						fwrite(&pBlock->m_block,sizeof(HdBlock) - sizeof(U32),1,pIndexFile);
					//else
						//fwrite(&pBlock->m_block,sizeof(HdBlock),1,m_pIndexFile);

					// 添加块内部的包				
					if (pBlock->m_block.hasSubParcel == 0)
					{
						continue;
					}
					CHdParcel* pParcel = NULL;
					map<I32,CHdParcel*>::const_iterator itPcl;
					for (itPcl = pBlock->m_pListParcel.begin();itPcl != pBlock->m_pListParcel.end();itPcl++)
					{
						pParcel = itPcl->second;
						//if(!m_header.isCompress)
							fwrite(&pParcel->m_parcel,sizeof(HdParcel) - sizeof(U32),1,pIndexFile);	
						//else
							//fwrite(&pParcel->m_parcel,sizeof(HdParcel),1,m_pIndexFile);	
					}				
				}//for (iBk = 0;iBk < pBlockSet->m_blockSet.m_numBlock;iBk++)
			}//for (iBs = 0;iBs < bsCount;iBs++)
		}//for (i = 0;i < lvlCount;i++)

		fclose(pIndexFile);
		LeaveCriticalSection(&m_cs);
//#pragma  pack(pop)
		return TRUE;
	}

	// 更新hlz3.2索引文件信息
	BOOL CHdCoreData::UpdateNewIndex()
	{
		// 打开索引文件
		FILE* index_file = fopen(m_strIndexFilePath.c_str(), "wb");
		if (index_file == NULL)
		{
			return FALSE;
		}

		// 先抹去原来内容
		fseek(index_file, 0, SEEK_END);
		long pos = ftell(index_file);
		long size = pos - 256;
		if (size > 0)
		{
			char* cTmp = new char[size];
			memset(cTmp,0,size);
			fseek(index_file, 256, SEEK_SET);
			fwrite(cTmp,1,size,index_file);
			delete[] cTmp;
			cTmp = NULL;
		}

		// 首先写入文件头,256字节
		fseek(index_file,0,SEEK_SET);

		// 写入文件头
		char buffer[256] = {0};
		m_hlzHeader.serialize(buffer);

		fwrite(buffer,sizeof(char),256,index_file);

		// 定位到索引起始位置
		fseek(index_file, 256, SEEK_SET);

		//层数
		int level_count = m_lstLevel.size();
		CHdLevel31* level = NULL;
		u16 i = 0;

		//遍历层记录
		for (i = 0; i < level_count; i++)
		{
			level = static_cast<CHdLevel31*>(m_lstLevel[i]);

			fwrite(&(level->m_level31), sizeof(HdLevel31), 1, index_file);

			CHdBlockset31* blockset = NULL;
			for (auto it_blockset = level->m_pListBlockset.begin(); it_blockset !=  level->m_pListBlockset.end(); it_blockset++)
			{
				blockset = static_cast<CHdBlockset31*>(it_blockset->second);
				HdBlockset31& blockset_info = blockset->m_blockSet31;

				// 空的块集记录
				//blockNum = 0说明块集没有分块，这时块集索引中含有数据地址信息
				if (blockset_info.getBlockNum() == 0)
				{
					//1.先写块集索引中定长部分
					u16 fix_len = sizeof(HdBlockset31) - 3 * sizeof(HdAddr);
					fwrite(&blockset_info, fix_len, 1, index_file);

					//2.判断是否包含颜色、时间数据，决定写入块集索引的长度
					//2.a 包含颜色时间数据
					if(blockset_info.hasColor() && blockset_info.hasTime())
					{
						fwrite(&blockset_info.addrColor, 2 * sizeof(HdAddr), 1, index_file);
					}
					//2.b 仅包含颜色数据
					else if(blockset_info.hasColor())
					{
						fwrite(&blockset_info.addrColor, sizeof(HdAddr), 1, index_file);
					}
					//2.c 仅包含时间数据
					else if(blockset_info.hasTime())
					{
						fwrite(&blockset_info.addrTime, sizeof(HdAddr), 1, index_file);
					}
					//2.d 不包含颜色、时间数据
					else
					{
					}

					//3.判断数据是否压缩
					if(blockset_info.isCompress())
					{
						fwrite(&blockset_info.attriCmpLen, sizeof(HdAddr), 1, index_file);
					}

					//当前块集不包含块，直接处理下一个块集
					continue;
				}
				else
				{
					//blockNum > 0说明块集有分块，这时块集索引中不包含数据地址信息及压缩长度信息
					fwrite(&blockset_info, sizeof(HdBlockset31) - 4 * sizeof(HdAddr), 1, index_file);
				}

				CHdBlock31* block = NULL;
				for (auto it_block = blockset->m_pListBlock.begin(); it_block != blockset->m_pListBlock.end(); it_block++)
				{
					block = static_cast<CHdBlock31*>(it_block->second);
					HdBlock31 & block_info = block->m_block31;

					// 添加块内部的包	
					//parcelNum = 0说明块没有分包，这时块索引中含有数据地址信息
					if(block_info.numParcel == 0)
					{
						//1.先写入定长索引部分
						fwrite(&block_info, sizeof(HdBlock31) - 3 * sizeof(HdAddr), 1, index_file);

						//2.判断是否包含颜色、时间数据，决定写入块索引的长度
						//2.a 包含颜色时间数据
						if(block_info.hasColor() && block_info.hasTime())
						{
							fwrite(&block_info.addrColor, 2 * sizeof(HdAddr), 1, index_file);
						}
						//2.b 仅包含颜色数据
						else if(block_info.hasColor())
						{
							fwrite(&block_info.addrColor, sizeof(HdAddr), 1, index_file);
						}
						//2.c 仅包含时间数据
						else if(block_info.hasTime())
						{
							fwrite(&block_info.addrTime, sizeof(HdAddr), 1, index_file);
						}
						//2.d 不包含颜色、时间数据
						else
						{
						}

						//3.判断数据是否压缩
						if(block_info.isCompress())
						{
							fwrite(&block_info.attriCmpLen, sizeof(HdAddr), 1, index_file);
						}

						//当前块不包含包，直接处理下一个块
						continue;
					}
					else
					{
						//parcelNum > 0说明块有分包，这时块索引中不包含数据地址信息及压缩长度信息
						fwrite(&block_info, sizeof(HdBlock31) - 4 * sizeof(HdAddr), 1, index_file);
					}

					CHdParcel31* parcel = NULL;
					for (auto it_parcel = block->m_pListParcel.begin(); it_parcel != block->m_pListParcel.end(); it_parcel++)
					{
						parcel = static_cast<CHdParcel31*>(it_parcel->second);
						HdParcel31& parcel_info = parcel->m_parcel31;

						//1.先写包索引中定长部分
						u16 fix_len = sizeof(HdParcel31) - 3* sizeof(HdAddr);
						fwrite(&parcel_info, fix_len, 1, index_file);

						//2.判断是否包含颜色、时间数据，决定写入包索引的长度
						//2.a 包含颜色时间数据
						if(parcel_info.hasColor() && parcel_info.hasTime())
						{
							fwrite(&parcel_info.addrColor, 2 * sizeof(HdAddr), 1, index_file);
						}
						//2.b 仅包含颜色数据
						else if(parcel_info.hasColor())
						{
							fwrite(&parcel_info.addrColor, sizeof(HdAddr), 1, index_file);
						}
						//2.c 仅包含时间数据
						else if(parcel_info.hasTime())
						{
							fwrite(&parcel_info.addrTime, sizeof(HdAddr), 1, index_file);
						}
						//2.d 不包含颜色、时间数据
						else
						{
						}

						//3.判断数据是否压缩
						if(parcel_info.isCompress())
						{
							fwrite(&parcel_info.attriCmpLen, sizeof(HdAddr), 1, index_file);
						}

					}//for (it_parcel = block->m_pListParcel.begin(); it_parcel != block->m_pListParcel.end(); it_parcel++)

				}//for (it_block = blockset->m_list_block.begin(); it_block != blockset->m_list_block.end(); it_block++)

			}//for (it_blockset = level->m_pListBlockset.begin(); it_blockset !=  level->m_pListBlockset.end(); it_blockset++)

		}//for (i = 0; i < level_count; i++)

		fclose(index_file);

		return TRUE;
	}

	// 获取指定层的Parcel范围列表
	int CHdCoreData::GetLevelAllAreas( CHdLevel* pLevelRec, CHdListAreaNoInf* pLstAreaNoInf )
	{
		if(pLevelRec == 0)
		{
			return 0;
		}
		EnterCriticalSection(&m_cs);
		// 遍历处理
		map<I32,CHdBlockset*>::iterator it;
		for (it = pLevelRec->m_pListBlockset.begin();it != pLevelRec->m_pListBlockset.end();it++)
		{
			CHdBlockset* pHdBlockset = it->second;

			if (!pHdBlockset->m_blockSet.hasSubBlock) // 不含子块，push
			{
				pLstAreaNoInf->push_back(pHdBlockset);
				continue;
			}
			else // 包括子块，空块集也要返回
			{
				pLstAreaNoInf->push_back(pHdBlockset);
			}

			// 遍历子块集
			map<I32,CHdBlock*>::iterator itBlk;
			for (itBlk = pHdBlockset->m_pListBlock.begin();itBlk != pHdBlockset->m_pListBlock.end();itBlk++)
			{
				CHdBlock* pBlock = itBlk->second;

				if (!pBlock->m_block.hasSubParcel)
				{
					pLstAreaNoInf->push_back(pBlock);
					continue;
				}
				else // 包括子包，空块也要返回
				{
					pLstAreaNoInf->push_back(pBlock);
				}

				// 遍历子块
				map<I32,CHdParcel*>::iterator itPcl;
				for (itPcl = pBlock->m_pListParcel.begin();itPcl != pBlock->m_pListParcel.end();itPcl++)
				{
					CHdParcel* pParcel = itPcl->second;

					pLstAreaNoInf->push_back(pParcel);				
				}
			}
		}
		LeaveCriticalSection(&m_cs);
		return pLstAreaNoInf->size();
	}

	void CHdCoreData::WriteHeader()
	{
		// 打开索引文件
		EnterCriticalSection(&m_cs);
		FILE* pIndexFile = fopen(m_strIndexFilePath.data(),"wb+");
		if (pIndexFile == NULL)
		{
			LeaveCriticalSection(&m_cs);
			return;
		}

		fseek(pIndexFile,0,SEEK_SET);

		// 写入文件头
		size_t headerSize = sizeof(HLZheader);
		char hdBuf[256] = {0};
		m_header.serialize(hdBuf);

		fwrite(hdBuf,sizeof(char),256,pIndexFile);
		fclose(pIndexFile);
		LeaveCriticalSection(&m_cs);
	}

	// 获得块集内的所有子包
	int CHdCoreData::GetBsAreas( CHdBlockset* poBlockSet,CHdListAreaNoInf* pListAreaNoinf )
	{
		if (!poBlockSet->m_blockSet.hasSubBlock)
		{
			//LoadSpecParcel(poBlockSet);
			pListAreaNoinf->push_back(poBlockSet);		
			return TRUE;
		}
		// 遍历子块集
		map<I32,CHdBlock*>::iterator itBlk;
		for (itBlk = poBlockSet->m_pListBlock.begin();itBlk != poBlockSet->m_pListBlock.end();itBlk++)
		{
			CHdBlock* pBlock = itBlk->second;

			if (!pBlock->m_block.hasSubParcel)
			{
				//LoadSpecParcel(pBlock);
				pListAreaNoinf->push_back(pBlock);
				continue;
			}

			// 遍历子块
			map<I32,CHdParcel*>::iterator itPcl;
			for (itPcl = pBlock->m_pListParcel.begin();itPcl != pBlock->m_pListParcel.end();itPcl++)
			{
				CHdParcel* pParcel = itPcl->second;
				//LoadSpecParcel(pParcel);			
				pListAreaNoinf->push_back(pParcel);
			}
		}
		poBlockSet->Update();
		return TRUE;
	}

	
	// 物理文件数据块到内存点云转换
	// 新增可选输入参数，颜色信息   袁亮   20160625
	int PtBlock2PtArray(
		const HdPointXYZ* pPtBlock,		// 输入,物理块数据
		int count,						// 输入,点个数
		const HdRefPoint* pRefPt,		// 输入,块左下角参考点
		HlzPoint** ppOut,				// 输出，解析后坐标
		U8* pIntensity,				    // 输入,可选
		const HdPtColor* pPtColor)	    // 可选输入，颜色信息   袁亮   20160625
	{
		if (pPtBlock == NULL || count <= 0)
		{
			return 0;
		}

		HlzPoint* pTmpPts = NULL;
		pTmpPts = new HlzPoint[count];
		(*ppOut) = pTmpPts;
		int i;
		if (pIntensity)
		{
			for (i = 0;i < count;i++)
			{
				pTmpPts[i].x = pRefPt->x + pRefPt->rx * pPtBlock[i].x;
				pTmpPts[i].y = pRefPt->y + pRefPt->ry * pPtBlock[i].y;
				pTmpPts[i].z = pRefPt->z + pRefPt->rz * pPtBlock[i].z;
				pTmpPts[i].intensity = pIntensity[i];
			}
		}
		else
		{
			for (i = 0;i < count;i++)
			{
				pTmpPts[i].x = pRefPt->x + pRefPt->rx * pPtBlock[i].x;
				pTmpPts[i].y = pRefPt->y + pRefPt->ry * pPtBlock[i].y;
				pTmpPts[i].z = pRefPt->z + pRefPt->rz * pPtBlock[i].z;
			}
		}
		//颜色信息保存到内存点云中  袁亮  20160625
		if(pPtColor != NULL)
		{
			for(i = 0; i < count; i++)
			{
				pTmpPts[i].color = pPtColor[i];
			}
		}
		return 1;
	}

	// 物理文件数据块到内存点云转换
	// 新增可选输入参数，颜色信息   袁亮   20160625
	HLS_API int PtBlock2PtArray( const HdPointXYZ* pPtBlock, /* 输入,物理块数据 */ int count, /* 输入,点个数 */ const HdRefPoint* pRefPt, /* 输入,块左下角参考点 */ HlzPoint** ppOut, /* 输出，解析后坐标 */ F32 offSetX, F32 offSetY, U8* pIntensity /*= NULL*/, const HdPtColor* pPtColor)
	{
		if (pPtBlock == NULL || count <= 0)
		{
			return 0;
		}

		HlzPoint* pTmpPts = NULL;
		pTmpPts = new HlzPoint[count];
		(*ppOut) = pTmpPts;
		int i;
		if (pIntensity)
		{
			for (i = 0;i < count;i++)
			{
				pTmpPts[i].x = pRefPt->x + pRefPt->rx * pPtBlock[i].x - 0/*offSetX*/;
				pTmpPts[i].y = pRefPt->y + pRefPt->ry * pPtBlock[i].y - 0/*offSetY*/;
				pTmpPts[i].z = pRefPt->z + pRefPt->rz * pPtBlock[i].z;
				pTmpPts[i].intensity = pIntensity[i];
			}
		}
		else
		{
			for (i = 0;i < count;i++)
			{
				pTmpPts[i].x = pRefPt->x + pRefPt->rx * pPtBlock[i].x - 0/*offSetX*/;
				pTmpPts[i].y = pRefPt->y + pRefPt->ry * pPtBlock[i].y - 0/*offSetY*/;
				pTmpPts[i].z = pRefPt->z + pRefPt->rz * pPtBlock[i].z;
			}
		}
		//颜色信息保存到内存点云中  袁亮  20160625
		if(pPtColor != NULL)
		{
			for(i = 0; i < count; i++)
			{
				pTmpPts[i].color = pPtColor[i];
			}
		}
		return 1;
	}

	HLS_API int PtBlock2PtArrayWithHeader(const HdPointXYZ* point_block, int count, const HdRefPointd* ref_point,HlzPoint** point_out, u8* intensity , const HdPtColor* point_color , f64 header_offset_x ,f64 header_offset_y ,f64 header_offset_z)
	{
		if (point_block == NULL || count <= 0)
		{
			return 0;
		}

		HlzPoint* temp_point_buffer = NULL;
		temp_point_buffer = new HlzPoint[count];
		(*point_out) = temp_point_buffer;

		int i;
		if (intensity)
		{
			for (i = 0;i < count;i++)
			{
				temp_point_buffer[i].x = (f32)(ref_point->x + ref_point-> rx* point_block[i].x - header_offset_x);
				temp_point_buffer[i].y = (f32)(ref_point->y + ref_point->ry * point_block[i].y - header_offset_y);
				temp_point_buffer[i].z = (f32)(ref_point->z + ref_point->rz * point_block[i].z - header_offset_z);
				temp_point_buffer[i].intensity = intensity[i];
			}
		}
		else
		{
			for (i = 0;i < count;i++)
			{
				temp_point_buffer[i].x = (f32)(ref_point->x + ref_point->rx * point_block[i].x - header_offset_x);
				temp_point_buffer[i].y = (f32)(ref_point->y + ref_point->ry * point_block[i].y - header_offset_y);
				temp_point_buffer[i].z = (f32)(ref_point->z + ref_point->rz * point_block[i].z - header_offset_z);
			}
		}

		//颜色信息保存到内存点云中
		if(point_color != NULL)
		{
			for(i = 0; i < count; i++)
			{
				temp_point_buffer[i].color = point_color[i];
			}
		}
		return 1;
	}

	// 内存点云到物理文件数据块-xyg
	int PtArray2PtBlock(
		const PointXYZIPRGBA* pPtAry,	// 输入,点数组
		int count,						// 输入,点数
		const HdRefPoint* pRefPt,		// 输入,块左下角参考点
		HdPointXYZ** pPtBlock,			// 输出,物理块数据坐标,外部使用完毕后要释放
		U8**		 pPIntensity,		// 输出,物理块数据强度,外部使用完毕后要释放
		HdPtColor**  pPtColor,          // 输出，物理块数据颜色，格式为R5G6B5，外部使用完毕后要释放   袁亮   20160625
		F64 offsetX,
		F64 offsetY,
		U16 minIntensity,				// 最小反射强度
		U16 maxIntensity)				// 最大反射强度
	{
		if (pPtAry == NULL || count <= 0)
		{
			return 0;
		}

		bool bHasIntentisy = (minIntensity >= 0 && maxIntensity > 0 && maxIntensity >= minIntensity);
		U16 intRange = maxIntensity - minIntensity;
		F32 fScaleIntentisy = 255.0f/intRange;

		HdPointXYZ* pTmpPts = NULL;
		pTmpPts = new HdPointXYZ[count];
		(*pPtBlock) = pTmpPts;
		//为颜色分配存储空间  袁亮   20160625
		HdPtColor* pTmpColors = NULL;
		pTmpColors = new HdPtColor[count];
		(*pPtColor) = pTmpColors;
		U8* pTmpIntensity = NULL;
		if (bHasIntentisy)
		{
			pTmpIntensity = new U8[count];
			(*pPIntensity) = pTmpIntensity;
		}

		int i,index = 0;
		int x,y,z;
		if (bHasIntentisy)
		{
			for (i = 0;i < count;i++)
			{
				x = hd_round32((pPtAry[i].x + offsetX - pRefPt->x)*pRefPt->rxRcp);
				y = hd_round32((pPtAry[i].y + offsetY - pRefPt->y)*pRefPt->ryRcp);
				z = hd_round32((pPtAry[i].z - pRefPt->z)*pRefPt->rzRcp);
				if(x < 0 || y < 0 || z < 0 || x > 65535 || y > 65535 || z > 65535)
				{					
					continue;
				}

				pTmpPts[index].x = x;
				pTmpPts[index].y = y;
				pTmpPts[index].z = z;
				pTmpIntensity[index] = hd_round32((pPtAry[i].getIntensity() - minIntensity) * fScaleIntentisy);
				pTmpColors[index].setColor(pPtAry[i].r, pPtAry[i].g, pPtAry[i].b); //写入颜色数据  袁亮   20160625
				index++;
			}
		}
		else
		{
			for (i = 0;i < count;i++)
			{
				pTmpPts[i].x = hd_round32((pPtAry[i].x + offsetX - pRefPt->x)*pRefPt->rxRcp);
				pTmpPts[i].y = hd_round32((pPtAry[i].y + offsetY - pRefPt->y)*pRefPt->ryRcp);
				pTmpPts[i].z = hd_round32((pPtAry[i].z - pRefPt->z)*pRefPt->rzRcp);
				pTmpColors[index].setColor(pPtAry[i].r, pPtAry[i].g, pPtAry[i].b); //写入颜色数据  袁亮   20160625
				index ++;
			}
		}		

		return index;
	}

	// 内存点云到物理文件数据块
	int PtArray2PtBlock(
		const PointXYZIPRGBA* pPtAry,	// 输入,点数组
		int count,						// 输入,点数
		const HdRefPoint* pRefPt,		// 输入,块左下角参考点
		HdPointXYZ** pPtBlock,			// 输出,物理块数据坐标,外部使用完毕后要释放
		U8**		 pPIntensity,		// 输出,物理块数据强度,外部使用完毕后要释放
		HdPtColor**  pPtColor,          // 输出，物理块数据颜色，格式为R5G6B5，外部使用完毕后要释放   袁亮   20160625
		U16 minIntensity,				// 最小反射强度
		U16 maxIntensity)				// 最大反射强度
	{
		if (pPtAry == NULL || count <= 0)
		{
			return 0;
		}

		bool bHasIntentisy = (minIntensity >= 0 && maxIntensity > 0 && maxIntensity >= minIntensity);
		U16 intRange = maxIntensity - minIntensity;
		F32 fScaleIntentisy = 255.0f/intRange;

		HdPointXYZ* pTmpPts = NULL;
		pTmpPts = new HdPointXYZ[count];
		(*pPtBlock) = pTmpPts;
		//为颜色分配存储空间  袁亮   20160625
		HdPtColor* pTmpColors = NULL;
		pTmpColors = new HdPtColor[count];
		(*pPtColor) = pTmpColors;
		U8* pTmpIntensity = NULL;
		if (bHasIntentisy)
		{
			pTmpIntensity = new U8[count];
			(*pPIntensity) = pTmpIntensity;
		}

		int i,index = 0;
		int x,y,z;
		if (bHasIntentisy)
		{
			for (i = 0;i < count;i++)
			{
				x = hd_round32((pPtAry[i].x - pRefPt->x)*pRefPt->rxRcp);
				y = hd_round32((pPtAry[i].y - pRefPt->y)*pRefPt->ryRcp);
				z = hd_round32((pPtAry[i].z - pRefPt->z)*pRefPt->rzRcp);
				if(x < 0 || y < 0 || z < 0 || x > 65535 || y > 65535 || z > 65535)
				{					
					continue;
				}

				pTmpPts[index].x = x;
				pTmpPts[index].y = y;
				pTmpPts[index].z = z;
				pTmpIntensity[index] = hd_round32((pPtAry[i].getIntensity() - minIntensity) * fScaleIntentisy);
				pTmpColors[index].setColor(pPtAry[i].r, pPtAry[i].g, pPtAry[i].b); //写入颜色数据  袁亮   20160625
				index++;
			}
		}
		else
		{
			for (i = 0;i < count;i++)
			{
				pTmpPts[i].x = hd_round32((pPtAry[i].x - pRefPt->x)*pRefPt->rxRcp);
				pTmpPts[i].y = hd_round32((pPtAry[i].y - pRefPt->y)*pRefPt->ryRcp);
				pTmpPts[i].z = hd_round32((pPtAry[i].z - pRefPt->z)*pRefPt->rzRcp);
				pTmpColors[index].setColor(pPtAry[i].r, pPtAry[i].g, pPtAry[i].b); //写入颜色数据  袁亮   20160625
				index ++;
			}
		}		

		return index;
	}

	// 内存点云到物理文件数据块
	int pPtArray2PtBlock(
		PointXYZIPRGBA* ppPtAry[],	// 输入,指向点的指针的数组
		int count,						// 输入,点数
		const HdRefPoint* pRefPt,		// 输入,块左下角参考点
		HdPointXYZ** pPtBlock,			// 输出,物理块数据坐标,外部使用完毕后要释放
		U8**		 pPIntensity,		// 输出,物理块数据强度,外部使用完毕后要释放
		HdPtColor**  pPtColor,          // 输出，物理块数据颜色，格式为R5G6B5，外部使用完毕后要释放   袁亮   20160625
		U16 minIntensity,				// 最小反射强度
		U16 maxIntensity)				// 最大反射强度
	{
		if (ppPtAry == NULL || *ppPtAry == NULL || count <= 0)
		{
			return 0;
		}

		bool bHasIntentisy = (minIntensity >= 0 && maxIntensity > 0 && maxIntensity >= minIntensity);
		U16 intRange = maxIntensity - minIntensity;
		F32 fScaleIntentisy = 255.0f/intRange;

		HdPointXYZ* pTmpPts = NULL;
		pTmpPts = new HdPointXYZ[count];
		(*pPtBlock) = pTmpPts;
		//为颜色分配存储空间  袁亮   20160625
		HdPtColor* pTmpColors = NULL;
		pTmpColors = new HdPtColor[count];
		(*pPtColor) = pTmpColors;
		U8* pTmpIntensity = NULL;
		if (bHasIntentisy)
		{
			pTmpIntensity = new U8[count];
			(*pPIntensity) = pTmpIntensity;
		}

		int i,index = 0;
		int x,y,z;
		if (bHasIntentisy)
		{
			for (i = 0;i < count;i++)
			{
				x = hd_round32((ppPtAry[i]->x - pRefPt->x)*pRefPt->rxRcp);
				y = hd_round32((ppPtAry[i]->y - pRefPt->y)*pRefPt->ryRcp);
				z = hd_round32((ppPtAry[i]->z - pRefPt->z)*pRefPt->rzRcp);
				if(x < 0 || y < 0 || z < 0 || x > 65535 || y > 65535 || z > 65535)
					continue;
				pTmpPts[index].x = x;
				pTmpPts[index].y = y;
				pTmpPts[index].z = z;
				pTmpIntensity[index] = hd_round32((ppPtAry[i]->getIntensity() - minIntensity) * fScaleIntentisy);
				pTmpColors[index].setColor(ppPtAry[i]->r, ppPtAry[i]->g, ppPtAry[i]->b); //写入颜色数据  袁亮   20160625
				index++;
			}
		}
		else
		{
			for (i = 0;i < count;i++)
			{
				pTmpPts[i].x = hd_round32((ppPtAry[i]->x - pRefPt->x)*pRefPt->rxRcp);
				pTmpPts[i].y = hd_round32((ppPtAry[i]->y - pRefPt->y)*pRefPt->ryRcp);
				pTmpPts[i].z = hd_round32((ppPtAry[i]->z - pRefPt->z)*pRefPt->rzRcp);
				pTmpColors[index].setColor(ppPtAry[i]->r, ppPtAry[i]->g, ppPtAry[i]->b); //写入颜色数据  袁亮   20160625
				index ++;
			}
		}		

		return index;
	}
	// 内存点云到物理文件数据块,加入块集偏移量--xyg
	int pPtArray2PtBlock(
		PointXYZIPRGBA* ppPtAry[],	     // 输入,指向点的指针的数组
		int count,						// 输入,点数
		const HdRefPoint* pRefPt,		// 输入,块左下角参考点
		HdPointXYZ** pPtBlock,			// 输出,物理块数据坐标,外部使用完毕后要释放
		U8**		 pPIntensity,		// 输出,物理块数据强度,外部使用完毕后要释放
		HdPtColor**  pPtColor,          // 输出，物理块数据颜色，格式为R5G6B5，外部使用完毕后要释放   袁亮   20160625
		F64 offsetX,
		F64 offsetY,
		U16 minIntensity,				// 最小反射强度
		U16 maxIntensity                // 最大反射强度
		)				
	{
		if (ppPtAry == NULL || *ppPtAry == NULL || count <= 0)
		{
			return 0;
		}

		bool bHasIntentisy = (minIntensity >= 0 && maxIntensity > 0 && maxIntensity >= minIntensity);
		U16 intRange = maxIntensity - minIntensity;
		F32 fScaleIntentisy = 255.0f/intRange;

		HdPointXYZ* pTmpPts = NULL;
		pTmpPts = new HdPointXYZ[count];
		(*pPtBlock) = pTmpPts;
		//为颜色分配存储空间  袁亮   20160625
		HdPtColor* pTmpColors = NULL;
		pTmpColors = new HdPtColor[count];
		(*pPtColor) = pTmpColors;
		U8* pTmpIntensity = NULL;
		if (bHasIntentisy)
		{
			pTmpIntensity = new U8[count];
			(*pPIntensity) = pTmpIntensity;
		}

		int i,index = 0;
		int x,y,z;
		if (bHasIntentisy)
		{
			for (i = 0;i < count;i++)
			{
				x = hd_round32((ppPtAry[i]->x +offsetX - pRefPt->x)*pRefPt->rxRcp);
				y = hd_round32((ppPtAry[i]->y +offsetY - pRefPt->y)*pRefPt->ryRcp);
				z = hd_round32((ppPtAry[i]->z - pRefPt->z)*pRefPt->rzRcp);
				if(x < 0 || y < 0 || z < 0 || x > 65535 || y > 65535 || z > 65535)
					continue;
				pTmpPts[index].x = x;
				pTmpPts[index].y = y;
				pTmpPts[index].z = z;
				pTmpIntensity[index] = hd_round32((ppPtAry[i]->getIntensity() - minIntensity) * fScaleIntentisy);
				pTmpColors[index].setColor(ppPtAry[i]->r, ppPtAry[i]->g, ppPtAry[i]->b); //写入颜色数据  袁亮   20160625
				index++;
			}
		}
		else
		{
			for (i = 0;i < count;i++)
			{
				pTmpPts[i].x = hd_round32((ppPtAry[i]->x + offsetX - pRefPt->x)*pRefPt->rxRcp);
				pTmpPts[i].y = hd_round32((ppPtAry[i]->y + offsetY - pRefPt->y)*pRefPt->ryRcp);
				pTmpPts[i].z = hd_round32((ppPtAry[i]->z - pRefPt->z)*pRefPt->rzRcp);
				pTmpColors[index].setColor(ppPtAry[i]->r, ppPtAry[i]->g, ppPtAry[i]->b); //写入颜色数据  袁亮   20160625
				index ++;
			}
		}		

		return index;
	}

	// 根据带号、层号和parcelData包围盒获取对应坐标转换绝对坐标偏移量
	int GetParcelDataOffsetXY(
		U8 ZoneID,                      // 投影区域带号
		U16 LevelNO,                    // 层号
		F64 boxMinX,F64 boxMinY,           // ParcelData包围盒minEdge 
		F64& offsetX,                   // X偏移量
		F64& offsetY)                   // Y偏移量
	{
		// 得到对应带区域中央经线
		F32 nCenter = ZoneID * 3;
		// 对应层的块集边长
		U16 nBlockSetStep = pow(2.0,LevelNO) * 64;
		// 保证投影带号和层号有效
		if (ZoneID == 0 || LevelNO < 0 )
		{
			return 0;
		}
		// 该对应Gauss3度带投影的区块划分起点经纬度坐标为(nCenter -1.5,0);
		CGeoPositionTran positionTran;
		double dLongitude = nCenter - 1.5;        // 对应区分界子午线
		double dLatitude = 0.0;                   // 对应区域起点纬度
		double dMinXStart = 0.0;                  // 起点坐标对应大地坐标X
		double dMinYStart = 0.0;                  // 起点坐标对应大地坐标Y
		positionTran.TranslateDegree2Gauss(dLongitude,dLatitude,dMinXStart,dMinYStart);
		// 得到对应块集编号
		U32 nBlockSetXNo = (U32)(boxMinX - dMinXStart) / nBlockSetStep;
		U32 nBLockSetYNo = (U32)(boxMinY - dMinYStart) / nBlockSetStep;
		// 通过块集编号得到对应块集点云偏移量
		offsetX = nBlockSetXNo * nBlockSetStep + dMinXStart;
		offsetY = nBLockSetYNo * nBlockSetStep + dMinYStart;

		return 1;
	}

	//不使用硬编码，获取更大的灵活性   袁亮  20161105
	// 根据带号、层号和bolckset包围盒获取对应BlockSetNO
	int GetBlockSetNO(
		//U16 LevelNO,                    // 层号
		F32 stepX,                      //块集网格尺寸
		F32 stepY,                      //块集网格尺寸
		CHdBox3df& fullBox,             // 点云包围盒
		CHdBox3df& blockSetBox,         // 块集包围盒
		U32& blocksetNO)                // 块集编号
	{
		//// 对应层的块集边长，云存储模式下块集X、Y方向等边长
		//U16 nBlockSetStep = pow(2.0,LevelNO) * 64;

		//// 块集X、Y方向划分数目numBlocksetX
		//U32 numBlocksetX = (U32)ceil((fullBox.MaxEdge.X - fullBox.MinEdge.X) / nBlockSetStep);
		//U32 numBlocksetY = (U32)ceil((fullBox.MaxEdge.Y - fullBox.MinEdge.Y) / nBlockSetStep);

		//// 保证投影带号和层号有效
		//if (/*ZoneID == 0 || */LevelNO < 0 )
		//{
		//	return 0;
		//}

		//// 得到对应块集行列号
		//U32 nBlockSetXNo = (U32)(blockSetBox.MinEdge.X - fullBox.MinEdge.X) / nBlockSetStep;
		//U32 nBlockSetYNo = (U32)(blockSetBox.MinEdge.Y - fullBox.MinEdge.Y) / nBlockSetStep;

		U32 numBlocksetX = (U32)ceil((fullBox.MaxEdge.X - fullBox.MinEdge.X) / stepX);
		U32 numBlocksetY = (U32)ceil((fullBox.MaxEdge.Y - fullBox.MinEdge.Y) / stepY);

		U32 nBlockSetXNo = (U32)((blockSetBox.MinEdge.X - fullBox.MinEdge.X) / stepX);
		U32 nBlockSetYNo = (U32)((blockSetBox.MinEdge.Y - fullBox.MinEdge.Y) / stepY);
		
		// 计算当前块集编号
		blocksetNO = numBlocksetX * nBlockSetYNo + nBlockSetXNo;

		return 1;
	}

	// 根据带号、层号和bolck包围盒获取对应块编号BlockNO
	int GetBlockNO(
		CHdBox3df& blockSetBox,         // 块集包围盒
		CHdBox3df& blockBox,            // 块包围盒
		U16& blockNO)                   // 块号
	{
		// 计算当前块编号
		CHdVector3df centerBlockSet = blockSetBox.getCenter();
		CHdVector3df centerBlock = blockBox.getCenter();
		// 按块集和块的空间位置关系进行块集编号确定右下到上，由左到右依次划分为
		if (centerBlock.Z < centerBlockSet.Z)
		{
			if (centerBlock.X < centerBlockSet.X)
			{
				if (centerBlock.Y < centerBlockSet.Y)
				{
					blockNO = 0;
				}
				else
				{
					blockNO = 2;
				}
			}
			else
			{
			    if (centerBlock.Y < centerBlockSet.Y)
			    {
					blockNO = 1;
			    }
				else
				{
					blockNO = 3;
				}
			}
		}
		else
		{
			if (centerBlock.X < centerBlockSet.X)
			{
				if (centerBlock.Y < centerBlockSet.Y)
				{
					blockNO = 4;
				}
				else
				{
					blockNO = 6;
				}
			}
			else
			{
			    if (centerBlock.Y < centerBlockSet.Y)
			    {
					blockNO = 5;
			    }
				else
				{
					blockNO = 7;
				}
			}
		}

		return 1;
	}
}