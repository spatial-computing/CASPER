// ===============================================================================================
// Evacuation Solver: Graph edge Definition
// Description: Definition of an edge in graph (road network) along with other relates containers
// and edge reservation strucutre
//
// Copyright (C) 2014 Kaveh Shahabi
// Distributed under the Apache Software License, Version 2.0. (See accompanying file LICENSE.txt)
//
// Author: Kaveh Shahabi
// URL: http://github.com/spatial-computing/CASPER
// ===============================================================================================

#pragma once

#include "StdAfx.h"
#include "Evacuee.h"
#include "TrafficModel.h"
#include "utils.h"

class EdgeReservations : private std::vector<EvcPathPtr>
{
private:
	double         ReservedPop;
	double         Capacity;
	EdgeDirtyState dirtyState;
	TrafficModel   * myTrafficModel;

public:
	EdgeReservations(float capacity, TrafficModel * trafficModel);
	EdgeReservations(const EdgeReservations& cpy);
	EdgeReservations & operator=(const EdgeReservations &) = delete;
	void AddReservation(double newFlow, EvcPathPtr path);
	void RemoveReservation(double flow, EvcPathPtr path);
	void SwapReservation(const EvcPathPtr oldPath, const EvcPathPtr newPath);

	friend class NAEdge;
};

typedef EdgeReservations * EdgeReservationsPtr;

// The NAEdge class is what sits on top of the INetworkEdge interface and holds extra
// information about each edge which are helpful for CASPER algorithm.
// Capacity: road initial capacity
// Cost: road initial travel cost
// reservations: a vector of evacuees committed to use this edge at a certain time/cost period
class NAEdge
{
private:
	IGeometryPtr myGeometry;
	EdgeReservations * reservations;
	double CleanCost;
	double GetTrafficSpeedRatio(double allPop, EvcSolverMethod method) const;

public:
	double OriginalCost;
	esriNetworkEdgeDirection Direction;
	NAVertex * ToVertex;
	NAEdge * TreePrevious;
	GrowingArrayList<NAEdge *> TreeNext;
	INetworkEdgePtr NetEdge;
	long EID;
	ArrayList<NAEdge *> AdjacentForward;
	ArrayList<NAEdge *> AdjacentBackward;

	EdgeDirtyState HowDirty(EvcSolverMethod method, double minPop2Route = 1.0, bool exhaustive = false);
	double GetCost(double newPop, EvcSolverMethod method, double * globalDeltaCost = nullptr) const;
	double GetCurrentCost(EvcSolverMethod method = EvcSolverMethod::CASPERSolver) const;
	double LeftCapacity() const;
	bool ApplyNewOriginalCostAndCapacity(double NewOriginalCost, double NewOriginalCapacity, bool DelayHowDirty, EvcSolverMethod method);
	bool IsNewOriginalCostAndCapacityDifferent(double NewOriginalCost, double NewOriginalCapacity) const;

	// Special function for Flocking: to check how much capacity the edge had originally
	double OriginalCapacity() const { return reservations->Capacity; }

	HRESULT QuerySourceStuff(long * sourceOID, long * sourceID, double * fromPosition, double * toPosition) const;
	void AddReservation(EvcPath * path, EvcSolverMethod method, bool delayedDirtyState = false);
	NAEdge(INetworkEdgePtr, long capacityAttribID, long costAttribID, const NAEdge * otherEdge, bool twoWayRoadsShareCap, std::list<EdgeReservationsPtr> & ResTable, TrafficModel * model);
	NAEdge(const NAEdge & cpy);
	NAEdge & operator=(const NAEdge &) = delete;

	inline EdgeDirtyState GetDirtyState() const { return reservations->dirtyState; }
	inline void SetClean(EvcSolverMethod method, double minPop2Route);
	inline double GetCleanCost() const { return CleanCost; }
	double GetReservedPop() const { return reservations->ReservedPop; }
	HRESULT GetGeometry(INetworkDatasetPtr ipNetworkDataset, IFeatureClassContainerPtr ipFeatureClassContainer, bool & sourceNotFoundFlag, IGeometryPtr & geometry);
	void RemoveReservation(EvcPathPtr path, EvcSolverMethod method, bool delayedDirtyState = false);
	void SwapReservation(const EvcPathPtr oldPath, const EvcPathPtr newPath) { reservations->SwapReservation(oldPath, newPath); }
	void GetUniqeCrossingPaths(std::vector<EvcPathPtr> & crossings, bool cleanVectorFirst = false);
	double MaxAddedCostOnReservedPathsWithNewFlow(double deltaCostOfNewFlow, double longestPathSoFar, double currentPathSoFar, double selfishRatio) const;
	HRESULT InsertEdgeToFeatureCursor(INetworkDatasetPtr ipNetworkDataset, IFeatureClassContainerPtr ipFeatureClassContainer, IFeatureBufferPtr ipFeatureBuffer, IFeatureCursorPtr ipFeatureCursor,
									  long eidFieldIndex, long sourceIDFieldIndex, long sourceOIDFieldIndex, long dirFieldIndex, long resPopFieldIndex, long travCostFieldIndex,
									  long orgCostFieldIndex, long congestionFieldIndex, bool & sourceNotFoundFlag);
	
