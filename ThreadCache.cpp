#include"ThreadCache.h"
#include"CentralCache.h"

void* ThreadCache::FetchFromCentralCache(size_t index, size_t bytes)
{
	//����ʼ���������㷨
	//1.һ��ʼ����һ����CentralCache����Ҫ̫�࣬��ֹ�ò����˷�
	//2.���������bytes��С�ڴ������batchNum�ͻ᲻��������ֱ������SizeClass::NumMoveSize(bytes)
	//3.bytesԽС��һ����CentralCacheҪ���ڴ��Խ��
	//4.bytesԽ��һ����CentralCacheҪ���ڴ��ԽС
	size_t batchNum = min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(bytes));//threadCacheһ�δ����Ļ����ȡ���ٸ�����
	if (batchNum == _freeLists[index].MaxSize())
	{
		_freeLists[index].MaxSize() += 1;
	}
	
	void* start = nullptr;//����Ͳ���
	void* end = nullptr;//����Ͳ���  ��¼��������ڴ����β��ַ
	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, bytes);//ʵ��Ҫ��������
	assert(actualNum >= 1);
	
	if (actualNum == 1)
	{
		assert(start == end);
		return start;//ֻ���뵽һ����ֱ�ӷ��أ����ò���
	}
	else
	{
		//���ص�һ���ڴ����ʣ����ڴ����ͷ���뵽ThreadCache��freeLists[index]
		_freeLists[index].PushRange(NextObj(start), end, actualNum - 1);
		return start;
	}
}

void* ThreadCache::Allocate(size_t bytes)
{
	assert(bytes <= MAX_BYTES);
	size_t alignBytes = SizeClass::RoundUp(bytes);
	size_t index = SizeClass::Index(bytes);
	if (!_freeLists[index].Empty())
	{
		return _freeLists[index].Pop();
	}
	else
	{
		return FetchFromCentralCache(index, alignBytes);
	}
}

void ThreadCache::Deallocate(void* ptr, size_t bytes)
{
	assert(ptr);
	assert(bytes <= MAX_BYTES);

	// �Ҷ�Ӧ����������Ͱ���Ѷ���ͷ���ȥ
	size_t index = SizeClass::Index(bytes);
	_freeLists[index].Push(ptr);
	// �������ȴ���һ������������ڴ�����ʱ�Ϳ�ʼ�ͷŹ黹һ��list��CentralCache
	if (_freeLists[index].Size() >= _freeLists[index].MaxSize())
	{
		ListToLong(_freeLists[index], bytes);
	}
}

void ThreadCache::ListToLong(FreeList& list, size_t bytes)
{
	void* start = nullptr;
	void* end = nullptr;//start��end����Բ���
	list.PopRange(start, end, list.MaxSize());
	CentralCache::GetInstance()->ReleaseListToSpans(start, bytes);
}
