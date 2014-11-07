#pragma once

#include "StdAfx.h"
#include "Evacuee.h"
#include "TrafficModel.h"
#include "utils.h"

class EdgeReservations : private std::vector<EvcPathPtr>
{
private:
	float          ReservedPop;
	float          Capacity;
	EdgeDirtyState dirtyState;
	TrafficModel   * myTrafficModel;

public:
	EdgeReservations(float capacity, TrafficModel * trafficModel);
	EdgeReservations(const EdgeReservations& cpy);
	void AddReservation(double newFlow, EvcPathPtr path);
	void RemoveReservation(double flow, EvcPathPtr path);

	friend class NAEdge;
};

// EdgeReservations hash map
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

	double GetCost(double newPop, EvcSolverMethod method, double * globalDeltaCost = NULL) const;
	double GetCurrentCost(EvcSolverMethod method = CASPERSolver) const;
	double LeftCapacity() const;

	// Special function for Flocking: to check how much capacity the edge had originally
	double OriginalCapacity() const { return reservations->Capacity; }

	HRESULT QuerySourceStuff(long * sourceOID, long * sourceID, double * fromPosition, double * toPosition) const;
	void AddReservation(EvcPath * path, EvcSolverMethod method, bool delayedDirtyState = false);
	NAEdge(INetworkEdgePtr, long capacityAttribID, long costAttribID, const NAEdge * otherEdge, bool twoWayRoadsShareCap, std::list<EdgeReservationsPtr> & ResTable, TrafficModel * model);
	NAEdge(const NAEdge& cpy);

	EdgeDirtyState HowDirty(EvcSolverMethod method, double minPop2Route = 1.0, bool exhaustive = false);
	inline void SetClean(EvcSolverMethod method, double minPop2Route);
	inline double GetCleanCost() const { return CleanCost; }
	float GetReservedPop() const { return reservations->ReservedPop; }
	void TreeNextEraseFirst(NAEdge * child);
	HRESULT GetGeometry(INetworkDatasetPtr ipNetworkDataset, IFeatureClassContainerPtr ipFeatureClassContainer, bool & sourceNotFoundFlag, IGeometryPtr & geometry);
	void RemoveReservation(EvcPathPtr path, EvcSolverMethod method, bool delayedDirtyState = false);
	void GetCrossingPaths(std::vector<EvcPathPtr> & crossings) { for (const auto & p : *reservations) crossings.push_back(p); }
	double MaxAddedCostOnReservedPathsWithNewFlow(double deltaCostOfNewFlow, double longestPathSoFar, double currentPathSoFar, double selfishRatio) const;
	HRESULT InsertEdgeToFeatureCursor(INetworkDatasetPtr ipNetworkDataset, IFeatureClassContainerPtr ipFeatureClassContainer, IFeatureBufferPtr ipFeatureBuffer, IFeatureCursorPtr ipFeatureCursor,
									  long eidFieldIndex, long sourceIDFieldIndex, long sourceOIDFieldIndex, long dirFieldIndex, long resPopFieldIndex, long travCostFieldIndex,
									  long orgCostFieldIndex, long congestionFieldIndex, bool & sourceNotFoundFlag);

	static bool CostLessThan(NAEdge * e1, NAEdge * e2, EvcSolverMethod method)
	{
		return e1->GetCurrentCost(method) < e2->GetCurrentCost(method);
	}

	static bool CongestionLessThan(NAEdge * e1, NAEdge * e2, EvcSolverMethod method)
	{
		return e1->GetTrafficSpeedRatio(e1->reservations->ReservedPop, method) > e2->GetTrafficSpeedRatio(e2->reservations->ReservedPop, method);
	}
};

double GetHeapKeyHur   (const NAEdge * e);
double GetHeapKeyNonHur(const NAEdge * e);
bool   IsEqual         (const NAEdge * n1, const NAEdge * n2);

