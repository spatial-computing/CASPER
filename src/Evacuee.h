#pragma once

#include "StdAfx.h"
#include "utils.h"

class NAVertex;
class NAEdge;
class NAEdgeCache;
class NAEdgeContainer;
class NAEdgeMap;
typedef NAVertex * NAVertexPtr;

class PathSegment
{
private:
	double fromRatio;
	double toRatio;

public:
    NAEdge * Edge;
    IPolylinePtr pline;

    double GetEdgePortion() const { return toRatio - fromRatio; }
	HRESULT GetGeometry(INetworkDatasetPtr ipNetworkDataset, IFeatureClassContainerPtr ipFeatureClassContainer, bool & sourceNotFoundFlag, IGeometryPtr & geometry);

    void SetFromRatio(double FromRatio)
    {
	    fromRatio = FromRatio;
	    if (fromRatio == toRatio) fromRatio = toRatio - 0.001;
    }

	double GetFromRatio() const { return fromRatio; }
	double GetToRatio()   const { return toRatio;   }

    PathSegment(NAEdge * edge, double FromRatio = 0.0, double ToRatio = 1.0)
    {
	    fromRatio = FromRatio;
	    toRatio = ToRatio;
		_ASSERT(FromRatio < ToRatio);
	    Edge = edge;
	    pline = NULL;
    }
};

typedef PathSegment * PathSegmentPtr;
class Evacuee;

class EvcPath : private std::list<PathSegmentPtr>
{
private:
	double  RoutedPop;
	double  ReserveEvacuationCost;
	double  FinalEvacuationCost;
	double  OrginalCost;
	int     Order;
	Evacuee * myEvc;

public:
	typedef std::list<PathSegmentPtr>::const_iterator  const_iterator;
	typedef std::list<PathSegmentPtr>::const_reference const_reference;

	const_reference Front()                  const { return this->front();         }
	const_iterator Begin()                   const { return this->begin();         }
	const_iterator End()                     const { return this->end();           }
	inline double GetRoutedPop()             const { return RoutedPop;             }
	inline double GetReserveEvacuationCost() const { return ReserveEvacuationCost; }
	inline double GetFinalEvacuationCost()   const { return FinalEvacuationCost;   }
	void CalculateFinalEvacuationCost(double initDelayCostPerPop, EvcSolverMethod method);

	EvcPath(double routedPop, int order, Evacuee * evc) : std::list<PathSegmentPtr>()
	{
		RoutedPop = routedPop;
		FinalEvacuationCost = 0.0;
		ReserveEvacuationCost = 0.0;
		OrginalCost = 0.0;
		Order = order;
		myEvc = evc;
	}

	void AddSegment(EvcSolverMethod method, PathSegmentPtr segment);
	HRESULT AddPathToFeatureBuffers(ITrackCancel * , INetworkDatasetPtr , IFeatureClassContainerPtr , bool & , IStepProgressorPtr , double & , double , IFeatureBufferPtr , IFeatureBufferPtr ,
									IFeatureCursorPtr , IFeatureCursorPtr , long , long , long , long ,	long , long , long , long , long , long , long , bool);
	static void DetachPathsFromEvacuee(Evacuee * evc, EvcSolverMethod method, std::vector<EvcPath *> * detachedPaths = NULL, NAEdgeMap * touchedEdges = NULL);
	void ReattachToEvacuee(EvcSolverMethod method);
	inline void CleanYourEvacueePaths(EvcSolverMethod method) { EvcPath::DetachPathsFromEvacuee(myEvc, method); }
	bool DoesItNeedASecondChance(double ThreasholdForFinalCost, std::vector<Evacuee *> & AffectingList, double ThisIterationMaxCost, EvcSolverMethod method);

	bool           Empty() const { return std::list<PathSegmentPtr>::empty(); }
	PathSegmentPtr Front()       { return std::list<PathSegmentPtr>::front(); }
	PathSegmentPtr Back()        { return std::list<PathSegmentPtr>::back();  }
	inline const int & GetKey()  const { return Order; }
	friend inline bool operator==(const EvcPath & lhs, const EvcPath & rhs) { return lhs.Order == rhs.Order; }

	~EvcPath(void)
	{
		for(const_iterator it = begin(); it != end(); it++) delete (*it);
		clear();
	}

