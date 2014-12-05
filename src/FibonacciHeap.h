// ====================================================================
//
//	Description
//
//	Written by: Kaveh Shahabi
//  URL
// ====================================================================

#pragma once

#include "StdAfx.h"

template<class T>
struct FibNode
{
	T data;
	double key;

	FibNode(const T & _data, double _key) : data(_data), key(_key) { }
	FibNode(const FibNode<T> & that) : data(that.data), key(that.key) { }

	// very important that we return greater than so that the hap act like a min heap. Internally it's designed to be a max heap
	bool operator<(const FibNode<T> & rhs) const { return key > rhs.key; }
	
	FibNode<T> & operator=(const FibNode<T> & rhs)
	{
		data = rhs.data;
		key = rhs.key;
		return *this;
	}
};

template <class T> double DefaultGetHeapKey(const T & value) { return (double)value; }

template<class T, typename Hasher = std::hash<T>, typename TEq = std::equal_to<T>>
class MyFibonacciHeap : protected boost::heap::fibonacci_heap<FibNode<T>>
{
private:
	typedef boost::heap::fibonacci_heap<FibNode<T>> baseheap;
	std::unordered_map<T, typename baseheap::handle_type, Hasher, TEq> nodeTable;
	std::function<double(const T &)> GetHeapKey;

public:
	using baseheap::size;
	using baseheap::empty;

	MyFibonacciHeap(const std::function<double(const T &)> & _getHeapKey = DefaultGetHeapKey<T>) : GetHeapKey(_getHeapKey) { }
	bool IsVisited(const T & node) { return nodeTable.find(node) != nodeTable.end(); }
	void Clear() { baseheap::clear(); nodeTable.clear(); }

	void Insert(const T & value)
	{
		if (nodeTable.find(value) != nodeTable.end()) throw std::logic_error("node already exists in heap");
		else nodeTable.insert(std::pair<T, typename baseheap::handle_type>(value, baseheap::push(FibNode<T>(value, GetHeapKey(value)))));
	}
	
	void UpdateKey(const T & value)
	{
		auto i = nodeTable.find(value);
		if (i == nodeTable.end()) throw std::logic_error("node does not exist in heap");
		else baseheap::update_lazy(i->second, FibNode<T>(value, GetHeapKey(value)));
	}

	T DeleteMin()
	{
		if (baseheap::empty()) throw std::logic_error("heap is empty");
		auto ret = baseheap::top();
		baseheap::pop();
		nodeTable.erase(ret.data);
		return ret.data;
	}
};
