#include "ImagePool.h"
#include <string.h>
#include <stdlib.h>

CImagePool::CImagePool()
	: m_blockSize(0)
	, m_blockCount(0)
	, m_poolInited(false)
{
	InitializeCriticalSection(&m_lock);
}

CImagePool::~CImagePool()
{
	ReleasePool();
	DeleteCriticalSection(&m_lock);
}

bool CImagePool::InitPool(size_t blockSize, int blockCount)
{
	EnterCriticalSection(&m_lock);

	while (!m_readyQueue.empty())
	{
		DISK_IMAGE_TASK task = m_readyQueue.front();
		m_readyQueue.pop();
		if (task.buff != NULL)
		{
			free(task.buff);
		}
	}

	while (!m_freeBlocks.empty())
	{
		free(m_freeBlocks.front());
		m_freeBlocks.pop();
	}

	m_blockSize = 0;
	m_blockCount = 0;
	m_poolInited = false;

	if (blockSize == 0 || blockCount <= 0)
	{
		LeaveCriticalSection(&m_lock);
		return false;
	}

	for (int i = 0; i < blockCount; ++i)
	{
		char* pBlock = (char*)malloc(blockSize);
		if (pBlock == NULL)
		{
			while (!m_freeBlocks.empty())
			{
				free(m_freeBlocks.front());
				m_freeBlocks.pop();
			}
			LeaveCriticalSection(&m_lock);
			return false;
		}
		m_freeBlocks.push(pBlock);
	}

	m_blockSize = blockSize;
	m_blockCount = blockCount;
	m_poolInited = true;
	LeaveCriticalSection(&m_lock);
	return true;
}

void CImagePool::ReleasePool()
{
	EnterCriticalSection(&m_lock);

	while (!m_readyQueue.empty())
	{
		DISK_IMAGE_TASK task = m_readyQueue.front();
		m_readyQueue.pop();
		if (task.buff != NULL)
		{
			m_freeBlocks.push(task.buff);
		}
	}

	while (!m_freeBlocks.empty())
	{
		free(m_freeBlocks.front());
		m_freeBlocks.pop();
	}

	m_blockSize = 0;
	m_blockCount = 0;
	m_poolInited = false;
	LeaveCriticalSection(&m_lock);
}

char* CImagePool::AcquireBlock()
{
	EnterCriticalSection(&m_lock);
	if (!m_poolInited || m_freeBlocks.empty())
	{
		LeaveCriticalSection(&m_lock);
		return NULL;
	}

	char* pBlock = m_freeBlocks.front();
	m_freeBlocks.pop();
	LeaveCriticalSection(&m_lock);
	return pBlock;
}

void CImagePool::RecycleBlock(char* pBlock)
{
	if (pBlock == NULL)
	{
		return;
	}

	EnterCriticalSection(&m_lock);
	if (m_poolInited)
	{
		m_freeBlocks.push(pBlock);
	}
	else
	{
		free(pBlock);
	}
	LeaveCriticalSection(&m_lock);
}

bool CImagePool::PushData(char* pData, size_t dataSize, int width, int height, int pixelFormat, int channelCount, int index, const char* curTime)
{
	if (pData == NULL)
	{
		return false;
	}

	EnterCriticalSection(&m_lock);
	DISK_IMAGE_TASK task;
	task.buff = pData;
	task.dataSize = dataSize;
	task.width = width;
	task.height = height;
	task.pixelFormat = pixelFormat;
	task.channelCount = channelCount;
	task.index = index;
	if (curTime != NULL)
	{
		strncpy_s(task.curTime, _countof(task.curTime), curTime, _TRUNCATE);
	}
	m_readyQueue.push(task);
	LeaveCriticalSection(&m_lock);
	return true;
}

bool CImagePool::GetDataOne(DISK_IMAGE_TASK& task)
{
	EnterCriticalSection(&m_lock);
	if (m_readyQueue.empty())
	{
		LeaveCriticalSection(&m_lock);
		return false;
	}

	task = m_readyQueue.front();
	m_readyQueue.pop();
	LeaveCriticalSection(&m_lock);
	return true;
}

int CImagePool::GetReadySize()
{
	EnterCriticalSection(&m_lock);
	int size = (int)m_readyQueue.size();
	LeaveCriticalSection(&m_lock);
	return size;
}

int CImagePool::GetFreeSize()
{
	EnterCriticalSection(&m_lock);
	int size = (int)m_freeBlocks.size();
	LeaveCriticalSection(&m_lock);
	return size;
}

int CImagePool::GetBlockCount() const
{
	return m_blockCount;
}

size_t CImagePool::GetBlockSize() const
{
	return m_blockSize;
}
