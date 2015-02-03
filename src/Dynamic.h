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

struct SingleDynamicChange
{
	EdgeDirection DisasterDirection;
	double      StartTime;
	double      EndTime;
	double      AffectedCostPercentage;
	double      AffectedCapacityPercentage;
	bool        EvacueesAreStuck;

	std::unordered_set<std::pair<long, esriNetworkEdgeDirection>, NAedgePairHasher, NAedgePairEqual> EnclosedEdges;
	std::unordered_set<long> EnclosedVertices;

	SingleDynamicChange() : DisasterDirection(EdgeDirection::None), StartTime(0.0), EndTime(-1.0), AffectedCostPercentage(0.0), AffectedCapacityPercentage(0.0), EvacueesAreStuck(true) { }
	SingleDynamicChange(EdgeDirection disasterDirection, double startTime, double endTime, double affectedCostPercentage, double affectedCapacityPercentage, bool evacueesAreStuck) :
		DisasterDirection(disasterDirection),  StartTime(startTime), EndTime(endTime), AffectedCostPercentage(affectedCostPercentage), AffectedCapacityPercentage(affectedCapacityPercentage),
		EvacueesAreStuck(evacueesAreStuck) { }
};

typedef SingleDynamicChange * SingleDynamicChangePtr;

class CriticalTime
{
private:
	double Time;
	mutable std::vector<SingleDynamicChangePtr> Apply;
	mutable std::vector<SingleDynamicChangePtr> Unapply;

public:
	CriticalTime(double time) : Time(time) { }
	void AddApplyChange  (const SingleDynamicChangePtr & item) const {   Apply.push_back(item); }
	void AddUnapplyChange(const SingleDynamicChangePtr & item) const { Unapply.push_back(item); }
	void ProcessAllChanges(std::shared_ptr<EvacueeList> AllEvacuees, std::shared_ptr<NAVertexCache> vcache, std::shared_ptr<NAEdgeCache> ecache, 
		 std::unordered_map<NAEdgePtr, std::pair<double, double>, NAEdgePtrHasher, NAEdgePtrEqual> & OriginalEdgeSettings) const;

	bool friend operator< (const CriticalTime & lhs, const CriticalTime & rhs) { return lhs.Time <  rhs.Time; }
};

class DynamicDisaster
{
private:
	std::vector<SingleDynamicChangePtr> allChanges;
	std::set<CriticalTime> dynamicTimeFrame;
	std::set<CriticalTime>::const_iterator currentTime;
	std::unordered_map<NAEdgePtr, std::pair<double, double>, NAEdgePtrHasher, NAEdgePtrEqual> OriginalEdgeSettings;
	DynamicMode myDynamicMode;

public:
	bool Enabled() const { return myDynamicMode != DynamicMode::Disabled; }
	DynamicDisaster(ITablePtr SingleDynamicChangesLayer, DynamicMode dynamicMode);
	void ResetDynamicChanges();
	bool NextDynamicChange(std::shared_ptr<EvacueeList> AllEvacuees, std::shared_ptr<NAVertexCache> vcache, std::shared_ptr<NAEdgeCache> ecache);

	virtual ~DynamicDisaster()
	{ 
		for (auto p : allChanges) delete p;
	}
};