	static void DynamicStep_ExtractAffectedPaths(std::unordered_set<EvcPathPtr, EvcPath::PtrHasher, EvcPath::PtrEqual> & AffectedPaths, const std::unordered_set<NAEdge *, NAEdgePtrHasher, NAEdgePtrEqual> & DynamicallyAffectedEdges);
	static bool CostLessThan(NAEdge * e1, NAEdge * e2, EvcSolverMethod method)
	{
		return e1->GetCurrentCost(method) < e2->GetCurrentCost(method);
	}

	static double GetHeapKeyHur(const NAEdge * e);
	static double GetHeapKeyNonHur(const NAEdge * e);
	static bool   IsEqualNAEdgePtr(const NAEdge * n1, const NAEdge * n2);
	
	template<class iterator_type> static void HowDirtyExhaustive(iterator_type begin, iterator_type end, EvcSolverMethod method, double minPop2Route)
	{
		NAEdgePtr edge = nullptr;
		for (iterator_type i = begin; i != end; ++i)
		{
			edge = *i;
			if (edge->CleanCost <= 0.0) edge->reservations->dirtyState = EdgeDirtyState::CostIncreased;
			else
			{
				edge->reservations->dirtyState = EdgeDirtyState::CleanState;
				double costchange = (edge->GetCost(minPop2Route, method) / edge->CleanCost) - 1.0;
				if (costchange > FLT_EPSILON) edge->reservations->dirtyState = EdgeDirtyState::CostIncreased;
				if (costchange < -FLT_EPSILON) edge->reservations->dirtyState = EdgeDirtyState::CostDecreased;
			}
		}
	}
};

typedef NAEdge * NAEdgePtr;

// hash functor for NAEdgePtr
struct NAEdgePtrHasher : public std::unary_function<NAEdgePtr, size_t>
{
	size_t operator()(const NAEdgePtr & edge) const { return edge->EID; }
};

// equal functor for NAEdgePtr
struct NAEdgePtrEqual : public std::binary_function<NAEdgePtr, NAEdgePtr, bool>
{
	size_t operator()(const NAEdgePtr & left, const NAEdgePtr & right) const { return left->EID == right->EID && left->Direction == right->Direction; }
};

struct NAedgePairHasher : public std::unary_function<std::pair<long, esriNetworkEdgeDirection>, size_t>
{
	size_t operator()(const std::pair<long, esriNetworkEdgeDirection> & pair) const { return pair.first; }
};

struct NAedgePairEqual : public std::binary_function<std::pair<long, esriNetworkEdgeDirection>, std::pair<long, esriNetworkEdgeDirection>, bool>
{
	bool operator()(const std::pair<long, esriNetworkEdgeDirection> & _Left, const std::pair<long, esriNetworkEdgeDirection> & _Right) const
	{
		return (_Left.first == _Right.first) && (_Left.second == _Right.second);
	}
};

typedef public std::unordered_map<long, NAEdgePtr> NAEdgeTable;
typedef std::unordered_map<long, NAEdgePtr>::const_iterator NAEdgeTableItr;
typedef std::pair<long, NAEdgePtr> _NAEdgeTablePair;
#define NAEdgeTablePair(a) _NAEdgeTablePair(a->EID, a)

class NAEdgeMap
{
private:
	NAEdgeTable * cacheAlong;
	NAEdgeTable * cacheAgainst;

public:
	NAEdgeMap(const NAEdgeMap & that) = delete;
	NAEdgeMap & operator=(const NAEdgeMap &) = delete;

	NAEdgeMap(void)
	{
		cacheAlong = new DEBUG_NEW_PLACEMENT std::unordered_map<long, NAEdgePtr>();
		cacheAgainst = new DEBUG_NEW_PLACEMENT std::unordered_map<long, NAEdgePtr>();
	}

	virtual ~NAEdgeMap(void)
	{
		Clear();
		delete cacheAlong;
		delete cacheAgainst;
	}
	
