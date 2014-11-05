// ====================================================================
//
//	A network optimization algorithm using Fibonacci Heap
//
//	Written by: Max Winkler
//  http://www.codeproject.com/KB/recipes/Dijkstras_Algorithm.aspx
// ====================================================================

#pragma once

#include "StdAfx.h"
#include "NAEdge.h"
#include "NAVertex.h"

#define HeapDataType NAEdge

class HeapNode
{
public:
	HeapNode     * parent;
	HeapNode     * leftSibling, * rightSibling;
	HeapNode     * children;
	HeapDataType * data;
	double         key;
	int            rank;

	HeapNode(HeapDataType * data, double key);
	HeapNode();

	bool addChild(HeapNode * node);
	bool addSibling(HeapNode * node);
	bool remove();
	HeapNode * leftMostSibling();
	HeapNode * rightMostSibling();
};

typedef HeapNode * HeapNodePtr;

class HeapNodeTable
{
private:
	std::unordered_map<long, HeapNodePtr> * cacheAlong;
	std::unordered_map<long, HeapNodePtr> * cacheAgainst;

public:
	HeapNodeTable(void)
	{
		cacheAlong = new DEBUG_NEW_PLACEMENT std::unordered_map<long, HeapNodePtr>();
		cacheAgainst = new DEBUG_NEW_PLACEMENT std::unordered_map<long, HeapNodePtr>();
	}

	~HeapNodeTable(void)
	{
		Clear();
		delete cacheAlong;
		delete cacheAgainst;
	}

	size_t Size() const { return cacheAlong->size() + cacheAgainst->size(); }
	void Clear()        { cacheAlong->clear(); cacheAgainst->clear(); }

	void Erase(HeapDataType * edge);
	void Insert(HeapNode * node);
	HeapNodePtr Find(HeapDataType * edge) const;
};

class FibonacciHeap
{
private:
	double maxHeakKeyValue;
	HeapNode ** rootListByRank;
	HeapNode * minRoot;
	HeapNodeTable * nodeTable;
	double (*GetHeapKey)(const HeapDataType *);
	bool link(HeapNode * root);

public:
	FibonacciHeap(double (*GetHeapKeyMethod)(const HeapDataType *));
	bool IsVisited(HeapDataType * vertex);
	double GetMaxHeapKey(void) const { return maxHeakKeyValue; }
	void ResetMaxHeapKey(void)       { maxHeakKeyValue = 0.0;  }

	~FibonacciHeap();

	bool IsEmpty();
	void Clear();
	bool Insert(HeapDataType * node);
	HRESULT DecreaseKey(HeapDataType * node);
	size_t Size() const { return nodeTable->Size(); }

	HeapDataType * FindMin();
	HeapDataType * DeleteMin();
};
