#pragma once

#include <hash_map>
#include "Evacuee.h"

[ export, uuid("096CB996-9144-4CC3-BB69-FCFAA5C273FC") ] enum EvcSolverMethod : unsigned char { SPSolver = 0x0, CCRPSolver = 0x1, CASPERSolver = 0x2 };
[ export, uuid("BFDD2DB3-DA25-42CA-8021-F67BF7D14948") ] enum EvcTrafficModel : unsigned char { FLATModel = 0x0, STEPModel = 0x1, LINEARModel = 0x2, POWERModel = 0x3 };

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

class EdgeReservations
{
private:
	float  ReservedPop;
	float  SaturationDensPerCap;
	float  CriticalDens;
	float  Capacity;
	float  initDelayCostPerPop;
	bool   isDirty;
	// EdgeDirtyFlagEnum  DirtyFlag;
	// std::vector<EdgeReservation> * List;

public:
	EdgeReservations(float capacity, float CriticalDensPerCap, float SaturationDensPerCap, float InitDelayCostPerPop);
	EdgeReservations(const EdgeReservations& cpy);
	
	void Clear()
	{
		//List->clear();
	}
	
	~EdgeReservations(void)
	{
		Clear();
		//delete List;
	}
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
	EvcTrafficModel trafficModel;
	double CASPERRatio;
	double CleanCost;
	mutable double cachedCost[2];
	// mutable unsigned short calcSaved;
	double GetTrafficSpeedRatio(double allPop) const;

public:
	double OriginalCost;
	esriNetworkEdgeDirection Direction;
	NAVertex * ToVertex;
	NAEdge * TreePrevious;
	std::vector<NAEdge *> TreeNext;
	INetworkEdgePtr NetEdge;
	INetworkEdgePtr LastExteriorEdge;	
	long EID;

	double GetCost(double newPop, EvcSolverMethod method) const;
	double GetCurrentCost() const;
	double LeftCapacity() const;	

	// Special function for Flocking: to check how much capacity the edge had originally
	double OriginalCapacity() const { return reservations->Capacity; }

	HRESULT QuerySourceStuff(long * sourceOID, long * sourceID, double * fromPosition, double * toPosition) const;	
	void AddReservation(/* Evacuee * evacuee, double fromCost, double toCost, */ double population, EvcSolverMethod method);
	NAEdge(INetworkEdgePtr, long capacityAttribID, long costAttribID, float CriticalDensPerCap, float SaturationDensPerCap, NAResTable *, float InitDelayCostPerPop, EvcTrafficModel);
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
	// inline unsigned short GetCalcSaved() const { return calcSaved; }
};

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
// important edges with new ones. The second job is just a GC. since all edges are being newed here,
// it can all be deleted at the end here as well.

class NAEdgeCache
{
private:
	// std::vector<NAEdgePtr>	* sideCache;
	NAEdgeTable				* cacheAlong;
	NAEdgeTable				* cacheAgainst;
	long					capacityAttribID;
	long					costAttribID;
	float					saturationPerCap;
	float					criticalDensPerCap;
	bool					twoWayRoadsShareCap;
	NAResTable				* resTableAlong;
	NAResTable				* resTableAgainst;
	float					initDelayCostPerPop;
	EvcTrafficModel		    trafficModel;

public:

	NAEdgeCache(long CapacityAttribID, long CostAttribID, float SaturationPerCap, float CriticalDensPerCap, bool TwoWayRoadsShareCap, float InitDelayCostPerPop, EvcTrafficModel TrafficModel)
	{
		// sideCache = new DEBUG_NEW_PLACEMENT std::vector<NAEdgePtr>();
		initDelayCostPerPop = InitDelayCostPerPop;
		capacityAttribID = CapacityAttribID;
		costAttribID = CostAttribID;
		cacheAlong = new DEBUG_NEW_PLACEMENT stdext::hash_map<long, NAEdgePtr>();
		cacheAgainst = new DEBUG_NEW_PLACEMENT stdext::hash_map<long, NAEdgePtr>();
		saturationPerCap = SaturationPerCap;
		criticalDensPerCap = CriticalDensPerCap;
		if (saturationPerCap <= criticalDensPerCap) saturationPerCap += criticalDensPerCap;
		twoWayRoadsShareCap = TwoWayRoadsShareCap;
		trafficModel = TrafficModel;

		resTableAlong = new DEBUG_NEW_PLACEMENT NAResTable();
		if (twoWayRoadsShareCap) resTableAgainst = resTableAlong;
		else resTableAgainst = new DEBUG_NEW_PLACEMENT NAResTable();
	}
	
	~NAEdgeCache(void) 
	{
		Clear();
		// delete sideCache;
		delete cacheAlong; 
		delete cacheAgainst;
		delete resTableAlong;
		if (!twoWayRoadsShareCap) delete resTableAgainst;
	}

	NAEdgePtr New(INetworkEdgePtr edge, bool replace = false);

	NAEdgeTableItr AlongBegin()   const { return cacheAlong->begin();   }
	NAEdgeTableItr AlongEnd()     const { return cacheAlong->end();     }
	NAEdgeTableItr AgainstBegin() const { return cacheAgainst->begin(); }
	NAEdgeTableItr AgainstEnd()   const { return cacheAgainst->end();   }
	NAEdgePtr Get(long eid, esriNetworkEdgeDirection dir) const;
	size_t Size() const { return cacheAlong->size() + cacheAgainst->size(); }
	void Clear();	
	void CleanAllEdgesAndRelease(double minPop2Route, EvcSolverMethod solver);
};
