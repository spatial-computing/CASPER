#pragma once

#include <hash_map>
#include "Evacuee.h"
#include "TrafficModel.h"

struct EdgeReservation
{
public:
	double FromCost;
	double ToCost;
	Evacuee * Who;

	EdgeReservation(Evacuee * who, double fromCost, double toCost)
	{
		Who = who;
		FromCost = fromCost;
		ToCost = toCost;
	}
};

// enum EdgeDirtyFlagEnum { EdgeFlagClean = 0x0, EdgeFlagMaybe = 0x1, EdgeFlagDirty = 0x2 };

class EdgeReservations : std::vector<EvcPathPtr>
{
private:
	float  ReservedPop;
	float  Capacity;
	bool   isDirty;
	TrafficModel * myTrafficModel;

public:
	EdgeReservations(float capacity, TrafficModel * trafficModel);
	EdgeReservations(const EdgeReservations& cpy);
	void AddReservation(double newFlow, EvcPathPtr path);
	
	friend class NAEdge;
};

// EdgeReservations hash map
typedef EdgeReservations * EdgeReservationsPtr;
typedef public stdext::hash_map<long, EdgeReservationsPtr> NAResTable;
typedef stdext::hash_map<long, EdgeReservationsPtr>::iterator NAResTableItr;
typedef std::pair<long, EdgeReservationsPtr> NAResTablePair;

// The NAEdge class is what sits on top of the INetworkEdge interface and holds extra
// information about each edge which are helpful for CASPER algorithm.
// Capacity: road initial capacity
// Cost: road initial travel cost
// reservations: a vector of evacuees committed to use this edge at a certain time/cost period
class NAEdge
{
private:	
	EdgeReservations * reservations;
	double CleanCost;
	double GetTrafficSpeedRatio(double allPop, EvcSolverMethod method) const;

public:
	double OriginalCost;
	esriNetworkEdgeDirection Direction;
	NAVertex * ToVertex;
	NAEdge * TreePrevious;
	std::vector<NAEdge *> TreeNext;
	INetworkEdgePtr NetEdge;
	// INetworkEdgePtr LastExteriorEdge;	
	long EID;
	std::vector<NAEdge *> * AdjacentForward;
	std::vector<NAEdge *> * AdjacentBackward;

	double GetCost(double newPop, EvcSolverMethod method, double * globalDeltaCost = NULL) const;
	double GetCurrentCost(EvcSolverMethod method = CASPERSolver) const;
	double LeftCapacity() const;

	// Special function for Flocking: to check how much capacity the edge had originally
	double OriginalCapacity() const { return reservations->Capacity; }

	HRESULT QuerySourceStuff(long * sourceOID, long * sourceID, double * fromPosition, double * toPosition) const;	
	void AddReservation(EvcPath * evacuee, double population, EvcSolverMethod method);
	NAEdge(INetworkEdgePtr, long capacityAttribID, long costAttribID, NAResTable * resTable, TrafficModel * model);
	NAEdge(const NAEdge& cpy);

	static bool LessThanNonHur(NAEdge * n1, NAEdge * n2);
	static bool LessThanHur   (NAEdge * n1, NAEdge * n2);
	
	// EdgeDirtyFlagEnum ClarifyEdgeFlag(double minPop2Route, EvcSolverMethod method);
	// inline void SetEdgeFlag(EdgeDirtyFlagEnum flag) { reservations->DirtyFlag = flag; if (flag == EdgeFlagClean) CleanCost = -1.0; }
	// inline EdgeDirtyFlagEnum GetEdgeFlag() const { return reservations->DirtyFlag; }

	inline void SetDirty() { reservations->isDirty = true; }
	inline bool IsDirty (double minPop2Route, EvcSolverMethod method);
	inline void SetClean(double minPop2Route, EvcSolverMethod method);
	inline double GetCleanCost() const { return CleanCost; }
	float GetReservedPop() const { return reservations->ReservedPop; }
	void TreeNextEraseFirst(NAEdge * child);
	double MaxAddedCostOnReservedPathsWithNewFlow(double deltaCostOfNewFlow, double longestPathSoFar, double currentPathSoFar, double selfishRatio) const;
};

double GetHeapKeyHur(const NAEdge * edge);
double GetHeapKeyNonHur(const NAEdge * edge);

typedef NAEdge * NAEdgePtr;
typedef public stdext::hash_map<long, NAEdgePtr> NAEdgeTable;
typedef stdext::hash_map<long, NAEdgePtr>::iterator NAEdgeTableItr;
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
		cacheAlong = new DEBUG_NEW_PLACEMENT stdext::hash_map<long, NAEdgePtr>();
		cacheAgainst = new DEBUG_NEW_PLACEMENT stdext::hash_map<long, NAEdgePtr>();
	}

	~NAEdgeMap(void) 
	{
		Clear();
		delete cacheAlong; 
		delete cacheAgainst; 
	}

	void GetDirtyEdges(std::vector<NAEdgePtr> * dirty, double minPop2Route, EvcSolverMethod method) const;
	void Erase(NAEdgePtr edge) {        Erase(edge->EID, edge->Direction)  ; }
	bool Exist(NAEdgePtr edge) { return Exist(edge->EID, edge->Direction)  ; }
	void Clear()               { cacheAlong->clear(); cacheAgainst->clear(); }
	size_t Size()              { return cacheAlong->size() + cacheAgainst->size(); }	
	HRESULT Insert(NAEdgePtr edge);
	HRESULT Insert(NAEdgeMap * edges);
	bool Exist(long eid, esriNetworkEdgeDirection dir);
	void Erase(long eid, esriNetworkEdgeDirection dir);
};

