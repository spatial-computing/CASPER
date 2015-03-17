// ===============================================================================================
// Evacuation Solver: Utility classes and functions
// Description: This is almost like my own mini STL library for internal use
//
// Copyright (C) 2014 Kaveh Shahabi
// Distributed under the Apache Software License, Version 2.0. (See accompanying file LICENSE.txt)
//
// Author: Kaveh Shahabi
// URL: http://github.com/spatial-computing/CASPER
// ===============================================================================================

#pragma once

#include "StdAfx.h"

#ifndef CASPER_INFINITY
#define CASPER_INFINITY 3.402823466e+38
#endif

enum class EdgeDirtyState      : unsigned char { CleanState = 0x0, CostIncreased = 0x1, CostDecreased = 0x2 };
enum class NAEdgeMapGeneration : unsigned char { None = 0x0, OldGen = 0x1, NewGen = 0x2, AllGens = 0x3 };
enum class EvacueeStatus       : unsigned char { Unprocessed = 0x0, Processed = 0x1, Unreachable = 0x2, CARMALooking = 0x3 };
enum class QueryDirection      : unsigned char { Forward = 0x1, Backward = 0x2 };
enum class FlockingStatus      : unsigned char { None = '\0', Init = 'I', Moving = 'M', End = 'E', Stopped = 'S', Collided = 'C' };
enum class EdgeDirection       : unsigned char { None = 0x0, Along = 0x1, Against = 0x2, Both = 0x3 };
enum class PathStatus          : unsigned char { ActiveComplete = 0x0, FrozenComplete = 0x1, FrozenSplitted = 0x2 };

[export, uuid("096CB996-9144-4CC3-BB69-FCFAA5C273FC")] enum class EvcSolverMethod : unsigned char { SPSolver = 0x0, CCRPSolver = 0x1, CASPERSolver = 0x2 };
[export, uuid("BFDD2DB3-DA25-42CA-8021-F67BF7D14948")] enum class EvcTrafficModel : unsigned char { FLATModel = 0x0, STEPModel = 0x1, LINEARModel = 0x2, POWERModel = 0x3, EXPModel = 0x4 };
[export, uuid("C46A6356-07A6-473A-B39F-FBB74469201D")] enum class EvacueeGrouping : unsigned char { None = 0x0, Merge = 0x1, Separate = 0x2, MergeSeparate = 0x3 };
[export, uuid("1B84C35A-9585-49DA-9B81-BB4873E8D331")] enum class DynamicMode     : unsigned char { Disabled = 0x0, Simple = 0x1, Full = 0x2, Smart = 0x3 };

// enum for carma sort setting
[export, uuid("AAC29CC5-80A9-454A-984B-43525917E53B")] enum CARMASort : unsigned char
{ Nothing = 0x0, FWSingle = 0x1, FWCont = 0x2, BWSingle = 0x3, BWCont = 0x4, ReverseFinalCost = 0x5, ReverseEvacuationCost = 0x6 };

DEFINE_ENUM_FLAG_OPERATORS(NAEdgeMapGeneration)
DEFINE_ENUM_FLAG_OPERATORS(EvacueeGrouping)
DEFINE_ENUM_FLAG_OPERATORS(EdgeDirection)
template <class T> inline bool CheckFlag(T var, T flag) { return (var & flag) != T::None; }
/*
#define FLOCK_OBJ_STAT char
#define FLOCK_OBJ_STAT_INIT		0x0
#define FLOCK_OBJ_STAT_MOVE		0x1
#define FLOCK_OBJ_STAT_STOP		0x2
#define FLOCK_OBJ_STAT_COLLID	0x3
#define FLOCK_OBJ_STAT_END		0x4
*/
#define FLOCK_PROFILE char
#define FLOCK_PROFILE_CAR		0x0
#define FLOCK_PROFILE_PERSON	0x1
#define FLOCK_PROFILE_BIKE		0x2

// utility functions
#define DoubleRangedRand(range_min, range_max)	((double)(rand()) * ((range_max) - (range_min)) / (RAND_MAX + 1.0) + (range_min))

