#pragma once

#include"Common.h"

class CentralCache
{
public:
	static CentralCache* GetInstance()
	{
		return &_sInst;
	}

	// ����һ�������Ķ����ThreadCache
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t bytes);
	// ��SpanLis��ȡһ���ǿյ�Span
	Span* GetOneSpan(SpanList& list, size_t byte_size);
	// ��һ�������Ķ����ͷŵ�span���
	void ReleaseListToSpans(void* start, size_t bytes);

private:
	SpanList _spanLists[NFREELISTS];//Ͱ����ThreadCache��ͬ

	//Ҫ��������Ŀֻ��һ��ȫ�ֵ�CentralCache
	//����ģʽ - ����ģʽ
private:
	CentralCache()
	{}
	CentralCache(const CentralCache&) = delete;

	static CentralCache _sInst;
};