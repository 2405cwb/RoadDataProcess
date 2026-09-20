#pragma once

#include <windows.h>
#include <queue>

struct DISK_IMAGE_TASK
{
	DISK_IMAGE_TASK()
		: buff(NULL)
		, dataSize(0)
		, width(0)
		, height(0)
		, pixelFormat(0)
		, channelCount(0)
		, index(0)
	{
		curTime[0] = 0;
	}

	char* buff;
	size_t dataSize;
	int width;
	int height;
	int pixelFormat;
	int channelCount;
	int index;
	char curTime[32];
};

class CImagePool
{
public:
	CImagePool();
	~CImagePool();

	bool InitPool(size_t blockSize, int blockCount);
	void ReleasePool();

	char* AcquireBlock();
	void RecycleBlock(char* pBlock);

	bool PushData(char* pData, size_t dataSize, int width, int height, int pixelFormat, int channelCount, int index, const char* curTime);
	bool GetDataOne(DISK_IMAGE_TASK& task);

	int GetReadySize();
	int GetFreeSize();
	int GetBlockCount() const;
	size_t GetBlockSize() const;

private:
	CRITICAL_SECTION m_lock;
	std::queue<DISK_IMAGE_TASK> m_readyQueue;
	std::queue<char*> m_freeBlocks;
	size_t m_blockSize;
	int m_blockCount;
	bool m_poolInited;
};
