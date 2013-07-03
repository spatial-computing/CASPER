#pragma once

#include <hash_map>
#include <fstream>

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
	double Value;
	long EdgeID;
	unsigned short CarmaLoop;
	
	HValue(long edgeID, double value, unsigned short carmaLoop)
	{
		EdgeID = edgeID;
		Value = value;
		CarmaLoop = carmaLoop;
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
	std::vector<HValue> * h;

public:
	double g;
	INetworkJunctionPtr Junction;
	NAVertex * Previous;
	long EID;
	size_t HCount() const { return h->size(); }

	__forceinline double minh() const
	{
		if (h->empty()) return 0.0;
		else return ((*h)[0]).Value;
	}

	double GetH(long eid)
	{
		for(std::vector<HValue>::iterator i = h->begin(); i != h->end(); i++) if (i->EdgeID == eid) return i->Value;
		return FLT_MAX;
	}

	void ResetHValues(void)
	{
		h->clear(); 
		h->reserve(2);
		// h->push_back(HValue(0l, 0.0));
	}

	inline void SetBehindEdge(NAEdge * behindEdge);
	NAEdge * GetBehindEdge() { return BehindEdge; }
	inline bool IsHEmpty() const { return h->empty(); }
	bool UpdateHeuristic(long edgeid, double hur, unsigned short carmaLoop);
	
	NAVertex(void);
	void ReleaseH(void) { delete h; }
	NAVertex(const NAVertex& cpy);
	NAVertex(INetworkJunctionPtr junction, NAEdge * behindEdge);
};

typedef NAVertex * NAVertexPtr;
typedef stdext::hash_map<long, NAVertexPtr> NAVertexTable;
typedef stdext::hash_map<long, NAVertexPtr>::_Pairib NAVertexTableInsertReturn;
typedef stdext::hash_map<long, NAVertexPtr>::iterator NAVertexTableItr;
typedef std::pair<long, NAVertexPtr> _NAVertexTablePair;
#define NAVertexTablePair(a) _NAVertexTablePair(a->EID, a)

typedef stdext::hash_map<int, std::list<long> *> NAVertexLoopCountList;
typedef NAVertexLoopCountList::_Pairib NAVertexLoopCountListReturn;
typedef NAVertexLoopCountList::iterator NAVertexLoopCountListItr;
typedef std::pair<int, std::list<long> *> NAVertexLoopCountListPair;

// This collection object has two jobs:
// it makes sure that there exist only one copy of a vertex in it that is connected to each INetworkJunction.
// this will be helpful to avoid duplicate copies pointing to the same junction structure. So data attached
// to vertex will be always fresh and there will be no inconsistency. Care has to be taken not to overwrite
// important vertices with new ones. The second job is just a GC. since all vertices are being created here,
// it can all be deleted at the end here as well.

class NAVertexCache
{
private:
	NAVertexTable * cache;
	std::vector<NAVertex *> * sideCache;
	double heuristicForOutsideVertices;

public:
	NAVertexCache(void)
	{
		cache = new DEBUG_NEW_PLACEMENT stdext::hash_map<long, NAVertexPtr>();
		sideCache = new DEBUG_NEW_PLACEMENT std::vector<NAVertex *>();
		heuristicForOutsideVertices = 0.0;
	}

	~NAVertexCache(void) 
	{
		Clear();
		delete cache;
		delete sideCache;
	}

	void PrintVertexHeuristicFeq();	
	NAVertexPtr New(INetworkJunctionPtr junction);
	void UpdateHeuristicForOutsideVertices(double hur, unsigned short carmaLoop);
	bool UpdateHeuristic(long edgeid, NAVertex * n, unsigned short carmaLoop);
	NAVertexPtr Get(long eid);
	NAVertexPtr Get(INetworkJunctionPtr junction);
	void Clear();
	void CollectAndRelease();
};

class NAVertexCollector
{
private:
	std::vector<NAVertexPtr> * cache;

public:
	NAVertexCollector(void)
	{
		cache = new DEBUG_NEW_PLACEMENT std::vector<NAVertexPtr>();
	}

	~NAVertexCollector(void) 
	{
		Clear();
		delete cache;
	}
	
	NAVertexPtr New(INetworkJunctionPtr junction);
	size_t Size() { return cache->size(); }
	void Clear();
};
