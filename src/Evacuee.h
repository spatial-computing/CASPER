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
	double GetCurrentCost(EvcSolverMethod method);
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
	    pline = nullptr;
    }
};

typedef PathSegment * PathSegmentPtr;
class Evacuee;

class EvcPath : private std::deque<PathSegmentPtr>
{
private:
	double  RoutedPop;
	double  ReserveEvacuationCost;
	double  FinalEvacuationCost;
	double  OrginalCost;
	int     Order;
	Evacuee * myEvc;
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
	inline double GetReserveEvacuationCost() const { return ReserveEvacuationCost; }
	inline double GetFinalEvacuationCost()   const { return FinalEvacuationCost;   }
	void CalculateFinalEvacuationCost(double initDelayCostPerPop, EvcSolverMethod method);

	EvcPath(double routedPop, int order, Evacuee * evc) : baselist()
	{
		RoutedPop = routedPop;
		FinalEvacuationCost = 0.0;
		ReserveEvacuationCost = 0.0;
		OrginalCost = 0.0;
		Order = order;
		myEvc = evc;
	}

	double GetMinCostRatio() const;
	void AddSegment(EvcSolverMethod method, PathSegmentPtr segment);
	HRESULT AddPathToFeatureBuffers(ITrackCancel * , INetworkDatasetPtr , IFeatureClassContainerPtr , bool & , IStepProgressorPtr , double & , double , IFeatureBufferPtr , IFeatureBufferPtr ,
									IFeatureCursorPtr , IFeatureCursorPtr , long , long , long , long ,	long , long , long , long , long , long , long , bool);
	static void DetachPathsFromEvacuee(Evacuee * evc, EvcSolverMethod method, std::shared_ptr<std::vector<EvcPath *>> detachedPaths = nullptr, NAEdgeMap * touchedEdges = nullptr);
	void ReattachToEvacuee(EvcSolverMethod method);
	inline void CleanYourEvacueePaths(EvcSolverMethod method) { EvcPath::DetachPathsFromEvacuee(myEvc, method); }
	void DoesItNeedASecondChance(double ThreasholdForCost, double ThreasholdForPathOverlap, std::vector<Evacuee *> & AffectingList, double ThisIterationMaxCost, EvcSolverMethod method);

	inline const int & GetKey()  const { return Order; }
	friend bool operator==(const EvcPath & lhs, const EvcPath & rhs) { return lhs.Order == rhs.Order; }
	friend bool operator!=(const EvcPath & lhs, const EvcPath & rhs) { return lhs.Order != rhs.Order; }

	virtual ~EvcPath(void)
	{
		for(const_iterator it = begin(); it != end(); it++) delete (*it);
		clear();
	}
	EvcPath(const EvcPath & that) = delete;
	EvcPath & operator=(const EvcPath &) = delete;

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
	virtual ~Evacuee(void);
	Evacuee(const Evacuee & that) = delete;
	Evacuee & operator=(const Evacuee &) = delete;

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
typedef std::pair<long, std::vector<EvacueePtr>> _NAEvacueeVertexTablePair;
typedef std::unordered_map<long, std::vector<EvacueePtr>>::const_iterator NAEvacueeVertexTableItr;

class EvacueeList : private DoubleGrowingArrayList<EvacueePtr, size_t>
{
private:
	EvacueeGrouping groupingOption;

public:
	EvacueeList(EvacueeGrouping GroupingOption, size_t capacity = 0) : groupingOption(GroupingOption), DoubleGrowingArrayList<EvacueePtr, size_t>(capacity) { }
	virtual ~EvacueeList();
	EvacueeList(const EvacueeList & that) = delete;
	EvacueeList & operator=(const EvacueeList &) = delete;

	void Insert(const EvacueePtr & item) { push_back(item); }
	void FinilizeGroupings(double OKDistance);
	DoubleGrowingArrayList<EvacueePtr, size_t>::const_iterator begin() { return DoubleGrowingArrayList<EvacueePtr, size_t>::begin(); }
	DoubleGrowingArrayList<EvacueePtr, size_t>::const_iterator end() { return DoubleGrowingArrayList<EvacueePtr, size_t>::end(); }
	size_t size() const { return DoubleGrowingArrayList<EvacueePtr, size_t, 0>::size(); }
};

class NAEvacueeVertexTable : protected std::unordered_map<long, std::vector<EvacueePtr>>
{
public:
	using std::unordered_map<long, std::vector<EvacueePtr>>::empty;

	void InsertReachable(std::shared_ptr<EvacueeList> list, CARMASort sortDir);
	void RemoveDiscoveredEvacuees(NAVertex * myVertex, NAEdge * myEdge, std::shared_ptr<std::vector<EvacueePtr>> SortedEvacuees, std::shared_ptr<NAEdgeContainer> leafs, double pop, EvcSolverMethod method);
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
	NAVertex * Vertex;

	inline void   Reserve(double pop)      { reservedPop += pop;   }
	inline double getPositionAlong() const { return positionAlong; }
	inline NAEdge * getBehindEdge()        { return behindEdge;    }

	SafeZone(const SafeZone & that) = delete;
	SafeZone & operator=(const SafeZone &) = delete;
	virtual ~SafeZone();
	SafeZone(INetworkJunctionPtr _junction, NAEdge * _behindEdge, double posAlong, VARIANT cap);
	HRESULT IsRestricted(std::shared_ptr<NAEdgeCache> ecache, NAEdge * leadingEdge, bool & restricted, double costPerDensity);
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
	virtual void insert(SafeZonePtr z);

	HRESULT CheckDiscoveredSafePoint(std::shared_ptr<NAEdgeCache>, NAVertexPtr, NAEdge *, NAVertexPtr &, double &, SafeZonePtr &, double, double, EvcSolverMethod, double &, bool &) const;
};