typedef NAEdge * NAEdgePtr;
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
	NAEdgeMap(void)
	{
		cacheAlong = new DEBUG_NEW_PLACEMENT std::unordered_map<long, NAEdgePtr>();
		cacheAgainst = new DEBUG_NEW_PLACEMENT std::unordered_map<long, NAEdgePtr>();
	}

	~NAEdgeMap(void)
	{
		Clear();
		delete cacheAlong;
		delete cacheAgainst;
	}
	
	void CallHowDirty(EvcSolverMethod method, double minPop2Route = 1.0, bool exhaustive = false);
	void GetDirtyEdges(std::vector<NAEdgePtr> * dirty, double minPop2Route, EvcSolverMethod method) const;
	void Erase(NAEdgePtr edge) {        Erase(edge->EID, edge->Direction)  ; }
	bool Exist(NAEdgePtr edge) { return Exist(edge->EID, edge->Direction)  ; }
	void Clear()               { cacheAlong->clear(); cacheAgainst->clear(); }
	size_t Size()              { return cacheAlong->size() + cacheAgainst->size(); }
	HRESULT Insert(NAEdgePtr   edge );
	HRESULT Insert(NAEdgeMap * edges);
	bool Exist(long eid, esriNetworkEdgeDirection dir);
	void Erase(long eid, esriNetworkEdgeDirection dir);
};

class NAEdgeMapTwoGen
{
public:
	NAEdgeMap * oldGen;
	NAEdgeMap * newGen;

	NAEdgeMapTwoGen(void)
	{
		oldGen = new DEBUG_NEW_PLACEMENT NAEdgeMap();
		newGen = new DEBUG_NEW_PLACEMENT NAEdgeMap();
	}

	~NAEdgeMapTwoGen(void)
	{
		Clear(NAEdgeMapGeneration::AllGens);
		delete oldGen;
		delete newGen;
	}

	void GetDirtyEdges(std::vector<NAEdgePtr> * dirty, double minPop2Route, EvcSolverMethod method) const
	{
		oldGen->GetDirtyEdges(dirty, minPop2Route, method);
		newGen->GetDirtyEdges(dirty, minPop2Route, method);
	}

	void MarkAllAsOldGen();
	bool Exist(NAEdgePtr edge, NAEdgeMapGeneration gen = NAEdgeMapGeneration::AllGens) { return Exist(edge->EID, edge->Direction, gen); }
	void Erase(NAEdgePtr edge, NAEdgeMapGeneration gen = NAEdgeMapGeneration::AllGens);
	void Clear(NAEdgeMapGeneration gen);
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
	NAEdgeContainer(size_t capacity)
	{
		cache = new DEBUG_NEW_PLACEMENT std::unordered_map<long, unsigned char>();
		cache->reserve(capacity);
	}

	~NAEdgeContainer(void)
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
	void    Insert(NAEdgeContainer * clone);
	bool Exist(INetworkEdgePtr edge);
	bool Exist(long eid, esriNetworkEdgeDirection dir);
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

	~NAEdgeCache(void)
	{
		Clear();
		delete myTrafficModel;
		delete cacheAlong;
		delete cacheAgainst;
	}

	NAEdgePtr New(INetworkEdgePtr edge, bool reuseEdgeElement);

	NAEdgeTableItr AlongBegin()   const { return cacheAlong->begin();   }
	NAEdgeTableItr AlongEnd()     const { return cacheAlong->end();     }
	NAEdgeTableItr AgainstBegin() const { return cacheAgainst->begin(); }
	NAEdgeTableItr AgainstEnd()   const { return cacheAgainst->end();   }
	NAEdgePtr Get(long eid, esriNetworkEdgeDirection dir) const;
	size_t Size() const { return cacheAlong->size() + cacheAgainst->size(); }
	void Clear();
	void CleanAllEdgesAndRelease(double minPop2Route, EvcSolverMethod solver);
	double GetCacheHitPercentage() const { return myTrafficModel->GetCacheHitPercentage(); }
	HRESULT QueryAdjacencies(NAVertexPtr ToVertex, NAEdgePtr Edge, QueryDirection dir, ArrayList<NAEdgePtr> ** neighbors);
};
