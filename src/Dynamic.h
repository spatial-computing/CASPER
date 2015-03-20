// ===============================================================================================
// Evacuation Solver: Dynamic classes definition
// Description: Contains definition for dynamic CASPER solver
//
// Copyright (C) 2015 Kaveh Shahabi
// Distributed under the Apache Software License, Version 2.0. (See accompanying file LICENSE.txt)
//
// Author: Kaveh Shahabi
// URL: http://github.com/spatial-computing/CASPER
// ===============================================================================================

#pragma once

#include "StdAfx.h"
#include "utils.h"
#include "NAEdge.h"

// forward declare some classes
class EvacueeList;
class NAVertexCache;

struct EdgeOriginalData
{
	double OriginalCost;
	double OriginalCapacity;
	double CostRatio;
	double CapacityRatio;

	// constants for range of valid ratio
	static const double MaxCostRatio;
	static const double MinCostRatio;
	static const double MaxCapacityRatio;
	static const double MinCapacityRatio;

	EdgeOriginalData(NAEdgePtr edge)
	{
		OriginalCost = edge->OriginalCost;
		OriginalCapacity = edge->OriginalCapacity();
		CostRatio = 1.0;
		CapacityRatio = 1.0;
	}

	void ResetRatios()
	{
		CostRatio = 1.0;
		CapacityRatio = 1.0;
	}

	bool IsRatiosNonOne() const { return CostRatio != 1.0 || CapacityRatio != 1.0; }

	inline double AdjustedCost()     const { return CostRatio     < MaxCostRatio     ? (CostRatio     > MinCostRatio     ? OriginalCost     * CostRatio : OriginalCost * MinCostRatio) : CASPER_INFINITY; }
	inline double AdjustedCapacity() const { return CapacityRatio < MaxCapacityRatio ? (CapacityRatio > MinCapacityRatio ? OriginalCapacity * CapacityRatio : 0.0) : OriginalCapacity * MaxCapacityRatio; }

	bool IsAffectedEdge(NAEdgePtr const edge) const
	{
		return edge->IsNewOriginalCostAndCapacityDifferent(AdjustedCost(), AdjustedCapacity());
	}

	bool ApplyNewOriginalCostAndCapacity(const NAEdgePtr edge)
	{
		return edge->ApplyNewOriginalCostAndCapacity(AdjustedCost(), AdjustedCapacity(), true, EvcSolverMethod::CASPERSolver);
	}
};

struct SingleDynamicChange
{
	std::unordered_set<long> EnclosedEdges;
	EdgeDirection DisasterDirection;

	double      StartTime;
	double      EndTime;
	double      AffectedCostRate;
	double      AffectedCapacityRate;

	SingleDynamicChange() : DisasterDirection(EdgeDirection::None), StartTime(0.0), EndTime(-1.0), AffectedCostRate(0.0), AffectedCapacityRate(0.0) { }

	bool IsValid()
	{
		AffectedCostRate = min(max(AffectedCostRate, EdgeOriginalData::MinCostRatio), EdgeOriginalData::MaxCostRatio);
		AffectedCapacityRate = min(max(AffectedCapacityRate, EdgeOriginalData::MinCapacityRatio), EdgeOriginalData::MaxCapacityRatio);
		EndTime = EndTime < 0.0 || EndTime > CASPER_INFINITY ? CASPER_INFINITY : EndTime;
		return StartTime >= 0.0 && StartTime < EndTime && !EnclosedEdges.empty() && (AffectedCostRate != 1.0 || AffectedCapacityRate != 1.0);
	}
};

typedef SingleDynamicChange * SingleDynamicChangePtr;

class CriticalTime
{
private:
	double Time;
	mutable std::vector<SingleDynamicChangePtr> Intersected;

public:
	CriticalTime(double time) : Time(time) { }
	void AddIntersectedChange(const SingleDynamicChangePtr & item) const { Intersected.push_back(item); }
	size_t ProcessAllChanges(std::shared_ptr<EvacueeList> AllEvacuees, std::shared_ptr<NAEdgeCache> ecache, double & EvcStartTime,
		std::unordered_map<NAEdgePtr, EdgeOriginalData, NAEdgePtrHasher, NAEdgePtrEqual> & OriginalEdgeSettings, DynamicMode myDynamicMode, EvcSolverMethod solverMethod, int & pathGenerationCount) const;

	bool friend operator< (const CriticalTime & lhs, const CriticalTime & rhs) { return lhs.Time <  rhs.Time; }
	static void MergeWithPreviousTimeFrame(std::set<CriticalTime> & dynamicTimeFrame);
};

class DynamicDisaster
{
private:
	std::vector<SingleDynamicChangePtr> allChanges;
	std::set<CriticalTime> dynamicTimeFrame;
	std::set<CriticalTime>::const_iterator currentTime;
	std::unordered_map<NAEdgePtr, EdgeOriginalData, NAEdgePtrHasher, NAEdgePtrEqual> OriginalEdgeSettings;
	DynamicMode myDynamicMode;
	EvcSolverMethod SolverMethod;

public:
	void Flush()
	{
		for (auto p : allChanges) delete p;
		allChanges.clear();
		dynamicTimeFrame.clear();
		OriginalEdgeSettings.clear();
	}

	DynamicMode GetDynamicMode() const { return myDynamicMode; }
	DynamicDisaster(ITablePtr SingleDynamicChangesLayer, DynamicMode dynamicMode, bool & flagBadDynamicChangeSnapping, EvcSolverMethod solverMethod);
	size_t ResetDynamicChanges();
	size_t NextDynamicChange(std::shared_ptr<EvacueeList> AllEvacuees, std::shared_ptr<NAEdgeCache> ecache, double & EvcStartTime, int & pathGenerationCount);
	virtual ~DynamicDisaster() { Flush(); }
};
