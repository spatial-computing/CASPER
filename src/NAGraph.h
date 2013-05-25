#pragma once

#include <hash_map>
#include <fstream>

class Evacuee;
class NAEdge;

#define EVC_SOLVER_METHOD			char
#define EVC_SOLVER_METHOD_SP		0x0
#define EVC_SOLVER_METHOD_CCRP		0x1
#define EVC_SOLVER_METHOD_CASPER	0x2

#define EVC_TRAFFIC_MODEL			char
#define EVC_TRAFFIC_MODEL_FLAT		0x0
#define EVC_TRAFFIC_MODEL_STEP		0x1
#define EVC_TRAFFIC_MODEL_LINEAR	0x2
#define EVC_TRAFFIC_MODEL_CASPER	0x3

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
	std::vector<HValue> * h;

public:
	double g;
	INetworkJunctionPtr Junction;
	NAVertex * Previous;
	long EID;
	// double posAlong;

	__forceinline double minh() const
	{
		if (h->empty()) return 0.0;
		else return ((*h)[0]).Value;
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
	bool UpdateHeuristic(long edgeid, double hur);
	
	NAVertex(void);
	~NAVertex(void) { delete h; }
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
// to vertex will be always fresh and there will be no inconsistancy. Care has to be taken not to overwrite
// important vertices with new ones. The second job is just a GC. since all vertices are being newed here,
// it can all be deleted at the end here as well.

class NAVertexCache
{
private:
	NAVertexTable * cache;
	std::vector<NAVertex *> * sideCache;
	// NAVertexLoopCountList * noHVertices;
	double heuristicForOutsideVertices;

public:
	NAVertexCache(void)
	{
		cache = new DEBUG_NEW_PLACEMENT stdext::hash_map<long, NAVertexPtr>();
		sideCache = new DEBUG_NEW_PLACEMENT std::vector<NAVertex *>();
		// noHVertices = new NAVertexLoopCountList();
		heuristicForOutsideVertices = 0.0;
	}

	~NAVertexCache(void) 
	{
		Clear();
		delete cache;
		delete sideCache;
		// for(NAVertexLoopCountListItr i = noHVertices->begin(); i != noHVertices->end(); i++) delete i->second;
		// delete noHVertices;
	}

	/*
	void GenerateZeroHurMsg(CString & msg) const
	{
		msg.Empty();
		msg.AppendFormat(_T("These are vertex IDs in each round that were missing huristic value: "));
		std::list<long> * l;
		NAVertexLoopCountListItr i;
		std::list<long>::iterator j;
		for(i = noHVertices->begin(); i != noHVertices->end(); i++)
		{
			l = i->second;
			msg.AppendFormat(_T("R%d="), i->first);
			for (j = l->begin(); j != l->end(); j++) msg.AppendFormat(_T("%d, "), *j);
		}
	}
	*/
	
	NAVertexPtr New(INetworkJunctionPtr junction);
	void UpdateHeuristicForOutsideVertices(double hur, bool goDeep);
	bool UpdateHeuristic(long edgeid, NAVertex * n);
	NAVertexPtr Get(long eid);
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

class EdgeReservations
{
private:
	// std::vector<EdgeReservation> * List;
	float ReservedPop;
	float SaturationDensPerCap;
	float CriticalDens;
	float Capacity;
	bool  DirtyFlag;
	float initDelayCostPerPop;

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
// reservations: a vector of evacuees commited to use this edge at a certain time/cost period
class NAEdge
{
private:	
	EdgeReservations * reservations;
	EVC_TRAFFIC_MODEL trafficModel;
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

	double GetCost(double newPop, EVC_SOLVER_METHOD method) const;
	double GetCurrentCost() const;
	double LeftCapacity() const;	

	// Special function for Flocking: to check how much capacity the edge had originally
	double OriginalCapacity() const { return reservations->Capacity; }

	HRESULT QuerySourceStuff(long * sourceOID, long * sourceID, double * fromPosition, double * toPosition) const;	
	bool AddReservation(/* Evacuee * evacuee, double fromCost, double toCost, */ double population);
	NAEdge(INetworkEdgePtr, long capacityAttribID, long costAttribID, float CriticalDensPerCap, float SaturationDensPerCap, NAResTable *, float InitDelayCostPerPop, EVC_TRAFFIC_MODEL);
	NAEdge(const NAEdge& cpy);

	static bool LessThanNonHur(NAEdge * n1, NAEdge * n2) { return n1->ToVertex->g < n2->ToVertex->g; }
	static bool LessThanHur   (NAEdge * n1, NAEdge * n2) { return n1->ToVertex->g + n1->ToVertex->minh() < n2->ToVertex->g + n2->ToVertex->minh(); }
	
	inline void SetDirty() { reservations->DirtyFlag = true; }
	inline void SetClean() { reservations->DirtyFlag = false; CleanCost = -1.0; }
	float GetReservedPop() const { return reservations->ReservedPop; }
	inline bool IsDirty()  const { return reservations->DirtyFlag;   }
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

	void GetDirtyEdges(std::vector<NAEdgePtr> * dirty)
	{
		if(dirty)
		{
			NAEdgeTableItr i;
			for (i = cacheAlong->begin();   i != cacheAlong->end();   i++) if (i->second->IsDirty()) dirty->push_back(i->second);
			for (i = cacheAgainst->begin(); i != cacheAgainst->end(); i++) if (i->second->IsDirty()) dirty->push_back(i->second);
		}
	}

	void Erase(NAEdgePtr edge);	
	void Clear() { cacheAlong->clear(); cacheAgainst->clear(); }
	size_t Size() { return cacheAlong->size() + cacheAgainst->size(); }	
	HRESULT Insert(NAEdgePtr edge);
	bool Exist(NAEdgePtr edge);
};

typedef std::pair<long, unsigned char> NAEdgeContainerPair;

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
	
	inline void Clear() { cache->clear(); }
	HRESULT Insert(INetworkEdgePtr edge);
	HRESULT Insert(long eid, esriNetworkEdgeDirection dir);
	bool Exist(INetworkEdgePtr edge);
	bool Exist(long eid, esriNetworkEdgeDirection dir);
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
	EVC_TRAFFIC_MODEL		trafficModel;

public:

	NAEdgeCache(long CapacityAttribID, long CostAttribID, float SaturationPerCap, float CriticalDensPerCap, bool TwoWayRoadsShareCap, float InitDelayCostPerPop, EVC_TRAFFIC_MODEL TrafficModel)
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
	size_t Size() const { return cacheAlong->size() + cacheAgainst->size(); }
	
	#pragma warning(push)
	#pragma warning(disable : 4100) /* Ignore warnings for unreferenced function parameters */
	void CleanAllEdgesAndRelease(double maxCost)
	{				
		#ifdef TRACE
		std::ofstream f;
		int count = 0;		
		for(NAEdgeTableItr cit = cacheAlong->begin(); cit != cacheAlong->end(); cit++)
		{
			if ((*cit).second->ToVertex != NULL && (*cit).second->ToVertex->g >= maxCost && (*cit).second->GetReservedPop() <= 0.0) count++;
		}
		for(NAEdgeTableItr cit = cacheAgainst->begin(); cit != cacheAgainst->end(); cit++)
		{
			if ((*cit).second->ToVertex != NULL && (*cit).second->ToVertex->g >= maxCost && (*cit).second->GetReservedPop() <= 0.0) count++;
		}
		f.open("c:\\evcsolver.log", std::ios_base::out | std::ios_base::app);
		f << "Outside Edges: " << count << " of " << Size() << std::endl;
		f.close();
		#endif

		for(NAEdgeTableItr cit = cacheAlong->begin(); cit != cacheAlong->end(); cit++) cit->second->SetClean();
		for(NAEdgeTableItr cit = cacheAgainst->begin(); cit != cacheAgainst->end(); cit++) cit->second->SetClean();
	}
	#pragma warning(pop)
	/*
	unsigned int TotalCalcSaved() const
	{
		unsigned int total = 0;
		for(NAEdgeTableItr cit = cacheAlong->begin(); cit != cacheAlong->end(); cit++) total += (*cit).second->GetCalcSaved();
		for(NAEdgeTableItr cit = cacheAgainst->begin(); cit != cacheAgainst->end(); cit++) total += (*cit).second->GetCalcSaved();
		return total;
	}
	*/
	void Clear();	
};