	void GetDirtyEdges(std::vector<NAEdgePtr> & dirty) const;
	void Erase(NAEdgePtr edge) {        Erase(edge->EID, edge->Direction)  ; }
	bool Exist(NAEdgePtr edge) { return Exist(edge->EID, edge->Direction)  ; }
	void Clear(bool destroyTreePrevious = false);
	size_t Size()              { return cacheAlong->size() + cacheAgainst->size(); }
	HRESULT Insert(NAEdgePtr   edge );
	HRESULT Insert(NAEdgeMap * edges);
	bool Exist(long eid, esriNetworkEdgeDirection dir);
	void Erase(long eid, esriNetworkEdgeDirection dir);
	const NAEdgePtr Find(const NAEdgePtr edge) const;
};

class NAEdgeMapTwoGen
{
public:
	NAEdgeMap * oldGen;
	NAEdgeMap * newGen;

	NAEdgeMapTwoGen(const NAEdgeMapTwoGen & that) = delete;
	NAEdgeMapTwoGen & operator=(const NAEdgeMapTwoGen &) = delete;

	NAEdgeMapTwoGen(void)
	{
		oldGen = new DEBUG_NEW_PLACEMENT NAEdgeMap();
		newGen = new DEBUG_NEW_PLACEMENT NAEdgeMap();
	}

	virtual ~NAEdgeMapTwoGen(void)
	{
		Clear(NAEdgeMapGeneration::AllGens);
		delete oldGen;
		delete newGen;
	}

	void MarkAllAsOldGen();
	bool Exist(NAEdgePtr edge, NAEdgeMapGeneration gen = NAEdgeMapGeneration::AllGens) { return Exist(edge->EID, edge->Direction, gen); }
	void Erase(NAEdgePtr edge, NAEdgeMapGeneration gen = NAEdgeMapGeneration::AllGens);
	void Clear(NAEdgeMapGeneration gen, bool destroyTreePrevious = false);
	size_t Size(NAEdgeMapGeneration gen = NAEdgeMapGeneration::NewGen);
	HRESULT Insert(NAEdgePtr edge, NAEdgeMapGeneration gen = NAEdgeMapGeneration::NewGen);
	bool Exist(long eid, esriNetworkEdgeDirection dir, NAEdgeMapGeneration gen = NAEdgeMapGeneration::AllGens);
};

typedef std::pair<long, unsigned char> NAEdgeContainerPair;
typedef std::unordered_map<long, unsigned char>::const_iterator NAEdgeIterator;

class NAEdgeContainer
{
private:
	std::unordered_map<long, unsigned char> * cache;

public:
	NAEdgeContainer(const NAEdgeContainer & that) = delete;
	NAEdgeContainer & operator=(const NAEdgeContainer &) = delete;

	NAEdgeContainer(size_t capacity)
	{
		cache = new DEBUG_NEW_PLACEMENT std::unordered_map<long, unsigned char>(capacity);
	}

	virtual ~NAEdgeContainer(void)
	{
		Clear();
		delete cache;
	}

	inline  NAEdgeIterator begin() { return cache->begin(); }
	inline  NAEdgeIterator end()   { return cache->end()  ; }
	HRESULT Insert(NAEdgePtr edge) { return Insert(edge->EID, edge->Direction); }
	inline void Clear() { cache->clear(); }
	HRESULT Insert(INetworkEdgePtr edge);
	HRESULT Insert(long eid, esriNetworkEdgeDirection dir);
	HRESULT Insert(long eid, unsigned char dir);
	HRESULT Remove(INetworkEdgePtr edge);
	HRESULT Remove(long eid, esriNetworkEdgeDirection dir);
	HRESULT Remove(long eid, unsigned char dir);
	void    Insert(std::shared_ptr<NAEdgeContainer> clone);
	bool Exist(NAEdge * edge) const { return Exist(edge->EID, edge->Direction); }
	bool Exist(INetworkEdgePtr edge) const;
	bool Exist(long eid, esriNetworkEdgeDirection dir) const;
	bool IsEmpty() const { return cache->empty(); }
};