template <class T, class S = UINT8, S ZeroSize = 0>
class ArrayList
{
private:
	T * data;
	S  mySize;
	inline void check(S index) const { if (index >= mySize || index < ZeroSize) throw std::out_of_range("index is out of range for ArrayList"); }

public:
	ArrayList(S size = ZeroSize) : mySize(size), data(nullptr) { Init(size); }
	virtual ~ArrayList() { if (data) delete [] data; }
	ArrayList & operator=(const ArrayList &) = delete;

	ArrayList(const ArrayList & that) : mySize(that.mySize), data(nullptr)
	{
		Init(mySize);
		for (S i = ZeroSize; i < mySize; ++i) data[i] = that.data[i];
	}

	inline S    size()  const { return mySize; }
	inline bool empty() const { return mySize == ZeroSize; }

	inline void at(S index, const T & item)    { check(index); data[index] = item; }
	inline T &  at(S index)                    { check(index); return data[index]; }
	inline T &  operator[](S index)            { check(index); return data[index]; }
	inline const T & at(S index) const         { check(index); return data[index]; }
	inline const T & operator[](S index) const { check(index); return data[index]; }
	
	void Init(S size)
	{
		mySize = size;
		if (data) delete[] data;
		if (mySize > ZeroSize) data = new DEBUG_NEW_PLACEMENT T[mySize]; else data = nullptr;
	}

	class Const_Iterator
	{
	private:
		const ArrayList<T, S, ZeroSize> & myList;
		S myIndex;

	public:
		Const_Iterator(const ArrayList<T, S, ZeroSize> & list, S index = ZeroSize) : myList(list), myIndex(index) { };
		Const_Iterator & operator++() { ++myIndex; return *this; }
		const T & operator*() const { return myList[myIndex]; }
		bool operator==(Const_Iterator const & rhs) const { return &myList == &(rhs.myList) && myIndex == rhs.myIndex; }
		bool operator!=(Const_Iterator const & rhs) const { return !(*this == rhs); }
	};

	Const_Iterator begin() const { return Const_Iterator(*this);         }
	Const_Iterator end()   const { return Const_Iterator(*this, mySize); }
};

template <class T, class S = UINT8, S ZeroSize = 0>
class GrowingArrayList
{
protected:
	S capacity;
	T * data;
	S  _size;
	inline void readcheck(S index) const { if (index >= _size || index < ZeroSize) throw std::out_of_range("read index is out of range for GrowingArrayList"); }

	void grow(S newCap)
	{
		// we need to grow
		if (newCap > capacity) shrink_or_grow(newCap);
	}

	void shrink(S newCap)
	{
		// check if we can shrink
		if (newCap < capacity && newCap >= _size) shrink_or_grow(newCap);
	}

	void shrink_or_grow(S newCap)
	{
		if (capacity == ZeroSize) data = new DEBUG_NEW_PLACEMENT T[newCap];
		else
		{
			S range = min(newCap, capacity);
			T * tempdata = new DEBUG_NEW_PLACEMENT T[newCap];
			for (S i = ZeroSize; i < range; ++i) tempdata[i] = data[i];
			delete [] data;
			data = tempdata;
		}
		capacity = newCap;
	}

public:
	GrowingArrayList(S cap = ZeroSize) : _size(ZeroSize), data(nullptr), capacity(ZeroSize) { grow(cap); }
	virtual ~GrowingArrayList() { if (data) delete[] data; }
	GrowingArrayList & operator=(const GrowingArrayList &) = delete;

	GrowingArrayList(const GrowingArrayList & that) : _size(that._size), capacity(ZeroSize), data(nullptr)
	{
		shrink_or_grow(that.capacity);
		for (S i = ZeroSize; i < _size; ++i) data[i] = that.data[i];
	}

	inline S    size()  const { return _size; }
	inline bool empty() const { return _size == ZeroSize; }
	inline void clear() { _size = ZeroSize; }
	inline void shrink_to_fit() { shrink(_size); }

