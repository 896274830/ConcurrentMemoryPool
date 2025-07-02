#pragma once

#include"Common.h"
#include"ObjectPool.h"
#include"PageMap.h"

class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_sInst;
	}

	// ����һ��kҳ��span
	Span* NewSpan(size_t k);
	// ��ȡ�Ӷ���span��ӳ��
	Span* MapObjectToSpan(void* obj);
	// �ͷſռ�span�ص�PageCache�����ϲ����ڵ�span
	void ReleaseSpanToPageCache(Span* span);

	std::mutex _pageMtx;
private:
	SpanList _spanLists[NPAGES];//�±���n��Ͱ�����ľ��Ǻ���nҳ��span
	//std::unordered_map<PAGE_ID, Span*> _idSpanMap;//�ڴ�ҳ����������span��ӳ��
	//���Է���Span* MapObjectToSpan(void* obj)���������������ĺܴ�
	TCMalloc_PageMap1<32 - PAGE_SHIFT> _idSpanMap;//�û���������unordered_map�洢ҳ����span��ӳ���ϵ���Ż�

	ObjectPool<Span> _spanPool;

	//Ҫ��������Ŀֻ��һ��ȫ�ֵ�PageCache
	//����ģʽ - ����ģʽ
private:
	PageCache()
	{}
	PageCache(const PageCache&) = delete;
	static PageCache _sInst;
};