// This collection object has two jobs:
// it makes sure that there exist only one copy of an edge in it that is connected to each INetworkEdge.
// this will be helpful to avoid duplicate copies pointing to the same edge structure. So data attached
// to edge will be always fresh and there will be no inconsistency. Care has to be taken not to overwrite
// important edges with new ones. The second job is just a GC. since all edges are being new-ed here,
// it can all be deleted at the end here as well.
class NAEdgeCache
{
private:
	long			capacityAttribID;
	long			costAttribID;
	bool			twoWayRoadsShareCap;
	mutable bool	IsSourceCache;
	NAEdgeTable		* cacheAlong;
	NAEdgeTable		* cacheAgainst;
	std::list<ArrayList<NAEdgePtr> *> GarbageNeighborList;
	std::list<EdgeReservationsPtr> ResTable;
	TrafficModel    * myTrafficModel;
	INetworkEdgePtr ipCurrentEdge;
	INetworkQueryPtr                  ipNetworkQuery;
	INetworkForwardStarExPtr          ipForwardStar;
	INetworkForwardStarExPtr          ipBackwardStar;
	INetworkForwardStarAdjacenciesPtr ipAdjacencies;

public:

	NAEdgeCache(long CapacityAttribID, long CostAttribID, double SaturationPerCap, double CriticalDensPerCap, bool TwoWayRoadsShareCap, double InitDelayCostPerPop,
		EvcTrafficModel model, INetworkForwardStarExPtr _ipForwardStar, INetworkForwardStarExPtr _ipBackwardStar, INetworkQueryPtr _ipNetworkQuery, HRESULT & hr)
	{
		IsSourceCache = false;
		capacityAttribID = CapacityAttribID;
		costAttribID = CostAttribID;
		cacheAlong = new DEBUG_NEW_PLACEMENT std::unordered_map<long, NAEdgePtr>();
		cacheAgainst = new DEBUG_NEW_PLACEMENT std::unordered_map<long, NAEdgePtr>();
		myTrafficModel = new DEBUG_NEW_PLACEMENT TrafficModel(model, CriticalDensPerCap, SaturationPerCap, InitDelayCostPerPop);
		twoWayRoadsShareCap = TwoWayRoadsShareCap;

		// network variables init
		INetworkElementPtr ipEdgeElement;
		ipNetworkQuery = _ipNetworkQuery;
		ipForwardStar = _ipForwardStar;
		ipBackwardStar = _ipBackwardStar;
		if (FAILED(hr = ipNetworkQuery->CreateForwardStarAdjacencies(&ipAdjacencies))) return;
		if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipEdgeElement))) return;
		ipCurrentEdge = ipEdgeElement;
	}

	void InitSourceCache() const
	{
		if (IsSourceCache) ipNetworkQuery->ClearIDCache();
		IsSourceCache = true;

		// create cache in network dataset object
		long SourceCount = 0, sourceID = 0;
		INetworkDataset2Ptr network(ipNetworkQuery);
		INetworkSourcePtr source = nullptr;
		if (SUCCEEDED(network->get_SourceCount(&SourceCount)))
		{
			for (long i = 0; i < SourceCount; ++i)
			{
				if (FAILED(network->get_Source(i, &source))) continue;
				if (FAILED(source->get_ID(&sourceID))) continue;
				ipNetworkQuery->PopulateIDCache(sourceID);
			}
		}
	}

	virtual ~NAEdgeCache(void)
	{
		Clear();
		if (IsSourceCache) ipNetworkQuery->ClearIDCache();
		delete myTrafficModel;
		delete cacheAlong;
		delete cacheAgainst;
	}

	NAEdgeCache(const NAEdgeCache & that) = delete;
	NAEdgeCache & operator=(const NAEdgeCache &) = delete;

	NAEdgePtr New(long EID, esriNetworkEdgeDirection dir);
	NAEdgePtr New(INetworkEdgePtr edge);

	INetworkQueryPtr GetNetworkQuery()  { return ipNetworkQuery;        }
	NAEdgeTableItr AlongBegin()   const { return cacheAlong->begin();   }
	NAEdgeTableItr AlongEnd()     const { return cacheAlong->end();     }
	NAEdgeTableItr AgainstBegin() const { return cacheAgainst->begin(); }
	NAEdgeTableItr AgainstEnd()   const { return cacheAgainst->end();   }
	double GetInitDelayPerPop()   const { return myTrafficModel->InitDelayCostPerPop;  }
	NAEdgePtr Get(long eid, esriNetworkEdgeDirection dir) const;
	size_t Size() const { return cacheAlong->size() + cacheAgainst->size(); }
	void Clear();
	void CleanAllEdgesAndRelease(double minPop2Route, EvcSolverMethod solver);
	double GetCacheHitPercentage() const { return myTrafficModel->GetCacheHitPercentage(); }
	HRESULT QueryAdjacencies(NAVertexPtr ToVertex, NAEdgePtr Edge, QueryDirection dir, ArrayList<NAEdgePtr> ** neighbors);
};