	inline T &  at(S index)                    { readcheck(index); return data[index]; }
	inline T &  operator[](S index)            { readcheck(index); return data[index]; }
	inline const T & at(S index) const         { readcheck(index); return data[index]; }
	inline const T & operator[](S index) const { readcheck(index); return data[index]; }

	void           erase(const T & item, std::function<bool(const T &, const T &)> isEqual) {           erase(find(item, isEqual)); }
	void unordered_erase(const T & item, std::function<bool(const T &, const T &)> isEqual) { unordered_erase(find(item, isEqual)); }

	virtual void push_back(const T & item)
	{
		grow(_size + 1);
		data[_size] = item;
		++_size;
	}

	S find(const T & item, std::function<bool(const T &, const T &)> isEqual) const
	{
		for (S i = ZeroSize; i < _size; ++i) if (isEqual(item, data[i])) return i;
		throw std::out_of_range("item not found in GrowingArrayList");
	}

	void erase(S index)
	{
		readcheck(index);
		for (S i = index + 1; i < _size; ++i) data[i - 1] = data[i];
		--_size;
	}

	void unordered_erase(S index)
	{
		readcheck(index);
		data[index] = data[--_size];
	}

	/// TODO this iterator is still bugy if you use it with std::sort ... or maybe other STL functions. Be careful.
	class iterator : public virtual std::iterator<std::random_access_iterator_tag, T>
	{
	private:
		GrowingArrayList<T, S, ZeroSize> & myList;
		S myIndex;

	public:
		iterator(GrowingArrayList<T, S, ZeroSize> & list, S index = ZeroSize) : myList(list), myIndex(index) { };
		iterator(const iterator & copy) : myList(copy.myList), myIndex(copy.myIndex) { };
		
		iterator & operator=(const iterator & rhs)
		{
			if (&myList != &(rhs.myList)) throw std::logic_error("operator call on different iterators");
			myIndex = rhs.myIndex;
			return *this;
		}

		bool operator<(const iterator & rhs) const
		{
			if (&myList != &(rhs.myList)) throw std::logic_error("operator call on different iterators");
			return myIndex < rhs.myIndex;
		}

		bool operator>(const iterator & rhs) const
		{
			if (&myList != &(rhs.myList)) throw std::logic_error("operator call on different iterators");
			return myIndex > rhs.myIndex;
		}

		iterator & operator++() { ++myIndex; return *this; }
		iterator operator++(int){ ++myIndex; return *this; }
		iterator & operator--() { --myIndex; return *this; }
		T & operator*() { return myList[myIndex]; }
		bool operator==(iterator const & rhs) const { return &myList == &(rhs.myList) && myIndex == rhs.myIndex; }
		bool operator!=(iterator const & rhs) const { return !(*this == rhs); }

		// random access operators
		iterator & operator+=(S n) { myIndex += n; return *this; }
		iterator & operator-=(S n) { myIndex -= n; return *this; }
		iterator operator+(S n) const { return iterator(myList, myIndex + n); }
		iterator operator-(S n) const { return iterator(myList, myIndex - n); }
		friend iterator operator+(S n, iterator const & rhs) { return iterator(rhs.myList, rhs.myIndex + n); }
		friend iterator operator-(S n, iterator const & rhs) { return iterator(rhs.myList, rhs.myIndex - n); }
		friend difference_type operator-(iterator const & lhs, iterator const & rhs) { return lhs.myIndex - rhs.myIndex; }
	};

	class const_iterator
	{
	private:
		const GrowingArrayList<T, S, ZeroSize> & myList;
		S myIndex;

	public:
		const_iterator(const GrowingArrayList<T, S, ZeroSize> & list, S index = ZeroSize) : myList(list), myIndex(index) { };
		const_iterator(const const_iterator & copy) : myList(copy.myList), myIndex(copy.myIndex) { };

		const_iterator & operator++() { ++myIndex; return *this; }
		const_iterator & operator--() { --myIndex; return *this; }
		const T & operator*() const { return myList[myIndex]; }
		bool operator==(const_iterator const & rhs) const { return &myList == &(rhs.myList) && myIndex == rhs.myIndex; }
		bool operator!=(const_iterator const & rhs) const { return !(*this == rhs); }
	};

