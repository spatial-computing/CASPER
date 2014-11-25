// ====================================================================
//
//	A network optimization algorithm using Fibonacci Heap
//
//	Written by: Max Winkler
//  http://www.codeproject.com/KB/recipes/Dijkstras_Algorithm.aspx
// ====================================================================

#include "StdAfx.h"
#include "FibonacciHeap.h"

// =========================================================================
//	Implementation of class Node
// =========================================================================

HeapNode::HeapNode(HeapDataType * data, double key)
{
	this->data = data;
	this->key = key;
	parent = nullptr;
	children = nullptr;
	leftSibling = nullptr;
	rightSibling = nullptr;
	rank = 0;
}

HeapNode::HeapNode()
{
	this->data = nullptr;
	this->key = 0.0;
	parent = nullptr;
	children = nullptr;
	leftSibling = nullptr;
	rightSibling = nullptr;
	rank = 0;
}

bool HeapNode::addChild(HeapNode * node)
{
	if(children != nullptr)
		children->addSibling(node);
	else
	{
		children = node;
		node->parent = this;
		rank = 1;
	}

	return true;
}

bool HeapNode::addSibling(HeapNode * node)
{
	HeapNode * temp = rightMostSibling();
	if(temp == nullptr) return false;

	temp->rightSibling = node;
	node->leftSibling = temp;
	node->parent = this->parent;
	node->rightSibling = nullptr;

	if(parent) parent->rank++;

	return true;
}

bool HeapNode::remove()
{
	if(parent)
	{
		parent->rank--;
		if(leftSibling) parent->children = leftSibling;
		else if(rightSibling) parent->children = rightSibling;
		else parent->children = nullptr;
	}

	if(leftSibling) leftSibling->rightSibling = rightSibling;
	if(rightSibling) rightSibling->leftSibling = leftSibling;

	leftSibling = nullptr;
	rightSibling = nullptr;
	parent = nullptr;

	return true;
}

HeapNode * HeapNode::leftMostSibling()
{
	if(this == nullptr) return nullptr;

	HeapNode * temp = this;
	while(temp->leftSibling) temp = temp->leftSibling;
	return temp;
}

HeapNode * HeapNode::rightMostSibling()
{
	if(this == nullptr) return nullptr;

	HeapNode * temp = this;
	while(temp->rightSibling) temp = temp->rightSibling;
	return temp;
}

// =========================================================================
//	Implementation of class Fibonacci Heap
// =========================================================================

FibonacciHeap::FibonacciHeap(double (*GetHeapKeyMethod)(const HeapDataType *))
{
	this->GetHeapKey = GetHeapKeyMethod;
	minRoot = nullptr;
	nodeTable = new DEBUG_NEW_PLACEMENT HeapNodeTable();
	rootListByRank = new DEBUG_NEW_PLACEMENT HeapNodePtr[100];
	ResetMaxHeapKey();
}

FibonacciHeap::~FibonacciHeap()
{
	Clear();
	delete nodeTable;
	delete [] rootListByRank;
}

bool FibonacciHeap::IsEmpty()
{
	return (minRoot == nullptr);
}

bool FibonacciHeap::Insert(HeapDataType * node)
{
	if(node == nullptr) return false;
	HeapNode * out = nodeTable->Find(node);
	if (out)
	{
		if (GetHeapKey(node) < out->key) return (DecreaseKey(node) == S_OK);
		else return false;
	}
	else
	{
		HeapNode * n = new DEBUG_NEW_PLACEMENT HeapNode(node, GetHeapKey(node));
		maxHeakKeyValue = max(maxHeakKeyValue, n->key);

		if(minRoot == nullptr) minRoot = n;
		else
		{
			minRoot->addSibling(n);
			if(n->key < minRoot->key) minRoot = n;
		}

		nodeTable->Insert(n);
		return true;
	}
}

HeapDataType * FibonacciHeap::FindMin()
{
	return minRoot->data;
}

