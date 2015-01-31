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

struct DynamicChange
{
	struct PairHasher : public std::unary_function<std::pair<long, esriNetworkEdgeDirection>, size_t>
	{
		size_t operator()(const std::pair<long, esriNetworkEdgeDirection> & pair) const { return pair.first; }
	};

	struct PairEqual : public std::binary_function<std::pair<long, esriNetworkEdgeDirection>, std::pair<long, esriNetworkEdgeDirection>, bool>
	{
		bool operator()(const std::pair<long, esriNetworkEdgeDirection> & _Left, const std::pair<long, esriNetworkEdgeDirection> & _Right) const
		{ 
			return (_Left.first == _Right.first) && (_Left.second == _Right.second);
		}
	};

	EdgeDirection DisasterDirection;
	double      StartTime;
	double      EndTime;
	double      AffectedCostPercentage;
	double      AffectedCapacityPercentage;
	bool        EvacueesAreStuck;
	bool        Applied;

	std::unordered_set<std::pair<long, esriNetworkEdgeDirection>, PairHasher, PairEqual> EnclosedEdges;
	std::unordered_set<long> EnclosedVertices;

	DynamicChange(EdgeDirection disasterDirection, double startTime, double endTime, double affectedCostPercentage, double affectedCapacityPercentage, bool evacueesAreStuck) :
		DisasterDirection(disasterDirection),  StartTime(startTime), EndTime(endTime), AffectedCostPercentage(affectedCostPercentage),
		AffectedCapacityPercentage(affectedCapacityPercentage), EvacueesAreStuck(evacueesAreStuck), Applied(false) { }

	DynamicChange() : DisasterDirection(EdgeDirection::None), StartTime(0.0), EndTime(-1.0), AffectedCostPercentage(0.0),
		AffectedCapacityPercentage(0.0), EvacueesAreStuck(true), Applied(false) { }
};

class DynamicDisaster
{
private:
	std::vector<DynamicChange *> allChanges;
	double currentTime;
	DynamicMode myDynamicMode;

public:
	bool Enabled() const { return myDynamicMode != DynamicMode::Disabled; }
	DynamicDisaster(ITablePtr DynamicChangesLayer, DynamicMode dynamicMode);

	virtual ~DynamicDisaster()
	{ 
		for (auto p : allChanges) delete p;
	}
};
