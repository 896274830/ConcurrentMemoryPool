#include"PageCache.h"

PageCache PageCache::_sInst;

// ����һ��kҳ��span
Span* PageCache::NewSpan(size_t k)
{
	assert(k > 0);

	if (k > NPAGES - 1)//������128ҳ��ֱ���Ҷ�����
	{
		void* ptr = SystemAlloc(k);
		//Span* span = new Span;
		Span* span = _spanPool.New();
		span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
		span->_n = k;
		//_idSpanMap[span->_pageId] = span;//����_pageId��span��ӳ��
		_idSpanMap.set(span->_pageId, (void*)span);

		return span;
	}

	//�Ȳ鿴kҳ��Ͱ�����޷ǿ�span
	if (!_spanLists[k].Empty())
	{
		Span* kSpan = _spanLists[k].PopFront();
		//����kSpan������ҳ��_pageId��span��ӳ�䣬����CentralCache����С���ڴ�ʱ���Ҷ�Ӧ��span
		for (PAGE_ID i = 0; i < kSpan->_n; i++)
		{
			//_idSpanMap[kSpan->_pageId + i] = kSpan;
			_idSpanMap.set(kSpan->_pageId + i, (void*)kSpan);
		}
		return kSpan;
	}

	//���һ�º�����Ͱ��û�зǿ�span������п��԰����з�
	for (size_t i = k + 1; i < NPAGES; i++)
	{
		if (!_spanLists[i].Empty())
		{
			Span* nSpan = _spanLists[i].PopFront();
			Span* kSpan = _spanPool.New();
			//��nSpan��ͷ����һ��kҳ����
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;
			nSpan->_pageId += k;
			nSpan->_n -= k;

			_spanLists[nSpan->_n].PushFront(nSpan);

			//����nSpan����βҳҳ�Ÿ�nSpan��ӳ�䣬����ϲ�spanʱ����ǰ�����ڵ�span
			//_idSpanMap[nSpan->_pageId] = nSpan;
			//_idSpanMap[nSpan->_pageId + nSpan->_n - 1] = nSpan;
			_idSpanMap.set(nSpan->_pageId, (void*)nSpan);
			_idSpanMap.set(nSpan->_pageId + nSpan->_n - 1, (void*)nSpan);


			//����kSpan������ҳ��_pageId��span��ӳ�䣬����CentralCache����С���ڴ�ʱ���Ҷ�Ӧ��span
			for (PAGE_ID i = 0; i < kSpan->_n; i++)
			{
				//_idSpanMap[kSpan->_pageId + i] = kSpan;
				_idSpanMap.set(kSpan->_pageId + i, (void*)kSpan);
			}

			return kSpan;
		}
	}
	//������ͰҲ��û�зǿ�span���Ӷ�����һ��128ҳ��span
	Span* bigSpan = _spanPool.New();
	void* ptr = SystemAlloc(NPAGES - 1);
	bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;//bigSpan��ʼҳ��ҳ��
	bigSpan->_n = NPAGES - 1;//bigSpan������ҳ��
	_spanLists[bigSpan->_n].PushFront(bigSpan);

	return NewSpan(k);
}


// ��ȡ�Ӷ���span��ӳ��
Span* PageCache::MapObjectToSpan(void* obj)
{
	PAGE_ID id = (PAGE_ID)obj >> PAGE_SHIFT;

	//std::unique_lock<std::mutex> lock(_pageMtx);//RAII�� �����������Զ�����
	//auto it = _idSpanMap.find(id);//findʱ�������߳̿�����д��unordered_map����һ��Ԫ�ؽṹ���ܻ�ı䣬��Ҫ����
	//if (it != _idSpanMap.end())
	//{
	//	return it->second;
	//}
	//else
	//{
	//	assert(false);
	//	return nullptr;
	//}

	//����������Ҫ����
	Span* span = (Span*)_idSpanMap.get(id);
	assert(span);
	return span;
}

// �ͷſռ�span�ص�PageCache�����ϲ����ڵ�span
void PageCache::ReleaseSpanToPageCache(Span* span)
{
	//����128ҳ��spanֱ�ӹ黹����
	if (span->_n > NPAGES - 1)
	{
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		SystemFree(ptr);
		//delete span;
		_spanPool.Delete(span);
	}
	else//���Զ�spanǰ��ҳ�ϲ�������ڴ���Ƭ����
	{
		while (1)//��ǰ�ϲ�
		{
			PAGE_ID prevId = span->_pageId - 1;
			//auto it = _idSpanMap.find(prevId);
			//if (it == _idSpanMap.end())//ǰһҳ�����ڣ����ϲ�
			//	break;
			//Span* prevSpan = it->second;
			Span* prevSpan = (Span*)_idSpanMap.get(prevId);
			if (prevSpan == nullptr)//ǰһҳ�����ڣ����ϲ�
				break;

			if (prevSpan->_isUsed == true)//����ǰһҳ��span��ʹ�ã����ϲ�
				break;

			if (prevSpan->_n + span->_n > NPAGES - 1)//����������128ҳ���ù������ϲ�
				break;

			span->_pageId = prevSpan->_pageId;
			span->_n += prevSpan->_n;
			_spanLists[prevSpan->_n].Erase(prevSpan); 
			//delete prevSpan;
			_spanPool.Delete(prevSpan);
		}
		while (1)//���ϲ�
		{
			PAGE_ID nextId = span->_pageId + span->_n;
			//auto it = _idSpanMap.find(nextId);
			//if (it == _idSpanMap.end())//��һҳ�����ڣ����ϲ�
			//	break;
			//Span* nextSpan = it->second;
			Span* nextSpan = (Span*)_idSpanMap.get(nextId);
			if (nextSpan == nullptr)//��һҳ�����ڣ����ϲ�
				break;

			if (nextSpan->_isUsed == true)//������һҳ��span��ʹ�ã����ϲ�
				break;

			if (nextSpan->_n + span->_n > NPAGES - 1)//����������128ҳ���ù������ϲ�
				break;

			span->_n += nextSpan->_n;
			_spanLists[nextSpan->_n].Erase(nextSpan);
			//delete nextSpan;
			_spanPool.Delete(nextSpan);
		}

		_spanLists[span->_n].PushFront(span);
		span->_isUsed = false;//����Ϊδʹ��״̬
		//���½���span��βҳҳ�Ÿ�span��ӳ���ϵ
		//_idSpanMap[span->_pageId] = span;
		//_idSpanMap[span->_pageId + span->_n - 1] = span;
		_idSpanMap.set(span->_pageId, (void*)span);
		_idSpanMap.set(span->_pageId + span->_n - 1, (void*)span);
	}
}