HeapDataType * FibonacciHeap::DeleteMin()
{
	HeapNode * temp = minRoot->children->leftMostSibling();
	HeapNode * nextTemp = nullptr;

	// Adding Children to root list
	while(temp != nullptr)
	{
		nextTemp = temp->rightSibling; // Save next Sibling
		temp->remove();
		minRoot->addSibling(temp);
		temp = nextTemp;
	}

	// Select the left-most sibling of minRoot
	temp = minRoot->leftMostSibling();

	// Remove minRoot and set it to any sibling, if there exists one
	if(temp == minRoot)
	{
		if(minRoot->rightSibling) temp = minRoot->rightSibling;
		else
		{
			// Heap is obviously empty
			HeapDataType * out = minRoot->data;
			minRoot->remove();
			delete minRoot;
			minRoot = nullptr;
			nodeTable->Erase(out);
			return out;
		}
	}
	HeapDataType * out = minRoot->data;
	minRoot->remove();
	delete minRoot;
	nodeTable->Erase(out);
	minRoot = temp;

	// Initialize list of roots
	for(int i = 0; i < 100; i++) rootListByRank[i] = nullptr;

	while(temp)
	{
		// Check if key of current vertex is smaller than the key of minRoot
		if(temp->key < minRoot->key) minRoot = temp;

		nextTemp = temp->rightSibling;
		link(temp);
		temp = nextTemp;
	}
	return out;
}

bool FibonacciHeap::IsVisited(HeapDataType * vertex)
{
	HeapNode * out = nodeTable->Find(vertex);
	return out != nullptr;
}

HRESULT FibonacciHeap::DecreaseKey(HeapDataType * edge)
{
	HeapNode * node = nodeTable->Find(edge);
	if (!node)
	{
		Insert(edge);
		return S_OK;
	}
	else
	{
		node->data = edge;
		node->key = GetHeapKey(edge);
		if(node->parent != nullptr) // The vertex has a parent
		{
			// Remove vertex and add to root list
			node->remove();
			minRoot->addSibling(node);
		}
		// Check if key is smaller than the key of minRoot
		if(node->key < minRoot->key) minRoot = node;
		return S_OK;
	}
}

void FibonacciHeap::Clear()
{
	while (!IsEmpty()) DeleteMin();
	nodeTable->Clear();
}

bool FibonacciHeap::link(HeapNode * root)
{
	// Insert Vertex into root list
	if(rootListByRank[root->rank] == nullptr)
	{
		rootListByRank[root->rank] = root;
		return false;
	}
	else
	{
		// Link the two roots
		HeapNode * linkVertex = rootListByRank[root->rank];
		rootListByRank[root->rank] = nullptr;

		if((root->key < linkVertex->key) || root == minRoot)
		{
			linkVertex->remove();
			root->addChild(linkVertex);
			if(rootListByRank[root->rank] != nullptr) link(root);
			else rootListByRank[root->rank] = root;
		}
		else
		{
			root->remove();
			linkVertex->addChild(root);
			if(rootListByRank[linkVertex->rank] != nullptr) link(linkVertex);
			else rootListByRank[linkVertex->rank] = linkVertex;
		}
		return true;
	}
}

// =========================================================================
//	Implementation of class HeapNodeTable
// =========================================================================

void HeapNodeTable::Erase(HeapDataType * edge)
{
	if (edge->Direction == esriNEDAlongDigitized) cacheAlong->erase(edge->EID);
	else cacheAgainst->erase(edge->EID);
}

void HeapNodeTable::Insert(HeapNodePtr node)
{
	if (node->data->Direction == esriNEDAlongDigitized) cacheAlong->insert(std::pair<long, HeapNodePtr>(node->data->EID, node));
	else cacheAgainst->insert(std::pair<long, HeapNodePtr>(node->data->EID, node));
}

HeapNodePtr HeapNodeTable::Find(HeapDataType * edge) const
{
	std::unordered_map<long, HeapNodePtr> * cache = nullptr;
	if (edge->Direction == esriNEDAlongDigitized) cache = cacheAlong;
	else cache  = cacheAgainst;

	std::unordered_map<long, HeapNodePtr>::const_iterator it = cache->find(edge->EID);
	HeapNodePtr o = nullptr;
	if (it != cache->end()) o = it->second;
	return o;
}
