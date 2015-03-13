// ===============================================================================================
// Evacuation Solver: Evacuee class definition
// Description: Contains definition for evacuee, evacuee path, and evacuee path segment
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

class NAVertex;
class NAVertexCache;
class NAEdge;
class NAEdgeCache;
class NAEdgeContainer;
class NAEdgeMap;
class EvacueeList;
struct NAEdgePtrHasher;
struct NAEdgePtrEqual;
class SafeZone;
struct EdgeOriginalData;
typedef NAVertex * NAVertexPtr;

class PathSegment
{
private:
	double fromRatio;
	double toRatio;

public:
    NAEdge     * Edge;
    IPolylinePtr pline;

    double GetEdgePortion() const { return toRatio - fromRatio; }
	double GetCurrentCost(EvcSolverMethod method) const;
	HRESULT GetGeometry(INetworkDatasetPtr ipNetworkDataset, IFeatureClassContainerPtr ipFeatureClassContainer, bool & sourceNotFoundFlag, IGeometryPtr & geometry);

    void SetFromRatio(double FromRatio)
    {
	    fromRatio = FromRatio;
	    if (fromRatio == toRatio) fromRatio = toRatio - 0.001;
	}

	void SetToRatio(double ToRatio)
	{
		toRatio = ToRatio;
		if (fromRatio == toRatio) toRatio = fromRatio + 0.001;
	}

	double GetFromRatio() const { return fromRatio; }
	double GetToRatio()   const { return toRatio;   }

    PathSegment(NAEdge * edge, double FromRatio = 0.0, double ToRatio = 1.0)
    {
	    fromRatio = FromRatio;
	    toRatio = ToRatio;
		_ASSERT(FromRatio < ToRatio);
	    Edge = edge;
	    pline = nullptr;
    }
};

typedef PathSegment * PathSegmentPtr;
class Evacuee;

class EvcPath : private std::deque<PathSegmentPtr>
{
private:
	SafeZone* MySafeZone;
	Evacuee * myEvc;
	int     Order;
	bool    Frozen;
	double  RoutedPop;
	double  ReserveEvacuationCost;
	double  PathStartCost;
	double  FinalEvacuationCost;
	double  OrginalCost;
	typedef std::deque<PathSegmentPtr> baselist;

public:
	using baselist::shrink_to_fit;
	using baselist::front;
	using baselist::back;
	using baselist::cbegin;
	using baselist::cend;
	using baselist::empty;
	using baselist::const_iterator;

	inline double GetRoutedPop()             const { return RoutedPop;             }
	inline bool   IsFrozen()                 const { return Frozen;                }
	inline double GetReserveEvacuationCost() const { return ReserveEvacuationCost; }
	inline double GetFinalEvacuationCost()   const { return FinalEvacuationCost;   }
	void CalculateFinalEvacuationCost(double initDelayCostPerPop, EvcSolverMethod method);

	EvcPath(double initDelayCostPerPop, double routedPop, int order, Evacuee * evc, SafeZone * mySafeZone);

	virtual ~EvcPath(void)
	{
		for(const_iterator it = begin(); it != end(); it++) delete (*it);
		clear();
	}
	EvcPath(const EvcPath & that) = delete;
	EvcPath & operator=(const EvcPath &) = delete;

	double GetMinCostRatio(double MaxEvacuationCost = 0.0) const;
	double GetAvgCostRatio(double MaxEvacuationCost = 0.0) const;
	void AddSegment(EvcSolverMethod method, PathSegmentPtr segment);
	HRESULT AddPathToFeatureBuffers(ITrackCancel *, INetworkDatasetPtr, IFeatureClassContainerPtr, bool &,
		IStepProgressorPtr, double &, IFeatureBufferPtr, IFeatureCursorPtr, long, long, long, long);
	void ReattachToEvacuee(EvcSolverMethod method, std::unordered_set<NAEdge *, NAEdgePtrHasher, NAEdgePtrEqual> & touchedEdges);
	inline void CleanYourEvacueePaths(EvcSolverMethod method, std::unordered_set<NAEdge *, NAEdgePtrHasher, NAEdgePtrEqual> & touchedEdges) { EvcPath::DetachPathsFromEvacuee(myEvc, method, touchedEdges); }
	void DoesItNeedASecondChance(double ThreasholdForCost, double ThreasholdForPathOverlap, std::vector<Evacuee *> & AffectingList, double ThisIterationMaxCost, EvcSolverMethod method);

