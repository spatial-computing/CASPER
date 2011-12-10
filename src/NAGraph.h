#pragma once

#include <hash_map>

class Evacuee;
class NAEdge;
#define MAX_COST 1000000000.0

// The NAVertex class is what sits on top of the INetworkJunction interface and holds extra
// information about each junction/vertex which are helpful for CASPER algorithm.
// g: cost from source
// h: estimated cost to nearest safe zone
// Edge: the previous edge leading to this vertex
// Previous: the previous vertex leading to this vertex

class NAVertex
{
private:
	NAEdge * BehindEdge;

public:
	double g;
	double h;
	INetworkJunctionPtr Junction;
	NAVertex * Previous;
	long EID;

	void SetBehindEdge(NAEdge * behindEdge);
	NAEdge * GetBehindEdge() { return BehindEdge; }
	
	NAVertex(void);
	NAVertex(const NAVertex& cpy);
	NAVertex(INetworkJunctionPtr junction, NAEdge * behindEdge);
	double GetKey() { return g + h; }
	bool LessThan(NAVertex * other) { return g + h < other->g + other->h; }
};

typedef NAVertex * NAVertexPtr;
typedef stdext::hash_map<long, NAVertexPtr> NAVertexTable;
typedef stdext::hash_map<long, NAVertexPtr>::iterator NAVertexTableItr;
typedef std::pair<long, NAVertexPtr> _NAVertexTablePair;
#define NAVertexTablePair(a) _NAVertexTablePair(a->EID, a)

// This collection object has two jobs:
// it makes sure that there exist only one copy of a vertex in it that is connected to each INetworkJunction.
// this will be helpful to avoid duplicate copies pointing to the same junction structure. So data attached
// to vertex will be always fresh and there will be no inconsistancy. Care has to be taken not to overwrite
// important vertices with new ones. The second job is just a GC. since all vertices are being newed here,
// it can all be deleted at the end here as well.

class NAVertexCache
{
private:
	NAVertexTable * cache;
	std::vector<NAVertex *> * sideCache;

public:
	NAVertexCache(void)
	{
		cache = new stdext::hash_map<long, NAVertexPtr>();
		sideCache = new std::vector<NAVertex *>();
	}

	~NAVertexCache(void) 
	{
		Clear();
		delete cache;
		delete sideCache;
	}
	
	NAVertexPtr New(INetworkJunctionPtr junction);
	void NAVertexCache::UdateHeuristic(NAVertexPtr vertex);
	NAVertexPtr Get(long eid);
	void Clear();
};

class NAVertexCollector
{
private:
	std::vector<NAVertexPtr> * cache;

public:
	NAVertexCollector(void)
	{
		cache = new std::vector<NAVertexPtr>();
	}

	~NAVertexCollector(void) 
	{
		Clear();
		delete cache;
	}
	
	NAVertexPtr New(INetworkJunctionPtr junction);
	int Size() { return cache->size(); }
	void Clear();
};

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

// The NAEdge class is what sits on top of the INetworkEdge interface and holds extra
// information about each edge which are helpful for CASPER algorithm.
// Capacity: road initial capacity
// Cost: road initial travel cost
// reservations: a vector of evacuees commited to use this edge at a certain time/cost period
class NAEdge
{
private:	
	std::vector<EdgeReservation> * reservations;
	double ReservedPop;
	double saturationDens;
	double criticalDens;

public:
	double originalCost;
	esriNetworkEdgeDirection Direction;
	NAVertex * ToVertex;
	INetworkEdgePtr NetEdge;
	INetworkEdgePtr LastExteriorEdge;	
	long EID;
	double GetCost(double newPop, char method) const;
	double CapacityLeft() const;
	double OriginalCapacity() const;

	HRESULT QuerySourceStuff(long * sourceOID, long * sourceID, double * fromPosition, double * toPosition) const;	
	void AddReservation(Evacuee * evacuee, double fromCost, double toCost, double population);
	NAEdge(INetworkEdgePtr edge, long capacityAttribID, long costAttribID, double CriticalDensPerCap,
		double SaturationDensPerCap);
	NAEdge(const NAEdge& cpy);

	void Clear()
	{
		reservations->clear();
	}

	double GetReservedPop() const { return ReservedPop; }
	bool LessThan(NAEdge * other) { return ToVertex->g + ToVertex->h < other->ToVertex->g + other->ToVertex->h; }