	static bool LessThanOrder(const EvcPath * p1, const EvcPath * p2)
	{
		return p1->Order < p2->Order;
	}

	static bool MoreThanFinalCost(const EvcPath * p1, const EvcPath * p2)
	{
		return p1->FinalEvacuationCost > p2->FinalEvacuationCost;
	}

	static bool MoreThanPathOrder(const Evacuee * e1, const Evacuee * e2);
};

typedef EvcPath * EvcPathPtr;

class Evacuee
{
public:
	std::vector<NAVertexPtr> * Vertices;
	std::list<EvcPathPtr>    * Paths;
	VARIANT                  Name;
	double                   Population;
	double                   PredictedCost;
	double                   FinalCost;
	UINT32                   ObjectID;
	EvacueeStatus            Status;
	int                      ProcessOrder;

	Evacuee(VARIANT name, double pop, UINT32 objectID);
	~Evacuee(void);

	static bool LessThan(const Evacuee * e1, const Evacuee * e2)
	{
		if (e1->PredictedCost == e2->PredictedCost) return e1->Population < e2->Population;
		else return e1->PredictedCost < e2->PredictedCost;
	}

	static bool MoreThan(const Evacuee * e1, const Evacuee * e2)
	{
		if (e1->PredictedCost == e2->PredictedCost) return e1->Population > e2->Population;
		else return e1->PredictedCost > e2->PredictedCost;
	}

	static bool LessThanObjectID(const Evacuee * e1, const Evacuee * e2)
	{
		return e1->ObjectID < e2->ObjectID;
	}

	static bool ReverseFinalCost(const Evacuee * e1, const Evacuee * e2)
	{
		return e1->FinalCost > e2->FinalCost;
	}

	static bool ReverseEvacuationCost(const Evacuee * e1, const Evacuee * e2)
	{
		return e1->Paths->back()->GetReserveEvacuationCost() > e2->Paths->back()->GetReserveEvacuationCost();
	}
};

typedef Evacuee * EvacueePtr;
typedef std::vector<EvacueePtr> EvacueeList;
typedef std::vector<EvacueePtr>::const_iterator EvacueeListItr;
typedef std::pair<long, std::vector<EvacueePtr> *> _NAEvacueeVertexTablePair;
typedef std::unordered_map<long, std::vector<EvacueePtr> *>::const_iterator NAEvacueeVertexTableItr;

class NAEvacueeVertexTable : protected std::unordered_map<long, std::vector<EvacueePtr> *>
{
private:
	std::vector<EvacueePtr> * Find(long junctionEID);
	void Erase(long junctionEID);
public:
	~NAEvacueeVertexTable();

	void InsertReachable(EvacueeList * list, CARMASort sortDir);
	void RemoveDiscoveredEvacuees(NAVertex * myVertex, NAEdge * myEdge, EvacueeList * SortedEvacuees, NAEdgeContainer * leafs, double pop, EvcSolverMethod method);
	bool Empty() const { return empty(); }
	void LoadSortedEvacuees(EvacueeList *) const;
};

class SafeZone
{
private:
	INetworkJunctionPtr junction;
	NAEdge * behindEdge;
	double   positionAlong;
	double   capacity;
	double   reservedPop;

public:
	NAVertex * Vertex;

	inline void   Reserve(double pop)      { reservedPop += pop;   }
	inline double getPositionAlong() const { return positionAlong; }
	inline NAEdge * getBehindEdge()        { return behindEdge;    }

	~SafeZone();
	SafeZone(INetworkJunctionPtr _junction, NAEdge * _behindEdge, double posAlong, VARIANT cap);
	HRESULT IsRestricted(NAEdgeCache * ecache, NAEdge * leadingEdge, bool & restricted, double costPerDensity);
	double SafeZoneCost(double population2Route, EvcSolverMethod solverMethod, double costPerDensity, double * globalDeltaCost = NULL);
};

typedef SafeZone * SafeZonePtr;
typedef std::unordered_map<long, SafeZonePtr> SafeZoneTable;
typedef std::unordered_map<long, SafeZonePtr>::_Pairib SafeZoneTableInsertReturn;
typedef std::unordered_map<long, SafeZonePtr>::const_iterator SafeZoneTableItr;
typedef std::pair<long, SafeZonePtr> _SafeZoneTablePair;
#define SafeZoneTablePair(a) _SafeZoneTablePair(a->Vertex->EID, a)
