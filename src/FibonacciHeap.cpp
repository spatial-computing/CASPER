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

HeapNode::HeapNode(HeapDataType * data)
{
	this->data = data;
	parent = NULL;
	children = NULL;
	leftSibling = NULL;
	rightSibling = NULL;
	rank = 0;
}

HeapNode::HeapNode()
{
	this->data = 0;
	parent = NULL;
	children = NULL;
	leftSibling = NULL;
	rightSibling = NULL;
	rank = 0;
}

bool HeapNode::addChild(HeapNode * node)
{
	if(children != NULL)
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
	if(temp == NULL) return false;

	temp->rightSibling = node;
	node->leftSibling = temp;
	node->parent = this->parent;
	node->rightSibling = NULL;

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
		else parent->children = NULL;
	}	
	
	if(leftSibling) leftSibling->rightSibling = rightSibling;
	if(rightSibling) rightSibling->leftSibling = leftSibling;
	
	leftSibling = NULL;
	rightSibling = NULL;
	parent = NULL;

	return true;
}

HeapNode * HeapNode::leftMostSibling()
{
	if(this == NULL) return NULL;

	HeapNode * temp = this;
	while(temp->leftSibling) temp = temp->leftSibling;
	return temp;
}

HeapNode * HeapNode::rightMostSibling()
{
	if(this == NULL) return NULL;

	HeapNode * temp = this;
	while(temp->rightSibling) temp = temp->rightSibling;
	return temp;
}

// =========================================================================
//	Implementation of class Fibonacci Heap
// =========================================================================

FibonacciHeap::FibonacciHeap(bool (*LessThanMethod)(NAEdge *, NAEdge *))
{
	this->LessThan = LessThanMethod;
	minRoot = NULL;
	nodeTable = new HeapNodeTable();
	rootListByRank = new HeapNodePtr[100];
}

/*
FibonacciHeap::FibonacciHeap(HeapDataType * root, (*LessThan)(NAEdge *, NAEdge *) LessThanMethod)
{
	this->LessThan = LessThanMethod;
	nodeTable = new HeapNodeTable();
	this->minRoot = new HeapNode(root);
	rootListByRank = new HeapNodePtr[100];
	minRoot->parent = NULL;
	minRoot->children = NULL;
	minRoot->leftSibling = NULL;
	minRoot->rightSibling = NULL;
	minRoot->rank = 0;
	nodeTable->Insert(minRoot);
}
*/

FibonacciHeap::~FibonacciHeap()
{
	Clear();
	delete nodeTable;
	delete [] rootListByRank;
}

bool FibonacciHeap::IsEmpty()
{
	return (minRoot == NULL);
}

bool FibonacciHeap::Insert(HeapDataType * node)
{
	if(node == NULL) return false;
	HeapNode * out = nodeTable->Find(node);
	if (out)
	{
		if ((*LessThan)(node, out->data)) return (DecreaseKey(node) == S_OK);
		else return false;
	}
	else
	{
		HeapNode * n = new HeapNode(node);

		if(minRoot == NULL) minRoot = n;
		else
		{
			minRoot->addSibling(n);
			if((*LessThan)(n->data, minRoot->data)) minRoot = n;
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
	HeapNode * nextTemp = NULL;

	// Adding Children to root list		
	while(temp != NULL)
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
			minRoot = NULL;
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
	for(int i = 0; i < 100; i++) rootListByRank[i] = NULL;
	
	while(temp)
	{
		// Check if key of current vertex is smaller than the key of minRoot
		if((*LessThan)(temp->data,minRoot->data)) minRoot = temp;		

		nextTemp = temp->rightSibling;		
		link(temp);
		temp = nextTemp;
	}
	return out;	
}

bool FibonacciHeap::IsVisited(HeapDataType * vertex)
{
	HeapNode * out = nodeTable->Find(vertex);
	//// HeapDataType * out = 0;
	//HeapNodeTableItr it = nodeTable->Find(EID);
	//return /* if (*/ it != nodeTable->end() /* ) out = it->second->data */;
	return out != 0;
}

HRESULT FibonacciHeap::DecreaseKey(HeapDataType * vertex)
{
	HeapNode * node = nodeTable->Find(vertex);
	if (!node)
	{
		Insert(vertex);
		return S_OK;
	}
	else 
	{
		node->data = vertex;
		if(node->parent != NULL) // The vertex has a parent
		{
			// Remove vertex and add to root list
			node->remove();
			minRoot->addSibling(node);		
		}
		// Check if key is smaller than the key of minRoot
		if((*LessThan)(node->data, minRoot->data)) minRoot = node;
		return S_OK;
	}
}

void FibonacciHeap::Clear()
{
	while (!IsEmpty()) DeleteMin();	
	nodeTable->Clear();
	// visitTable->clear();
}

bool FibonacciHeap::link(HeapNode * root)
{
	// Insert Vertex into root list
	if(rootListByRank[root->rank] == NULL)
	{
		rootListByRank[root->rank] = root;
		return false;
	}
	else
	{
		// Link the two roots
		HeapNode * linkVertex = rootListByRank[root->rank];
		rootListByRank[root->rank] = NULL;
		
		if((*LessThan)(root->data, linkVertex->data) || root == minRoot)
		{
			linkVertex->remove();
			root->addChild(linkVertex);
			if(rootListByRank[root->rank] != NULL) link(root);
			else rootListByRank[root->rank] = root;
		}
		else
		{
			root->remove();
			linkVertex->addChild(root);
			if(rootListByRank[linkVertex->rank] != NULL) link(linkVertex);
			else rootListByRank[linkVertex->rank] = linkVertex;
		}
		return true;
	}
}

void HeapNodeTable::Erase(NAEdge * edge)
{
	if (edge->Direction == esriNEDAlongDigitized) cacheAlong->erase(edge->EID);
	else cacheAgainst->erase(edge->EID);
}

void HeapNodeTable::Insert(HeapNodePtr node)
{
	if (node->data->Direction == esriNEDAlongDigitized) cacheAlong->insert(std::pair<long, HeapNodePtr>(node->data->EID, node));
	else cacheAgainst->insert(std::pair<long, HeapNodePtr>(node->data->EID, node));
}

HeapNodePtr HeapNodeTable::Find(NAEdge * edge)
{
	stdext::hash_map<long, HeapNodePtr> * cache = 0;
	if (edge->Direction == esriNEDAlongDigitized) cache = cacheAlong;
	else cache  = cacheAgainst;

	stdext::hash_map<long, HeapNodePtr>::iterator it = cache->find(edge->EID);
	HeapNodePtr o = 0;
	if (it != cache->end()) o = it->second;
	return o;
}