	~NAEdge(void)
	{
		Clear();
		delete reservations;
	}
};

typedef NAEdge * NAEdgePtr;
typedef public stdext::hash_map<long, NAEdgePtr> NAEdgeTable;
typedef stdext::hash_map<long, NAEdgePtr>::iterator NAEdgeTableItr;
typedef std::pair<long, NAEdgePtr> _NAEdgeTablePair;
#define NAEdgeTablePair(a) _NAEdgeTablePair(a->EID, a)

class NAEdgeClosed
{
private:
	NAEdgeTable * cacheAlong;
	NAEdgeTable * cacheAgainst;

public:
	NAEdgeClosed(void)
	{ 
		cacheAlong = new stdext::hash_map<long, NAEdgePtr>();
		cacheAgainst = new stdext::hash_map<long, NAEdgePtr>();
	}

	~NAEdgeClosed(void) 
	{
		Clear();
		delete cacheAlong; 
		delete cacheAgainst; 
	}
	
	void Clear() { cacheAlong->clear(); cacheAgainst->clear(); }
	int Size() { return cacheAlong->size() + cacheAgainst->size(); }
	HRESULT Insert(NAEdgePtr edge);
	bool IsClosed(NAEdgePtr edge);
};

typedef std::pair<long, char> _NAEdgeContainerPair;
#define NAEdgeContainerPair(a) _NAEdgeContainerPair(a, 1)

class NAEdgeContainer
{
private:
	stdext::hash_map<long, char> * cacheAlong;
	stdext::hash_map<long, char> * cacheAgainst;

public:
	NAEdgeContainer(void)
	{ 
		cacheAlong = new stdext::hash_map<long, char>();
		cacheAgainst = new stdext::hash_map<long, char>();
	}

	~NAEdgeContainer(void) 
	{
		Clear();
		delete cacheAlong; 
		delete cacheAgainst; 
	}
	
	void Clear() { cacheAlong->clear(); cacheAgainst->clear(); }
	int Size() { return cacheAlong->size() + cacheAgainst->size(); }
	HRESULT Insert(INetworkEdgePtr edge);
	bool Exist(INetworkEdgePtr edge);
};

// This collection object has two jobs:
// it makes sure that there exist only one copy of an edge in it that is connected to each INetworkEdge.
// this will be helpful to avoid duplicate copies pointing to the same edge structure. So data attached
// to edge will be always fresh and there will be no inconsistancy. Care has to be taken not to overwrite
// important edges with new ones. The second job is just a GC. since all edges are being newed here,
// it can all be deleted at the end here as well.

class NAEdgeCache
{
private:
	NAEdgeTable * cacheAlong;
	NAEdgeTable * cacheAgainst;
	std::vector<NAEdge *> * sideCache;
	long capacityAttribID;
	long costAttribID;
	double saturationPerCap;
	double criticalDensPerCap;

public:

	NAEdgeCache(long CapacityAttribID, long CostAttribID, double SaturationPerCap, double CriticalDensPerCap)
	{
		capacityAttribID = CapacityAttribID;
		costAttribID = CostAttribID;
		cacheAlong = new stdext::hash_map<long, NAEdgePtr>();
		cacheAgainst = new stdext::hash_map<long, NAEdgePtr>();
		saturationPerCap = SaturationPerCap;
		criticalDensPerCap = CriticalDensPerCap;
		if (saturationPerCap > criticalDensPerCap) saturationPerCap -= criticalDensPerCap;
		sideCache = new std::vector<NAEdge *>();
	}

	~NAEdgeCache(void) 
	{
		Clear();
		delete sideCache;
		delete cacheAlong; 
		delete cacheAgainst; 
	}

	NAEdgePtr New(INetworkEdgePtr edge, bool replace = false);

	NAEdgeTableItr AlongBegin() const { return cacheAlong->begin(); }
	NAEdgeTableItr AlongEnd() const { return cacheAlong->end(); }
	NAEdgeTableItr AgainstBegin() const { return cacheAgainst->begin(); }
	NAEdgeTableItr AgainstEnd() const { return cacheAgainst->end(); }
	int Size() { return cacheAlong->size() + cacheAgainst->size(); }

	void Clear();	
};
