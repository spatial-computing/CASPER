// ====================================================================
//
//	A network optimization algorithm using Fibonacci Heap
//
//	Written by: Max Winkler
//  http://www.codeproject.com/KB/recipes/Dijkstras_Algorithm.aspx
// ====================================================================

#pragma once

#include <hash_map>
#include "NAEdge.h"
#include "NAVertex.h"

#define HeapDataType NAEdge
 
class HeapNode
{
public:
	HeapNode * parent;
	HeapNode * leftSibling, * rightSibling;
	HeapNode * children; 

	HeapNode(HeapDataType * data);
	HeapNode();

	HeapDataType * data;
	int rank;
	
	bool addChild(HeapNode * node);
	bool addSibling(HeapNode * node);
	bool remove();	
	HeapNode * leftMostSibling();
	HeapNode * rightMostSibling();
};

typedef HeapNode * HeapNodePtr;
/*
#define HeapDataType NAVertex

class HeapNodeTable
{
private:
	stdext::hash_map<long, HeapNodePtr> * cache;

public:
	HeapNodeTable(void) { cache = new DEBUG_NEW_PLACEMENT stdext::hash_map<long, HeapNodePtr>(); }
	~HeapNodeTable(void)
	{
		Clear();
		delete cache;
	}
	
	int Size() { return cache->size(); }
	void Clear() { cache->clear(); }

	void Erase(NAVertex * vertex);
	void Insert(HeapNodePtr node);
	HeapNodePtr Find(NAVertex * vertex);
};
*/

class HeapNodeTable
{
private:
	stdext::hash_map<long, HeapNodePtr> * cacheAlong;
	stdext::hash_map<long, HeapNodePtr> * cacheAgainst;

public:
	HeapNodeTable(void)
	{
		cacheAlong = new DEBUG_NEW_PLACEMENT stdext::hash_map<long, HeapNodePtr>();
		cacheAgainst = new DEBUG_NEW_PLACEMENT stdext::hash_map<long, HeapNodePtr>();
	}

	~HeapNodeTable(void)
	{
		Clear();
		delete cacheAlong;
		delete cacheAgainst;
	}
	
	size_t Size() { return cacheAlong->size() + cacheAgainst->size(); }
	void Clear() { cacheAlong->clear(); cacheAgainst->clear(); }

	void Erase(HeapDataType * edge);
	void Insert(HeapNode * node);
	HeapNodePtr Find(HeapDataType * edge);
};

class FibonacciHeap
{
private:
	HeapNode ** rootListByRank;
	HeapNode * minRoot;
	HeapNodeTable * nodeTable;
	bool (*LessThan)(HeapDataType *, HeapDataType *);
	bool link(HeapNode * root);

public:
	FibonacciHeap(bool (*LessThanMethod)(HeapDataType *, HeapDataType *));
	bool IsVisited(HeapDataType * vertex);

	~FibonacciHeap();

	bool IsEmpty();
	void Clear();
	bool Insert(HeapDataType * node);	
	HRESULT DecreaseKey(HeapDataType * node);
	size_t Size() const { return nodeTable->Size(); }

	HeapDataType * FindMin();
	HeapDataType * DeleteMin();
};