	const_iterator begin() const { return const_iterator(*this); }
	const_iterator end()   const { return const_iterator(*this, _size); }
	iterator begin()  { return iterator(*this); }
	iterator end()    { return iterator(*this, _size); }
};

template <class T, class S = UINT8, S ZeroSize = 0>
class DoubleGrowingArrayList : public GrowingArrayList<T, S, ZeroSize>
{
public:
	DoubleGrowingArrayList(S cap = ZeroSize) : GrowingArrayList<T, S, ZeroSize>(cap) { }
	virtual void push_back(const T & item)
	{
		if (_size == capacity) { if (_size == 0) grow(2); else grow(_size * 2); }
		data[_size] = item;
		++_size;
	}
};

template <class K, class V, class S = UINT8, S ZeroSize = 0, class KeyEqual = std::equal_to<K>, class CompareValue = std::less<V>>
class MinimumArrayList : protected GrowingArrayList <std::pair<K, V>, S, ZeroSize>
{
private:
	S minValueIndex;
	typedef GrowingArrayList <std::pair<K, V>, S, ZeroSize> baseArray;

public:
	inline S    size()  const { return this->_size; }
	inline bool empty() const { return this->_size == ZeroSize; }

	using baseArray::begin;
	using baseArray::end;
	using baseArray::iterator;

	MinimumArrayList(S cap = ZeroSize) : GrowingArrayList<std::pair<K, V>, S, ZeroSize>(cap), minValueIndex(ZeroSize) { }

	void InsertOrUpdate(const K & key, const V & value)
	{
		CompareValue lessThan;
		KeyEqual keyEqual;
		bool valueIncreased = false;
		S insert = this->_size;
		
		// insert new or update exisiting pair
		for (S i = ZeroSize; i < this->_size; ++i)
			if (keyEqual(key, this->data[i].first))
			{
				insert = i;
				valueIncreased = lessThan(this->data[i].second, value);
				break;
			}
		if (insert == this->_size) baseArray::push_back(std::pair<K, V>(key, value));
		else this->data[insert].second = value;

		// update index of min value
		if (minValueIndex != insert) minValueIndex = lessThan(value, this->data[minValueIndex].second) ? insert : minValueIndex;
		else if (valueIncreased)
		{
			for (S i = ZeroSize; i < this->_size; ++i)
				if (lessThan(this->data[i].second, this->data[minValueIndex].second)) minValueIndex = i;
		}
	}

	const V & GetByKey(const K & key) const
	{
		KeyEqual q;
		for (S i = ZeroSize; i < this->_size; ++i) if (q(key, this->data[i].first)) return this->data[i].second;
		throw std::out_of_range("key not found in MinimumArrayList");
	}

	inline const V & GetMinValueOrDefault(const V & DefaultValue) const
	{
		if (empty()) return DefaultValue;
		return this->data[minValueIndex].second;
	}
};

template <class T, typename _Hasher = std::hash<T>, typename _Keyeq = std::equal_to<T>, typename _Alloc = std::allocator<std::pair<const T, double> >>
class Histogram : protected std::unordered_map<T, double, _Hasher, _Keyeq, _Alloc>
{
private:
	typedef std::unordered_map<T, double, _Hasher, _Keyeq, _Alloc> map;

public:
	double maxWeight;

	using map::size;
	using map::begin;
	using map::end;
	using map::cbegin;
	using map::cend;

	Histogram(size_t capacity = 0) : map(capacity), maxWeight(-CASPER_INFINITY) { }
	void WeightedAdd(const std::vector<T> & list, double weight) { for (const auto & i : list) WeightedAdd(i, weight); }
	virtual ~Histogram() { }

	void WeightedAdd(const T & item, double weight)
	{
		if (map::find(item) == map::end()) map::insert(std::pair<T, double>(item, 0.0));
		double & newWeight = map::at(item);
		newWeight += weight;
		maxWeight = max(maxWeight, newWeight);
	}
};
