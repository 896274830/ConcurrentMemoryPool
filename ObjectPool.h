#pragma once

#include"Common.h"

//�����ڴ��
//template<size_t N>
//class ObjectPool
//{};
template<class T>
class ObjectPool
{
public:
	T* New()
	{
		T* obj = nullptr;
		//�����ظ����û��������ڴ�
		if (_freeList)
		{
			void* next = *(void**)_freeList;
			obj = (T*)_freeList;
			_freeList = next;
		}
		//û�û��������ڴ���ظ�������ȡδʹ�ù���ʣ���ڴ�
		else
		{
			size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
			if (_remainBytes < objSize)//ʣ���ڴ治��һ�������С
			{
				_remainBytes = objSize * 1024;
				//_memory = (char*)malloc(_remainBytes);
				_memory = (char*)SystemAlloc(_remainBytes >> PAGE_SHIFT);//����ʹ��malloc
				if (_memory == nullptr)
				{
					throw std::bad_alloc();
				}
			}
			obj = (T*)_memory;
			//���ٸ�һ��ָ���С���ڴ棬ʹ�䱻�黹����������һ�����黹�ڴ�ڵ��ָ��
			_memory += objSize;
			_remainBytes -= objSize;
		}

		//��λnew����ʾ����T�Ĺ��캯����ʼ��
		new(obj)T;
		return obj;
	}

	void Delete(T* obj)
	{
		//��ʾ������������
		obj->~T();

		//ͷ��
		//*(int*)obj = _freeList;//��һ��int��С���ڴ�������һ���ڴ�ڵ�ĵ�ַ���޷�����32/64λƽ̨
		*(void**)obj = _freeList;//��һ��void*��С(��һ��ָ���С)���ڴ�������һ���ڴ�ڵ�ĵ�ַ��һ��ָ��Ĵ�С��ƽ̨�Զ��䶯
		_freeList = obj;
	}

private:
	char* _memory = nullptr;//ָ�����ڴ�
	void* _freeList = nullptr;//������С�ڴ�������ͷָ��
	size_t _remainBytes = 0;//����ڴ��з�ʣ����ڴ��ֽ���
};




struct TreeNode
{
	int _val;
	TreeNode* _left;
	TreeNode* _right;
	TreeNode()
		:_val(0)
		, _left(nullptr)
		, _right(nullptr)
	{}
};

static void TestObjectPool()
{
	const size_t N = 500000;//�����ͷŴ���

	//new/delete���ܲ���
	size_t begin1 = clock();
	std::vector<TreeNode*> v1;
	v1.reserve(N);
	for (int i = 0; i < N; ++i)
	{
		v1.push_back(new TreeNode);
	}
	for (int i = 0; i < N; ++i)
	{
		delete v1[i];
	}
	size_t end1 = clock();

	//ObjectPool���ܲ���
	ObjectPool<TreeNode> TNPool;
	size_t begin2 = clock();
	std::vector<TreeNode*> v2;
	v2.reserve(N);
	for (int i = 0; i < N; ++i)
	{
		v2.push_back(TNPool.New());
	}
	for (int i = 0; i < N; ++i)
	{
		TNPool.Delete(v2[i]);
	}
	size_t end2 = clock();

	cout << "new/delete cost time:" << end1 - begin1 << endl;
	cout << "ObjectPool cost time:" << end2 - begin2 << endl;
}