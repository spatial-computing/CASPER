// ====================================================================
//
//	A network optimization algorithm using Fibonacci Heap
//
//	Written by: Max Winkler
//  http://www.codeproject.com/KB/recipes/Dijkstras_Algorithm.aspx
// ====================================================================

#pragma once

#include "StdAfx.h"

template<class T>
class HeapNode
{
public:
	HeapNode     * parent;
	HeapNode     * leftSibling, * rightSibling;
	HeapNode     * children;
	T            data;
	double       key;
	int          rank;

	HeapNode(const T & _data, double _key) : data(_data), key(_key)
	{
		parent = nullptr;
		children = nullptr;
		leftSibling = nullptr;
		rightSibling = nullptr;
		rank = 0;
	}

	HeapNode()
	{
		this->data = T();
		this->key = 0.0;
		parent = nullptr;
		children = nullptr;
		leftSibling = nullptr;
		rightSibling = nullptr;
		rank = 0;
	}

	bool addChild(HeapNode * node)
	{
		if (children) children->addSibling(node);
		else
		{
			children = node;
			node->parent = this;
			rank = 1;
		}
		return true;
	}

	bool addSibling(HeapNode * node)
	{
		HeapNode * temp = rightMostSibling();
		if (!temp) return false;

		temp->rightSibling = node;
		node->leftSibling = temp;
		node->parent = this->parent;
		node->rightSibling = nullptr;

		if (parent) parent->rank++;

		return true;
	}

	bool remove()
	{
		if (parent)
		{
			parent->rank--;
			if (leftSibling) parent->children = leftSibling;
			else if (rightSibling) parent->children = rightSibling;
			else parent->children = nullptr;
		}

		if (leftSibling) leftSibling->rightSibling = rightSibling;
		if (rightSibling) rightSibling->leftSibling = leftSibling;

		leftSibling = nullptr;
		rightSibling = nullptr;
		parent = nullptr;

		return true;
	}

	HeapNode * leftMostSibling()
	{
		HeapNode * temp = this;
		while (temp->leftSibling) temp = temp->leftSibling;
		return temp;
	}

	HeapNode * rightMostSibling()
	{
		HeapNode * temp = this;
		while (temp->rightSibling) temp = temp->rightSibling;
		return temp;
	}
};

template<class T, typename Hasher = std::hash<T>, typename TEq = std::equal_to<T>>
class FibonacciHeap
{
private:
	typedef std::unordered_map<T, HeapNode<T> *, Hasher, TEq> table;

	double maxHeakKeyValue;
	HeapNode<T> ** rootListByRank;
	HeapNode<T> * minRoot;
	table * nodeTable;
	std::function<double(const T &)> GetHeapKey;

	bool link(HeapNode<T> * root)
	{
		// Insert Vertex into root list
		if (!rootListByRank[root->rank])
		{
			rootListByRank[root->rank] = root;
			return false;
		}
		else
		{
			// Link the two roots
			HeapNode<T> * linkVertex = rootListByRank[root->rank];
			rootListByRank[root->rank] = nullptr;

			if ((root->key < linkVertex->key) || root == minRoot)
			{
				linkVertex->remove();
				root->addChild(linkVertex);
				if (rootListByRank[root->rank]) link(root);
				else rootListByRank[root->rank] = root;
			}
			else
			{
				root->remove();
				linkVertex->addChild(root);
				if (rootListByRank[linkVertex->rank]) link(linkVertex);
				else rootListByRank[linkVertex->rank] = linkVertex;
			}
			return true;
		}
	}

public:
	bool IsVisited(const T & node)   { return nodeTable->find(node) != nodeTable->end(); }
	double GetMaxHeapKey(void) const { return maxHeakKeyValue; }
	void ResetMaxHeapKey(void)       { maxHeakKeyValue = 0.0;  }
	bool IsEmpty()                   { return (minRoot == nullptr); }
	size_t Size() const              { return nodeTable->size(); }
	T FindMin()                      { return minRoot->data; }

	FibonacciHeap(const FibonacciHeap<T, Hasher, TEq> & that) = delete;
	FibonacciHeap<T, Hasher, TEq> & operator=(const FibonacciHeap<T, Hasher, TEq> &) = delete;

	FibonacciHeap(const std::function<double(const T &)> & _getHeapKey) : GetHeapKey(_getHeapKey)
	{
		minRoot = nullptr;
		nodeTable = new DEBUG_NEW_PLACEMENT table();
		rootListByRank = new DEBUG_NEW_PLACEMENT HeapNode<T>*[100];
		ResetMaxHeapKey();
	}

	virtual ~FibonacciHeap()
	{
		Clear();
		delete nodeTable;
		delete [] rootListByRank;
	}
	
	void Clear()
	{
		while (!IsEmpty()) DeleteMin();
		nodeTable->clear();
	}

	bool Insert(const T & value)
	{
		auto i = nodeTable->find(value);
		if (i != nodeTable->end())
		{
			auto out = i->second;
			if (GetHeapKey(value) < out->key) return DecreaseKey(value);
			else return false;
		}
		else
		{
			auto node = new DEBUG_NEW_PLACEMENT HeapNode<T>(value, GetHeapKey(value));
			maxHeakKeyValue = max(maxHeakKeyValue, node->key);

			if (!minRoot) minRoot = node;
			else
			{
				minRoot->addSibling(node);
				if (node->key < minRoot->key) minRoot = node;
			}

			nodeTable->insert(std::pair<T, HeapNode<T> *>(value, node));
			return true;
		}
	}

	bool DecreaseKey(const T & value)
	{
		auto i = nodeTable->find(value);
		if (i == nodeTable->end()) return Insert(value);
		else
		{
			auto node = i->second;
			node->data = value;
			node->key = GetHeapKey(value);
			if (node->parent) // The vertex has a parent
			{
				// Remove vertex and add to root list
				node->remove();
				minRoot->addSibling(node);
			}
			// Check if key is smaller than the key of minRoot
			if (node->key < minRoot->key) minRoot = node;
			return true;
		}
	}

	T DeleteMin()
	{
		HeapNode<T> * temp = minRoot->children ? minRoot->children->leftMostSibling() : nullptr;
		HeapNode<T> * nextTemp = nullptr;

		// Adding Children to root list
		while (temp)
		{
			nextTemp = temp->rightSibling; // Save next Sibling
			temp->remove();
			minRoot->addSibling(temp);
			temp = nextTemp;
		}

		// Select the left-most sibling of minRoot
		temp = minRoot->leftMostSibling();

		// Remove minRoot and set it to any sibling, if there exists one
		if (temp == minRoot)
		{
			if (minRoot->rightSibling) temp = minRoot->rightSibling;
			else
			{
				// Heap is obviously empty
				T out = minRoot->data;
				minRoot->remove();
				delete minRoot;
				minRoot = nullptr;
				nodeTable->erase(out);
				return out;
			}
		}
		T out = minRoot->data;
		minRoot->remove();
		delete minRoot;
		nodeTable->erase(out);
		minRoot = temp;

		// Initialize list of roots
		for (int i = 0; i < 100; i++) rootListByRank[i] = nullptr;

		while (temp)
		{
			// Check if key of current vertex is smaller than the key of minRoot
			if (temp->key < minRoot->key) minRoot = temp;

			nextTemp = temp->rightSibling;
			link(temp);
			temp = nextTemp;
		}
		return out;
	}
};