	inline const int & GetKey()  const { return Order; }
	friend bool operator==(const EvcPath & lhs, const EvcPath & rhs) { return lhs.Order == rhs.Order; }
	friend bool operator!=(const EvcPath & lhs, const EvcPath & rhs) { return lhs.Order != rhs.Order; }

	static void DetachPathsFromEvacuee(Evacuee * evc, EvcSolverMethod method, std::unordered_set < NAEdge *, NAEdgePtrHasher, NAEdgePtrEqual> & touchedEdges,
		std::shared_ptr<std::vector<EvcPath *>> detachedPaths = nullptr);
	template<class iterator_type> static size_t DynamicStep_MoveOnPath(const iterator_type & begin, const iterator_type & end,
		std::unordered_set<NAEdge *, NAEdgePtrHasher, NAEdgePtrEqual> & DynamicallyAffectedEdges, double CurrentTime, EvcSolverMethod method, INetworkQueryPtr ipNetworkQuerys);
	static void DynamicStep_MergePaths(std::shared_ptr<EvacueeList> AllEvacuees);
	static size_t DynamicStep_UnreachableEvacuees(std::shared_ptr<EvacueeList> AllEvacuees, double StartCost);

	
	static bool MoreThanFinalCost (const EvcPath * p1, const EvcPath * p2) { return p1->FinalEvacuationCost > p2->FinalEvacuationCost; }
	static bool MoreThanPathOrder1(const Evacuee * e1, const Evacuee * e2);
	static bool LessThanPathOrder1(const Evacuee * e1, const Evacuee * e2);
	static bool MoreThanPathOrder2(const EvcPath * p1, const EvcPath * p2) { return p1->Order > p2->Order; }
	static bool LessThanPathOrder2(const EvcPath * p1, const EvcPath * p2) { return p1->Order < p2->Order; }
};

typedef EvcPath * EvcPathPtr;

class Evacuee
{
public:
	std::vector<NAVertexPtr> * VerticesAndRatio;
	std::list<EvcPathPtr>    * Paths;
	NAEdge                   * DiscoveryLeaf;
	VARIANT                  Name;
	double                   Population;
	double                   PredictedCost;
	double                   FinalCost;
	double                   StartingCost;
	UINT32                   ObjectID;
	EvacueeStatus            Status;
	int                      ProcessOrder;

	Evacuee(VARIANT name, double pop, UINT32 objectID);
	virtual ~Evacuee(void);
	Evacuee(const Evacuee & that) = delete;
	Evacuee & operator=(const Evacuee &) = delete;
	void DynamicMove(NAEdge * edge, double FromRatio, INetworkQueryPtr ipNetworkQuery, double startTime);

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

	static bool LessThanObjectID(const Evacuee * e1, const Evacuee * e2) { return e1->ObjectID  < e2->ObjectID;  }
	static bool ReverseFinalCost(const Evacuee * e1, const Evacuee * e2) { return e1->FinalCost > e2->FinalCost; }
	static bool ReverseEvacuationCost(const Evacuee * e1, const Evacuee * e2) { return e1->Paths->front()->GetReserveEvacuationCost() > e2->Paths->front()->GetReserveEvacuationCost(); }
};

typedef Evacuee * EvacueePtr;
typedef std::pair<long, std::vector<EvacueePtr>> _NAEvacueeVertexTablePair;
typedef std::unordered_map<long, std::vector<EvacueePtr>>::const_iterator NAEvacueeVertexTableItr;

