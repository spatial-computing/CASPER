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

struct EdgeOriginalData
{
	double OriginalCost;
	double OriginalCapacity;
	double CostRatio;
	double CapacityRatio;

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

	bool IsAffectedEdge() const { return CostRatio != 1.0 || CapacityRatio != 1.0; }

	bool ApplyNewOriginalCostAndCapacity(const NAEdgePtr edge)
	{
		/// TODO figure out when we should mark the cost as infinite
		return edge->ApplyNewOriginalCostAndCapacity(OriginalCost * CostRatio, OriginalCapacity * CapacityRatio, EvcSolverMethod::CASPERSolver);
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
		AffectedCostRate = min(max(AffectedCostRate, 0.0001), 10000.0);
		AffectedCapacityRate = min(max(AffectedCapacityRate, 0.0001), 10000.0);
		if (EndTime < 0.0) EndTime = FLT_MAX;
		return StartTime >= 0.0 && StartTime < EndTime && !EnclosedEdges.empty();
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
	void ProcessAllChanges(std::shared_ptr<EvacueeList> AllEvacuees, std::shared_ptr<NAEdgeCache> ecache, 
		 std::unordered_map<NAEdgePtr, EdgeOriginalData, NAEdgePtrHasher, NAEdgePtrEqual> & OriginalEdgeSettings, DynamicMode myDynamicMode) const;

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

public:
	bool Enabled() const { return myDynamicMode != DynamicMode::Disabled; }
	DynamicDisaster(ITablePtr SingleDynamicChangesLayer, DynamicMode dynamicMode, bool & flagBadDynamicChangeSnapping);
	void ResetDynamicChanges();
	bool NextDynamicChange(std::shared_ptr<EvacueeList> AllEvacuees, std::shared_ptr<NAEdgeCache> ecache);
	virtual ~DynamicDisaster() { for (auto p : allChanges) delete p; }
};