#define NAEdgeMap_NOGEN   0x0
#define NAEdgeMap_OLDGEN  0x1
#define NAEdgeMap_NEWGEN  0x2
#define NAEdgeMap_ALLGENS 0x3

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
		Clear(NAEdgeMap_ALLGENS);
		delete oldGen; 
		delete newGen; 
	}

	void GetDirtyEdges(std::vector<NAEdgePtr> * dirty, double minPop2Route, EvcSolverMethod method) const
	{
		oldGen->GetDirtyEdges(dirty, minPop2Route, method);
		newGen->GetDirtyEdges(dirty, minPop2Route, method);
	}

	void MarkAllAsOldGen();
	bool Exist(NAEdgePtr edge, UCHAR gen = NAEdgeMap_ALLGENS) { return Exist(edge->EID, edge->Direction, gen)  ; }
	void Erase(NAEdgePtr edge, UCHAR gen = NAEdgeMap_ALLGENS);
	void Clear(UCHAR gen);
	size_t Size(UCHAR gen = NAEdgeMap_NEWGEN);
	HRESULT Insert(NAEdgePtr edge, UCHAR gen = NAEdgeMap_NEWGEN);
	bool Exist(long eid, esriNetworkEdgeDirection dir, UCHAR gen = NAEdgeMap_ALLGENS);
};

typedef std::pair<long, unsigned char> NAEdgeContainerPair;
typedef stdext::hash_map<long, unsigned char>::iterator NAEdgeIterator;

class NAEdgeContainer
{
private:
	stdext::hash_map<long, unsigned char> * cache;

public:
	NAEdgeContainer(void)
	{ 
		cache = new DEBUG_NEW_PLACEMENT stdext::hash_map<long, unsigned char>();
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
	void    Insert(NAEdgeContainer * clone);
	bool Exist(INetworkEdgePtr edge);
	bool Exist(long eid, esriNetworkEdgeDirection dir);
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
	NAResTable		* resTableAlong;
	NAResTable		* resTableAgainst;
	TrafficModel    * myTrafficModel;
	INetworkEdgePtr ipCurrentEdge;
	INetworkQueryPtr                  ipNetworkQuery;
	INetworkForwardStarExPtr          ipForwardStar;
	INetworkForwardStarExPtr          ipBackwardStar;
	INetworkForwardStarAdjacenciesPtr ipForwardAdjacencies;
	INetworkForwardStarAdjacenciesPtr ipBackwardAdjacencies;

public:

	NAEdgeCache(long CapacityAttribID, long CostAttribID, double SaturationPerCap, double CriticalDensPerCap, bool TwoWayRoadsShareCap, double InitDelayCostPerPop,
		EvcTrafficModel model, INetworkForwardStarExPtr _ipForwardStar, INetworkForwardStarExPtr _ipBackwardStar, INetworkQueryPtr _ipNetworkQuery, HRESULT & hr)
	{
		capacityAttribID = CapacityAttribID;
		costAttribID = CostAttribID;
		cacheAlong = new DEBUG_NEW_PLACEMENT stdext::hash_map<long, NAEdgePtr>();
		cacheAgainst = new DEBUG_NEW_PLACEMENT stdext::hash_map<long, NAEdgePtr>();
		myTrafficModel = new DEBUG_NEW_PLACEMENT TrafficModel(model, CriticalDensPerCap, SaturationPerCap, InitDelayCostPerPop);	
		twoWayRoadsShareCap = TwoWayRoadsShareCap;

		resTableAlong = new DEBUG_NEW_PLACEMENT NAResTable();
		if (twoWayRoadsShareCap) resTableAgainst = resTableAlong;
		else resTableAgainst = new DEBUG_NEW_PLACEMENT NAResTable();

		// network variables init		
		INetworkElementPtr ipEdgeElement;
		ipNetworkQuery = _ipNetworkQuery;
		ipForwardStar = _ipForwardStar;
		ipBackwardStar = _ipBackwardStar;
		if (FAILED(hr = ipNetworkQuery->CreateForwardStarAdjacencies(&ipForwardAdjacencies))) return;
		if (FAILED(hr = ipNetworkQuery->CreateForwardStarAdjacencies(&ipBackwardAdjacencies))) return; 
		if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipEdgeElement))) return;
		ipCurrentEdge = ipEdgeElement;
	}
	
	~NAEdgeCache(void) 
	{
		Clear();
		delete myTrafficModel;
		delete cacheAlong; 
		delete cacheAgainst;
		delete resTableAlong;
		if (!twoWayRoadsShareCap) delete resTableAgainst;
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
	HRESULT QueryAdjacenciesForward(NAVertexPtr ToVertex, NAEdgePtr Edge);
	HRESULT QueryAdjacenciesBackward(NAVertexPtr ToVertex, NAEdgePtr Edge);
};
