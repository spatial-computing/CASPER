// ===============================================================================================
// Evacuation Solver: Graph vertex Definition
// Description: Definition of a vertex in graph (road network) along with requiered containers
// and h-value structure
//
// Copyright (C) 2014 Kaveh Shahabi
// Distributed under the Apache Software License, Version 2.0. (See accompanying file LICENSE.txt)
//
// Author: Kaveh Shahabi
// URL: http://github.com/spatial-computing/CASPER
// ===============================================================================================

#pragma once

#include "StdAfx.h"
#include "utils.h"

class Evacuee;
class NAEdge;

// The NAVertex class is what sits on top of the INetworkJunction interface and holds extra
// information about each junction/vertex which are helpful for CASPER algorithm.
// g: cost from source
// h: estimated cost to nearest safe zone
// Edge: the previous edge leading to this vertex
// Previous: the previous vertex leading to this vertex

struct HValue
{
public:
	double   Value;
	long     EdgeID;

	HValue(long edgeID, double value)
	{
		EdgeID = edgeID;
		Value = value;
	}

	static inline bool LessThan(HValue & a, HValue & b)
	{
		return a.Value < b.Value;
	}
};

class NAVertex
{
private:
	NAEdge * BehindEdge;
	MinimumArrayList<long, double> * h;
	bool     isShadowCopy;

public:
	double GVal;
	double GlobalPenaltyCost;
	INetworkJunctionPtr Junction;
	NAVertex * Previous;
	long EID;
	bool ParentCostIsDecreased;

	double GetMinHOrZero() const { return h->GetMinValueOrDefault(0.0); }
	double GetH(long eid) const { return h->GetByKey(eid); }
	size_t HCount() const { return h->size(); }

	inline void SetBehindEdge(NAEdge * behindEdge);
	NAEdge * GetBehindEdge() { return BehindEdge; }
	inline bool IsHEmpty()   const { return h->empty(); }
	void UpdateHeuristic(long edgeid, double hur);
	void UpdateYourHeuristic();

	inline void Clone (NAVertex * cpy);
	NAVertex(void);
	NAVertex(const NAVertex& cpy) = delete;
	NAVertex & operator=(const NAVertex &) = delete;
	NAVertex(INetworkJunctionPtr junction, NAEdge * behindEdge);
	virtual ~NAVertex(void) { if (!isShadowCopy) delete h; }
};

typedef NAVertex * NAVertexPtr;
typedef std::unordered_map<long, NAVertexPtr> NAVertexTable;
typedef std::unordered_map<long, NAVertexPtr>::_Pairib NAVertexTableInsertReturn;
typedef std::unordered_map<long, NAVertexPtr>::const_iterator NAVertexTableItr;
typedef std::pair<long, NAVertexPtr> _NAVertexTablePair;
#define NAVertexTablePair(a) _NAVertexTablePair(a->EID, a)

// This collection object has two jobs:
// it makes sure that there exist only one copy of a vertex in it that is connected to each INetworkJunction.
// this will be helpful to avoid duplicate copies pointing to the same junction structure. So data attached
// to vertex will be always fresh and there will be no inconsistency. Care has to be taken not to overwrite
// important vertices with new ones. The second job is just a GC. since all vertices are being created here,
// it can all be deleted at the end here as well.

#define NAVertexCache_BucketSize 500

class NAVertexCache
{
private:
	NAVertexTable * cache;
	std::vector<NAVertex *> * bucketCache;
	NAVertex * currentBucket;
	size_t currentBucketIndex;
	double heuristicForOutsideVertices;

public:
	NAVertexCache(const NAVertexCache & that) = delete;
	NAVertexCache & operator=(const NAVertexCache &) = delete;

	NAVertexCache(void)
	{
		cache = new DEBUG_NEW_PLACEMENT std::unordered_map<long, NAVertexPtr>();
		bucketCache = new DEBUG_NEW_PLACEMENT std::vector<NAVertex *>();
		heuristicForOutsideVertices = 0.0;
		currentBucket = nullptr;
		currentBucketIndex = 0;
	}

	virtual ~NAVertexCache(void)
	{
		Clear();
		delete cache;
		delete bucketCache;
	}

	void PrintVertexHeuristicFeq();
	NAVertexPtr New(INetworkJunctionPtr junction, INetworkQueryPtr ipNetworkQuery = nullptr);
	void UpdateHeuristicForOutsideVertices(double hur, bool goDeep);
	NAVertexPtr Get(long eid);
	NAVertexPtr Get(INetworkJunctionPtr junction);
	NAVertexPtr NewFromBucket(NAVertexPtr clone);
	void Clear();
	void CollectAndRelease();
};

class NAVertexCollector
{
private:
	std::vector<NAVertexPtr> * cache;

public:
	NAVertexCollector(const NAVertexCollector & that) = delete;
	NAVertexCollector & operator=(const NAVertexCollector &) = delete;

	NAVertexCollector(void)
	{
		cache = new DEBUG_NEW_PLACEMENT std::vector<NAVertexPtr>();
	}

	virtual ~NAVertexCollector(void)
	{
		Clear();
		delete cache;
	}

	NAVertexPtr New(INetworkJunctionPtr junction);
	size_t Size() { return cache->size(); }
	void Clear();
};
