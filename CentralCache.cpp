#include"CentralCache.h"
#include"PageCache.h"

CentralCache CentralCache::_sInst;

// ����һ�������Ķ����ThreadCache
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t bytes)
{
	size_t index = SizeClass::Index(bytes);
	_spanLists[index]._mtx.lock();
	//��ȡһ���ǿ�Span
	Span* span = GetOneSpan(_spanLists[index], bytes);
	assert(span);
	assert(span->_freeList);
	//��span��ͷ�г�batchNum�������ThreadCache
	//���������ж����ö���
	start = span->_freeList;//������һ����ͷ������β��
	end = start;
	size_t actualNum = 1;
	size_t i = 1;
	while (i < batchNum && NextObj(end) != nullptr)
	{
		end = NextObj(end);
		++i;
		++actualNum;
	}
	span->_freeList = NextObj(end);//ʣ��Ļ���span
	NextObj(end) = nullptr;//���ߵĽ�β��nullptr
	span->_useCount += actualNum;

	_spanLists[index]._mtx.unlock();
	return actualNum;
}

// ��SpanList��ȡһ���ǿյ�Span
Span* CentralCache::GetOneSpan(SpanList& list, size_t bytes)
{
	//�Ȳ鿴��ǰSpanList�Ƿ�ǿյ�Span
	Span* it = list.Begin();
	while (it != list.End())
	{
		if (it->_freeList != nullptr)
			return it;
		else
			it = it->_next;
	}
	//�ߵ�����˵����ǰSpanListû�ÿ��е�Span��ֻ����PageCacheҪ
	list._mtx.unlock();//����PageCacheǰ�������Ȱ�CentralCache��Ͱ�������������������߳��ͷ��ڴ���������������
	PageCache::GetInstance()->_pageMtx.lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(bytes));
	span->_isUsed = true;//��־span��ʹ�ã���ֹ�ͷ��ڴ�ϲ�spanʱ����Ϊ��span��ȡ������û�и�_useCountΪ0�ֱ��ûغϲ�
	span->_objSize = bytes;
	PageCache::GetInstance()->_pageMtx.unlock();
	//��span�е�_pageId��_n������_freeListӦָ��Ĵ���ڴ����ʼ��ַ�ʹ�С�ֽ���
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t size = span->_n << PAGE_SHIFT;
	char* end = start + size; 
	//��span�д���ڴ��г�����������������
	span->_freeList = start;//������һ����ͷ������β���з�
	void* tail = span->_freeList;
	start += bytes;
	//int i = 1;//��֤�Ƿ��г�����Ҫ�ķ���
	while (start < end)
	{
		//++i;
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += bytes;
	}
	NextObj(tail) = nullptr;//��β�ÿ�
	//�к�span����Ҫ��span�ҵ�CentralCache��Ӧ��Ͱ���棬������Ҫ��Ͱ���ӻ�
	list._mtx.lock();
	list.PushFront(span);

	return span;
}


void CentralCache::ReleaseListToSpans(void* start, size_t bytes)
{
	size_t index = SizeClass::Index(bytes);
	_spanLists[index]._mtx.lock();
	while (start)
	{
		void* next = NextObj(start);
		//ÿ��С�ڴ��������Բ�ͬ��span��ͨ��ӳ���ϵ�ҵ�������span
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);//���С���ڴ���������span
		//ֱ�ӽ�С���ڴ�ͷ��黹��span�����ش���С���ڴ��˳��
		NextObj(start) = span->_freeList;
		span->_freeList = start;
		--span->_useCount;
		if (span->_useCount == 0)//˵��span�зֳ�ȥ��С���ڴ涼�����ˣ����Խ�һ���黹��PageCache��PageCache��������ǰ��ҳ�ĺϲ�
		{
			_spanLists[index].Erase(span);
			span->_freeList = nullptr;
			span->_next = nullptr;
			span->_prev = nullptr;
			//����span->_pageId�����ڼ���span��ָ���ڴ����ʼ��ַ
			_spanLists[index]._mtx.unlock();//�ͷ�span��PageCacheʱ��ʹ��PageCache�����Ϳ����ˣ�������ʱ��Ͱ�����
			PageCache::GetInstance()->_pageMtx.lock();
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
			PageCache::GetInstance()->_pageMtx.unlock();
			_spanLists[index]._mtx.lock();
		}

		start = next;
	}
	_spanLists[index]._mtx.unlock();
}
