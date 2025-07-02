#pragma once

#include"Common.h"
#include"ThreadCache.h"
#include"PageCache.h"
#include"ObjectPool.h"

static void* ConcurrentAlloc(size_t bytes)
{
	if (bytes > MAX_BYTES)//����256k
	{
		size_t alignBytes = SizeClass::RoundUp(bytes);
		size_t kpage = alignBytes >> PAGE_SHIFT;
		PageCache::GetInstance()->_pageMtx.lock();//������256kС��128ҳ�������PageCache����Ҫ����
		Span* span = PageCache::GetInstance()->NewSpan(kpage);
		PageCache::GetInstance()->_pageMtx.unlock();
		span->_objSize = bytes;

		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		return ptr;
	}
	else
	{
		// ͨ��TLSÿ���߳������Ļ�ȡ�Լ���ר��ThreadCache����
		if (pTLSThreadCache == nullptr)
		{
			//pTLSThreadCache = new ThreadCache;
			static ObjectPool<ThreadCache> tcPool;
			pTLSThreadCache = tcPool.New();//         ������
		}

		//cout << "�߳�(id " << std::this_thread::get_id() << ")��" << "pTLSThreadCache" << " : " << pTLSThreadCache << endl;

		return pTLSThreadCache->Allocate(bytes);
	}
	
}

static void ConcurrentFree(void* ptr)
{
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	size_t bytes = span->_objSize;
	if (bytes > MAX_BYTES)//����256k
	{
		PageCache::GetInstance()->_pageMtx.lock();//������256kС��128ҳ�������PageCache����Ҫ����
		PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		PageCache::GetInstance()->_pageMtx.unlock();
	}
	else
	{
		assert(pTLSThreadCache);
		pTLSThreadCache->Deallocate(ptr, bytes);
	}
}