class EvacueeList : private DoubleGrowingArrayList<EvacueePtr, size_t>
{
private:
	EvacueeGrouping groupingOption;
	bool SeperationDisabledForDynamicCASPER;

public:
	EvacueeList(EvacueeGrouping GroupingOption, size_t capacity = 0) : groupingOption(GroupingOption), SeperationDisabledForDynamicCASPER(false), DoubleGrowingArrayList<EvacueePtr, size_t>(capacity) { }
	virtual ~EvacueeList();
	void FinilizeGroupings(double OKDistance, DynamicMode DynamicCASPEREnabled);

	EvacueeList(const EvacueeList & that) = delete;
	EvacueeList & operator=(const EvacueeList &) = delete;

	bool IsSeperable() const { return CheckFlag(groupingOption, EvacueeGrouping::Separate); }
	bool IsSeperationDisabledForDynamicCASPER() const { return SeperationDisabledForDynamicCASPER; }
	void Insert(const EvacueePtr & item) { push_back(item); }
	DoubleGrowingArrayList<EvacueePtr, size_t>::const_iterator begin() const { return DoubleGrowingArrayList<EvacueePtr, size_t>::begin(); }
	DoubleGrowingArrayList<EvacueePtr, size_t>::const_iterator end()   const { return DoubleGrowingArrayList<EvacueePtr, size_t>::end(); }
	size_t size() const { return DoubleGrowingArrayList<EvacueePtr, size_t, 0>::size(); }
};

class NAEvacueeVertexTable : protected std::unordered_map<long, std::vector<EvacueePtr>>
{
public:
	using std::unordered_map<long, std::vector<EvacueePtr>>::empty;

	void InsertReachable(std::shared_ptr<EvacueeList> list, CARMASort sortDir, std::shared_ptr<NAEdgeContainer> leafs);
	void RemoveDiscoveredEvacuees(NAVertex * myVertex, NAEdge * myEdge, std::shared_ptr<std::vector<EvacueePtr>> SortedEvacuees, double pop, EvcSolverMethod method);
	void LoadSortedEvacuees(std::shared_ptr<std::vector<EvacueePtr>>) const;
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
	NAVertex * VertexAndRatio;

	inline void   Reserve(double pop)      { reservedPop += pop;   }
	inline double getPositionAlong() const { return positionAlong; }
	inline NAEdge * getBehindEdge()        { return behindEdge;    }

	SafeZone(const SafeZone & that) = delete;
	SafeZone & operator=(const SafeZone &) = delete;
	virtual ~SafeZone();
	SafeZone(INetworkJunctionPtr _junction, NAEdge * _behindEdge, double posAlong, VARIANT cap);
	bool IsRestricted(std::shared_ptr<NAEdgeCache> ecache, NAEdge * leadingEdge, double costPerDensity);
	double SafeZoneCost(double population2Route, EvcSolverMethod solverMethod, double costPerDensity, double * globalDeltaCost = nullptr);
};

typedef SafeZone * SafeZonePtr;

class SafeZoneTable : protected std::unordered_map<long, SafeZonePtr>
{
public:
	using std::unordered_map<long, SafeZonePtr>::size;
	using std::unordered_map<long, SafeZonePtr>::const_iterator;
	using std::unordered_map<long, SafeZonePtr>::begin;
	using std::unordered_map<long, SafeZonePtr>::end;

	SafeZoneTable(size_t capacity) :std::unordered_map<long, SafeZonePtr>(capacity) { }
	SafeZoneTable(const SafeZoneTable & that) = delete;
	SafeZoneTable & operator=(const SafeZoneTable &) = delete;
	virtual ~SafeZoneTable() { for (auto z : *this) delete z.second; }
	virtual bool insert(SafeZonePtr z);

	bool CheckDiscoveredSafePoint(std::shared_ptr<NAEdgeCache>, NAVertexPtr, NAEdge *, NAVertexPtr &, double &, SafeZonePtr &, double, double, EvcSolverMethod, double &, bool &) const;
};
