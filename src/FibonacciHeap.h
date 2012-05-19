// ====================================================================
//
//	A network optimization algorithm using Fibonacci Heap
//
//	Written by: Max Winkler
//  http://www.codeproject.com/KB/recipes/Dijkstras_Algorithm.aspx
// ====================================================================

#pragma once

#include <hash_map>
#include "NAGraph.h"

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
	HeapNodeTable(void) { cache = new stdext::hash_map<long, HeapNodePtr>(); }
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
		cacheAlong = new stdext::hash_map<long, HeapNodePtr>();
		cacheAgainst = new stdext::hash_map<long, HeapNodePtr>();
	}

	~HeapNodeTable(void)
	{
		Clear();
		delete cacheAlong;
		delete cacheAgainst;
	}
	
	int Size() { return cacheAlong->size() + cacheAgainst->size(); }
	void Clear() { cacheAlong->clear(); cacheAgainst->clear(); }

	void Erase(NAEdge * edge);
	void Insert(HeapNodePtr node);
	HeapNodePtr Find(NAEdge * edge);
};

class FibonacciHeap
{
private:
	HeapNode ** rootListByRank;
	HeapNode * minRoot;
	HeapNodeTable * nodeTable;
	bool (*LessThan)(NAEdge *, NAEdge *);
	bool link(HeapNode * root);

public:
	FibonacciHeap(bool (*LessThanMethod)(NAEdge *, NAEdge *));
	bool IsVisited(HeapDataType * vertex);

	~FibonacciHeap();

	bool IsEmpty();
	void Clear();
	bool Insert(HeapDataType * node);	
	HRESULT DecreaseKey(HeapDataType * node);

	HeapDataType * FindMin();
	HeapDataType * DeleteMin();
};
